"""Read-only UE 5.7 asset/configuration/placement validation. Never saves packages."""
import json
import time
import traceback
from pathlib import Path
import unreal as u

root = Path(__file__).resolve().parents[2]
out = root / 'TunaSweeper/Saved/EthanolWork/asset_validation.json'
out.parent.mkdir(parents=True, exist_ok=True)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Stop PIE before validation'
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Save maps before validation'
dest = '/Game/Interaction/QuestFacilities/Ethanol'
report = {'passed': False}

def verify():
    mesh = u.load_asset(dest + '/SM_EthanolSilo')
    assert mesh
    size = [v * 2 for v in mesh.get_bounds().box_extent.to_tuple()]
    assert all(abs(a-b) < .1 for a, b in zip(size, (280, 270, 350))), size
    assert mesh.get_num_sections(0) == 1
    assert mesh.get_material(0).get_path_name() == dest + '/M_EthanolSilo.M_EthanolSilo'
    sms = u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
    assert sms.get_num_uv_channels(mesh, 0) >= 1
    assert sms.get_simple_collision_count(mesh) >= 1
    texture = u.load_asset(dest + '/T_EthanolSilo_BaseColor')
    assert texture and texture.get_editor_property('srgb')
    bp = u.load_asset(dest + '/BP_EthanolFacility')
    cdo = u.get_default_object(bp.generated_class())
    assert isinstance(cdo, u.TunaSweeperEthanolFacilityActor)
    expected = {'progress_object_id': 'facility.village.ethanol', 'repair_quest_id': 52,
                'repair_objective_id': 'repair_ethanol', 'interaction_string_key': 'ui.ethanol.repair',
                'production_input_item_id': 3007, 'production_input_quantity': 2,
                'production_output_item_id': 6009, 'production_output_quantity': 1,
                'production_string_key': 'ui.ethanol.produce', 'missing_material_string_key': 'ui.ethanol.no_material',
                'require_active_quest': True, 'requires_item': True, 'consume_required_item': True}
    for key, value in expected.items():
        actual = cdo.get_editor_property(key)
        assert (str(actual) if isinstance(value, str) else actual) == value, (key, actual)
    req = [(x.get_editor_property('item_id'), x.get_editor_property('quantity'))
           for x in cdo.get_editor_property('required_items')]
    assert req == [(6027, 1), (6002, 2)], req
    assert cdo.get_editor_property('facility_mesh').get_editor_property('static_mesh') == mesh
    actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
    found = [a for a in actors if a.get_actor_label() == 'Quest_Ethanol_Facility']
    assert len(found) == 1, len(found)
    actor = found[0]
    assert actor.get_class() == bp.generated_class()
    p = actor.get_actor_location()
    r = actor.get_actor_rotation()
    assert abs(r.pitch) < .01 and abs(r.roll) < .01
    assert str(actor.get_editor_property('progress_object_id')) == expected['progress_object_id']
    world = editor.get_editor_world()
    ground = u.SystemLibrary.line_trace_single(world, p+u.Vector(0, 0, 30), p-u.Vector(0, 0, 50),
        u.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [actor], u.DrawDebugTrace.NONE, True)
    assert ground and ground.to_tuple()[0] and abs(ground.to_tuple()[4].z-p.z) < 3, 'Not grounded'
    front = p + actor.get_actor_forward_vector()*210 + u.Vector(0, 0, 85)
    hit = u.SystemLibrary.sphere_trace_single(world, front, front+u.Vector(.1, 0, 0), 35,
        u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [actor], u.DrawDebugTrace.NONE, True)
    assert not hit or not hit.to_tuple()[0], 'Front approach blocked'
    report.update(dimensions_cm=size, requirements=req, settings=expected, position=list(p.to_tuple()))

state = {'loaded': False, 'loaded_at': 0, 'busy': False}
def tick(dt):
    if state['busy']:
        return
    state['busy'] = True
    try:
        if not state['loaded']:
            assert u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/MainRaid/RaidVillage')
            state['loaded'] = True
            state['loaded_at'] = time.monotonic()
            return
        if time.monotonic() - state['loaded_at'] < 3:
            return
        verify()
        report['passed'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        u.log_error(report['error'])
    finally:
        state['busy'] = False
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('ETHANOL_VERIFY_' + ('PASS' if report['passed'] else 'FAIL'))
    u.unregister_slate_post_tick_callback(handle)
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    if u.SystemLibrary.is_unattended():
        u.SystemLibrary.quit_editor()

u.EditorPythonScripting.set_keep_python_script_alive(True)
handle = u.register_slate_post_tick_callback(tick)
