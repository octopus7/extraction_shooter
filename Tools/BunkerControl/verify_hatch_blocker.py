"""Verify the saved invisible hatch blocker with the real player capsule in PIE.

Dedicated rendered editor: -unattended -ExecutePythonScript=<this file>
-saveddirsuffix=BunkerHatchQA. No map/asset saves or regular gameplay saves.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/BunkerControl/hatch_collision_validation.json'
assert u.SystemLibrary.is_unattended()
assert Path(u.Paths.project_saved_dir()).resolve().name=='Saved_BunkerHatchQA'
def protected_hashes():
    paths=[ROOT/'TunaSweeper/Content/Maps/BunkerMap.umap']+list((ROOT/'TunaSweeper/Saved/SaveGames').rglob('*'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
baseline=protected_hashes()
es=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
u.EditorLoadingAndSavingUtils.load_map('/Game/Maps/BunkerMap')
labels={a.get_actor_label():a for a in es.get_all_level_actors()}
saved=labels['Bunker_BasementHatchVoidBlocker'];mesh=saved.static_mesh_component
assert saved.get_editor_property('hidden') and mesh.get_editor_property('hidden_in_game')
assert not mesh.get_editor_property('cast_shadow')
assert mesh.get_collision_enabled()==u.CollisionEnabled.QUERY_AND_PHYSICS
assert mesh.get_collision_response_to_channel(u.CollisionChannel.ECC_PAWN)==u.CollisionResponseType.ECR_BLOCK
for channel in [u.CollisionChannel.ECC_CAMERA,u.CollisionChannel.ECC_VISIBILITY]:
    assert mesh.get_collision_response_to_channel(channel)==u.CollisionResponseType.ECR_IGNORE
report={'passed':False,'hidden_pawn_only_collision':True,'swept_directions':[]}
start=time.monotonic();stage='settle';ticks=0;busy=False
def finish(error=None):
    report['map_and_regular_saves_unchanged']=baseline==protected_hashes()
    report['passed']=not error and report['map_and_regular_saves_unchanged']
    if error:report['error']=error;u.log_error(error)
    OUT.write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('BUNKER_HATCH_COLLISION_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def tick(dt):
    global stage,ticks,busy
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-start<180,'Hatch QA timeout: '+stage
        if stage=='settle' and ticks>=45:
            levels.editor_request_begin_play();stage='pie';ticks=0
        elif stage=='pie' and ticks>=100:
            game=editor.get_game_world();assert game
            pawn=u.GameplayStatics.get_player_pawn(game,0);assert pawn
            actors=u.GameplayStatics.get_all_actors_of_class(game,u.Actor)
            labels={a.get_actor_label():a for a in actors};blocker=labels['Bunker_BasementHatchVoidBlocker']
            pawn.character_movement.disable_movement()
            # Isolate this collider to prove the invisible actor blocks all four approaches.
            original=[(a,a.get_actor_enable_collision()) for a in actors if a not in [pawn,blocker]]
            for a,enabled in original:a.set_actor_enable_collision(False)
            p=blocker.get_actor_location();z=12+pawn.capsule_component.get_scaled_capsule_half_height()+2
            center=u.Vector(p.x,p.y,z)
            for name,direction in [('north',u.Vector(1,0,0)),('south',u.Vector(-1,0,0)),('east',u.Vector(0,1,0)),('west',u.Vector(0,-1,0))]:
                pawn.set_actor_location(center+direction*300,False,True)
                # Negative control: without the new collider the capsule reaches the hole.
                blocker.set_actor_enable_collision(False);pawn.set_actor_location(center,True,False)
                assert (pawn.get_actor_location()-center).length()<.1
                pawn.set_actor_location(center+direction*300,False,True)
                blocker.set_actor_enable_collision(True);pawn.set_actor_location(center,True,False)
                distance=(pawn.get_actor_location()-center).length()
                assert distance>90,(name,distance)
                report['swept_directions'].append({'approach':name,'stopped_cm_from_center':round(distance,2)})
            for a,enabled in original:a.set_actor_enable_collision(enabled)
            b=labels['Bunker_TravelToBasement'];p=b.get_actor_location();p.y-=90;p.z=z
            pawn.set_actor_location(p,False,True)
            assert b.is_within_interaction_distance(pawn)
            assert b.request_interaction(pawn),'Hatch blocker prevented the existing interaction'
            report['basement_interaction_reachable']=True;stage='travel'
        elif stage=='travel':
            game=editor.get_game_world()
            if game and u.GameplayStatics.get_current_level_name(game,True)=='L_Basement':
                assert u.GameplayStatics.get_player_pawn(game,0)
                report['actual_destination']='L_Basement';finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
