"""Read-only fresh-process UE 5.7 validation; retained after importer removal."""
from pathlib import Path
import hashlib
import json
import unreal

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/FacilityRooms'
PROJECT=Path(unreal.Paths.project_dir()).resolve().parent
DEST='/Game/Environment/FacilityRooms'
TEXTURE_NAME='T_FacilityRooms_Atlas'
TEXTURE_PATH=f'{DEST}/Textures/{TEXTURE_NAME}'
MASTER_PATH=f'{DEST}/Materials/M_FacilityRooms_Master'
FOREST_TEXTURE_NAME='T_FacilityRooms_Forest'
FOREST_MATERIAL_PATH=f'{DEST}/Materials/M_FacilityRooms_ForestImpostor'
IMPORT_REPORT=SOURCE/'unreal_import_validation.json'

def read_manifest():
    return json.loads((SOURCE/'model_manifest.json').read_text(encoding='utf-8'))

def read_layout():
    return json.loads((SOURCE/'level_layout.json').read_text(encoding='utf-8'))

def material_specs(manifest):
    return {s['name']:s for s in manifest['materials'].values()}

def mesh_subsystem():
    return unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)

def protected_hashes():
    content=PROJECT/'TunaSweeper/Content'
    new=content/'Environment/FacilityRooms'
    paths=set(p for p in content.rglob('*.umap') if not p.is_relative_to(new))
    paths.update((content/'Environment/Bunker/Agit').rglob('*.uasset'))
    return {p.relative_to(PROJECT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(paths)}

def validate(manifest=None,baseline=None,verification='fresh-process reload'):
    manifest=manifest or read_manifest()
    previous=json.loads(IMPORT_REPORT.read_text(encoding='utf-8')) if baseline is None else None
    if previous:assert previous['passed']
    baseline=baseline if baseline is not None else previous['protected_assets_sha256']
    assert protected_hashes()==baseline,'Pre-existing maps or base kit changed'
    texture=unreal.load_asset(TEXTURE_PATH)
    assert isinstance(texture,unreal.Texture2D)
    assert texture.get_editor_property('srgb')
    forest_texture=unreal.load_asset(f'{DEST}/Textures/{FOREST_TEXTURE_NAME}')
    assert isinstance(forest_texture,unreal.Texture2D)
    forest_mat=unreal.load_asset(FOREST_MATERIAL_PATH)
    assert isinstance(forest_mat,unreal.Material)
    assert forest_mat.get_editor_property('two_sided')
    assert forest_mat.get_editor_property('shading_model')==unreal.MaterialShadingModel.MSM_UNLIT
    assert unreal.MaterialEditingLibrary.get_material_default_texture_parameter_value(forest_mat,'ForestTexture')==forest_texture
    master=unreal.load_asset(MASTER_PATH)
    assert isinstance(master,unreal.Material)
    lib=unreal.MaterialEditingLibrary
    assert isinstance(lib.get_material_property_input_node(master,unreal.MaterialProperty.MP_BASE_COLOR),unreal.MaterialExpressionMultiply)
    material_report=[]
    for name,spec in material_specs(manifest).items():
        mat=unreal.load_asset(f'{DEST}/Materials/{name}')
        assert isinstance(mat,unreal.MaterialInstanceConstant),name
        assert mat.get_editor_property('parent')==master
        assert 'Tint' in {str(n) for n in lib.get_vector_parameter_names(mat)}
        assert lib.get_material_instance_texture_parameter_value(mat,'Atlas')==texture
        for key,field in [('Metallic','metallic'),('Roughness','roughness'),('EmissionStrength','emission')]:
            assert abs(lib.get_material_instance_scalar_parameter_value(mat,key)-spec[field])<1e-5,(name,key)
        material_report.append({'name':name,'editable_color':'Tint','parent':MASTER_PATH})
    editor=mesh_subsystem()
    old={a['name']:a for a in previous['assets']} if previous else {}
    assets=[]
    for entry in manifest['assets']:
        mesh=unreal.load_asset(f"{DEST}/Meshes/{entry['name']}")
        assert isinstance(mesh,unreal.StaticMesh),entry['name']
        triangles=mesh.get_num_triangles(0)
        assert entry['triangles']*.8<=triangles<=entry['triangles'],(entry['name'],triangles,entry['triangles'])
        if entry['name'] in old:assert triangles==old[entry['name']]['triangles_lod0']
        channels=editor.get_num_uv_channels(mesh,0)
        collisions=editor.get_simple_collision_count(mesh)+editor.get_convex_collision_count(mesh)
        assert channels>=2,(entry['name'],'UV',channels)
        assert collisions==entry['collision_boxes'],(entry['name'],'collision',collisions)
        build=editor.get_lod_build_settings(mesh,0)
        assert build.get_editor_property('use_full_precision_u_vs')
        assert build.get_editor_property('generate_lightmap_u_vs')
        assert mesh.get_editor_property('light_map_coordinate_index')==1
        assigned=[s.get_editor_property('material_interface').get_name() for s in mesh.get_editor_property('static_materials')]
        assert set(assigned)==set(entry['materials']),entry['name']
        bounds=mesh.get_bounds();c=bounds.origin;e=bounds.box_extent
        actual=[c.x-e.x,c.y-e.y,c.z-e.z,c.x+e.x,c.y+e.y,c.z+e.z]
        error=max(abs(a-b) for a,b in zip(actual,entry['bounds_cm']))
        assert error<.1,(entry['name'],'bounds',actual,entry['bounds_cm'])
        assets.append({'name':entry['name'],'triangles_lod0':triangles,'uv_channels':channels,
                       'collision_boxes':collisions,'materials':assigned,'bounds_cm':actual,'bounds_error_cm':error})
    levels=[]
    for spec in read_layout()['levels']:
        path=f"{DEST}/Maps/{spec['name']}"
        world=unreal.EditorLoadingAndSavingUtils.load_map(path)
        assert isinstance(world,unreal.World),path
        actors={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.StaticMeshActor)}
        attic='Control' in spec['name']
        assert len(actors)==len(spec['placements'])+int(attic),(path,len(actors),len(spec['placements']))
        for placement in spec['placements']:
            actor=actors[placement['label']];comp=actor.static_mesh_component
            assert comp.get_editor_property('static_mesh').get_name()==placement['mesh']
            xyz=actor.get_actor_location();yaw=actor.get_actor_rotation().yaw;scale=actor.get_actor_scale3d()
            assert max(abs(a-b) for a,b in zip([xyz.x,xyz.y,xyz.z],placement['location_cm']))<.01
            assert abs((yaw-placement['yaw_deg']+180)%360-180)<.01
            assert max(abs(v-1) for v in [scale.x,scale.y,scale.z])<1e-5
            assert str(comp.get_collision_profile_name())=='BlockAll'
            assert comp.get_collision_enabled()==unreal.CollisionEnabled.QUERY_AND_PHYSICS
        starts=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.PlayerStart)
        assert len(starts)==1
        assert len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor))==1+int(attic)
        cameras=[]
        for camera in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.CameraActor):
            camera_spec=spec['interior_camera'] if camera.get_actor_label()=='Facility_InteriorCamera' else spec['camera']
            xyz=camera.get_actor_location();rot=camera.get_actor_rotation()
            expected=unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*camera_spec['location_cm']),unreal.Vector(*camera_spec['target_cm']))
            assert max(abs(a-b) for a,b in zip([xyz.x,xyz.y,xyz.z],camera_spec['location_cm']))<.01
            assert abs((rot.yaw-expected.yaw+180)%360-180)<.01,(camera.get_actor_label(),rot,expected)
            assert abs(rot.pitch-expected.pitch)<.01,(camera.get_actor_label(),rot,expected)
            cameras.append({'label':camera.get_actor_label(),'location':[xyz.x,xyz.y,xyz.z],'rotation':[rot.pitch,rot.yaw,rot.roll]})
        assert len(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.DirectionalLight))==2
        if 'Control' in spec['name']:
            assert all(not p['mesh'].endswith(('Wall200','Floor200','HatchLanding200')) or 'Wood' in p['mesh'] for p in spec['placements'])
            assert any('WoodGable' in p['mesh'] for p in spec['placements'])
            assert any('WoodRoof600' in p['mesh'] for p in spec['placements'])
            assert any('WoodWindowWall200' in p['mesh'] for p in spec['placements'])
            forest=actors[spec['forest']['label']];comp=forest.static_mesh_component
            assert comp.get_editor_property('static_mesh').get_path_name()=='/Engine/BasicShapes/Plane.Plane'
            assert comp.get_material(0)==forest_mat
            assert comp.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION
            assert not comp.get_editor_property('cast_shadow')
            xyz=forest.get_actor_location();scale=forest.get_actor_scale3d()
            assert max(abs(a-b) for a,b in zip([xyz.x,xyz.y,xyz.z],spec['forest']['location_cm']))<.01
            assert max(abs(a-b) for a,b in zip([scale.x,scale.y,scale.z],spec['forest']['scale']))<.01
            fogs=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.ExponentialHeightFog)
            assert len(fogs)==1
            comp=fogs[0].get_component_by_class(unreal.ExponentialHeightFogComponent)
            assert comp.get_editor_property('enable_volumetric_fog')
            assert abs(comp.get_editor_property('fog_density')-spec['window_light']['fog_density'])<1e-5
            spots=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.SpotLight)
            assert len(spots)==1 and spots[0].get_actor_label()=='Lighting_WindowSunBeam'
            comp=spots[0].get_component_by_class(unreal.SpotLightComponent)
            assert comp.get_editor_property('cast_volumetric_shadow')
            assert abs(comp.get_editor_property('volumetric_scattering_intensity')-spec['window_light']['volumetric_scattering_intensity'])<1e-5
            assert abs(comp.get_editor_property('intensity')-spec['window_light']['intensity_candela'])<.01
        levels.append({'name':spec['name'],'path':path,'mesh_actors':len(actors),'floor_z_cm':spec['floor_z_cm'],
                       'player_start':True,'collision_profile':'BlockAll (forest: NoCollision)',
                       'window_forest_volumetric_setup':attic,'cameras':cameras,'verified_placements':True})
    assert protected_hashes()==baseline,'Pre-existing map/base-kit hash changed during validation'
    return {'passed':True,'verification':verification,'engine':unreal.SystemLibrary.get_engine_version(),
            'destination':DEST,'assets':assets,'materials':material_report,'levels':levels,
            'total_triangles':sum(a['triangles_lod0'] for a in assets),
            'total_collision_boxes':sum(a['collision_boxes'] for a in assets),
            'protected_assets_sha256':baseline,'preexisting_assets_unchanged':True}

if __name__=='__main__':
    report=validate()
    (SOURCE/'unreal_reload_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('FACILITY_UE_RELOAD_VALIDATION_PASSED')
