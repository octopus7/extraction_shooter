"""Verification of saved shallow-puddle assets; in-memory probes are never saved."""
import json
from pathlib import Path
import unreal

DEST = "/Game/Environment/ShallowPuddle"
FOOTSTEP = DEST + "/Audio/SW_ShallowPuddle_Footstep.SW_ShallowPuddle_Footstep"
REPORT = Path(__file__).resolve().parents[2] / "TunaSweeper/Saved/Automation/ShallowPuddle/assets.json"


def verify():
    paths = {
        "water": f"{DEST}/Materials/M_ShallowPuddle_Water",
        "wet": f"{DEST}/Materials/M_ShallowPuddle_WetEdge",
        "blueprint": f"{DEST}/BP_ShallowPuddle",
        "map": f"{DEST}/Maps/L_ShallowPuddle_Review",
    }
    for label, path in paths.items():
        assert unreal.EditorAssetLibrary.does_asset_exist(path), f"Missing shallow-puddle {label}: {path}"
    water = unreal.load_asset(paths["water"])
    assert water.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_MASKED
    assert water.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER
    wet = unreal.load_asset(paths["wet"])
    assert wet.get_editor_property("material_domain") == unreal.MaterialDomain.MD_DEFERRED_DECAL
    cls = unreal.EditorAssetLibrary.load_blueprint_class(paths["blueprint"])
    assert cls is not None, "Puddle Blueprint failed to load"
    defaults = unreal.get_default_object(cls)
    expected_surface = f"{DEST}/Materials/MI_ShallowPuddle_Surface.MI_ShallowPuddle_Surface"
    assert defaults.get_editor_property("water_material").get_path_name() == expected_surface
    assert defaults.get_editor_property("wet_edge_material") is not None
    sound = defaults.get_editor_property("water_footstep_sound")
    assert sound and sound.get_path_name() == FOOTSTEP, "Puddle must use its new dedicated footstep sound"
    assert isinstance(sound, unreal.SoundWave)
    assert sound.get_editor_property("num_channels") == 1
    assert sound.get_editor_property("imported_sample_rate") == 48000
    assert abs(sound.get_editor_property("duration") - 0.42) < 0.001
    assert not sound.get_editor_property("looping")
    native_sound = unreal.get_default_object(unreal.TunaSweeperShallowPuddleActor).get_editor_property("water_footstep_sound")
    assert native_sound and native_sound.get_path_name() == FOOTSTEP, "Native actor default must use the dedicated sound too"
    world = unreal.EditorLoadingAndSavingUtils.load_map(paths["map"])
    assert world, "Review map failed to reload"
    puddles = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
    assert len(puddles) >= 3, "Review map must include three puddle examples"
    for actor in puddles:
        assert actor.get_editor_property("water_material").get_path_name() == expected_surface
        assert actor.get_editor_property("water_footstep_sound").get_path_name() == FOOTSTEP
        meshes = actor.get_components_by_class(unreal.StaticMeshComponent)
        assert len(meshes) == 1
        surface = meshes[0]
        assert surface.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION
        assert surface.get_editor_property("cast_shadow") is False
        assert surface.get_material(0) is not None
        assert abs(surface.get_up_vector().z - 1.0) < 0.0001, "Water surface must stay horizontal"
        pos = actor.get_actor_location()
        assert actor.contains_ground_point(unreal.Vector(pos.x, pos.y, pos.z - 2)), "Puddle center must be wet"
        assert not actor.contains_ground_point(unreal.Vector(pos.x, pos.y, pos.z + 50)), "Elevated floor must stay dry"
    # Exercise the actual Blueprint-facing event, including invalid positions and disabled visuals.
    actor = puddles[0]
    pos = actor.get_actor_location()
    ground = unreal.Vector(pos.x, pos.y, pos.z - 2)
    events = []

    def record_step(location, speed, sprinting, instigator):
        events.append((location, speed, sprinting, instigator))

    delegate = actor.get_editor_property("on_puddle_footstep")
    delegate.add_callable(record_step)
    original_material = actor.get_editor_property("water_material")
    try:
        actor.notify_footstep(ground, 340.0, True, actor)
        assert len(events) == 1 and events[0][1:] == (340.0, True, actor)
        assert abs(events[0][0].z - pos.z) < 0.001, "Effect must use water height, not ground height"
        actor.notify_footstep(unreal.Vector(pos.x + 10000, pos.y, pos.z), 340.0, True, actor)
        actor.notify_footstep(unreal.Vector(pos.x, pos.y, pos.z + 50), 340.0, True, actor)
        assert len(events) == 1, "Dry points must not emit effects"
        actor.set_editor_property("water_material", None)
        actor.refresh_puddle()
        actor.notify_footstep(ground, 340.0, True, actor)
        assert len(events) == 1, "Invisible material-less water must not emit effects"
    finally:
        delegate.remove_callable(record_step)
        actor.set_editor_property("water_material", original_material)
        actor.refresh_puddle()
    result = {"status": "passed", "assets": paths, "review_puddles": len(puddles),
              "footstep_sound": FOOTSTEP, "sound_duration_seconds": 0.42,
              "sound_sample_rate": 48000, "sound_channels": 1}
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(result, indent=2), encoding="utf-8")
    unreal.log("SHALLOW_PUDDLE_VERIFY_PASSED " + json.dumps(result))


verify()
