"""Fresh UE editor load and screenshot in an unsaved transient map."""
import unreal, time, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse'
world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh=unreal.load_asset('/Game/Nature/ForestProps/GrassSparse/SM_GrassSparse');assert mesh
grass=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0));grass.static_mesh_component.set_static_mesh(mesh)
ground=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,-1));ground.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'));ground.set_actor_scale3d(unreal.Vector(30,30,1))
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-50,-35,0));sun.light_component.set_intensity(100)
fill=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,300),unreal.Rotator(-40,145,0));fill.light_component.set_intensity(45)
camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(150,-220,155))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,0,12)),False)
camera.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC);camera.camera_component.set_editor_property('ortho_width',180)
camera.camera_component.set_editor_property('aspect_ratio',4/3)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).pilot_level_actor(camera)
actors.clear_actor_selection_set()
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_game_view(True)
for cmd in ['viewmode lit','r.EyeAdaptationQuality 0','r.ScreenPercentage 100','r.MotionBlurQuality 0']:
    unreal.SystemLibrary.execute_console_command(world,cmd)
capture=actors.spawn_actor_from_class(unreal.SceneCapture2D,camera.get_actor_location(),camera.get_actor_rotation())
component=capture.get_component_by_class(unreal.SceneCaptureComponent2D)
component.set_editor_property('projection_type',unreal.CameraProjectionMode.ORTHOGRAPHIC)
component.set_editor_property('ortho_width',180)
component.set_editor_property('capture_source',unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
component.set_editor_property('capture_every_frame',False)
component.set_editor_property('capture_on_movement',False)
target=unreal.RenderingLibrary.create_render_target2d(world,1200,900,unreal.TextureRenderTargetFormat.RTF_RGBA8)
component.set_editor_property('texture_target',target)
start=time.monotonic();wall_start=time.time();state={'requested':False,'exported':False,'bracket':0}
def tick(dt):
    try:
        elapsed=time.monotonic()-start
        if not state['requested'] and elapsed>12:
            component.capture_scene()
            state['requested']=True
        if state['requested'] and not state['exported'] and elapsed>17:
            unreal.RenderingLibrary.export_render_target(world,target,str(OUT/'Previews'),'06_UE_Reload.png')
            state['exported']=True
        # Additional native exposure checks stay in Saved, outside deliverables.
        if state['exported'] and state['bracket']==0 and elapsed>19:
            sun.light_component.set_intensity(30);fill.light_component.set_intensity(13.5);component.capture_scene();state['bracket']=1
        if state['bracket']==1 and elapsed>22:
            unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'TunaSweeper/Saved/GrassSparseAudit'),'Exposure30.png');state['bracket']=2
        if state['bracket']==2 and elapsed>24:
            sun.light_component.set_intensity(300);fill.light_component.set_intensity(135);component.capture_scene();state['bracket']=3
        if state['bracket']==3 and elapsed>27:
            unreal.RenderingLibrary.export_render_target(world,target,str(ROOT/'TunaSweeper/Saved/GrassSparseAudit'),'Exposure300.png');state['bracket']=4
        if state['requested'] and elapsed>30:
            path=OUT/'Previews/06_UE_Reload.png'
            if path.exists() and path.stat().st_mtime>wall_start:
                (OUT/'unreal_visual_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'fresh_editor_process':True,'asset':mesh.get_path_name(),'screenshot':path.name,'unsaved_transient_map':True,'passed':True},indent=2))
                unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
        if elapsed>100:raise RuntimeError('UE screenshot timed out')
    except Exception as exc:
        unreal.log_error(str(exc));unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
