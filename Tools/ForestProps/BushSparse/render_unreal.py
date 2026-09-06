"""UE 5.7 GPU review of reloaded assets in an unsaved transient level.

Launch the disposable audit project with -ExecCmds="py <this script>" and
-RenderOffscreen. Uses the documented AutomationLibrary screenshot task API:
https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AutomationLibrary?application_version=5.7
No level or material is saved. Exit after the three screenshots finish.
"""
import unreal,math,json,time,traceback
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse'
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
mesh=unreal.load_asset('/Game/Nature/ForestProps/BushSparse/SM_BushSparse')
assert mesh
def spawn(cls,loc,rot=unreal.Rotator()):return actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rot,transient=True)
plant=spawn(unreal.StaticMeshActor,(0,0,0));plant.static_mesh_component.set_static_mesh(mesh)
floor=spawn(unreal.StaticMeshActor,(0,0,-.05));floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Plane'));floor.set_actor_scale3d(unreal.Vector(200,200,1))
ground_material=unreal.Material()
color=unreal.MaterialEditingLibrary.create_material_expression(ground_material,unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant',unreal.LinearColor(.20,.22,.18,1))
unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(ground_material)
floor.static_mesh_component.set_material(0,ground_material)
sun=spawn(unreal.DirectionalLight,(0,0,500),unreal.Rotator(pitch=-50,yaw=-35,roll=0));sun.light_component.set_intensity(4)
fill=spawn(unreal.DirectionalLight,(0,0,400),unreal.Rotator(pitch=-40,yaw=145,roll=0));fill.light_component.set_intensity(2.5);fill.light_component.set_cast_shadows(False)
sky=spawn(unreal.SkyLight,(0,0,300));skycomp=sky.get_component_by_class(unreal.SkyLightComponent)
skycomp.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
skycomp.set_cubemap(unreal.load_asset('/Engine/EngineResources/GrayLightTextureCube'))
skycomp.set_intensity(1.2)
for component in [sun.light_component,fill.light_component,skycomp]:component.set_mobility(unreal.ComponentMobility.MOVABLE)
cam=spawn(unreal.CameraActor,(210,-270,180))
camera=cam.get_component_by_class(unreal.CameraComponent);camera.set_editor_property('projection_mode',unreal.CameraProjectionMode.PERSPECTIVE);camera.set_editor_property('aspect_ratio',1.4)
settings=camera.get_editor_property('post_process_settings')
for k,v in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':0.0,'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False}.items():settings.set_editor_property(k,v)
camera.set_editor_property('post_process_settings',settings)
unreal.AutomationLibrary.set_editor_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
unreal.AutomationLibrary.finish_loading_before_screenshot()
copies=[];index=0;frames=0;task=None;start=time.monotonic();files=[]
views=[('BushSparse_UE_Hero.png',(210,-270,180),(0,0,30),170),('BushSparse_UE_Back.png',(-180,280,160),(0,0,30),170),('BushSparse_UE_Repeated.png',(530,-720,740),(50,0,20),710)]
def tick(delta):
    global index,frames,task
    try:
        frames+=1
        if time.monotonic()-start>240:raise RuntimeError('GPU screenshot timeout')
        if task:
            if not task.is_task_done():return
            files.append(views[index][0]);index+=1;task=None;frames=0
            if index==len(views):
                for f in files:assert (OUT/'Previews'/f).exists(),f
                (OUT/'unreal_render_validation.json').write_text(json.dumps({'engine':unreal.SystemLibrary.get_engine_version(),'render':'UE GPU lit perspective viewport, reloaded uassets, unsaved transient level','screenshots':files,'mesh':mesh.get_path_name(),'passed':True},indent=2),encoding='utf-8')
                unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor();return
        if frames==1:
            name,loc,target,width=views[index]
            cam.set_actor_location(unreal.Vector(*loc),False,False)
            cam.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)),False)
            distance=math.sqrt(sum((a-b)**2 for a,b in zip(loc,target)))
            camera.set_editor_property('field_of_view',math.degrees(2*math.atan(width/(2*distance))))
            if index==2:
                for row in range(3):
                    for col in range(4):
                        if row==1 and col==1:continue
                        a=spawn(unreal.StaticMeshActor,((col-1)*135,(row-1)*130,0),unreal.Rotator(pitch=0,yaw=(row*131+col*73)%360,roll=0))
                        a.static_mesh_component.set_static_mesh(mesh);s=.88+((row+col)%4)*.08;a.set_actor_scale3d(unreal.Vector(s,s,s));copies.append(a)
        if frames>=90:
            actors.set_selected_level_actors([])
            unreal.AutomationLibrary.set_editor_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
            task=unreal.AutomationLibrary.take_high_res_screenshot(1400,1000,str(OUT/'Previews'/views[index][0]),camera=cam,force_game_view=True)
            unreal.AutomationLibrary.set_editor_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
            assert task.is_valid_task()
    except Exception:
        (OUT/'unreal_render_error.txt').write_text(traceback.format_exc(),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(tick)
