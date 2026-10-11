"""Read-only UE 5.7 audit of the dedicated memo anchor and its prop."""
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Memo/StorageDevice'
OUT.mkdir(parents=True, exist_ok=True)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
base = unreal.load_class(None, '/Script/TunaSweeper.TunaSweeperMemoAnchor')
assert base
cdo = unreal.get_default_object(base)
def path(value):
    if isinstance(value, unreal.Vector):
        return [value.x, value.y, value.z]
    return value.get_path_name() if hasattr(value, 'get_path_name') else str(value)
def visual(obj):
    component = obj.get_editor_property('visual_mesh')
    return {
        'memo_id': obj.get_editor_property('memo_id'),
        'visual_scale': path(component.get_editor_property('relative_scale3d')),
        'component_mesh': path(component.get_editor_property('static_mesh')),
        'component_materials': [path(x) for x in component.get_materials()]}
derived_paths = registry.get_derived_class_names([unreal.TopLevelAssetPath('/Script/TunaSweeper', 'TunaSweeperMemoAnchor')], [])
derived = [str(p.get_editor_property('package_name'))+'.'+str(p.get_editor_property('asset_name')) for p in derived_paths]
derived = [p for p in derived if p != '/Script/TunaSweeper.TunaSweeperMemoAnchor']
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'native_defaults': visual(cdo),
          'derived_classes': [str(x) for x in derived], 'blueprints': [], 'references': {}}
for cls in derived:
    loaded = unreal.load_class(None, str(cls))
    if loaded:
        report['blueprints'].append({'class': str(cls), 'defaults': visual(unreal.get_default_object(loaded))})
opts = unreal.AssetRegistryDependencyOptions(True, True, False, False, False)
for name in ['/Game/Interaction/M_MemoStorageDevice', '/Game/Interaction/T_MemoStorageDevice']:
    report['references'][name] = [str(x) for x in registry.get_referencers(name, opts)]
mat = unreal.load_asset('/Game/Interaction/M_MemoStorageDevice')
tex = unreal.load_asset('/Game/Interaction/T_MemoStorageDevice')
report['existing_material'] = {'path': path(mat), 'two_sided': mat.get_editor_property('two_sided'),
    'base_color_node': path(unreal.MaterialEditingLibrary.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR))}
report['existing_texture'] = {'path': path(tex), 'width': tex.blueprint_get_size_x(), 'height': tex.blueprint_get_size_y()}
# Serialized native class references also catch directly placed memo actors in maps.
report['serialized_memo_references'] = []
for asset in (ROOT / 'TunaSweeper/Content').rglob('*'):
    if asset.suffix not in ('.uasset', '.umap'):
        continue
    raw = asset.read_bytes()
    if b'TunaSweeperMemoAnchor' in raw or 'TunaSweeperMemoAnchor'.encode('utf-16le') in raw:
        report['serialized_memo_references'].append(str(asset.relative_to(ROOT)))
report['placement_workflow'] = 'BP_MemoAnchor with MemoId only; no MemoSpawns.json'
(OUT/'existing_state_audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('MEMO_EXISTING_AUDIT_COMPLETE')
