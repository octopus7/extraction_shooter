"""Read-only saved-map and actual loot-generation checks in an isolated PIE session.

UnrealEditor.exe <uproject> -unattended -ExecutePythonScript=<this file>
-saveddirsuffix=FacilitySupplyQA. Reports to Saved/FacilitySupplyWork.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[2];WORK=ROOT/'TunaSweeper/Saved/FacilitySupplyWork'
assert u.SystemLibrary.is_unattended()
assert Path(u.Paths.project_saved_dir()).resolve().name=='Saved_FacilitySupplyQA'
MAP=ROOT/'TunaSweeper/Content/MainRaid/RaidForest.umap'
baseline=hashlib.sha256(MAP.read_bytes()).hexdigest()
expected={6002:2,6023:1,6022:1,6021:1}
data=json.loads((ROOT/'TunaSweeper/Content/Data/LootContainerContents.json').read_text(encoding='utf8'))
assert len({r['id'] for r in data})==len(data)
row=next(r for r in data if r['id']==8022)
assert {r['item_id']:r['quantity'] for r in row['items']}==expected
assert all(r['drop_chance']==10000 and 'quantity_min' not in r and 'quantity_max' not in r for r in row['items'])
es=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
u.EditorLoadingAndSavingUtils.load_map('/Game/MainRaid/RaidForest')
labels={a.get_actor_label():a for a in es.get_all_level_actors()}
label='Forest_FacilityMaterialsCache_Power';a=labels[label];source=labels['Forest_ToolCache_Power']
assert a.get_container_definition_id()==7001 and a.get_contents_id()==8022
assert source.get_contents_id()==8017
distance=(a.get_actor_location()-source.get_actor_location()).length();assert 140<distance<155
center,extent=a.get_actor_bounds(True);p=a.get_actor_location()
assert extent.x>0 and extent.y>0
report={'passed':False,'distance_from_electrical_crate_cm':distance,'fixed_contents':expected,'spawn':'placed actor; no spawn probability roll'}
start=time.monotonic();stage='settle';ticks=0;busy=False
def finish(error=None):
    report['saved_map_unchanged']=baseline==hashlib.sha256(MAP.read_bytes()).hexdigest()
    report['passed']=not error and report['saved_map_unchanged']
    if error:report['error']=error;u.log_error(error)
    (WORK/'validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('FACILITY_SUPPLY_QA_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
    global stage,ticks,busy
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-start<180,'Supply QA timeout: '+stage
        if stage=='settle' and ticks>=45:
            levels.editor_request_begin_play();stage='pie';ticks=0
        elif stage=='pie' and ticks>=100:
            game=editor.get_game_world();assert game
            boxes=[a for a in u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperLootContainerActor) if a.get_actor_label()==label];assert len(boxes)==1
            a=boxes[0];assert not a.get_editor_property('hidden')
            for index in range(100):
                result=a.build_container_instance()
                if isinstance(result,tuple):result=next(x for x in result if isinstance(x,u.TunaSweeperLootContainerInstance))
                assert result and result.capacity>=4
                assert {s.item_id:s.quantity for s in result.items}==expected,index
            report['actual_loot_generation_runs']=100
            pawn=u.GameplayStatics.get_player_pawn(game,0);assert pawn
            pawn.character_movement.disable_movement()
            p=a.get_actor_location()+u.Vector(-140,0,pawn.capsule_component.get_scaled_capsule_half_height()+2)
            pawn.set_actor_location(p,False,True)
            stage='open';ticks=0
        elif stage=='open' and ticks>=10:
            game=editor.get_game_world();pawn=u.GameplayStatics.get_player_pawn(game,0)
            a=next(a for a in u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperLootContainerActor) if a.get_actor_label()==label)
            assert a.get_interactable_component().request_interaction(pawn),'New crate interaction rejected'
            report['actual_open_interaction']=True
            finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
