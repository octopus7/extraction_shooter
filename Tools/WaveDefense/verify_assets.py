"""Read-only validation of the saved Plains defense encounter in a fresh editor process."""
import csv
import json
import os
import time
import traceback
import unreal

ROOT = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..'))
REPORT = os.path.join(unreal.Paths.project_saved_dir(), 'WaveDefense', 'verification.json')
ASSET_REPORT = os.path.join(unreal.Paths.project_saved_dir(), 'Automation', 'WaveDefenseAssetsFinal', 'index.json')
checks = []
def check(label, condition):
    checks.append({'name': label, 'passed': bool(condition)})
    if not condition:
        raise AssertionError(label)

try:
    check('load saved Plains', unreal.EditorLoadingAndSavingUtils.load_map('/Game/MainRaid/RaidPlains'))
    world = unreal.EditorLevelLibrary.get_editor_world()
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    labels = {}
    for actor in actors:
        labels.setdefault(actor.get_actor_label(), []).append(actor)
    with open(os.path.join(ROOT, 'Tools', 'WaveDefense', 'preserved_actors.json'), encoding='utf-8-sig') as stream:
        original = json.load(stream)
    for saved in original:
        matches = labels.get(saved['name'], [])
        check('preserved actor ' + saved['name'], len(matches) == 1 and matches[0].get_class().get_path_name() == saved['class'] and
              all(abs(a-b) < .1 for a,b in zip(matches[0].get_actor_location().to_tuple(), saved['pos'])))
    def only(label):
        found = labels.get(label, [])
        check('unique ' + label, len(found) == 1)
        return found[0]
    director = only('WD_Plains_Director'); core = only('WD_Plains_Core'); terminal = only('WD_Plains_Terminal')
    check('independent base transform', abs(director.get_actor_location().x-5000) < .1 and abs(director.get_actor_location().y) < .1 and
          abs(core.get_actor_location().x-5000) < .1 and abs(core.get_actor_location().y) < .1 and
          abs(terminal.get_actor_location().x-4400) < .1 and abs(terminal.get_actor_location().y) < .1)
    check('south entrance', terminal.get_actor_location().x < core.get_actor_location().x)
    check('terminal references director', terminal.get_editor_property('director') == director)
    check('director references core', director.get_editor_property('core') == core)
    definition = director.get_editor_property('definition')
    check('definition reference', definition and definition.get_path_name() == '/Game/MainRaid/WaveDefense/DA_PlainsWaveDefense.DA_PlainsWaveDefense')
    check('stable encounter id', str(definition.get_editor_property('encounter_id')) == 'plains.wave_defense.01')
    check('warning sound configured', bool(definition.get_editor_property('warning_sound')))
    waves = definition.get_editor_property('waves'); check('six waves', len(waves) == 6)
    totals=[]
    for index, wave in enumerate(waves):
        rows=wave.get_editor_property('enemies')
        total=sum(row.get_editor_property('count') for row in rows); totals.append(total)
        check('wave %d count and cap' % (index+1), total == [6,8,10,12,14,16][index] and wave.get_editor_property('alive_cap') == [4,5,6,7,8,8][index])
        for row in rows:
            check('wave %d enemy definition' % (index+1), bool(row.get_editor_property('enemy_class')) and bool(str(row.get_editor_property('combat_profile'))) and row.get_editor_property('count') > 0)
    check('66 enemies total', sum(totals) == 66)
    points=director.get_editor_property('spawn_points'); slots=director.get_editor_property('facility_slots')
    check('six candidates and slots', len(points) == 6 and len(slots) == 6)
    check('unique slots', len(set(str(s.get_editor_property('slot_id')) for s in slots)) == 6)
    for lane in ('north','west','east'):
        check('two candidates '+lane, sum(str(p.get_editor_property('lane_id')) == lane for p in points) == 2)
    nav=only('WD_Plains_Navigation'); _, extent=nav.get_actor_bounds(False)
    check('navigation bounds', extent.x >= 4400 and extent.y >= 4400 and extent.z >= 900)
    end=unreal.NavigationSystemV1.project_point_to_navigation(world,core.get_actor_location(),None,None,unreal.Vector(500,500,500))
    check('core on navigation', end is not None)
    for point in points:
        start=unreal.NavigationSystemV1.project_point_to_navigation(world,point.get_actor_location(),None,None,unreal.Vector(150,150,300))
        path=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,start,end) if start else None
        check('navigation path '+point.get_actor_label(), path is not None and path.is_valid() and not path.is_partial())
    for name in ('WBP_WaveDefenseHUD_GPT61Terra','WBP_WaveDefensePanel_GPT61Terra'):
        check('widget class '+name, unreal.EditorAssetLibrary.load_blueprint_class('/Game/MainRaid/WaveDefense/UI/'+name) is not None)
    check('fresh widget-tree automation report', os.path.isfile(ASSET_REPORT) and time.time()-os.path.getmtime(ASSET_REPORT) < 180)
    with open(ASSET_REPORT, encoding='utf-8-sig') as stream: automation=json.load(stream)
    check('widget-tree automation passed', automation['failed']==0 and automation['succeeded']+automation['succeededWithWarnings'] >= 1 and any(t['fullTestPath']=='TunaSweeper.WaveDefense.Assets.Saved' and t['state']=='Success' for t in automation['tests']))
    with open(os.path.join(unreal.Paths.project_content_dir(),'Data','UITextStrings.csv'),encoding='utf-8-sig',newline='') as stream:
        rows={row['string_key']:row for row in csv.DictReader(stream)}
    keys=['title','intro','status','details','start','ready','abort','claim','close','barrier','turret','repair_core','repair_facility','reload','direction_north','direction_west','direction_east','direction_line','result_success','result_core','result_player','result_abandoned','result_interrupted','result_faulted','result_pending','reward_pending','action_failed','facility_status','facility_empty','core_warning','leave_warning']
    for key in keys:
        row=rows.get('ui.wave_defense.'+key)
        check('six language strings '+key, bool(row) and all(row.get(lang,'').strip() for lang in ('ko','en','ja','zh-Hans','zh-Hant','ru')))
    result={'passed':True,'checks':checks,'generated_at':time.time()}
except Exception:
    result={'passed':False,'checks':checks,'error':traceback.format_exc(),'generated_at':time.time()}
os.makedirs(os.path.dirname(REPORT),exist_ok=True)
with open(REPORT,'w',encoding='utf8') as stream:json.dump(result,stream,ensure_ascii=False,indent=2)
if not result['passed']:
    unreal.log_error(result['error'])
    raise RuntimeError('WaveDefense asset verification failed; see '+REPORT)
unreal.log('WAVE_DEFENSE_VERIFICATION_PASSED '+str(len(checks)))
