"""Saved-asset and actual PIE travel verification; never saves levels/assets.

Run a dedicated rendered UnrealEditor.exe with -unattended -NoSound
-ExecutePythonScript=<this file> -saveddirsuffix=BunkerControlQA.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/BunkerControl'
assert u.SystemLibrary.is_unattended()
assert Path(u.Paths.project_saved_dir()).resolve().name=='Saved_BunkerControlQA'
def protected_hashes():
    paths=[ROOT/'TunaSweeper/Content/Maps/BunkerMap.umap']
    paths+=list((ROOT/'TunaSweeper/Content/Environment/BunkerControl').rglob('*.uasset'))
    paths+=list((ROOT/'TunaSweeper/Saved/SaveGames').rglob('*'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
baseline=protected_hashes()
es=u.get_editor_subsystem(u.EditorActorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
u.EditorLoadingAndSavingUtils.load_map('/Game/Maps/BunkerMap')
labels={a.get_actor_label():a for a in es.get_all_level_actors()}
entry=labels['Bunker_ControlRoomEntrance'];exit=labels['ControlRoom_ReturnToBunker'];basement=labels['Bunker_TravelToBasement']
assert entry.get_editor_property('target_endpoint')==exit and exit.get_editor_property('target_endpoint')==entry
for a in [entry,exit]:
    assert a.get_editor_property('use_screen_fade')
    assert abs(a.get_editor_property('screen_fade_seconds')-.2)<.001
assert basement.get_destination()==u.TunaSweeperLevelTravelDestination.BASEMENT
for a in [entry,exit,basement]:
    meshes=a.get_components_by_class(u.StaticMeshComponent)
    assert meshes and all(c.static_mesh is None and c.get_collision_enabled()==u.CollisionEnabled.NO_COLLISION for c in meshes)
    assert a.get_interactable_component().get_editor_property('marker_widget_class')
    assert str(a.get_interactable_component().get_editor_property('interaction_display_name_string_key')).startswith('ui.interaction.')
mesh_editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
manifest=json.loads((SOURCE/'model_manifest.json').read_text())
for spec in manifest['assets']:
    mesh=u.load_asset('/Game/Environment/BunkerControl/Meshes/'+spec['name']);assert mesh
    assert mesh_editor.get_num_uv_channels(mesh,0)==2
    assert mesh_editor.get_simple_collision_count(mesh)+mesh_editor.get_convex_collision_count(mesh)==spec['collision_boxes']
    assert all(str(s.material_interface.get_path_name()).startswith('/Game/Environment/BunkerControl/Materials/') for s in mesh.static_materials)
assert manifest['wall_count']==3 and not manifest['roof']
assert labels['ControlRoom_ThreeWalls'].get_attach_parent_actor()==labels['ROOT_ControlRoom']
room_rotation=labels['ControlRoom_ThreeWalls'].get_actor_rotation()
assert abs(room_rotation.yaw+90)<.01 and abs(room_rotation.pitch)<.01 and abs(room_rotation.roll)<.01
stair=labels['Bunker_ControlRoomSpiralStair']
assert (stair.get_actor_location()-u.Vector(-720,-175,6)).length()<.01
report=dict(passed=False,saved_pairing_and_fade=True,meshless_interactions=True,mesh_uv_collision_materials=True,three_wall_room_same_level=True)
start=time.monotonic();stage='settle';ticks=0;busy=False;transfer_start=0;source_position=None
def finish(error=None):
    report['saved_assets_and_regular_saves_unchanged']=protected_hashes()==baseline
    report['passed']=not error and report['saved_assets_and_regular_saves_unchanged']
    if error:report['error']=error;u.log_error(error)
    (SOURCE/'travel_validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('BUNKER_CONTROL_TRAVEL_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def game_context():
    game=editor.get_game_world();assert game
    pawn=u.GameplayStatics.get_player_pawn(game,0);assert pawn
    actors={a.get_actor_label():a for a in u.GameplayStatics.get_all_actors_of_class(game,u.Actor)}
    return game,pawn,actors
def begin(a,pawn):
    global transfer_start,source_position
    pc=pawn.get_controller();assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
    assert a.can_transfer_player(pawn),str(pawn.get_actor_location())+' -> '+a.get_actor_label()
    source_position=pawn.get_actor_location()
    assert a.request_interaction(pawn)
    assert a.is_transfer_active() and pc.is_move_input_ignored() and pc.is_look_input_ignored()
    assert (pawn.get_actor_location()-source_position).length()<.01,'Transfer must wait for black'
    assert not a.try_transfer_player(pawn),'Repeated fade request accepted'
    transfer_start=time.monotonic()
def check_finished(a,pawn):
    if a.is_transfer_active():return False
    assert time.monotonic()-transfer_start<10,'Fade did not complete promptly'
    pc=pawn.get_controller();assert u.GameplayStatics.get_player_pawn(pawn,0)==pawn
    assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
    target=a.get_editor_property('target_endpoint').get_arrival_point().get_world_location()
    p=pawn.get_actor_location();assert u.Vector(p.x-target.x,p.y-target.y,0).length()<1
    assert abs(p.z-pawn.capsule_component.get_scaled_capsule_half_height()-target.z)<5
    assert u.GameplayStatics.get_current_level_name(pawn,True)=='BunkerMap'
    return True
def tick(dt):
    global ticks,stage,busy,source_position
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-start<240,'Bunker control travel timeout: '+stage
        if stage=='settle' and ticks>=60:
            levels.editor_request_begin_play();stage='pie';ticks=0
        elif stage=='pie' and ticks>=120:
            game,pawn,actors=game_context();a=actors['Bunker_ControlRoomEntrance']
            floor=a.get_arrival_point().get_world_location();floor.z+=pawn.capsule_component.get_scaled_capsule_half_height()+2
            pawn.set_actor_location(floor,False,True);pawn.character_movement.stop_movement_immediately()
            begin(a,pawn);stage='outbound'
        elif stage=='outbound':
            game,pawn,actors=game_context()
            if check_finished(actors['Bunker_ControlRoomEntrance'],pawn):
                report['outbound_fade_same_world_and_input_restore']=True
                begin(actors['ControlRoom_ReturnToBunker'],pawn);stage='return'
        elif stage=='return':
            game,pawn,actors=game_context()
            if check_finished(actors['ControlRoom_ReturnToBunker'],pawn):
                report['return_fade_and_safe_floor']=True
                a=actors['Bunker_ControlRoomEntrance'];begin(a,pawn)
                a.set_target_endpoint(None);stage='cancel'
        elif stage=='cancel':
            game,pawn,actors=game_context();a=actors['Bunker_ControlRoomEntrance']
            if not a.is_transfer_active():
                assert (pawn.get_actor_location()-source_position).length()<5
                assert not pawn.get_controller().is_move_input_ignored()
                assert not pawn.get_controller().is_look_input_ignored()
                report['target_removed_mid_fade_recovers']=True
                a.set_target_endpoint(actors['ControlRoom_ReturnToBunker']);begin(a,pawn)
                a.destroy_actor();assert not pawn.get_controller().is_move_input_ignored()
                assert not pawn.get_controller().is_look_input_ignored()
                report['source_destroyed_mid_fade_releases_input']=True
                b=actors['Bunker_TravelToBasement'];p=b.get_actor_location();p.y-=90;p.z+=pawn.capsule_component.get_scaled_capsule_half_height()+2
                pawn.set_actor_location(p,False,True);pawn.character_movement.stop_movement_immediately()
                assert b.request_interaction(pawn);stage='basement'
        elif stage=='basement':
            game=editor.get_game_world()
            if game and u.GameplayStatics.get_current_level_name(game,True)=='L_Basement':
                assert u.GameplayStatics.get_player_pawn(game,0)
                report['actual_basement_destination']='L_Basement';finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
