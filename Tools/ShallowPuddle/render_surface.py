"""Offscreen shallow/deep comparison; all world changes are transient."""
import json
import traceback
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/Saved/Automation/ShallowPuddle/Surface"
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "render.json").write_text(json.dumps({"status": "starting"}), encoding="utf-8")
BASE = "/Game/Environment/ShallowPuddle"
world = unreal.EditorLoadingAndSavingUtils.load_map(BASE + "/Maps/L_ShallowPuddle_Review")
assert world
old = unreal.load_asset(BASE + "/Materials/M_ShallowPuddle_Water")
new = unreal.load_asset(BASE + "/Materials/M_ShallowPuddle_Surface")
assert old and new
cls = unreal.EditorAssetLibrary.load_blueprint_class(BASE + "/BP_ShallowPuddle")
puddles = unreal.GameplayStatics.get_all_actors_of_class(world, cls)
puddle = min(puddles, key=lambda a: abs(a.get_actor_location().x))
for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    label = actor.get_actor_label()
    if (actor in puddles and actor != puddle) or label.startswith("Review_Stone"):
        actor.set_actor_hidden_in_game(True)
    if label == "Review_Ground":
        ground = actor
    if isinstance(actor, unreal.DirectionalLight):
        actor.light_component.set_editor_property("light_source_angle", 0.5357)
        actor.light_component.set_editor_property("light_source_soft_angle", 0.0)
puddle.set_actor_location(unreal.Vector(0, 0, 2), False, False)
puddle.set_actor_rotation(unreal.Rotator(), False)
puddle.set_editor_property("half_extent_cm", unreal.Vector2D(230, 240))
puddle.set_editor_property("water_roughness", 0.035)
puddle.set_editor_property("ripple_strength", 0.008)
puddle.set_editor_property("wetness", 0.0)

character_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter")
character = unreal.EditorLevelLibrary.spawn_actor_from_class(character_class, unreal.Vector(0, 30, 100), unreal.Rotator(yaw=180.0))
mesh = character.get_editor_property("mesh")
mesh.set_editor_property("visible_in_ray_tracing", True)
mesh.play_animation(unreal.load_asset("/Game/Characters/Player/LunaMk2/Animations/MF_Rifle_Idle_Hipfire1"), False)
mesh.set_position(0.5, False)
mesh.set_play_rate(0.0)
mesh.set_update_animation_in_editor(True)

capture = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, -430, 570))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(), unreal.Vector(0, 0, 40)), False)
camera = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
camera.set_editor_property("fov_angle", 55.0)
camera.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
camera.set_editor_property("capture_every_frame", False)
camera.set_editor_property("always_persist_rendering_state", True)
settings = camera.get_editor_property("post_process_settings")
for name, value in {
    "override_auto_exposure_method": True, "auto_exposure_method": unreal.AutoExposureMethod.AEM_MANUAL,
    "override_auto_exposure_bias": True, "auto_exposure_bias": 1.0,
    "override_auto_exposure_apply_physical_camera_exposure": True, "auto_exposure_apply_physical_camera_exposure": False,
    "override_dynamic_global_illumination_method": True, "dynamic_global_illumination_method": unreal.DynamicGlobalIlluminationMethod.LUMEN,
    "override_reflection_method": True, "reflection_method": unreal.ReflectionMethod.LUMEN,
    "override_lumen_front_layer_translucency_reflections": True, "lumen_front_layer_translucency_reflections": True,
    "override_lumen_reflection_quality": True, "lumen_reflection_quality": 2.0,
}.items():
    settings.set_editor_property(name, value)
camera.set_editor_property("post_process_settings", settings)
camera.set_editor_property("post_process_blend_weight", 1.0)
target = unreal.RenderingLibrary.create_render_target2d(world, 1200, 1000, unreal.TextureRenderTargetFormat.RTF_RGBA8)
camera.set_editor_property("texture_target", target)
for command in ("r.Lumen.HardwareRayTracing 1", "r.Streaming.FullyLoadUsedTextures 1", "r.Lumen.TranslucencyReflections.FrontLayer.Enable 1"):
    unreal.SystemLibrary.execute_console_command(world, command)

variants = [("single_layer_2cm", old, 2), ("surface_2cm", new, 2), ("single_layer_40cm", old, 40), ("surface_40cm", new, 40)]
frames = 0
index = -1
busy = False
result = {"status": "running", "captures": []}

def tick(delta):
    global frames, index, busy
    if busy:
        return
    busy = True
    try:
        frames += 1
        if frames % 120 == 1:
            index += 1
            if index >= len(variants):
                result["status"] = "rendered"
                (OUT / "render.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
                unreal.unregister_slate_post_tick_callback(handle)
                unreal.SystemLibrary.quit_editor()
                return
            name, material, depth = variants[index]
            # Keep water, subject and camera fixed: only the floor depth changes.
            ground.set_actor_location(unreal.Vector(0, 0, -8 - depth), False, False)
            puddle.set_editor_property("water_material", material)
            puddle.refresh_puddle()
            unreal.log("PUDDLE_SURFACE_VARIANT " + name)
        if frames % 4 == 0:
            camera.capture_scene()
        if frames % 120 == 115:
            name = variants[index][0]
            unreal.RenderingLibrary.export_render_target(world, target, str(OUT), name + ".png")
            result["captures"].append(name)
    except Exception:
        result.update(status="failed", error=traceback.format_exc())
        (OUT / "render.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
        unreal.log_error(result["error"])
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
    finally:
        busy = False

handle = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
