"""Render the freshly loaded UE asset in a transient editor world; save no map."""
from pathlib import Path
import unreal,json,time
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading/Previews'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
mesh=unreal.load_asset('/Game/Nature/ForestProps/BushSpreading/SM_BushSpreading')
actor.static_mesh_component.set_static_mesh(mesh)
actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
sun=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-48,-35,0))
sun.light_component.set_intensity(8.0)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,250))
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_intensity(.6)
fill=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-35,145,0))
fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
fill.light_component.set_intensity(5.)
floor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-1))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
floor.set_actor_scale3d(unreal.Vector(200,200,1))
capture=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(240,-320,245))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(0,0,30)),False)
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
component.set_editor_property('ortho_width',290)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
settings=component.get_editor_property('post_process_settings')
for key,value in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':2.,'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False}.items():settings.set_editor_property(key,value)
component.set_editor_property('post_process_settings',settings)
target=unreal.RenderingLibrary.create_render_target2d(world,1280,960,unreal.TextureRenderTargetFormat.RTF_RGBA8)
target.set_editor_property('target_gamma',2.2)
component.set_editor_property('texture_target',target)
unreal.SystemLibrary.execute_console_command(world,'r.Streaming.FullyLoadUsedTextures 1')
# The full editor must tick to register the new world/components with the RHI.
# A one-shot commandlet capture runs before that update and returns a black image.
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started=time.monotonic();captured=False
def tick(delta):
    global captured
    elapsed=time.monotonic()-started
    if elapsed>12 and not captured:
        component.capture_scene();captured=True
    if elapsed>14:
        unreal.RenderingLibrary.export_render_target(world,target,str(OUT),'unreal_hero.png')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.log('BUSH_SPREADING_UE_RENDER_EXPORTED')
handle=unreal.register_slate_post_tick_callback(tick)
