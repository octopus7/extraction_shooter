"""Read-only UE5.7 validation of quest facility assets and existing level placements.
Run in a clean editor; this loads maps but never saves packages.
"""
import json, time, traceback
from pathlib import Path
import unreal as u
root=Path(__file__).resolve().parents[2]
ls=u.get_editor_subsystem(u.LevelEditorSubsystem);actors=u.get_editor_subsystem(u.EditorActorSubsystem)
assert not u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world(),'Stop PIE before validation'
assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages(),'Save current maps before validation'
dest='/Game/Interaction/QuestFacilities';report={'assets':{},'placements':{}}
for name,size in [('SM_AIVendingMachine',(100,100,150)),('SM_ATVPowerCabinet',(70,100,160))]:
 m=u.load_asset(dest+'/'+name);assert m,name
 bounds=m.get_bounds();actual=[bounds.box_extent.x*2,bounds.box_extent.y*2,bounds.box_extent.z*2]
 assert all(abs(a-b)<.1 for a,b in zip(actual,size)),(name,actual)
 assert m.get_num_sections(0)==1 and m.get_material(0),name
 assert u.get_editor_subsystem(u.StaticMeshEditorSubsystem).get_num_uv_channels(m,0)>=1
 report['assets'][name]={'dimensions_cm':actual,'material':m.get_material(0).get_path_name()}
tex=u.load_asset(dest+'/T_ATVPowerCabinet_Palette');assert tex.get_editor_property('filter')==u.TextureFilter.TF_NEAREST
assert tex.get_editor_property('mip_gen_settings')==u.TextureMipGenSettings.TMGS_NO_MIPMAPS
for name,role in [('BP_AIVendingMachine_Field',u.TunaSweeperQuestVendingRole.FIELD_OFFER),('BP_AIVendingMachine_Base',u.TunaSweeperQuestVendingRole.BASE_RESIDENT)]:
 b=u.load_asset(dest+'/'+name);d=u.get_default_object(b.generated_class());m=d.get_editor_property('visual_mesh')
 assert d.get_editor_property('presence_role')==role and d.get_editor_property('shop_id')==2
 assert m.get_editor_property('static_mesh').get_name()=='SM_AIVendingMachine'
 assert max(abs(x) for x in m.get_editor_property('relative_location').to_tuple())<.01
 assert all(abs(x-1)<.001 for x in m.get_editor_property('relative_scale3d').to_tuple())
icon=u.load_asset('/Game/UI/Icons/T_UIIcon_RabbitToken');assert icon,'Rabbit token icon must be imported'
assert icon.get_editor_property('compression_settings')==u.TextureCompressionSettings.TC_EDITOR_ICON
def verify_map(mapname,labels):
 allactors=actors.get_all_level_actors();rows=[]
 for label in labels:
  found=[a for a in allactors if a.get_actor_label()==label];assert len(found)==1,(mapname,label,len(found));a=found[0]
  rotation=a.get_actor_rotation();assert abs(rotation.pitch)<.01 and abs(rotation.roll)<.01,(label,rotation)
  world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world();p=a.get_actor_location()
  ground=u.SystemLibrary.line_trace_single(world,p+u.Vector(0,0,30),p-u.Vector(0,0,50),u.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[a],u.DrawDebugTrace.NONE,True)
  assert ground and ground.to_tuple()[0] and abs(ground.to_tuple()[4].z-p.z)<3,(label,'Not on ground',str(p),str(ground.to_tuple()) if ground else None)
  front=p+a.get_actor_forward_vector()*130+u.Vector(0,0,85)
  obstacle=u.SystemLibrary.sphere_trace_single(world,front,front+u.Vector(.1,0,0),35,u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[a],u.DrawDebugTrace.NONE,True)
  assert not obstacle or not obstacle.to_tuple()[0],(label,'Front approach blocked')
  rows.append({'label':label,'class':a.get_class().get_name(),'position':list(a.get_actor_location().to_tuple())})
  if label=='Quest_ATV_Warehouse_Power':
   assert str(a.get_editor_property('progress_object_id'))=='facility.village.atv_warehouse'
   assert a.get_editor_property('required_item_id')==6029 and a.get_editor_property('required_item_quantity')==1
 if 'RaidVillage' in mapname:
  spawners=[a for a in allactors if isinstance(a,u.TunaSweeperSpecialVehicleSpawner)];assert len(spawners)==1
  assert str(spawners[0].get_editor_property('required_world_progress_object_id'))=='facility.village.atv_warehouse'
 report['placements'][mapname]=rows
out=root/'TunaSweeper/Saved/QuestFacilitiesWork/asset_validation.json'
report['passed']=False
out.write_text(json.dumps(report,indent=2),encoding='utf-8')
maps=[('/Game/MainRaid/RaidVillage',['Quest_Vending_Field','Quest_ATV_Warehouse_Power']),('/Game/Maps/BunkerMap',['Quest_Vending_Base'])]
state={'index':0,'loaded':False,'started':time.monotonic(),'loaded_at':0.0,'busy':False}
def finish(error=None):
 report['passed']=error is None
 if error:report['error']=error;u.log_error(error)
 out.write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('QUEST_FACILITIES_VERIFY_'+('PASS' if not error else 'FAIL'))
 u.unregister_slate_post_tick_callback(handle)
 u.EditorPythonScripting.set_keep_python_script_alive(False)
 if u.SystemLibrary.is_unattended():u.SystemLibrary.quit_editor()
def tick(dt):
 if state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['started']<120,'Map validation timeout'
  mapname,labels=maps[state['index']]
  if not state['loaded']:
   assert ls.load_level(mapname)
   state['loaded']=True;state['loaded_at']=time.monotonic()
  elif time.monotonic()-state['loaded_at']>=3:
   # Allow editor ticks to finish collision registration after switching worlds.
   verify_map(mapname,labels)
   state['index']+=1;state['loaded']=False
   if state['index']==len(maps):finish()
 except Exception:finish(traceback.format_exc())
 finally:state['busy']=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
