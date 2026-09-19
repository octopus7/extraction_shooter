"""Read saved Niagara assets and exercise the Blueprint in an unsaved review world."""
import json
import traceback
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/Saved/Automation/ShallowPuddle"
BASE = "/Game/Environment/ShallowPuddle"
system = unreal.load_asset(BASE + "/FX/NS_ShallowPuddle_Footstep")
assert isinstance(system, unreal.NiagaraSystem)
for name in ("Ripple", "Splash"):
    material = unreal.load_asset(BASE + "/Materials/M_ShallowPuddle_" + name)
    assert material and material.get_editor_property("two_sided")
    assert material.get_editor_property("shading_model") == unreal.MaterialShadingModel.MSM_UNLIT
    unreal.log("PUDDLE_MATERIAL " + name + " opacity=" + str(unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_OPACITY))
               + " emissive=" + str(unreal.MaterialEditingLibrary.get_material_property_input_node(material, unreal.MaterialProperty.MP_EMISSIVE_COLOR)))

world = unreal.EditorLoadingAndSavingUtils.load_map(BASE + "/Maps/L_ShallowPuddle_Review")
cls = unreal.EditorAssetLibrary.load_blueprint_class(BASE + "/BP_ShallowPuddle")
puddles = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
assert len(puddles) == 3
OUT.mkdir(parents=True, exist_ok=True)

def components():
    return [c for owner in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
            for c in owner.get_components_by_class(unreal.NiagaraComponent)
            if c.get_asset() == system]

capture = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(500, -900, 1100))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(), unreal.Vector()), False)
camera = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
camera.set_editor_property("fov_angle", 55.0)
camera.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
camera.set_editor_property("capture_every_frame", False)
camera.set_editor_property("always_persist_rendering_state", True)
settings = camera.get_editor_property("post_process_settings")
for name, value in {
    "override_auto_exposure_method": True,
    "auto_exposure_method": unreal.AutoExposureMethod.AEM_MANUAL,
    "override_auto_exposure_bias": True,
    "auto_exposure_bias": 1.0,
    "override_auto_exposure_apply_physical_camera_exposure": True,
    "auto_exposure_apply_physical_camera_exposure": False,
}.items():
    settings.set_editor_property(name, value)
camera.set_editor_property("post_process_settings", settings)
target = unreal.RenderingLibrary.create_render_target2d(world, 1440, 900, unreal.TextureRenderTargetFormat.RTF_RGBA8)
camera.set_editor_property("texture_target", target)
frame = 0
in_tick = False
spawned = []
result = {"status": "running", "steps": []}

def footstep(actor, sprinting=False, offset=0.0):
    pos = actor.get_actor_location()
    before = set(components())
    actor.notify_footstep(unreal.Vector(pos.x, pos.y + offset, pos.z - 2), 340.0 if sprinting else 200.0, sprinting, actor)
    created = list(set(components()) - before)
    assert len(created) == 1, f"Expected one component, got {len(created)}"
    c = created[0]
    strength, valid = c.get_variable_float("User.StepStrength")
    assert valid and abs(strength - (1.4 if sprinting else 1.0)) < 0.0001
    assert abs(c.get_world_location().z - pos.z - 0.3) < 0.001
    unreal.log("PUDDLE_SPAWN_ACTIVE " + str(c.is_active()))
    return c

def tick(delta):
    global frame, in_tick, world, puddles, camera, target
    if in_tick:
        return
    in_tick = True
    frame += 1
    try:
        if frame == 90:
            unreal.EditorLevelLibrary.editor_play_simulate()
        if frame == 180:
            world = unreal.EditorLevelLibrary.get_game_world()
            assert world, "PIE world must be running"
            unreal.GameplayStatics.set_game_paused(world, True)
            puddles = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
            camera = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.SceneCapture2D)[0].get_component_by_class(unreal.SceneCaptureComponent2D)
            target = camera.get_editor_property("texture_target")
            for actor in puddles:
                for sprinting, offset in [(False, -45.0), (True, 45.0)]:
                    c = footstep(actor, sprinting, offset)
                    spawned.append(c)
                    result["steps"].append({"puddle": actor.get_actor_label(), "sprinting": sprinting})
        if frame == 185:
            for c in spawned:
                unreal.log("PUDDLE_DELAYED_ACTIVE " + str(c.is_active()))
                assert c.is_active(), "Blueprint must activate after Niagara compilation is ready"
                c.advance_simulation(12, 1.0 / 60.0)
                scratch_cache = unreal.NiagaraSimCacheFunctionLibrary.create_niagara_sim_cache(world)
                cache = unreal.NiagaraSimCacheFunctionLibrary.capture_niagara_sim_cache_immediate(
                    scratch_cache, unreal.NiagaraSimCacheCreateParameters(), c)
                assert cache, "Could not inspect simulated particles"
                for emitter in cache.get_emitter_names():
                    unreal.log("PUDDLE_PARTICLES " + str(emitter) + " pos=" + str(cache.read_position_attribute("Position", emitter))
                               + " size=" + str(cache.read_vector2_attribute("SpriteSize", emitter))
                               + " color=" + str(cache.read_color_attribute("Color", emitter)))
                    if str(emitter) == "Ripple":
                        unreal.log("PUDDLE_FACING " + str(cache.read_vector_attribute("SpriteFacing", emitter)) + " alignment=" + str(cache.read_vector_attribute("SpriteAlignment", emitter)))
                unreal.log("PUDDLE_RENDER_FLAGS " + str({n: c.get_editor_property(n) for n in ("visible", "hidden_in_game", "render_in_main_pass", "hidden_in_scene_capture")}))
                c.set_paused(True)
        if frame == 190:
            camera.capture_scene()
        if frame == 195:
            unreal.RenderingLibrary.export_render_target(world, target, str(OUT), "ShallowPuddle_Niagara.png")
            camera.set_world_location(unreal.Vector(0, -250, 450), False, False)
            camera.set_world_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_world_location(), unreal.Vector(0, 0, 6)), False, False)
        if frame == 200:
            camera.capture_scene()
        if frame == 205:
            unreal.RenderingLibrary.export_render_target(world, target, str(OUT), "ShallowPuddle_Niagara_Close.png")
            for c in components():
                c.set_paused(False)
                c.advance_simulation(120, 1.0 / 60.0)
            unreal.log("PUDDLE_AFTER_TWO_SECONDS " + str([(c.get_path_name(), c.is_active()) for c in components()]))
            assert not components(), "AutoDestroy must remove completed components"
            peak = 0
            for sim_frame in range(3600):
                if sim_frame % 20 == 0:
                    footstep(puddles[0], True)
                peak = max(peak, len(components()))
                for c in components():
                    c.advance_simulation(1, 1.0 / 60.0)
            for c in components():
                c.advance_simulation(120, 1.0 / 60.0)
            assert peak <= 4 and not components(), "One-minute sprint must have bounded component lifetime"
            result["simulated_sprint_seconds"] = 60
            result["sprint_footsteps"] = 180
            result["peak_components"] = peak
            result["remaining_components"] = len(components())
            result["status"] = "passed"
            (OUT / "niagara.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
            unreal.log("SHALLOW_PUDDLE_NIAGARA_VERIFY_PASSED " + json.dumps(result))
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
    except Exception:
        result["status"] = "failed"
        result["error"] = traceback.format_exc()
        (OUT / "niagara.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        unreal.log_error(result["error"])
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    finally:
        in_tick = False

handle = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
