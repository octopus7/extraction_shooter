"""One-off UE visual validation scene; removed after validated delivery."""
from pathlib import Path
import json
import traceback
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Art/CombatPatterns/Validation'
DEST = '/Game/Characters/CombatPatterns'
WORLD = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def spawn(cls, pos, rot=None):
    return ACTORS.spawn_actor_from_class(cls, unreal.Vector(*pos), rot or unreal.Rotator())

def mesh(name, pos, scale=(1,1,1), yaw=0):
    a=spawn(unreal.StaticMeshActor,pos,unreal.Rotator(yaw=yaw))
    a.static_mesh_component.set_static_mesh(unreal.load_asset(DEST+'/Meshes/'+name))
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(name)
    return a

floor=spawn(unreal.StaticMeshActor,(0,0,-12))
floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
floor.set_actor_scale3d(unreal.Vector(18,12,.2))
floor.static_mesh_component.set_material(0,unreal.load_asset(DEST+'/Materials/M_CP_Dark'))
floor.set_actor_label('Neutral review floor')

# Same asset offsets and dimensions as the native actors.
for name,offset in [('SM_CP_TurretBase',(0,0,0)),('SM_CP_TurretHead',(0,0,69)),('SM_CP_TurretTube',(0,-28,65)),('SM_CP_TurretTube',(0,28,65))]:
    mesh(name,(-470+offset[0],-70+offset[1],offset[2]))
missile=mesh('SM_CP_Missile',(-490,140,60))
mesh('SM_CP_RobotShell',(-90,-130,36))
mesh('SM_CP_RobotEye',(-90,-130,36))
mesh('SM_CP_RobotShell',(-80,90,96))
mesh('SM_CP_RobotEye',(-80,90,96))
for side in [-1,1]:
    mesh('SM_CP_RobotLeg',(-80,90+side*36*.46,35),(1,1,50/42))
    mesh('SM_CP_RobotFoot',(-72,90+side*36*.46,5))
mesh('SM_CP_ChargeChassis',(410,-70,0))

light=spawn(unreal.DirectionalLight,(0,0,700),unreal.Rotator(pitch=-55,yaw=-35))
light.light_component.set_editor_property('intensity',5.0)
light.light_component.set_editor_property('light_color',unreal.Color(255,236,210,255))
fill=spawn(unreal.DirectionalLight,(0,0,600),unreal.Rotator(pitch=-35,yaw=140))
fill.light_component.set_editor_property('intensity',2.0)
fill.light_component.set_editor_property('cast_shadows',False)
fill.light_component.set_editor_property('light_color',unreal.Color(182,224,255,255))
post=spawn(unreal.PostProcessVolume,(0,0,0))
post.set_editor_property('unbound',True)
settings=post.get_editor_property('settings')
for key,value in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':0.0,'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':False,'override_bloom_intensity':True,'bloom_intensity':0.25}.items():
    settings.set_editor_property(key,value)
post.set_editor_property('settings',settings)

warning=spawn(unreal.TunaSweeperAttackTelegraph,(-450,-65,0))
warning.init_circle(unreal.Vector(-450,-65,0),145,2)
warning.set_progress(.65)
lane=spawn(unreal.TunaSweeperAttackTelegraph,(310,-60,0))
lane.init_lane(unreal.Vector(310,-60,0),unreal.Vector(760,-60,0),55,1)
lane.set_progress(.62)

# Runtime effect classes are transient, so use gameplay spawning rather than editor placement.
fx_records=[]
for kind,pos,radius,age,direction in [
    (unreal.TunaSweeperCombatPatternEffect.IMPACT,(-410,300,0),115,.23,(0,0,1)),
    (unreal.TunaSweeperCombatPatternEffect.ROBOT_UNFOLD,(-80,90,0),36,.3,(1,0,0)),
    (unreal.TunaSweeperCombatPatternEffect.CHARGE_TRAIL,(360,-70,8),65,.35,(-1,0,0)),
    (unreal.TunaSweeperCombatPatternEffect.MISSILE_TRAIL,(-490,140,155),24,.25,(0,0,1))]:
    effect=unreal.TunaSweeperCombatPatternEffectActor.spawn(WORLD,kind,unreal.Vector(*pos),radius,unreal.Vector(*direction),None)
    effect.set_actor_tick_enabled(False)
    effect.set_life_span(0)
    effect.preview_at_normalized_age(age)
    fx_records.append(str(kind))

camera=spawn(unreal.CameraActor,(1250,-1600,1900))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(),unreal.Vector(0,30,0)),False)
camera.camera_component.set_projection_mode(unreal.CameraProjectionMode.PERSPECTIVE)
camera.camera_component.set_field_of_view(35)
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(camera.get_actor_location(),camera.get_actor_rotation())
ACTORS.clear_actor_selection_set()
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).pilot_level_actor(camera)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_game_view(True)
unreal.AutomationLibrary.set_editor_active_viewport_view_mode(unreal.ViewModeIndex.VMI_LIT)
unreal.SystemLibrary.execute_console_command(WORLD,"ShowFlag.Wireframe 0")
unreal.SystemLibrary.execute_console_command(WORLD,"ShowFlag.PostProcessing 1")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_set_viewport_realtime(True)
unreal.SystemLibrary.execute_console_command(WORLD,'r.ScreenPercentage 100')
unreal.SystemLibrary.execute_console_command(WORLD,'r.EyeAdaptationQuality 0')
state={'elapsed':0.,'captured':False}

def tick(delta):
    state['elapsed']+=delta
    if state['elapsed']>12 and not state['captured']:
        try:
            unreal.AutomationLibrary.take_high_res_screenshot(1600,1000,str(OUT/'unreal_showcase.png'),camera=camera,delay=1.0,force_game_view=False)
            state['captured']=True
        except Exception:
            (OUT/'showcase_error.txt').write_text(traceback.format_exc(),encoding='utf-8')
            unreal.unregister_slate_post_tick_callback(handle)
    if state['captured'] and state['elapsed']>22:
        (OUT/'unreal_showcase.json').write_text(json.dumps({'passed':(OUT/'unreal_showcase.png').exists(),'meshes':9,'effect_samples':fx_records},indent=2),encoding='utf-8')
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.log('COMBAT_ART_SHOWCASE_CAPTURE_COMPLETE')
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
handle=unreal.register_slate_post_tick_callback(tick)
unreal.log('COMBAT_ART_SHOWCASE_READY')
