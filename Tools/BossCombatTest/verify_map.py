"""Read-only structural validation of the authored boss combat test level.

Run in a fresh UE 5.7 editor Python process. This script never creates or saves
assets. Runtime navigation/interaction/combat still require the play test.
"""
from pathlib import Path
import hashlib
import json
import math
import unreal

ROOT = Path(__file__).resolve().parents[2]
MAP = "/Game/Maps/BossCombatTestMap"
REPORT = ROOT / "TunaSweeper/Saved/Automation/BossCombatTest/map_validation.json"
EXPECTED = {
    "charge": {"origin": (10000, -10000), "half": 1800, "class": "TunaSweeperChargeTeachingMiniboss"},
    "robots": {"origin": (10000, 0), "half": 1800, "class": "TunaSweeperRobotTeachingMiniboss"},
    "main": {"origin": (10000, 10000), "half": 2200, "class": "TunaSweeperPatternEnemyCharacter"},
}


def vec(value):
    return [value.x, value.y, value.z]


def close(actual, expected, tolerance=0.1):
    assert len(actual) == len(expected)
    assert max(abs(a - b) for a, b in zip(actual, expected)) <= tolerance, (actual, expected)


def distance_from_box(point, origin, extent):
    return math.sqrt(sum(max(abs(a - b) - c, 0.0) ** 2 for a, b, c in zip(point[:2], origin[:2], extent[:2])))


def tracked_content_hashes():
    paths = [ROOT / "TunaSweeper/Content/Maps/BossCombatTestMap.umap"]
    paths.extend((ROOT / "TunaSweeper/Content/Environment/BossCombatTest").rglob("*.uasset"))
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def validate():
    before = tracked_content_hashes()
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    assert isinstance(world, unreal.World), "Saved map did not reload"
    expected_mode = unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperBossTestGameMode")
    assert world.get_world_settings().get_editor_property("default_game_mode") == expected_mode
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    labels = {actor.get_actor_label(): actor for actor in actors}
    assert len(labels) == len(actors), "Duplicate actor labels"
    enemy_cls = unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperEnemyCharacter")
    assert not unreal.GameplayStatics.get_all_actors_of_class(world, enemy_cls), "Bosses must be deferred until combat entry"
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    assert len(starts) == 1
    close(vec(starts[0].get_actor_location()), [-230, 0, 100])
    portals = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperWarpPointActor"))
    by_id = {str(portal.get_warp_point_id()): portal for portal in portals}
    assert len(portals) == 6 and len(by_id) == 6, "Six uniquely identified endpoints required"
    assert set(by_id) == {f"bct_{place}_{ident}" for place in ("hub", "stage") for ident in EXPECTED}
    encounters = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperBossEncounter"))
    by_encounter = {str(actor.get_editor_property("encounter_id")): actor for actor in encounters}
    assert len(encounters) == 3 and set(by_encounter) == set(EXPECTED)
    floor_report = []
    combat_report = []
    portal_report = []
    for ident, spec in EXPECTED.items():
        prefix = ident.capitalize()
        hub, stage = by_id[f"bct_hub_{ident}"], by_id[f"bct_stage_{ident}"]
        assert str(hub.get_target_warp_point_id()) == str(stage.get_warp_point_id())
        assert str(stage.get_target_warp_point_id()) == str(hub.get_warp_point_id())
        origin = spec["origin"]
        close(vec(stage.get_actor_location()), [*origin, 96])
        assert abs(stage.get_actor_rotation().yaw) < 0.1
        close(vec(stage.get_editor_property("exit_offset")), [250, 0, 0])
        for portal in (hub, stage):
            assert portal.get_editor_property("use_target_rotation")
            assert portal.get_interaction_distance() >= 180
            assert str(portal.get_interaction_display_name())
            for comp in portal.get_components_by_class(unreal.StaticMeshComponent):
                assert comp.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION, "Portal energy must not obstruct landing"
        encounter = by_encounter[ident]
        boss_cls = unreal.load_class(None, "/Script/TunaSweeper." + spec["class"])
        assert encounter.get_editor_property("boss_class") == boss_cls
        box = encounter.get_editor_property("combat_bounds")
        center, extent = vec(encounter.get_actor_location()), vec(box.get_scaled_box_extent())
        half = spec["half"]
        close(center, [origin[0] + 2200 + half, origin[1], 200])
        close(extent, [half, half, 400])
        assert box.get_editor_property("generate_overlap_events")
        assert box.get_collision_enabled() == unreal.CollisionEnabled.QUERY_ONLY
        offset = vec(encounter.get_editor_property("boss_spawn_offset"))
        spawn = [a + b for a, b in zip(center, offset)]
        assert spawn[2] >= 105, "Carrier spawn must clear its 100cm capsule half-height"
        assert all(abs(o) + margin < e for o, margin, e in zip(offset, [150, 150, 5], extent))
        landing = [origin[0] + 250, origin[1], 96]
        margin = distance_from_box(landing, center, extent)
        assert margin >= 1900, "Warp arrival must have at least 19m before combat"
        # The entire safe platform, including its north edge, remains outside combat.
        assert center[0] - extent[0] - (origin[0] + 900) >= 1200
        close(vec(labels[prefix + "_CombatThreshold"].get_actor_location()), [origin[0] + 2200, origin[1], 3])
        for name, dimensions in ((prefix + "_StageFloor", [1800, 2200, 60]),
                                 (prefix + "_ApproachFloor", [1300, 700, 60]),
                                 (prefix + "_ArenaFloor", [half * 2, half * 2, 60])):
            actor = labels[name]
            component = actor.static_mesh_component
            assert component.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
            assert str(component.get_collision_profile_name()) == "BlockAll"
            assert component.get_editor_property("can_ever_affect_navigation")
            close([value * 100 for value in vec(actor.get_actor_scale3d())], dimensions)
            assert abs(actor.get_actor_location().z + 30) < 0.1
            floor_report.append({"label": name, "dimensions_cm": dimensions, "top_z_cm": 0})
        covers = [a for label, a in labels.items() if label.startswith(prefix + "_Cover_")]
        assert len(covers) == 4
        for cover in covers:
            assert abs(cover.get_actor_location().y - origin[1]) >= half - 400, "Cover must preserve central dodge path"
            assert cover.static_mesh_component.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
        # Perimeter collision blocks running into void and entering from behind.
        for label in (prefix + "_Arena_WallNorth", prefix + "_Arena_WallWest", prefix + "_Arena_WallEast",
                      prefix + "_Arena_WallSouth_-1", prefix + "_Arena_WallSouth_1",
                      prefix + "_ApproachWall_-1", prefix + "_ApproachWall_1", prefix + "_StageSouthWall"):
            assert labels[label].static_mesh_component.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
        combat_report.append({"id": ident, "class": boss_cls.get_path_name(), "bounds_center_cm": center,
                              "bounds_extent_cm": extent, "spawn_cm": spawn, "arrival_margin_cm": margin,
                              "deferred_spawn": True, "cover_keeps_central_lane_clear": True})
        portal_report.append({"hub_id": str(hub.get_warp_point_id()), "stage_id": str(stage.get_warp_point_id()),
                              "arrival_cm": landing, "return_is_reciprocal": True})
    volumes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.NavMeshBoundsVolume)
    assert len(volumes) == 4, "Hub plus each isolated arena need navigation bounds"
    nav_report = []
    for volume in volumes:
        center, extent = volume.get_actor_bounds(False)
        assert min(extent.x, extent.y) >= 1100 and extent.z >= 350
        nav_report.append({"label": volume.get_actor_label(), "center_cm": vec(center), "extent_cm": vec(extent)})
    for ident, spec in EXPECTED.items():
        nav = next(item for item in nav_report if item["label"] == ident.capitalize() + "_Navigation")
        lower = [a - b for a, b in zip(nav["center_cm"], nav["extent_cm"])]
        upper = [a + b for a, b in zip(nav["center_cm"], nav["extent_cm"])]
        ox, oy = spec["origin"]
        half = spec["half"]
        assert lower[0] <= ox - 900 and upper[0] >= ox + 2200 + half * 2
        assert lower[1] <= oy - half and upper[1] >= oy + half
        assert lower[2] < 0 < upper[2], "Navigation must include continuous stage, approach and arena surfaces"
    nav_data = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RecastNavMesh)
    assert nav_data, "No persisted Recast navigation configuration"
    for nav in nav_data:
        assert nav.get_editor_property("runtime_generation") == unreal.RuntimeGenerationType.DYNAMIC
        assert nav.get_editor_property("force_rebuild_on_load")
    mapping_context = unreal.load_asset("/Game/Input/IMC_Player")
    mappings = [{"action": item.get_editor_property("action").get_name(),
                 "key": str(item.get_editor_property("key").get_editor_property("key_name"))}
                for item in mapping_context.get_editor_property("default_key_mappings").get_editor_property("mappings")]
    assert any(item == {"action": "IA_Roll", "key": "SpaceBar"} for item in mappings), ("Hub dodge instruction must match real controls", mappings)
    text_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor)
    assert len(text_actors) >= 25
    for actor in text_actors:
        rotation = actor.get_component_by_class(unreal.TextRenderComponent).get_world_rotation()
        # TextRender's normal is +X, glyph up is +Z and glyph right is -Y.
        close(vec(unreal.MathLibrary.get_forward_vector(rotation)), [0, 0, 1], 0.001)
        close(vec(unreal.MathLibrary.get_up_vector(rotation)), [1, 0, 0], 0.001)
        close([-v for v in vec(unreal.MathLibrary.get_right_vector(rotation))], [0, 1, 0], 0.001)
    lights = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    assert len(lights) == 2
    priorities = {light.get_actor_label(): light.get_component_by_class(unreal.DirectionalLightComponent).get_editor_property("forward_shading_priority") for light in lights}
    assert priorities == {"Lighting_Key": 1, "Lighting_Fill": 0}, "The forward/translucency light must have an unambiguous priority"
    assert tracked_content_hashes() == before, "Read-only validation changed assets"
    return {"passed": True, "verification": "fresh-process saved asset reload; structural checks only",
            "map": MAP, "engine": unreal.SystemLibrary.get_engine_version(), "game_mode": expected_mode.get_path_name(),
            "encounters": combat_report, "portals": portal_report, "floors": floor_report,
            "navigation_bounds": nav_report, "navigation_mode": "Dynamic Recast, rebuild on load",
            "input_mappings": mappings,
            "directional_light_forward_priorities": priorities,
            "ground_text_basis": {"normal": "+Z", "glyph_up": "+X", "glyph_right": "+Y", "verified_labels": len(text_actors)},
            "runtime_play_test_required": True, "assets_sha256": before}


if __name__ == "__main__":
    report = validate()
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("BOSS_COMBAT_TEST_MAP_VALIDATION_PASSED")
