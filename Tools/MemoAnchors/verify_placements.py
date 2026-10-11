"""Read-only validation of the 60 reserved memos and their saved regional placements.

Run with regular Python and --data-only for content checks, or UE's pythonscript
commandlet for fresh map/asset validation. Does not create or move actors.
"""
import csv
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
MAP_IDS = {'RaidForest': list(range(1,16)), 'RaidVillage': list(range(16,51)), 'RaidPlains': list(range(51,61))}

def verify_data():
    data = ROOT/'TunaSweeper/Content/Data'
    rows = json.loads((data/'MemoDefinitions.json').read_text(encoding='utf-8-sig'))
    assert [r['memo_id'] for r in rows] == list(range(1,61)), 'Expected exactly IDs 1 through 60'
    keys = set()
    for row in rows:
        for part in ['title','body']:
            key = f"memo.{row['memo_id']}.{part}"
            assert row[part+'_string_key'] == key
            keys.add(key)
    for path in [data/'MemoTextStrings.csv', data/'Translations/MemoTextStrings.csv']:
        with path.open(encoding='utf-8-sig', newline='') as f:
            translations = list(csv.DictReader(f))
        assert len(translations) == 120 and {r['string_key'] for r in translations} == keys
        for row in translations:
            assert all(row.values()), f'Empty translation: {row}'
            if 'ko' in row:
                assert row['ko'] == f"메모 {int(row['string_key'].split('.')[1])} 번"
    return {'definitions': 60, 'string_keys': 120, 'languages': 7}

def verify_maps():
    import unreal
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    native = unreal.load_class(None, '/Script/TunaSweeper.TunaSweeperMemoAnchor')
    bp = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Interaction/BP_MemoAnchor')
    assert native and bp
    report = {'passed': False, 'data': verify_data(), 'maps': {}}
    all_ids = []
    for name, ids in MAP_IDS.items():
        file = ROOT/f'TunaSweeper/Content/MainRaid/{name}.umap'
        before = hashlib.sha256(file.read_bytes()).hexdigest()
        assert levels.load_level('/Game/MainRaid/'+name)
        memos = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.TunaSweeperMemoAnchor)]
        found = sorted(a.get_editor_property('memo_id') for a in memos)
        assert found == ids, (name, found, ids)
        records = []
        for a in sorted(memos, key=lambda a:a.get_editor_property('memo_id')):
            memo_id = a.get_editor_property('memo_id')
            assert a.get_class() == bp
            assert a.get_actor_label() == f'Memo_{memo_id:03d}'
            assert str(a.get_folder_path()) == 'Memos'
            mesh = a.get_editor_property('visual_mesh')
            assert mesh.get_editor_property('static_mesh').get_name() == 'SM_MemoStorageDevice_LowPoly'
            assert len(mesh.get_materials()) == 4
            assert not a.get_editor_property('is_editor_only_actor')
            pos = a.get_actor_location()
            records.append({'memo_id':memo_id, 'label':a.get_actor_label(), 'position_cm':[round(pos.x,2),round(pos.y,2),round(pos.z,2)]})
        all_ids.extend(found)
        assert hashlib.sha256(file.read_bytes()).hexdigest() == before, 'Verifier must not modify maps'
        report['maps'][name] = records
    assert sorted(all_ids) == list(range(1,61))
    report['passed'] = True
    output = ROOT/'TunaSweeper/Saved/MemoPlacementTask/verification.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    unreal.log('MEMO_PLACEMENTS_VERIFIED: Forest=15 Village=35 Plains=10; IDs=1..60')

if __name__ == '__main__':
    if '--data-only' in sys.argv:
        print(json.dumps(verify_data()))
    else:
        verify_maps()
