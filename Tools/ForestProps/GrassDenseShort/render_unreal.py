"""Render persisted UE assets in a transient blank world; never save a map."""
import json
import time
import traceback
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GrassDenseShort'
mesh=unreal.load_asset('/Game/Nature/ForestProps/GrassDenseShort/SM_GrassDenseShort')
assert mesh
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem) or unreal.get_default_object(unreal.EditorActorSubsystem)
def spawn(cls,xyz,rot=(0,0,0)):
    return actors.spawn_actor_from_class(cls,unreal.Vector(*xyz),unreal.Rotator(*rot),transient=True)
plant=spawn(unreal.StaticMeshActor,(0,0,0))
plant.static_mesh_component.set_static_mesh(mesh)
floor=spawn(unreal.StaticMeshActor,(0,0,-1))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'))
floor.set_actor_scale3d(unreal.Vector(200,200,1))
floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
sky=spawn(unreal.SkyLight,(0,0,250))
skycomp=sky.get_component_by_class(unreal.SkyLightComponent)
skycomp.set_mobility(unreal.ComponentMobility.MOVABLE)
skycomp.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
skycomp.set_cubemap(unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
skycomp.set_intensity(1)
for rotation,intensity in [((-48,-35,0),15.0),((-35,145,0),8.0),((-70,55,0),5.0)]:
    light=spawn(unreal.DirectionalLight,(0,0,250),rotation)
    light.light_component.set_intensity(intensity)
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
capture=spawn(unreal.SceneCapture2D,(110,-150,120))
comp=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
comp.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
comp.set_editor_property('ortho_width',110)
comp.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
comp.set_mobility(unreal.ComponentMobility.MOVABLE)
comp.set_editor_property('capture_every_frame',True)
comp.set_editor_property('always_persist_rendering_state',True)
comp.set_editor_property('capture_on_movement',False)
post=comp.get_editor_property('post_process_settings')
post.set_editor_property('override_auto_exposure_method',True)
post.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
post.set_editor_property('override_auto_exposure_bias',True)
post.set_editor_property('auto_exposure_bias',0)
post.set_editor_property('override_auto_exposure_apply_physical_camera_exposure',True)
post.set_editor_property('auto_exposure_apply_physical_camera_exposure',False)
comp.set_editor_property('post_process_settings',post)
rt=unreal.RenderingLibrary.create_render_target2d(world,1024,768,unreal.TextureRenderTargetFormat.RTF_RGBA8)
comp.set_editor_property('texture_target',rt)
unreal.SystemLibrary.execute_console_command(world,'AssetCompilingManager.FinishAllCompilation')
unreal.SystemLibrary.execute_console_command(world,'r.Streaming.FullyLoadUsedTextures 1')
# Real editor frames are required to publish the world render state. A commandlet
# can create a PNG before that state exists, so captures run on Slate ticks.
views=[('07_ue_reload_hero',(110,-150,120)),('08_ue_reload_reverse',(-110,150,100)),
       ('09_ue_camera_12m',(-600,0,1046.2305))]
state={'start':time.monotonic(),'step':0,'report':[]}
def tick(dt):
    try:
        if time.monotonic()-state['start']<8:return
        step=state['step'];index=step//2
        name,loc=views[index]
        if step%2==0:
            if index==2:
                comp.set_editor_property('projection_type',unreal.CameraProjectionMode.PERSPECTIVE)
                comp.set_editor_property('fov_angle',70)
            location=unreal.Vector(*loc)
            capture.set_actor_location_and_rotation(location,unreal.MathLibrary.find_look_at_rotation(location,unreal.Vector(0,0,7)),False,False)
            comp.capture_scene()
        else:
            pixels=[unreal.RenderingLibrary.read_render_target_pixel(world,rt,x,y) for x in range(200,900,100) for y in range(150,700,100)]
            colors={(p.r,p.g,p.b) for p in pixels}
            if index<2:assert len(colors)>8 and max(max(c) for c in colors)>30,('blank render',colors)
            unreal.RenderingLibrary.export_render_target(world,rt,str(OUT/'Previews'),name+'.png')
            camera_location=capture.get_actor_location()
            state['report'].append({'file':name+'.png','distinct_sample_colors':len(colors),
                'camera_location_cm':[camera_location.x,camera_location.y,camera_location.z]})
        state['step']+=1;state['start']=time.monotonic()
        if state['step']==len(views)*2:
            (OUT/'unreal_render_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),
                'asset':mesh.get_path_name(),'render_backend':'D3D12 offscreen editor SceneCapture2D','world_saved':False,
                'views':state['report'],'nonblank_pixel_checks':True},indent=2),encoding='utf-8')
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
    except Exception:
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
