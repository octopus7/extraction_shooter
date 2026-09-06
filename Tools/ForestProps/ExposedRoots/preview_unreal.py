"""Render the reloaded actual UE material/mesh in a transient editor world; save no level."""
import unreal, time, json, traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh=unreal.load_asset('/Game/Nature/ForestProps/ExposedRoots/SM_ExposedRoots');assert mesh
actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));actor.static_mesh_component.set_static_mesh(mesh)
ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-5))
ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));ground.set_actor_scale3d(unreal.Vector(20,20,.1))
light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,500),unreal.Rotator(-45,-35,0));light.light_component.set_intensity(3)
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,300));sky.light_component.set_intensity(1)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(330,-420,330),unreal.Rotator(-31,128,0))
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
target=unreal.RenderingLibrary.create_render_target2d(world,1400,1000,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
component.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC);component.set_editor_property('ortho_width',430)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
settings=component.get_editor_property('post_process_settings')
settings.set_editor_property('override_auto_exposure_method',True);settings.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
settings.set_editor_property('override_auto_exposure_bias',True);settings.set_editor_property('auto_exposure_bias',1)
component.set_editor_property('post_process_settings',settings)
component.set_editor_property('capture_every_frame',True)
start=time.time();phase=0
def tick(delta):
    global phase
    try:
        elapsed=time.time()-start
        if elapsed>20 and phase==0:
            component.capture_scene();phase=1
        if elapsed>25:
            unreal.RenderingLibrary.export_render_target(world,target,str(OUT/'Previews'),'ExposedRoots_UE.png')
            (OUT/'unreal_render_validation.json').write_text(json.dumps({'rendered_asset':mesh.get_path_name(),'material':mesh.get_material(0).get_path_name(),'transient_world':True,'saved_levels':0},indent=2))
            unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
    except Exception:
        unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
