"""One-off SIE smoke review; dedicated map only, no travel or authored gameplay changes."""
from pathlib import Path
import unreal,json,traceback
OUT=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/ExtractionMarkers'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Interaction/ExtractionMarkers/Maps/L_ExtractionMarkers')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.GameModeBase.static_class())
ex=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='ExistingExtraction_R300_SampleOnly');ex.set_editor_property('target_level_name','None')
assert levels.save_current_level()
unreal.SystemLibrary.execute_console_command(world,'r.Streaming.FullyLoadUsedTextures 1')
unreal.EditorLevelLibrary.editor_play_simulate()
_ticks=0;_shot=None;_stage=0;_busy=False
report={'passed':False,'mode':'Simulate in Editor; dedicated GameModeBase; extraction target None','screenshots':[]}
def tick(dt):
 global _ticks,_shot,_stage,_busy
 if _busy:return
 _busy=True;_ticks+=1
 try:
  assert _ticks<1600,'SIE timeout'
  if _ticks<140:return
  if _shot and not _shot.is_task_done():return
  worlds=unreal.EditorLevelLibrary.get_pie_worlds(False);assert worlds,'No SIE world'
  pie=worlds[0]
  if _stage<2:
   name=['UE_SimulatedSmoke_Overview','UE_SimulatedSmoke_PlayCamera'][_stage];label=['UE_Extraction_Overview','UE_Extraction_PlayCamera'][_stage]
   cameras=unreal.GameplayStatics.get_all_actors_of_class(pie,unreal.CameraActor);cam=next(a for a in cameras if a.get_actor_label()==label)
   _shot=unreal.AutomationLibrary.take_high_res_screenshot(1200,900,str(OUT/'Previews'/(name+'.png')),camera=cam,delay=3.);assert _shot;report['screenshots'].append(name);_stage+=1
  else:
   report['passed']=True;unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_smoke_preview.json').write_text(json.dumps(report,indent=2));unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
 except Exception:
  report['error']=traceback.format_exc();unreal.unregister_slate_post_tick_callback(handle);(OUT/'unreal_smoke_preview.json').write_text(json.dumps(report,indent=2));unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()
 finally:_busy=False
handle=unreal.register_slate_post_tick_callback(tick)
