"""Verify the five split forest facility supplies through normal loot and interaction APIs.

Run UE 5.7 with -ExecutePythonScript=<this file> -unattended -saveddirsuffix=FacilitySupplyQA.
"""
import hashlib
import json
from pathlib import Path
import time
import traceback

import unreal as u


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "TunaSweeper/Content/Data"
MAP = ROOT / "TunaSweeper/Content/MainRaid/RaidForest.umap"
REPORT = ROOT / "TunaSweeper/Saved/FacilitySupplyWork/validation.json"
# contents ID -> (item ID, quantity). Placement coordinates belong to the map.
SUPPLIES = {8022: (6023, 1), 8036: (6002, 2), 8037: (6022, 1),
            8038: (6021, 1), 8039: (6032, 1)}

assert u.SystemLibrary.is_unattended()
assert Path(u.Paths.project_saved_dir()).resolve().name == "Saved_FacilitySupplyQA"
baseline = hashlib.sha256(MAP.read_bytes()).digest()
rows = json.loads((DATA / "LootContainerContents.json").read_text(encoding="utf8"))
contents = {row["id"]: row for row in rows}
assert len(contents) == len(rows)
tiers = json.loads((DATA / "LootDropChanceTiers.json").read_text(encoding="utf8"))
assert tiers["quest"] == {"story": 10000, "normal_hard": 2500}
for contents_id, (item_id, count) in SUPPLIES.items():
    assert contents[contents_id]["items"] == [
        {"item_id": item_id, "quantity": count, "drop_chance_tier": "quest"}
    ], contents_id

levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
assert levels.load_level("/Game/MainRaid/RaidForest")
start = time.monotonic()
stage = "settle"
ticks = 0
busy = False
report = {"passed": False, "supplies": []}


def finish(error=None):
    report["saved_map_unchanged"] = baseline == hashlib.sha256(MAP.read_bytes()).digest()
    report["passed"] = not error and report["saved_map_unchanged"]
    if error:
        report["error"] = error
        u.log_error(error)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2), encoding="utf8")
    u.log("FACILITY_SUPPLY_QA_" + ("PASSED" if report["passed"] else "FAILED"))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():
        levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False)
    u.SystemLibrary.quit_editor()


def approach(world, box, pawn):
    position = box.get_actor_location()
    half_height = pawn.capsule_component.get_scaled_capsule_half_height()
    radius = pawn.capsule_component.get_scaled_capsule_radius()
    trace = u.TraceTypeQuery.TRACE_TYPE_QUERY1
    args = (trace, False, [pawn], u.DrawDebugTrace.NONE, True)
    for dx, dy in ((0, -115), (0, 115), (-125, 0), (125, 0)):
        p = position + u.Vector(dx, dy, 0)
        hit = u.SystemLibrary.line_trace_single(world, p + u.Vector(0, 0, 35), p - u.Vector(0, 0, 45), *args)
        if not hit or not hit.to_tuple()[0]:
            continue
        foot = hit.to_tuple()[5]
        center = foot + u.Vector(0, 0, half_height + 2)
        blocked = u.SystemLibrary.capsule_trace_single(
            world, center, center + u.Vector(0, 0, .1), radius, half_height, *args)
        if blocked and blocked.to_tuple()[0]:
            continue
        pawn.set_actor_location(center, False, True)
        if box.get_interactable_component().is_within_interaction_distance(pawn):
            return
    raise AssertionError(("No clear interaction approach", box.get_actor_label()))


def verify(world):
    game = u.GameplayStatics.get_game_instance(world)
    game.set_active_save_slot_difficulty_stage(1, False)
    assert game.get_active_save_slot_difficulty_stage() == 1
    pawn = u.GameplayStatics.get_player_pawn(world, 0)
    assert pawn
    pawn.character_movement.disable_movement()
    boxes = u.GameplayStatics.get_all_actors_of_class(world, u.TunaSweeperLootContainerActor)
    for contents_id, (item_id, count) in SUPPLIES.items():
        matches = [a for a in boxes if a.get_contents_id() == contents_id]
        assert len(matches) == 1, contents_id
        box = matches[0]
        assert not box.get_editor_property("hidden")
        for trial in range(100):
            loot = box.build_container_instance()
            assert loot and [(s.item_id, s.quantity) for s in loot.items] == [(item_id, count)], (contents_id, trial)
        approach(world, box, pawn)
        assert box.get_interactable_component().request_interaction(pawn), contents_id
        report["supplies"].append({"contents_id": contents_id, "item_id": item_id,
                                   "quantity": count, "story_rolls": 100, "opened": True})


def tick(dt):
    global stage, ticks, busy
    if busy:
        return
    busy = True
    ticks += 1
    try:
        assert time.monotonic() - start < 180, "Supply QA timeout: " + stage
        if stage == "settle" and ticks >= 45:
            levels.editor_request_begin_play()
            stage = "pie"
            ticks = 0
        elif stage == "pie" and ticks >= 100:
            world = editor.get_game_world()
            assert world
            verify(world)
            finish()
    except Exception:
        finish(traceback.format_exc())
    finally:
        busy = False


u.EditorPythonScripting.set_keep_python_script_alive(True)
handle = u.register_slate_post_tick_callback(tick)
