"""Verify actual player-kill attribution, quest progress and first-kill achievement.

Dedicated rendered UnrealEditor.exe -unattended -ExecutePythonScript=<this file>
-saveddirsuffix=Region1BossQA. Never modifies canonical data or normal saves.
"""
import hashlib,json,time,traceback
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[2];WORK=ROOT/'TunaSweeper/Saved/Region1BossWork'
assert u.SystemLibrary.is_unattended()
SAVED=Path(u.Paths.project_saved_dir()).resolve();assert SAVED.name=='Saved_Region1BossQA'
ID='enemy.region1.boss';LABEL='Forest_Region1Boss_Red'
PATH='/Game/Blueprints/Enemies/Bosses/BP_Region1Boss_Red'
def protected():
    files=[ROOT/'TunaSweeper/Content/MainRaid/RaidForest.umap',ROOT/'TunaSweeper/Content/Blueprints/Enemies/Bosses/BP_Region1Boss_Red.uasset']
    files+=list((ROOT/'TunaSweeper/Saved/SaveGames').rglob('*'))
    return {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.is_file()}
baseline=protected()
definitions=json.loads((ROOT/'TunaSweeper/External/MainPayload/Data/QuestDefinitions.json').read_text(encoding='utf8'))
quest=next(q for q in definitions if q['quest_id']==19)
assert len(quest['objectives'])==1 and quest['objectives'][0]['enemy_id']==ID and quest['objectives'][0]['required_count']==1
achievement=next(a for a in json.loads((ROOT/'TunaSweeper/Content/Data/AchievementDefinitions.json').read_text())['achievements'] if a['achievement_id']=='06')
assert achievement['enabled'] and achievement['target_id']==ID
assert achievement['condition_type']=='specific_enemy_first_kill'
# Build an isolated, explicitly progressed test save through Unreal's serializer.
# The native safe-save reader supports this legacy envelope and validates its flavor.
save=u.GameplayStatics.create_save_game_object(u.TunaSweeperSaveGame)
save.set_editor_property('build_flavor','Main');save.set_editor_property('save_slot_index',1)
save.set_editor_property('difficulty_selected',True)
progress=[]
for q in definitions:
    if q['quest_id']>=19:continue
    row=u.TunaSweeperQuestProgressSaveData();row.quest_id=str(q['quest_id']);row.state=u.TunaSweeperQuestState.REWARD_COMPLETED;progress.append(row)
save.set_editor_property('quest_progress_states',progress)
(SAVED/'SaveGames/FullGame').mkdir(parents=True,exist_ok=True)
for prior in (SAVED/'SaveGames/FullGame').glob('Achievements_*.sav*'):
    prior.unlink()
assert u.GameplayStatics.save_game_to_slot(save,'FullGame/TunaSweeperSave_Slot01',0)
es=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem);editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
u.EditorLoadingAndSavingUtils.load_map('/Game/MainRaid/RaidForest')
labels={a.get_actor_label():a for a in es.get_all_level_actors()}
assert LABEL not in labels,'Duplicate directly placed boss must be removed'
for label,placement in [('Forest_Power_Enemy_Gray',1),('Forest_Power_Enemy_Blue',2),('Forest_Power_Enemy_Red',3)]:
    assert label in labels and labels[label].get_editor_property('placement_id')==placement
boss=u.get_default_object(u.EditorAssetLibrary.load_blueprint_class(PATH))
assert str(boss.get_editor_property('enemy_id'))==ID
for cls,label in [(u.EditorAssetLibrary.load_blueprint_class('/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_Red'),'QA_Ordinary_Red'),(u.EditorAssetLibrary.load_blueprint_class(PATH),'QA_Hazard_Boss')]:
    actor=es.spawn_actor_from_class(cls,u.Vector(0,0,300));assert actor
    actor.set_actor_label(label)
report={'passed':False,'shared_enemy_id':ID,'quest_id':19,'achievement_id':'06','red_material':True}
start=time.monotonic();stage='settle';ticks=0;busy=False
def finish(error=None):
    report['map_assets_and_normal_saves_unchanged']=protected()==baseline
    report['passed']=not error and report['map_assets_and_normal_saves_unchanged']
    if error:report['error']=error;u.log_error(error)
    (WORK/'validation.json').write_text(json.dumps(report,indent=2),encoding='utf8')
    u.log('REGION1_BOSS_QA_'+('PASSED' if report['passed'] else 'FAILED'))
    u.unregister_slate_post_tick_callback(handle)
    if levels.is_in_play_in_editor():levels.editor_request_end_play()
    u.EditorPythonScripting.set_keep_python_script_alive(False);u.SystemLibrary.quit_editor()
def count(quests):
    result=quests.get_quest_objective_progress(19)
    if isinstance(result,tuple):result=result[-1]
    assert len(result)==1
    return result[0].current_count
def tick(dt):
    global stage,ticks,busy
    if busy:return
    busy=True;ticks+=1
    try:
        assert time.monotonic()-start<180,'Boss QA timeout: '+stage
        if stage=='settle' and ticks>=45:
            levels.editor_request_begin_play();stage='pie';ticks=0
        elif stage=='pie' and ticks>=120:
            game=editor.get_game_world();assert game
            pawn=u.GameplayStatics.get_player_pawn(game,0);assert pawn
            gi=u.GameplayStatics.get_game_instance(game)
            assert gi.activate_save_slot(1,False),'Isolated slot activation failed'
            raid=next(s for s in u.ObjectIterator(u.TunaSweeperRaidPlacementSubsystem) if s.get_outer()==gi)
            raid.set_raid_seed(48271)
            assert raid.ensure_raid_placement_actors_spawned_for_world(game)
            quests=next(s for s in u.ObjectIterator(u.TunaSweeperQuestSubsystem) if s.get_outer()==gi)
            achievements=next(s for s in u.ObjectIterator(u.TunaSweeperAchievementSubsystem) if s.get_outer()==gi)
            assert quests and achievements
            assert not achievements.is_achievement_unlocked('06'),'QA save must start without boss achievement'
            assert quests.accept_quest(19),'Isolated prerequisites did not load'
            assert count(quests)==0
            enemies=u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperEnemyCharacter)
            other=next(a for a in enemies if a.get_actor_label()=='QA_Ordinary_Red');u.GameplayStatics.apply_damage(other,1000000,pawn.get_controller(),pawn,None)
            assert other.is_dead() and count(quests)==0 and not achievements.is_achievement_unlocked('06')
            report['ordinary_red_does_not_count']=True
            hazard_victim=next(a for a in enemies if a.get_actor_label()=='QA_Hazard_Boss');u.GameplayStatics.apply_damage(hazard_victim,1000000,None,None,None)
            assert hazard_victim.is_dead() and count(quests)==0 and not achievements.is_achievement_unlocked('06')
            report['non_player_death_does_not_count']=True
            enemies=u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperEnemyCharacter)
            bosses=[a for a in enemies if a.get_class().get_path_name()==PATH+'.BP_Region1Boss_Red_C' and not a.is_dead()]
            assert len(bosses)==1,'Expected exactly one live boss from anchor 3'
            boss=bosses[0]
            assert str(boss.get_editor_property('enemy_id'))==ID,'Anchor spawn replaced fixed boss identity'
            assert boss.get_editor_property('max_health')==60
            assert any(c.get_material(i) and 'MI_Robot_Red' in c.get_material(i).get_path_name() for c in boss.get_components_by_class(u.SkeletalMeshComponent) for i in range(c.get_num_materials()))
            assert sum(str(a.get_editor_property('enemy_id')).startswith('raid_runtime_') for a in enemies)>=2
            before=achievements.get_total_enemy_kills()
            u.GameplayStatics.apply_damage(boss,1000000,pawn.get_controller(),pawn,None)
            assert boss.is_dead() and count(quests)==1
            assert quests.get_quest_state(19)==u.TunaSweeperQuestState.REWARD_AVAILABLE
            assert achievements.has_killed_enemy(ID) and achievements.is_achievement_unlocked('06')
            assert achievements.get_total_enemy_kills()==before+1
            u.GameplayStatics.apply_damage(boss,1000000,pawn.get_controller(),pawn,None)
            assert count(quests)==1 and achievements.get_total_enemy_kills()==before+1
            report.update(actual_player_kill_completes_both=True,repeated_damage_not_credited_twice=True)
            raid.set_raid_seed(48272)
            u.GameplayStatics.open_level(game,'/Game/MainRaid/RaidForest')
            stage='reload';ticks=0
        elif stage=='reload' and ticks>=120:
            game=editor.get_game_world();assert game
            gi=u.GameplayStatics.get_game_instance(game)
            raid=next(s for s in u.ObjectIterator(u.TunaSweeperRaidPlacementSubsystem) if s.get_outer()==gi)
            assert raid.get_raid_seed()==48272
            assert raid.ensure_raid_placement_actors_spawned_for_world(game)
            bosses=[a for a in u.GameplayStatics.get_all_actors_of_class(game,u.TunaSweeperEnemyCharacter) if a.get_class().get_path_name()==PATH+'.BP_Region1Boss_Red_C' and not a.get_actor_label().startswith('QA_')]
            assert len(bosses)==1 and str(bosses[0].get_editor_property('enemy_id'))==ID
            assert any(str(tag).endswith('_3_48272') for tag in bosses[0].tags)
            report['stable_boss_id_across_raid_seeds']=[48271,48272]
            finish()
    except Exception:finish(traceback.format_exc())
    finally:busy=False
u.EditorPythonScripting.set_keep_python_script_alive(True)
handle=u.register_slate_post_tick_callback(tick)
