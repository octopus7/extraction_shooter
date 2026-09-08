"""Read-only validation of the saved normal combat test arena (UE 5.7)."""
from pathlib import Path
import hashlib
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
MAP = "/Game/Maps/NormalCombatTestMap"
REPORT = ROOT / "TunaSweeper/Saved/Automation/NormalCombatTest/map_validation.json"
MARKERS = [(1080, -850, 100), (1170, 0, 100), (900, 850, 100)]


def vec(v):
    return [v.x, v.y, v.z]


def close(actual, expected, tolerance=0.01):
    assert max(abs(a - b) for a, b in zip(actual, expected)) < tolerance, (actual, expected)


def hashes():
    paths = [ROOT / "TunaSweeper/Content/Maps/NormalCombatTestMap.umap"]
    paths.extend((ROOT / "TunaSweeper/Content/Environment/NormalCombatTest").rglob("*.uasset"))
    return {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def validate():
    before = hashes()
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    assert world
    game_mode = unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperCombatLabGameMode")
    assert world.get_world_settings().get_editor_property("default_game_mode") == game_mode
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    labels = {a.get_actor_label(): a for a in actors}
    assert len(labels) == len(actors)
    enemies = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.load_class(None, "/Script/TunaSweeper.TunaSweeperEnemyCharacter"))
    assert not enemies, "GameMode owns repeated-round enemy spawning"
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    assert len(starts) == 1
    close(vec(starts[0].get_actor_location()), [-1120, 0, 100])
    close(vec(starts[0].get_actor_forward_vector()), [1, 0, 0])
    points = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TargetPoint)
    assert len(points) == 3
    marker_report = []
    for index, position in enumerate(MARKERS, 1):
        name = "CombatLabEnemy_" + str(index)
        actor = labels[name]
        close(vec(actor.get_actor_location()), position)
        close(vec(actor.get_actor_forward_vector()), [-1, 0, 0])
        close(vec(actor.get_actor_up_vector()), [0, 0, 1])
        assert "CombatLabEnemy" in [str(tag) for tag in actor.get_editor_property("tags")]
        marker_report.append({"label": name, "position_cm": position, "yaw": 180})
    floor = labels["Arena_Floor"]
    close(vec(floor.get_actor_location()), [0, 0, -30])
    close(vec(floor.get_actor_scale3d()), [32, 26, 0.6])
    assert floor.static_mesh_component.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
    covers = [a for name, a in labels.items() if name.startswith("Cover_")]
    assert len(covers) == 4
    cover_report = []
    for cover in covers:
        component = cover.static_mesh_component
        assert str(component.get_collision_profile_name()) == "BlockAll"
        assert component.get_editor_property("can_ever_affect_navigation")
        origin, extent = cover.get_actor_bounds(False)
        assert abs(origin.z - extent.z) < 0.01
        assert origin.z + extent.z >= 100
        for position in MARKERS + [(-1120, 0, 100)]:
            # Every spawn has a generous 100cm horizontal capsule clearance.
            dx, dy = abs(position[0] - origin.x) - extent.x, abs(position[1] - origin.y) - extent.y
            assert max(dx, dy) >= 100, (cover.get_actor_label(), position)
        assert abs(origin.y) - extent.y > 100, "Keep a clear center approach"
        assert abs(origin.y) + extent.y <= 950, "Keep a clear outer flank"
        cover_report.append({"label": cover.get_actor_label(), "center_cm": vec(origin), "extent_cm": vec(extent)})
    for direction in ("North", "South", "West", "East"):
        assert labels["Perimeter_" + direction].static_mesh_component.get_collision_enabled() == unreal.CollisionEnabled.QUERY_AND_PHYSICS
    text_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TextRenderActor)
    assert len(text_actors) == 7
    for actor in text_actors:
        rotation = actor.get_actor_rotation()
        close(vec(unreal.MathLibrary.get_forward_vector(rotation)), [0, 0, 1])
        close(vec(unreal.MathLibrary.get_up_vector(rotation)), [1, 0, 0])
        close([-x for x in vec(unreal.MathLibrary.get_right_vector(rotation))], [0, 1, 0])
    volumes = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.NavMeshBoundsVolume)
    assert len(volumes) == 1
    nav_origin, nav_extent = volumes[0].get_actor_bounds(False)
    close(vec(nav_origin), [0, 0, 150])
    close(vec(nav_extent), [1700, 1400, 400])
    navs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RecastNavMesh)
    assert navs
    for nav in navs:
        assert nav.get_editor_property("runtime_generation") == unreal.RuntimeGenerationType.DYNAMIC
        assert nav.get_editor_property("force_rebuild_on_load")
    lights = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight)
    assert len(lights) == 2
    priorities = {a.get_actor_label(): a.get_component_by_class(unreal.DirectionalLightComponent).get_editor_property("forward_shading_priority") for a in lights}
    assert priorities == {"Lighting_Key": 1, "Lighting_Fill": 0}
    for name, pitch, yaw in (("Lighting_Key", -62, -28), ("Lighting_Fill", -38, 140)):
        rotation = labels[name].get_actor_rotation()
        close([rotation.pitch, rotation.yaw, rotation.roll], [pitch, yaw, 0])
    assert hashes() == before, "Read-only validation changed an asset"
    return {"passed": True, "map": MAP, "game_mode": game_mode.get_path_name(), "arena_dimensions_cm": [3200, 2600],
            "player_start_cm": [-1120, 0, 100], "enemy_markers": marker_report, "covers": cover_report,
            "text_orientation": "glyph right +Y, glyph up +X, normal +Z", "navigation": "dynamic Recast + full arena bounds",
            "light_priorities": priorities, "runtime_play_test_required": True, "assets_sha256": before}


if __name__ == "__main__":
    report = validate()
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("NORMAL_COMBAT_TEST_MAP_VALIDATION_PASSED")
