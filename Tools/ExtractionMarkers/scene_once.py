"""One-off dedicated sample and camera capture. Removed after asset commit."""
from pathlib import Path
import unreal,json,math,traceback
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers';DEST='/Game/Interaction/ExtractionMarkers';MAP=DEST+'/Maps/L_ExtractionMarkers'
m=json.loads((OUT/'model_manifest.json').read_text());actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
 assert levels.load_level(MAP)
 old=[a for a in actors.get_all_level_actors() if isinstance(a,(unreal.StaticMeshActor,unreal.CameraActor,unreal.Light,unreal.SkyLight)) or a.get_actor_label()=='ExistingExtraction_R300_SampleOnly']
 assert actors.destroy_actors(old)
else:assert levels.new_level(MAP)
def spawn(cls,name,loc,rot=None):
 a=actors.spawn_actor_from_class(cls,unreal.Vector(*loc),rot or unreal.Rotator());a.set_actor_label(name);return a
bylabel={}
for p in m['placements']:
 folder='/Game/Environment/ModularInteriorExpansion' if p['mesh']=='SM_MIE_EmergencyLight' else DEST
 a=spawn(unreal.StaticMeshActor,p['name'],[v*100 for v in p['location_m']]);c=a.static_mesh_component;c.set_static_mesh(unreal.load_asset(folder+'/Meshes/'+p['mesh']));c.set_collision_profile_name('NoCollision');c.set_mobility(unreal.ComponentMobility.STATIC)
 c.set_editor_property('generate_overlap_events',False);c.set_editor_property('can_ever_affect_navigation',False)
 for i,v in enumerate([.25,1,0,0]):c.set_default_custom_primitive_data_float(i,v)
 a.set_folder_path('ExtractionMarkers/Props');bylabel[p['name']]=a
floor=spawn(unreal.StaticMeshActor,'ReviewGround',[0,0,-6]);floor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));floor.set_actor_scale3d(unreal.Vector(50,50,.1));floor.static_mesh_component.set_collision_profile_name('BlockAll');floor.static_mesh_component.set_material(0,unreal.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
bp=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Interaction/BP_ExtractionPoint');assert bp
ex=spawn(bp,'ExistingExtraction_R300_SampleOnly',[0,0,0]);ex.set_editor_property('extraction_radius',300)
ex.set_editor_property('target_level_name','None')
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase.static_class())
# Ring and FX stay owned by the original actor implementation.
ex.set_editor_property('extraction_particle_system',unreal.load_asset('/Game/FX/NS_ExtractionSmoke'))
sun=spawn(unreal.DirectionalLight,'ReviewSun',[0,0,700],unreal.Rotator(pitch=-55,yaw=-35,roll=0));sun.light_component.set_intensity(3);sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky=spawn(unreal.SkyLight,'ReviewSky',[0,0,500]);sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP);sky.light_component.set_editor_property('cubemap',unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'));sky.light_component.set_intensity(.7)
def camera(name,loc,target,fov=50,rot=None):
 a=spawn(unreal.CameraActor,name,loc,rot or unreal.MathLibrary.find_look_at_rotation(unreal.Vector(*loc),unreal.Vector(*target)));c=a.camera_component;c.set_field_of_view(fov);c.set_aspect_ratio(4/3)
 pp=c.get_editor_property('post_process_settings')
 for k,v in {'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,'override_auto_exposure_bias':True,'auto_exposure_bias':9.,'override_bloom_intensity':True,'bloom_intensity':.15}.items():pp.set_editor_property(k,v)
 c.set_editor_property('post_process_settings',pp);c.set_editor_property('post_process_blend_weight',1);bylabel[name]=a;return a
camera('UE_Day_Assembly',[-630,-295,220],[-430,5,20],45)
camera('UE_TrueTopDown',[-430,5,440],[-430,5,0],48,unreal.Rotator(pitch=-90,yaw=0,roll=0))
camera('UE_Extraction_PlayCamera',[-202.35,0,1587.09],[-150,0,88],70,unreal.Rotator(pitch=-88,yaw=0,roll=0))
camera('UE_Extraction_Overview',[-850,-550,720],[-150,0,100],60)
assert levels.save_current_level()
# Capture editor simulation only, never trigger extraction gameplay/PIE.
for c in ex.get_components_by_class(unreal.NiagaraComponent):
 c.set_force_solo(True);c.set_age_update_mode(unreal.NiagaraAgeUpdateMode.TICK_DELTA_TIME);c.activate(True);c.advance_simulation(120,1/60)
unreal.SystemLibrary.execute_console_command(levels.get_current_level().get_outer(),'r.Streaming.FullyLoadUsedTextures 1')
report={'passed':False,'map':MAP,'sample_radius_cm':300,'prop_instances':3,'prop_triangles':m['unique_triangles'],'new_prop_light_actors':0,'existing_extraction_bp':bp.get_path_name(),'smoke_system':'/Game/FX/NS_ExtractionSmoke','capture_kind':'editor preview, no PIE'}
queue=[('UE_Day_Assembly','UE_Day_Assembly','day',True),('UE_TrueTopDown','UE_TrueTopDown','day',True),('UE_Extraction_PlayCamera','UE_Extraction_PlayCamera','day',True),('UE_Extraction_Overview','UE_Extraction_Overview','day',True),('UE_Day_Off','UE_Day_Assembly','day',False),('UE_Dark_Off','UE_Day_Assembly','dark',False),('UE_Dark_On','UE_Day_Assembly','dark',True)]
_task=None;_ticks=0;_busy=False;names=[]
def tick(dt):
 global _task,_ticks,_busy
 if _busy:return
 _busy=True;_ticks+=1
 try:
  assert _ticks<2400,'Capture timeout'
  for c in ex.get_components_by_class(unreal.NiagaraComponent):c.advance_simulation(1,1/30)
  if _ticks<100:return
  if _task and not _task.is_task_done():return
  if queue:
   name,cam,day,on=queue.pop(0);names.append(name)
   sun.light_component.set_intensity(3 if day=='day' else .08);sky.light_component.set_intensity(.7 if day=='day' else .025)
   bylabel['Beacon'].static_mesh_component.set_material(1,unreal.load_asset(DEST+'/Materials/'+('M_EM_Green' if on else 'MI_EM_GreenOff')))
   bylabel['EmergencyLight'].static_mesh_component.set_material(1,unreal.load_asset('/Game/Environment/ModularInteriorExpansion/Materials/M_MI_ExpansionAmber' if on else DEST+'/Materials/MI_EM_AmberOff'))
   _task=unreal.AutomationLibrary.take_high_res_screenshot(1200,900,str(OUT/'Previews'/(name+'.png')),camera=bylabel[cam],delay=3.0);assert _task
  else:
   assert all((OUT/'Previews'/(n+'.png')).is_file() for n in names)
   report['passed']=True;report['screenshots']=names;unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_scene_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
 except Exception:
  report['error']=traceback.format_exc();unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_scene_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
 finally:_busy=False
handle=unreal.register_slate_post_tick_callback(tick)
