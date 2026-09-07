"""ONE-OFF saved-map authoring. Remove immediately after the UE asset commit."""
from pathlib import Path
import json
import math
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
DEST = '/Game/Environment/LabSupplyProps'
BASE = '/Game/Environment/ModularInteriorPreview'
MAP = DEST + '/Maps/L_LabSupplyProps'
manifest = json.loads((OUT / 'model_manifest.json').read_text(encoding='utf-8'))
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert actors and levels
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    assert levels.load_level(MAP)
    old = [a for a in actors.get_all_level_actors() if a.get_actor_label().startswith('LSP_')]
    if old:
        assert actors.destroy_actors(old)
else:
    assert levels.new_level(MAP)


def spawn(cls, name, location, rotation=None):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), rotation or unreal.Rotator())
    assert actor, name
    actor.set_actor_label(name)
    return actor


placements = [p for p in manifest['placements'] if p['group'] in ('Lab', 'Warehouse')]
for p in placements:
    folder, prefix = (BASE, 'SM_MI_') if p['base'] else (DEST, 'SM_LSP_')
    mesh = unreal.load_asset(folder + '/Meshes/' + prefix + p['key'])
    assert mesh, p['key']
    location = [100 * value for value in p['location_m']]
    location[0] += 1200 if p['group'] == 'Warehouse' else 0
    actor = spawn(unreal.StaticMeshActor, p['name'], location,
                  unreal.Rotator(pitch=0, yaw=p['yaw_deg'], roll=0))
    actor.set_actor_scale3d(unreal.Vector(*p['scale']))
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_mobility(unreal.ComponentMobility.STATIC)
    component.set_collision_profile_name('BlockAll')
    for i, value in enumerate([0.35, 1.0, *p['dirt_offset']]):
        component.set_default_custom_primitive_data_float(i, value)
    actor.set_folder_path('LabSupplyProps/' + p['group'] + ('/Room' if p['base'] else '/Props'))

proxy_mesh = unreal.load_asset('/Engine/BasicShapes/Cylinder')
assert proxy_mesh
proxy_bounds = proxy_mesh.get_bounds()
extent = proxy_bounds.box_extent
origin = proxy_bounds.origin
proxy_scale = [68 / (2 * extent.x), 68 / (2 * extent.y), 176 / (2 * extent.z)]
for group, shift in (('Lab', 0), ('Warehouse', 1200)):
    # Native cylinder used solely to visualize character scale; no gameplay actor.
    center = [shift, -70, 88]
    proxy = spawn(unreal.StaticMeshActor, 'LSP_' + group + '_ScaleProxy',
                  [center[i] - value * proxy_scale[i] for i, value in enumerate([origin.x, origin.y, origin.z])])
    proxy.static_mesh_component.set_static_mesh(proxy_mesh)
    proxy.set_actor_scale3d(unreal.Vector(*proxy_scale))
    proxy.static_mesh_component.set_collision_profile_name('NoCollision')
    proxy.set_folder_path('LabSupplyProps/' + group + '/Preview')
    for i, location in enumerate(([shift - 180, -100, 750], [shift + 180, 180, 750])):
        light = spawn(unreal.RectLight, 'LSP_' + group + '_Softbox_' + str(i), location,
                      unreal.Rotator(pitch=-90, yaw=0, roll=0))
        component = light.get_component_by_class(unreal.RectLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(1600)
        component.set_attenuation_radius(1500)
        component.set_source_width(550)
        component.set_source_height(550)
        light.set_folder_path('LabSupplyProps/' + group + '/Preview')
    target = [shift, 0, 88]
    camera = spawn(unreal.CameraActor, 'LSP_' + group + '_GameCamera',
                   [target[0] - 1500 * math.cos(math.radians(88)), target[1],
                    target[2] + 1500 * math.sin(math.radians(88))],
                   unreal.Rotator(pitch=-88, yaw=0, roll=0))
    camera.camera_component.set_field_of_view(70)
    oblique_location = [shift + 950, -1150, 1100]
    oblique = spawn(unreal.CameraActor, 'LSP_' + group + '_ObliqueCamera', oblique_location,
                    unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*oblique_location),
                                                            unreal.Vector(shift, 0, 80)))
    oblique.camera_component.set_field_of_view(55)
    for camera in (camera, oblique):
        camera.set_folder_path('LabSupplyProps/' + group + '/Preview')
        component = camera.camera_component
        component.set_aspect_ratio(1.4)
        settings = component.get_editor_property('post_process_settings')
        settings.set_editor_property('override_auto_exposure_bias', True)
        settings.set_editor_property('auto_exposure_bias', -1)
        component.set_editor_property('post_process_settings', settings)
        component.set_editor_property('post_process_blend_weight', 1)
assert levels.save_current_level()
(OUT / 'unreal_map_creation.json').write_text(json.dumps({
    'passed': True, 'map': MAP, 'manifest_instances': len(placements),
    'rooms': 2, 'room_footprint_m': [8, 8], 'warehouse_shift_m': [12, 0, 0],
    'rect_lights': 4, 'cameras': 4, 'scale_proxies': 2, 'roofless': True,
    'note': 'Map saved only; validation requires an independent full-editor process.'
}, indent=2), encoding='utf-8')
unreal.log('LSP_MAP_CREATION_PASSED')
unreal.SystemLibrary.quit_editor()
