"""Read-only fresh-editor map, camera, placement and simple-collision audit."""
from pathlib import Path
import json
import math
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/LabSupplyProps'
DEST = '/Game/Environment/LabSupplyProps'
BASE = '/Game/Environment/ModularInteriorPreview'
MAP = DEST + '/Maps/L_LabSupplyProps'
manifest = json.loads((OUT / 'model_manifest.json').read_text(encoding='utf-8'))
placements = [p for p in manifest['placements'] if p['group'] in ('Lab', 'Warehouse')]
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert actors and levels, 'Run in full editor, not commandlet'
assert levels.load_level(MAP)
all_actors = actors.get_all_level_actors()
by_label = {a.get_actor_label(): a for a in all_actors}
assert not any(any(word in a.get_actor_label().lower() for word in ('ceiling', 'beam')) for a in all_actors)
meshes = [a for a in all_actors if isinstance(a, unreal.StaticMeshActor)]
assert len(meshes) == len(placements) + 2
assert len([a for a in all_actors if isinstance(a, unreal.RectLight)]) == 4
assert len([a for a in all_actors if isinstance(a, unreal.CameraActor)]) == 4


def point(p, local):
    x, y, z = [local[i] * p['scale'][i] for i in range(3)]
    angle = math.radians(p['yaw_deg'])
    location = p['location_m']
    return [100 * (location[0] + x * math.cos(angle) - y * math.sin(angle)) +
            (1200 if p['group'] == 'Warehouse' else 0),
            100 * (location[1] + x * math.sin(angle) + y * math.cos(angle)),
            100 * (location[2] + z)]


for p in placements:
    actor = by_label[p['name']]
    component = actor.static_mesh_component
    folder, prefix = (BASE, 'SM_MI_') if p['base'] else (DEST, 'SM_LSP_')
    assert component.static_mesh.get_path_name().split('.')[0] == folder + '/Meshes/' + prefix + p['key']
    location, scale, rotation = actor.get_actor_location(), actor.get_actor_scale3d(), actor.get_actor_rotation()
    assert max(abs(a - b) for a, b in zip([location.x, location.y, location.z], point(p, [0, 0, 0]))) < 0.1, p['name']
    assert max(abs(a - b) for a, b in zip([scale.x, scale.y, scale.z], p['scale'])) < 1e-4
    assert abs((rotation.yaw - p['yaw_deg'] + 180) % 360 - 180) < 0.01
    assert abs(rotation.pitch) < 0.01 and abs(rotation.roll) < 0.01
    cpd = component.get_editor_property('custom_primitive_data').get_editor_property('data')
    assert len(cpd) >= 4
    assert max(abs(a - b) for a, b in zip(cpd, [0.35, 1, *p['dirt_offset']])) < 1e-4
for group, shift in (('Lab', 0), ('Warehouse', 1200)):
    camera = by_label['LSP_' + group + '_GameCamera']
    location, rotation = camera.get_actor_location(), camera.get_actor_rotation()
    expected = [shift - 1500 * math.cos(math.radians(88)), 0, 88 + 1500 * math.sin(math.radians(88))]
    assert max(abs(a - b) for a, b in zip([location.x, location.y, location.z], expected)) < 0.1
    assert abs(rotation.pitch + 88) < 0.01 and abs(rotation.yaw) < 0.01
    assert abs(camera.camera_component.field_of_view - 70) < 0.01
    proxy = by_label['LSP_' + group + '_ScaleProxy']
    assert proxy.static_mesh_component.static_mesh.get_path_name().split('.')[0] == '/Engine/BasicShapes/Cylinder'
    bounds = proxy.static_mesh_component.static_mesh.get_bounds()
    scale = proxy.get_actor_scale3d()
    assert max(abs(a - b) for a, b in zip([2 * bounds.box_extent.x * scale.x,
               2 * bounds.box_extent.y * scale.y, 2 * bounds.box_extent.z * scale.z], [68, 68, 176])) < 0.1

report = {'passed': True, 'mode': 'fresh_editor_saved_map_reload', 'map': MAP,
          'manifest_instances': len(placements), 'sample_totals': manifest['sample_totals'],
          'rect_lights': 4, 'cameras': 4, 'scale_proxies': 2, 'proxy_dimensions_cm': [68, 68, 176],
          'transforms_mesh_paths_and_dirt_cpd_verified': True, 'roofless': True,
          'warehouse_shift_m': [12, 0, 0], 'pie_started': False,
          'simple_collision_traces': [], 'camera_route_visibility': []}


def finish_verification():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ignored = [by_label['LSP_' + group + '_ScaleProxy'] for group in ('Lab', 'Warehouse')]

    def hit(start, end):
        result = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(*start), unreal.Vector(*end),
                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignored, unreal.DrawDebugTrace.NONE, True)
        if result is None:
            return False
        if isinstance(result, unreal.HitResult):
            return True
        return next(value for value in result if isinstance(value, bool))

    def check(name, start, end, expected):
        blocked = hit(start, end)
        report['simple_collision_traces'].append({'name': name, 'start_cm': start,
                 'end_cm': end, 'blocking_hit': blocked, 'expected': expected})
        assert blocked == expected, (name, blocked, expected)

    for group, shift in (('Lab', 0), ('Warehouse', 1200)):
        check(group + '_floor', [shift + 20, 20, 50], [shift + 20, 20, -30], True)
        check(group + '_central_hall', [shift, -320, 88], [shift, 280, 88], False)
    shelf = next(p for p in placements if p['key'] == 'Shelf' and p['group'] == 'Lab')
    check('shelf_open_tier', point(shelf, [0, -0.32, 0.45]), point(shelf, [0, 0.32, 0.45]), False)
    check('shelf_board', point(shelf, [0.45, 0, 0.80]), point(shelf, [0.45, 0, 0.65]), True)
    pallet = next(p for p in placements if p['key'] == 'Pallet')
    check('pallet_fork_gap_Y', point(pallet, [-0.26, -0.52, 0.06]), point(pallet, [-0.26, 0.52, 0.06]), False)
    check('pallet_fork_gap_X', point(pallet, [-0.62, -0.21, 0.06]), point(pallet, [0.62, -0.21, 0.06]), False)
    check('pallet_deck', point(pallet, [-0.49, 0.45, 0.20]), point(pallet, [-0.49, 0.45, 0.10]), True)
    sink = next(p for p in placements if p['key'] == 'Sink')
    check('sink_basin_open_above', point(sink, [-0.35, -0.05, 1.05]), point(sink, [-0.35, -0.05, 0.70]), False)
    check('sink_basin_bottom', point(sink, [-0.35, -0.05, 1.05]), point(sink, [-0.35, -0.05, 0.60]), True)

    bp = unreal.load_asset('/Game/Characters/Player/BP_TunaSweeperPlayerCharacter')
    assert bp
    cdo = unreal.get_default_object(bp.generated_class())
    settings = cdo.get_editor_property('top_down_camera_mode_settings')
    report['project_camera'] = {'arm_cm': settings.get_editor_property('target_arm_length'),
        'pitch_deg': settings.get_editor_property('boom_rotation').pitch,
        'yaw_deg': settings.get_editor_property('boom_rotation').yaw,
        'fov_deg': settings.get_editor_property('default_fov')}
    assert abs(report['project_camera']['arm_cm'] - 1500) < 0.1
    assert abs(report['project_camera']['pitch_deg'] + 88) < 0.1
    assert abs(report['project_camera']['yaw_deg']) < 0.1
    assert abs(report['project_camera']['fov_deg'] - 70) < 0.1
    # Sample a 68 cm-wide central route at torso/head heights. These geometry
    # rays complement visual review; they do not simulate locomotion or PIE.
    for group, shift in (('Lab', 0), ('Warehouse', 1200)):
        camera_location = by_label['LSP_' + group + '_GameCamera'].get_actor_location()
        start = [camera_location.x, camera_location.y, camera_location.z]
        for x in (-0.34, 0, 0.34):
            for y in (-2.5, -0.5, 1.5, 2.65):
                for z in (0.88, 1.65):
                    end = [shift + x * 100, y * 100, z * 100]
                    blocked = hit(start, end)
                    report['camera_route_visibility'].append({'group': group, 'target_cm': end, 'occluded': blocked})
                    assert not blocked, ('camera_route_occlusion', group, end)
    report['camera_route_samples'] = len(report['camera_route_visibility'])
    unreal.log('LSP_MAP_VALIDATION_PASSED')


_ticks = 0
_busy = False


def _after_map_tick(delta):
    global _ticks, _busy
    if _busy:
        return
    _ticks += 1
    if _ticks < 30:
        return
    _busy = True
    unreal.unregister_slate_post_tick_callback(_tick_handle)
    try:
        finish_verification()
    except Exception:
        report['passed'] = False
        report['error'] = traceback.format_exc()
        unreal.log_error(report['error'])
    report['editor_ticks_before_physics_query'] = _ticks
    (OUT / 'unreal_map_reload_validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    unreal.SystemLibrary.quit_editor()


_tick_handle = unreal.register_slate_post_tick_callback(_after_map_tick)
