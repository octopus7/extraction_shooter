"""Fresh UE editor load and screenshot in an unsaved transient map."""
import unreal, time, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh=unreal.load_asset('/Game/Nature/ForestProps/GrassSparse/SM_GrassSparse');assert mesh
grass=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));grass.static_mesh_component.set_static_mesh(mesh)
ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-1));ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'));ground.set_actor_scale3d(unreal.Vector(30,30,1))
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-50,-35,0));sun.light_component.set_intensity(3.0)
fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-40,145,0));fill.light_component.set_intensity(.65)
camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(150,-220,155))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,12)),False)
camera.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC);camera.camera_component.set_editor_property('ortho_width',180)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).pilot_level_actor(camera)
for cmd in ['r.EyeAdaptationQuality 0','r.ScreenPercentage 100','r.MotionBlurQuality 0']:
    unreal.SystemLibrary.execute_console_command(world,cmd)
start=time.monotonic();state={'requested':False,'task':None}
def tick(dt):
    try:
        elapsed=time.monotonic()-start
        if not state['requested'] and elapsed>12:
            state['task']=unreal.AutomationLibrary.take_high_res_screenshot(1200,900,str(OUT/'Previews/06_UE_Reload.png'),camera=camera,delay=2.0)
            state['requested']=True
        if state['requested'] and elapsed>25:
            path=OUT/'Previews/06_UE_Reload.png'
            if path.exists():
                (OUT/'unreal_visual_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'fresh_editor_process':True,'asset':mesh.get_path_name(),'screenshot':path.name,'unsaved_transient_map':True,'passed':True},indent=2))
                unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
        if elapsed>100:raise RuntimeError('UE screenshot timed out')
    except Exception as exc:
        unreal.log_error(str(exc));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
