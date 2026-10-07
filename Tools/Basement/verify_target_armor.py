"""Read-only verification of saved range armor presets and actual PIE damage/recovery.

Run in a dedicated unattended rendered editor with -ExecutePythonScript.
No map, Blueprint or item definitions are saved by this verifier.
"""
import json
from pathlib import Path
import sys
import time
import traceback
import unreal as u

command_line = u.SystemLibrary.get_command_line().lower()
if (not u.SystemLibrary.is_unattended() or '-executepythonscript' not in command_line
        or Path(__file__).name.lower() not in command_line or '-nullrhi' in command_line):
    raise RuntimeError('Run in a dedicated unattended rendered editor')
sys.path.insert(0, str(Path(__file__).parent))
from verify_target import verify_asset, BP_PATH, SOURCE

report = verify_asset()
report['passed'] = False
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
assert levels.load_level('/Game/Environment/Basement/Maps/L_Basement')
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
targets = sorted((a for a in actors if 'TS_RangeTarget' in [str(t) for t in a.tags]),
                 key=lambda a: a.get_actor_label())
assert [a.get_actor_label() for a in targets] == [f'Target_{i:02}' for i in range(1, 6)]
cls = u.load_asset(BP_PATH).generated_class()
cdo = u.get_default_object(cls)
assert cdo.get_editor_property('body_armor_tier') == 0
assert cdo.get_editor_property('head_armor_tier') == 0
placements = json.loads((SOURCE / 'RangeTarget/placement.json').read_text())['targets']
assert len(placements) == 5
report['saved_presets'] = []
for tier, (actor, placement) in enumerate(zip(targets, placements)):
    assert actor.get_class() == cls
    assert actor.get_actor_label() == placement['label']
    assert actor.get_attach_parent_actor().get_actor_label() == 'ROOT_Range'
    assert (actor.get_actor_location() - u.Vector(*placement['location'])).length() < .01
    for slot in ['body', 'head']:
        assert actor.get_editor_property(f'{slot}_armor_tier') == tier
        assert placement[f'{slot}_armor_tier'] == tier
    report['saved_presets'].append({'label': actor.get_actor_label(), 'body_tier': tier, 'head_tier': tier})

body_ids = [-1, 5010, 5001, 5022, 5024]
head_ids = [-1, 5021, 5006, 5023, 5025]
defense_table = [[0, 0, 0, 0], [1.5, .75, 0, 0], [4.5, 3, 1.5, 0],
                 [9, 6.75, 4.5, 2.25], [12, 12, 9, 6]]
state = 'settle'
ticks = 0
busy = False
started = time.monotonic()
u.EditorPythonScripting.set_keep_python_script_alive(True)


def finish(error=None):
    report['passed'] = error is None
    if error:
        report['error'] = error
        u.log_error(error)
    (SOURCE / 'target_armor_validation.json').write_text(json.dumps(report, indent=2))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.log('RANGE_TARGET_ARMOR_' + ('FAILED' if error else 'PASSED'))
    u.SystemLibrary.quit_editor()


def tick(dt):
    global ticks, busy, state, game, live, recovery_started
    if busy:
        return
    busy = True
    ticks += 1
    try:
        assert time.monotonic() - started < 180, 'Runtime verification timeout'
        if state == 'settle' and ticks >= 30:
            levels.editor_play_simulate()
            state, ticks = 'pie_wait', 0
        elif state == 'pie_wait' and ticks >= 40:
            game = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
            assert game
            live = sorted(u.GameplayStatics.get_all_actors_of_class(game, cls),
                          key=lambda a: a.get_actor_label())
            assert len(live) == 5
            report['runtime_equipment_and_damage'] = []
            for tier, actor in enumerate(live):
                assert actor.get_editor_property('body_armor_tier') == tier
                assert actor.get_editor_property('head_armor_tier') == tier
                assert actor.get_body_armor_item_id() == body_ids[tier]
                assert actor.get_head_armor_item_id() == head_ids[tier]
                assert actor.get_effective_defense(0) == tier * 3
                defenses = [actor.get_effective_defense(p) for p in range(1, 5)]
                assert defenses == defense_table[tier], (tier, defenses)
                assert abs(actor.get_health_fraction() - 1) < .0001
                assert actor.get_editor_property('health_bar_widget_component').get_user_widget_object()
                damage = u.GameplayStatics.apply_damage(actor, 20, None, None, u.DamageType)
                assert abs(damage - (20 - tier * 3)) < .001, (tier, damage)
                assert abs(actor.get_health_fraction() * 100 - (100 - damage)) < .001
                report['runtime_equipment_and_damage'].append({
                    'tier': tier, 'body_item_id': body_ids[tier], 'head_item_id': head_ids[tier],
                    'defense_by_penetration_1_to_4': defenses, 'generic_input_damage': 20,
                    'returned_damage': damage, 'health_after': actor.get_health_fraction() * 100})
            # Runtime BP changes do not change the saved preset or other targets.
            live[4].configure_practice_dummy_armor(2, 3)
            assert live[4].get_effective_defense(2) == 4.25
            live[4].configure_practice_dummy_armor(-1, 99)
            assert live[4].get_body_armor_item_id() == -1
            assert live[4].get_head_armor_item_id() == 5025
            live[4].configure_practice_dummy_armor(4, 4)
            before = live[4].get_health_fraction()
            assert u.GameplayStatics.apply_damage(live[4], 1, None, None, u.DamageType) == 0
            assert live[4].get_health_fraction() == before
            for actor in live:
                u.GameplayStatics.apply_damage(actor, 10000, None, None, u.DamageType)
                assert abs(actor.get_health_fraction() - .01) < .001
            recovery_started = u.GameplayStatics.get_time_seconds(game)
            state = 'recovery'
        elif state == 'recovery':
            elapsed = u.GameplayStatics.get_time_seconds(game) - recovery_started
            if elapsed >= 2.1:
                assert all(abs(a.get_health_fraction() - 1) < .001 for a in live)
                report['recovery'] = {'targets': len(live), 'game_seconds': elapsed, 'final_health': 100}
                levels.editor_request_end_play()
                state, ticks = 'end', 0
        elif state == 'end' and ticks >= 15:
            finish()
    except Exception:
        finish(traceback.format_exc())
    finally:
        busy = False


handle = u.register_slate_post_tick_callback(tick)
