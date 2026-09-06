"""Fresh UE process: render actual saved leaf litter in an unsaved preview world."""
from pathlib import Path
import unreal, json
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
DEST='/Game/Nature/ForestProps/LeafLitter'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
mesh=unreal.load_asset(DEST+'/SM_LeafLitter');assert mesh
actor=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
actor.static_mesh_component.set_static_mesh(mesh)
# Engine plane and a transient matte floor material; no preview packages saved.
ground=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-.3))
ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
ground.set_actor_scale3d(unreal.Vector(30,30,1))
ground.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
sun=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,400),unreal.Rotator(-55,-35,0))
sun.light_component.set_editor_property('intensity',12.0)
sun.set_actor_rotation(unreal.Rotator(-55,-35,0),False)
sky=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,200))
sky.light_component.set_editor_property('intensity',3.0)
sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
cube=unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap')
if cube:sky.light_component.set_editor_property('cubemap',cube)
capture=unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(190,-230,300))
capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(capture.get_actor_location(),unreal.Vector(0,0,2)),False)
comp=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
comp.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
comp.set_editor_property('ortho_width',245.0)
comp.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
comp.set_editor_property('capture_every_frame',False)
settings=comp.get_editor_property('post_process_settings')
settings.set_editor_property('override_auto_exposure_method',True)
settings.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
settings.set_editor_property('override_auto_exposure_bias',True)
settings.set_editor_property('auto_exposure_bias',1.8)
comp.set_editor_property('post_process_settings',settings)
comp.set_editor_property('post_process_blend_weight',1.0)
target=unreal.RenderingLibrary.create_render_target2d(world,1200,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
comp.set_editor_property('texture_target',target)
# A real editor tick loop warms shaders, scene proxies and exposure before capture.
unreal.SystemLibrary.execute_console_command(world,'r.Streaming.FullyLoadUsedTextures 1')
unreal.SystemLibrary.execute_console_command(world,'r.TextureStreaming 0')
frames=0
def tick(delta):
    global frames
    frames+=1
    if frames==180:comp.capture_scene()
    if frames==190:
        unreal.RenderingLibrary.export_render_target(world,target,str(OUT/'Previews'),'LeafLitter_UE.png')
        (OUT/'unreal_render_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'mesh':mesh.get_path_name(),'saved_map':False,'warmup_frames':180,'image':'Previews/LeafLitter_UE.png','render_source':'UE editor SceneCapture2D; actual reloaded material and texture'},indent=2))
        unreal.log('LEAFLITTER_UE_RENDER_EXPORTED')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
