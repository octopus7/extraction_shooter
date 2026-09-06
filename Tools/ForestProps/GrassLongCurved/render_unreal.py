"""Reload persisted asset and render it in a transient world, without saving a map.
Run in the editor with -ExecutePythonScript so rendered frames can advance.
"""
import unreal, json, time, traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved'
DEST='/Game/Nature/ForestProps/GrassLongCurved'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
mesh=unreal.load_asset(DEST+'/SM_GrassLongCurved');assert mesh
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem) or unreal.get_default_object(unreal.EditorActorSubsystem)
def spawn_mesh(mesh,location,scale=(1,1,1)):
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location))
    component=actor.static_mesh_component;component.set_static_mesh(mesh);component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    actor.set_actor_scale3d(unreal.Vector(*scale));return actor
plant=spawn_mesh(mesh,(0,0,0))
ground=spawn_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'),(0,0,-3),(20,20,.05))
ground.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
for light_index,(rotation,intensity) in enumerate([((-48,-35,0),3.0),((-20,55,0),1.2),((-20,145,0),1.2),((-20,235,0),1.2)]):
    light=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(*rotation))
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);light.light_component.set_intensity(intensity)
    light.light_component.set_editor_property('cast_shadows',light_index==0)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,unreal.Vector(170,-240,175))
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
component.set_editor_property('ortho_width',170.0)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame',True)
component.set_editor_property('capture_on_movement',True)
component.set_editor_property('always_persist_rendering_state',True)
settings=component.get_editor_property('post_process_settings')
for key,val in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':2.0,'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False}.items():settings.set_editor_property(key,val)
component.set_editor_property('post_process_settings',settings)
target=unreal.RenderingLibrary.create_render_target2d(world,1024,1024,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
views=[('UE_Hero',(170,-240,175),(0,0,38),170),('UE_Back',(-180,240,160),(0,0,38),170)]
def set_view(view):
    name,location,lookat,width=view
    capture.set_actor_location(unreal.Vector(*location),False,False)
    capture.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*location),unreal.Vector(*lookat)),False)
    component.set_editor_property('ortho_width',float(width))
    component.capture_scene()
report={'mesh':mesh.get_path_name(),'engine':unreal.SystemLibrary.get_engine_version(),'views':[],'world':'transient unsaved blank map','saved_existing_map':False,'rhi':'D3D11 SM5; hardware rendering; exported SceneCapture2D after editor ticks','project':'temporary content-only UE 5.7 project; byte-identical packages verified by runner','passed':False}
report_path=OUT/'unreal_render_validation.json'
report_path.write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
state={'start':time.monotonic(),'view':0}
set_view(views[0])
def tick(delta):
    if time.monotonic()-state['start']<12:return
    try:
        name=views[state['view']][0]
        # Offscreen editor windows do not necessarily drive deferred captures.
        # Capture explicitly after the scene has received its editor updates.
        component.capture_scene()
        unreal.RenderingLibrary.export_render_target(world,target,str(OUT/'Previews'),name+'.png')
        colors=[unreal.RenderingLibrary.read_render_target_pixel(world,target,x,y) for x in [256,384,512,640,768] for y in [256,384,512,640,768]]
        lit=sum(1 for c in colors if c.r+c.g+c.b>24)
        report['sample_rgb']=[[c.r,c.g,c.b] for c in colors]
        assert lit>5,('Render is blank or too dark',lit)
        assert (OUT/'Previews'/(name+'.png')).exists()
        report['views'].append({'name':name,'non_black_samples':lit})
        state['view']+=1
        if state['view']<len(views):
            set_view(views[state['view']]);state['start']=time.monotonic();return
        report['passed']=True
        unreal.log('GRASS_LONG_RENDER_PASSED')
    except Exception:
        report['error']=traceback.format_exc();unreal.log_error(report['error'])
    report_path.write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
handle=unreal.register_slate_post_tick_callback(tick)
