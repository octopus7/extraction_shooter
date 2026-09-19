"""Render the saved review map offscreen; never saves or changes source assets."""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "TunaSweeper/Saved/Automation/ShallowPuddle"
OUT.mkdir(parents=True, exist_ok=True)
world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Environment/ShallowPuddle/Maps/L_ShallowPuddle_Review")
assert world
capture = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(500, -1200, 1350))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(), unreal.Vector(0, 0, 0)), False)
component = capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property("projection_type", unreal.CameraProjectionMode.PERSPECTIVE)
component.set_editor_property("fov_angle", 55.0)
component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property("capture_every_frame", False)
component.set_editor_property("always_persist_rendering_state", True)
settings = component.get_editor_property("post_process_settings")
settings.set_editor_property("override_auto_exposure_method", True)
settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
settings.set_editor_property("override_auto_exposure_bias", True)
settings.set_editor_property("auto_exposure_bias", 1.0)
settings.set_editor_property("auto_exposure_apply_physical_camera_exposure", False)
settings.set_editor_property("override_auto_exposure_apply_physical_camera_exposure", True)
component.set_editor_property("post_process_settings", settings)
component.set_editor_property("post_process_blend_weight", 1.0)
target = unreal.RenderingLibrary.create_render_target2d(world, 1440, 900, unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property("texture_target", target)
unreal.log("PUDDLE_CAPTURE_TRANSFORM " + str(capture.get_actor_transform()))
unreal.log("PUDDLE_WORLD_ACTORS " + str(len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor))))
unreal.SystemLibrary.execute_console_command(world, "r.Streaming.FullyLoadUsedTextures 1")
frames = 0


def tick(delta):
    global frames
    frames += 1
    if frames == 100:
        component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_BASE_COLOR)
        component.capture_scene()
    if frames == 110:
        unreal.RenderingLibrary.export_render_target(world, target, str(OUT), "ShallowPuddle_BaseColor.png")
        component.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
    if frames == 180:
        component.capture_scene()
    if frames == 190:
        unreal.RenderingLibrary.export_render_target(world, target, str(OUT), "ShallowPuddle_Review.png")
        unreal.log("SHALLOW_PUDDLE_RENDER_EXPORTED")
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
