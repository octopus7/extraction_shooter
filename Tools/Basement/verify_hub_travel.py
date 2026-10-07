"""Verify saved meshless ladder interaction and actual PIE travel in isolated saves.

Run with -ExecutePythonScript and -saveddirsuffix=HubTravelQA in a dedicated
rendered editor. Does not save maps/assets or touch the regular save directory.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/Basement'
MAP='/Game/Environment/Basement/Maps/L_Basement'

def validate_saved(labels):
    assert 'Hub_HatchLanding200' not in labels
    ladder=labels['Hub_Ladder300'];a=labels['Hub_TravelToBunker']
    assert isinstance(a,u.TunaSweeperLevelTravelInteractableActor)
    assert a.get_destination()==u.TunaSweeperLevelTravelDestination.BUNKER
    assert (a.get_actor_location()-ladder.get_actor_location()).length()<.01
    assert a.get_attach_parent_actor()==ladder.get_attach_parent_actor()==labels['ROOT_LadderHub']
    meshes=a.get_components_by_class(u.StaticMeshComponent)
    assert meshes and all(c.static_mesh is None and c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION for c in meshes)
    c=a.get_interactable_component()
    assert c.get_interaction_type()==u.TunaSweeperInteractionType.LEVEL_TRAVEL
    assert str(c.get_editor_property('interaction_display_name_string_key'))=='ui.interaction.travel'
    assert c.get_interaction_distance()==200
    assert c.get_editor_property('marker_widget_class')
    assert u.EditorAssetLibrary.does_asset_exist('/Game/Maps/BunkerMap')
    return {'actor':a.get_actor_label(),'meshless':True,'destination':'Bunker',
            'location_cm':[250,-210,0],'interaction_distance_cm':200,
            'interaction_key':'ui.interaction.travel','upper_platform_removed':True,
            'shared_platform_asset_retained_for':'/Game/Environment/FacilityRooms/Maps/L_Facility_Basement'}

if __name__=='__main__':
    assert u.SystemLibrary.is_unattended()
    assert '-saveddirsuffix=hubtravelqa' in u.SystemLibrary.get_command_line().lower()
    assert Path(u.Paths.project_saved_dir()).resolve().name=='Saved_HubTravelQA'
    def protected_hashes():
        paths=list((ROOT/'TunaSweeper/Content/Environment/Basement').rglob('*.umap'))
        paths+=list((ROOT/'TunaSweeper/Saved/SaveGames').rglob('*'))
        return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
    baseline=protected_hashes()
    es=u.get_editor_subsystem(u.EditorActorSubsystem)
    editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
    levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
    world=u.EditorLoadingAndSavingUtils.load_map(MAP)
    report=validate_saved({a.get_actor_label():a for a in es.get_all_level_actors()})
    report['passed']=False
    start=time.monotonic();state='settle';ticks=0;busy=False
    def finish(error=None):
        report['saved_map_and_regular_saves_unchanged']=protected_hashes()==baseline
        report['passed']=not error and report['saved_map_and_regular_saves_unchanged']
        if error:report['error']=error;u.log_error(error)
        (SOURCE/'hub_travel_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
        u.log('HUB_TRAVEL_'+('PASSED' if report['passed'] else 'FAILED'))
        u.unregister_slate_post_tick_callback(handle)
        if levels.is_in_play_in_editor():levels.editor_request_end_play()
        u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
    def tick(dt):
        global ticks,state,busy
        if busy:return
        busy=True;ticks+=1
        try:
            assert time.monotonic()-start<240,'Hub travel timeout'
            if state=='settle' and ticks>=60:
                levels.editor_request_begin_play();state='pie';ticks=0
            elif state=='pie' and ticks>=90:
                game=editor.get_game_world();assert game
                pawn=u.GameplayStatics.get_player_pawn(game,0);assert pawn
                a=next(a for a in u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperLevelTravelInteractableActor) if a.get_actor_label()=='Hub_TravelToBunker')
                pawn.character_movement.disable_movement()
                pawn.set_actor_location(u.Vector(250,90,95),False,True)
                assert not a.is_within_interaction_distance(pawn)
                pawn.set_actor_location(u.Vector(250,-100,95),False,True)
                assert a.is_within_interaction_distance(pawn)
                report['near_and_far_distance_checks']=True
                assert a.request_interaction(pawn),'Existing interaction subsystem rejected travel'
                report['interaction_request_accepted']=True
                state='travel';ticks=0
            elif state=='travel':
                game=editor.get_game_world()
                if game and u.GameplayStatics.get_current_level_name(game,True)=='BunkerMap':
                    assert u.GameplayStatics.get_player_pawn(game,0)
                    report['actual_destination']='BunkerMap';finish()
        except Exception:finish(traceback.format_exc())
        finally:busy=False
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    handle=u.register_slate_post_tick_callback(tick)
