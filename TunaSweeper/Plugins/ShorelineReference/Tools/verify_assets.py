"""Read-only UE 5.7 verification. Writes reports under project Saved only."""
import hashlib
import json
import math
from pathlib import Path
import traceback
import unreal as u

if 'ExecutePythonScript=' not in u.SystemLibrary.get_command_line():
    raise RuntimeError('Run this tool in a dedicated -ExecutePythonScript process; it exits that process.')

ROOT = Path(u.Paths.project_dir()).resolve() / 'Plugins/ShorelineReference'
OUT = Path(u.Paths.project_saved_dir()).resolve() / 'ShorelineReferenceWork'
OUT.mkdir(parents=True, exist_ok=True)
BASE = '/ShorelineReference'
EXPECTED = [
    '/Materials/M_ShorelineReference', '/Materials/MI_ShorelineReference',
    '/Materials/M_IslandSurface', '/Meshes/SM_ReferenceIsland',
    '/Meshes/SM_ReferenceWaterPlane', '/Textures/T_IslandShoreData',
    '/Textures/T_ShoreDissolve', '/Maps/L_ShorelineReferenceIsland',
]
report = {'status': 'running'}
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (ROOT / 'Content').rglob('*') if p.is_file()}
try:
    for suffix in EXPECTED:
        assert u.EditorAssetLibrary.does_asset_exist(BASE + suffix), 'Missing isolated asset: ' + BASE + suffix
    registry = u.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    opts = u.AssetRegistryDependencyOptions(True, True, False, False, False)
    dependencies = {}
    for asset in registry.get_assets_by_path(BASE, recursive=True):
        package = str(asset.package_name)
        deps = [str(n) for n in registry.get_dependencies(package, opts)]
        assert not any(n.startswith('/Game/') or n.startswith('/StylizedWater/') for n in deps), (package, deps)
        dependencies[package] = deps
    material = u.load_asset(BASE + '/Materials/M_ShorelineReference')
    ground = u.load_asset(BASE + '/Materials/M_IslandSurface')
    assert isinstance(u.MaterialEditingLibrary.get_material_property_input_node(ground, u.MaterialProperty.MP_BASE_COLOR), u.MaterialExpressionVertexColor)
    assert material.get_editor_property('blend_mode') == u.BlendMode.BLEND_TRANSLUCENT
    assert material.get_editor_property('shading_model') == u.MaterialShadingModel.MSM_UNLIT
    lib = u.MaterialEditingLibrary
    rgb = lib.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
    alpha = lib.get_material_property_input_node(material, u.MaterialProperty.MP_OPACITY)
    assert rgb and alpha, 'Material outputs missing'
    rgb_inputs = lib.get_inputs_for_material_expression(material, rgb)
    alpha_inputs = lib.get_inputs_for_material_expression(material, alpha)
    assert len(rgb_inputs) == 1 and len(alpha_inputs) == 1, 'Custom output disconnected'
    custom = rgb_inputs[0]
    assert isinstance(custom, u.MaterialExpressionCustom) and alpha_inputs[0] == custom
    code = custom.get_editor_property('code')
    assert code == (ROOT / 'Resources/Shaders/CalmShoreline.hlsl').read_text(encoding='utf-8')
    report['material_code_hash'] = hashlib.sha256(code.encode()).hexdigest()
    report['custom_inputs'] = [str(n.get_name()) if n else None for n in lib.get_inputs_for_material_expression(material, custom)]
    assert all(n is not None for n in lib.get_inputs_for_material_expression(material, custom))
    noise = u.load_asset(BASE + '/Textures/T_ShoreDissolve')
    data = u.load_asset(BASE + '/Textures/T_IslandShoreData')
    assert not noise.get_editor_property('srgb') and not data.get_editor_property('srgb')
    assert data.blueprint_get_size_x() == 1024 and data.blueprint_get_size_y() == 1024
    assert data.get_editor_property('address_x') == u.TextureAddress.TA_CLAMP
    assert data.get_editor_property('address_y') == u.TextureAddress.TA_CLAMP
    island = u.load_asset(BASE + '/Meshes/SM_ReferenceIsland')
    vertices, triangles, normals, uvs, tangents = u.ProceduralMeshLibrary.get_section_from_static_mesh(island, 0, 0)
    assert len(vertices) >= 66049 and len(triangles) == 131072 * 3
    errors = []
    for p in vertices:
        source_x, source_y = p.x, -p.y  # FBX handedness conversion measured in UE.
        theta = math.atan2(source_y, source_x)
        radius = 1830 + 140 * math.sin(3 * (theta + .12)) + 95 * math.cos(2 * (theta - .4)) + 45 * math.cos(5 * (theta + .3))
        distance = math.hypot(p.x, p.y) - radius
        h0 = max(-320, min(210, -.10 * distance))
        blend = max(0, min(1, (-distance - 450) / 1000))
        blend = blend * blend * (3 - 2 * blend)
        plateau = 190 + 17 * math.sin(source_x / 850) + 8 * math.cos(source_y / 600)
        expected_z = h0 * (1 - blend) + plateau * blend
        errors.append(abs(p.z - expected_z))
    report['ue_mesh_vs_mask_max_vertex_error_cm'] = max(errors)
    report['source_to_ue_axes'] = 'UE=(source X, -source Y, source Z); data UV=(UE XY+4096)/8192'
    if max(errors) >= .01:
        report['axis_probe'] = []
        for swap in [False, True]:
            for sx, sy in [(1,1),(1,-1),(-1,1),(-1,-1)]:
                max_error = 0
                for p in vertices:
                    x, y = (p.y * sx, p.x * sy) if swap else (p.x * sx, p.y * sy)
                    angle = math.atan2(y, x)
                    d = math.hypot(x, y) - (1830 + 140*math.sin(3*(angle+.12)) + 95*math.cos(2*(angle-.4)) + 45*math.cos(5*(angle+.3)))
                    t = max(0,min(1,(-d-450)/1000)); t=t*t*(3-2*t)
                    z = max(-320,min(210,-.10*d))*(1-t)+(190+17*math.sin(x/850)+8*math.cos(y/600))*t
                    max_error=max(max_error,abs(p.z-z))
                report['axis_probe'].append({'swap':swap,'sx':sx,'sy':sy,'max_error':max_error})
    assert max(errors) < .01, 'Imported UE axes do not match the baked world mask'
    world = u.EditorLoadingAndSavingUtils.load_map(BASE + '/Maps/L_ShorelineReferenceIsland')
    assert world
    assert world.get_world_settings().get_editor_property('default_game_mode') == u.GameModeBase.static_class()
    actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    water = [a for a in actors if a.get_actor_label() == 'SR_Water']
    assert len(water) == 1
    component = water[0].get_component_by_class(u.StaticMeshComponent)
    assert component.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
    assert component.get_material(0).get_path_name().startswith(BASE + '/')
    assert abs(water[0].get_actor_location().z) < 0.001
    assert abs(component.get_up_vector().z - 1) < 0.0001
    assert not any(a.get_class().get_name() == 'StylizedWaterBodyActor' for a in actors)
    report.update(status='passed', dependencies=dependencies, actors=len(actors), water_z=0,
                  saved_files_unchanged=all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in before.items()))
    assert report['saved_files_unchanged']
except Exception:
    report.update(status='failed', error=traceback.format_exc())
    u.log_error(report['error'])
finally:
    (OUT / 'asset_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    u.log('SHORELINE_ASSET_VERIFY_' + report['status'].upper())
    u.SystemLibrary.quit_editor()
