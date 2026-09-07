"""Read-only UE 5.7 reload validation for the additive stone-vault control room.

The validator only loads assets/maps and writes its own validation report. It
never saves or edits Unreal packages and remains after the importer is removed.
"""
from pathlib import Path
import hashlib
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/Environment/MoleControlV2'
PROJECT = Path(unreal.Paths.project_dir()).resolve().parent
DEST = '/Game/Environment/MoleControlV2'
TEXTURE_NAME = 'T_MoleControlV2_Atlas'
TEXTURE_PATH = f'{DEST}/Textures/{TEXTURE_NAME}'
MASTER_PATH = f'{DEST}/Materials/M_MoleControlV2_Master'
FOREST_TEXTURE_NAME = 'T_MoleControlV2_Forest'
FOREST_MATERIAL_PATH = f'{DEST}/Materials/M_MoleControlV2_ForestImpostor'
IMPORT_REPORT = SOURCE / 'unreal_import_validation.json'


def read_manifest():
    return json.loads((SOURCE / 'model_manifest.json').read_text(encoding='utf-8'))


def read_layout():
    return json.loads((SOURCE / 'level_layout.json').read_text(encoding='utf-8'))


def material_specs(manifest):
    return {spec['name']: spec for spec in manifest['materials'].values()}


def mesh_subsystem():
    return (unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
            or unreal.get_default_object(unreal.StaticMeshEditorSubsystem))


def protected_hashes():
    content = PROJECT / 'TunaSweeper/Content'
    additive = content / 'Environment/MoleControlV2'
    paths = {p for p in content.rglob('*.umap') if not p.is_relative_to(additive)}
    for existing in ('Environment/Bunker/Agit', 'Environment/FacilityRooms'):
        paths.update((content / existing).rglob('*.uasset'))
    return {p.relative_to(PROJECT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(paths)}


def xyz_error(actual, expected):
    return max(abs(a-b) for a,b in zip((actual.x, actual.y, actual.z), expected))


def angle_error(actual, expected):
    return abs((actual-expected+180) % 360 - 180)


def verify_look_at(actor, spec):
    assert xyz_error(actor.get_actor_location(), spec['location_cm']) < .01, actor.get_actor_label()
    expected = unreal.MathLibrary.find_look_at_rotation(
        unreal.Vector(*spec['location_cm']), unreal.Vector(*spec['target_cm']))
    actual = actor.get_actor_rotation()
    assert angle_error(actual.yaw, expected.yaw) < .01, (actor.get_actor_label(), actual, expected)
    assert angle_error(actual.pitch, expected.pitch) < .01, (actor.get_actor_label(), actual, expected)
    assert angle_error(actual.roll, expected.roll) < .01, (actor.get_actor_label(), actual, expected)
    return {'label': actor.get_actor_label(), 'location': spec['location_cm'],
            'rotation': [actual.pitch, actual.yaw, actual.roll]}


def validate(manifest=None, baseline=None, verification='fresh-process reload'):
    manifest = manifest or read_manifest()
    previous = json.loads(IMPORT_REPORT.read_text(encoding='utf-8')) if baseline is None else None
    if previous:
        assert previous['passed'], 'Previous import validation did not pass'
    baseline = baseline if baseline is not None else previous['protected_assets_sha256']
    assert protected_hashes() == baseline, 'Pre-existing maps, Agit or FacilityRooms assets changed'
    lib = unreal.MaterialEditingLibrary
    texture = unreal.load_asset(TEXTURE_PATH)
    assert isinstance(texture, unreal.Texture2D) and texture.get_editor_property('srgb')
    forest_texture = unreal.load_asset(f'{DEST}/Textures/{FOREST_TEXTURE_NAME}')
    assert isinstance(forest_texture, unreal.Texture2D)
    forest_mat = unreal.load_asset(FOREST_MATERIAL_PATH)
    assert isinstance(forest_mat, unreal.Material)
    assert forest_mat.get_editor_property('two_sided')
    assert forest_mat.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_UNLIT
    assert lib.get_material_default_texture_parameter_value(forest_mat, 'ForestTexture') == forest_texture
    assert lib.get_material_property_input_node(forest_mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    master = unreal.load_asset(MASTER_PATH)
    assert isinstance(master, unreal.Material)
    assert isinstance(lib.get_material_property_input_node(master, unreal.MaterialProperty.MP_BASE_COLOR),
                      unreal.MaterialExpressionMultiply)
    assert 'Atlas' in {str(n) for n in lib.get_texture_parameter_names(master)}
    materials = []
    for name, spec in material_specs(manifest).items():
        mat = unreal.load_asset(f'{DEST}/Materials/{name}')
        assert isinstance(mat, unreal.MaterialInstanceConstant), name
        assert mat.get_editor_property('parent') == master, name
        assert 'Tint' in {str(n) for n in lib.get_vector_parameter_names(mat)}, name
        assigned_texture = (unreal.load_asset(f"{DEST}/Textures/{spec['texture']}")
                            if spec.get('texture') else texture)
        assert isinstance(assigned_texture, unreal.Texture2D), (name, spec.get('texture'))
        assert assigned_texture.get_editor_property('srgb'), name
        assert lib.get_material_instance_texture_parameter_value(mat, 'Atlas') == assigned_texture, name
        for parameter, field in [('Metallic', 'metallic'), ('Roughness', 'roughness'),
                                  ('EmissionStrength', 'emission')]:
            value = lib.get_material_instance_scalar_parameter_value(mat, parameter)
            assert abs(value-spec[field]) < 1e-5, (name, parameter, value, spec[field])
        materials.append({'name': name, 'editable_color': 'Tint', 'parent': MASTER_PATH,
                          'texture': assigned_texture.get_path_name()})
    assert any(m['name'] == 'M_MoleControlV2_Rug' for m in materials), 'Missing independent rug material'
    editor = mesh_subsystem()
    old = {a['name']: a for a in previous['assets']} if previous else {}
    assets = []
    for entry in manifest['assets']:
        mesh = unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh, unreal.StaticMesh), entry['name']
        triangles = mesh.get_num_triangles(0)
        assert entry['triangles']*.8 <= triangles <= entry['triangles'], (entry['name'], triangles, entry['triangles'])
        if entry['name'] in old:
            assert triangles == old[entry['name']]['triangles_lod0'], entry['name']
        channels = editor.get_num_uv_channels(mesh, 0)
        collisions = editor.get_simple_collision_count(mesh) + editor.get_convex_collision_count(mesh)
        assert channels >= 2, (entry['name'], 'UV', channels)
        assert collisions == entry['collision_boxes'], (entry['name'], 'collision', collisions)
        build = editor.get_lod_build_settings(mesh, 0)
        assert build.get_editor_property('use_full_precision_u_vs'), entry['name']
        assert build.get_editor_property('generate_lightmap_u_vs'), entry['name']
        assert mesh.get_editor_property('light_map_coordinate_index') == 1, entry['name']
        assigned = [slot.get_editor_property('material_interface').get_name()
                    for slot in mesh.get_editor_property('static_materials')]
        assert set(assigned) == set(entry['materials']), (entry['name'], assigned)
        bounds = mesh.get_bounds(); origin = bounds.origin; extent = bounds.box_extent
        actual = [origin.x-extent.x, origin.y-extent.y, origin.z-extent.z,
                  origin.x+extent.x, origin.y+extent.y, origin.z+extent.z]
        error = max(abs(a-b) for a,b in zip(actual, entry['bounds_cm']))
        assert error < .1, (entry['name'], 'bounds', actual, entry['bounds_cm'])
        assets.append({'name': entry['name'], 'triangles_lod0': triangles, 'uv_channels': channels,
                       'collision_boxes': collisions, 'materials': assigned, 'bounds_cm': actual,
                       'bounds_error_cm': error})
    levels = []
    for spec in read_layout()['levels']:
        path = f"{DEST}/Maps/{spec['name']}"
        world = unreal.EditorLoadingAndSavingUtils.load_map(path)
        assert isinstance(world, unreal.World), path
        static = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
        actors = {actor.get_actor_label(): actor for actor in static}
        assert len(actors) == len(static), 'Duplicate static mesh actor labels'
        assert len(actors) == len(spec['placements']) + 1, (path, len(actors), len(spec['placements']))
        for placement in spec['placements']:
            actor = actors[placement['label']]; comp = actor.static_mesh_component
            expected_path = placement.get('asset_path', f"{DEST}/Meshes/{placement['mesh']}")
            expected_mesh = unreal.load_asset(expected_path)
            assert isinstance(expected_mesh, unreal.StaticMesh), expected_path
            assert comp.get_editor_property('static_mesh') == expected_mesh, placement['label']
            assert xyz_error(actor.get_actor_location(), placement['location_cm']) < .01, placement['label']
            assert angle_error(actor.get_actor_rotation().yaw, placement['yaw_deg']) < .01, placement['label']
            assert xyz_error(actor.get_actor_scale3d(), placement.get('scale', [1,1,1])) < 1e-5, placement['label']
            assert str(comp.get_collision_profile_name()) == 'BlockAll', placement['label']
            assert comp.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS, placement['label']
        names = {p['mesh'] for p in spec['placements']}
        assert 'SM_MoleControlV2_StoneVault600' in names, 'Missing vaulted ceiling'
        assert 'SM_MoleControlV2_StoneWindowWall200' in names, 'Missing real window opening'
        starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
        assert len(starts) == 1
        assert xyz_error(starts[0].get_actor_location(), spec['player_start']) < .01
        all_cameras = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CameraActor)
        cameras_by_label = {a.get_actor_label(): a for a in all_cameras}
        assert len(all_cameras) == 2
        cameras = [verify_look_at(cameras_by_label[label], spec[key]) for label, key in
                   [('MoleV2_ReviewCamera', 'camera'), ('MoleV2_InteriorCamera', 'interior_camera')]]
        assert len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)) == 2
        forest_spec = spec['forest']; forest = actors[forest_spec['label']]
        comp = forest.static_mesh_component
        assert comp.get_editor_property('static_mesh').get_path_name() == '/Engine/BasicShapes/Plane.Plane'
        assert comp.get_material(0) == forest_mat
        assert comp.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
        assert str(comp.get_collision_profile_name()) == 'NoCollision'
        assert not comp.get_editor_property('cast_shadow')
        assert xyz_error(forest.get_actor_location(), forest_spec['location_cm']) < .01
        assert xyz_error(forest.get_actor_scale3d(), forest_spec['scale']) < .01
        rot = forest.get_actor_rotation()
        assert max(angle_error(a,b) for a,b in zip([rot.pitch,rot.yaw,rot.roll], forest_spec['rotation_deg'])) < .01
        fogs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ExponentialHeightFog)
        assert len(fogs) == 1
        fog = fogs[0].get_component_by_class(unreal.ExponentialHeightFogComponent)
        light_spec = spec['window_light']
        assert fog.get_editor_property('enable_volumetric_fog')
        assert abs(fog.get_editor_property('fog_density')-light_spec['fog_density']) < 1e-5
        spots = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SpotLight)
        assert len(spots) == 1 and spots[0].get_actor_label() == 'Lighting_WindowSunBeam'
        verify_look_at(spots[0], light_spec)
        comp = spots[0].get_component_by_class(unreal.SpotLightComponent)
        assert comp.get_editor_property('cast_volumetric_shadow')
        assert abs(comp.get_editor_property('volumetric_scattering_intensity')-light_spec['volumetric_scattering_intensity']) < 1e-5
        assert abs(comp.get_editor_property('intensity')-light_spec['intensity_candela']) < .01
        levels.append({'name': spec['name'], 'path': path, 'mesh_actors': len(actors),
                       'floor_z_cm': spec['floor_z_cm'], 'player_start': True,
                       'collision_profile': 'BlockAll (forest: NoCollision)',
                       'window_forest_volumetric_setup': True, 'cameras': cameras,
                       'verified_placements': True})
    assert protected_hashes() == baseline, 'Pre-existing maps/base kits changed during validation'
    return {'passed': True, 'verification': verification, 'engine': unreal.SystemLibrary.get_engine_version(),
            'destination': DEST, 'assets': assets, 'materials': materials, 'levels': levels,
            'total_triangles': sum(a['triangles_lod0'] for a in assets),
            'total_collision_boxes': sum(a['collision_boxes'] for a in assets),
            'protected_assets_sha256': baseline, 'preexisting_assets_unchanged': True}


if __name__ == '__main__':
    report = validate()
    (SOURCE / 'unreal_reload_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    unreal.log('MOLE_V2_UE_RELOAD_VALIDATION_PASSED')
