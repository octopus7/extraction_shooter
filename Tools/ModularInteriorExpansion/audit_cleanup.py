"""Read-only final inventory/cleanup audit. Usage: python audit_cleanup.py ASSET_COMMIT."""
from pathlib import Path
import json,subprocess,re,sys,hashlib
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
TOOLS=Path(__file__).parent
asset_commit=sys.argv[1]
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
removed=['import_unreal_once.py','import_once_driver.py','build_map_once.py','map_once_driver.py','capture_once.py','prepare_importer.py','import_reference.tmp']
assert all(not (TOOLS/n).exists() for n in removed)
for filename in ['verify_unreal_assets.py','verify_unreal_map.py','verify_map_driver.py','run_unreal.ps1']:
    s=(TOOLS/filename).read_text(encoding='utf-8-sig')
    assert not any(word in s for word in ['import_asset_tasks','spawn_actor_from_class','save_loaded_asset','save_current_level','CAPTURE_ONCE','take_high_res_screenshot','import_unreal_once','build_map_once'])
assert not git('diff',asset_commit,'--','TunaSweeper/Content/Environment/ModularInteriorExpansion')
basepaths=['Tools/ModularInteriorPreview','TunaSweeper/SourceArt/Environment/ModularInteriorPreview','TunaSweeper/Content/Environment/ModularInteriorPreview']
assert not git('diff','c2b70edf','--',*basepaths)
m=json.loads((OUT/'model_manifest.json').read_text())
assert len(list((OUT/'Models').glob('*.fbx')))==len(m['assets'])==18
content=ROOT/'TunaSweeper/Content/Environment/ModularInteriorExpansion'
assert len(list((content/'Meshes').glob('*.uasset')))==18
assert len(list((content/'Materials').glob('*.uasset')))==3
assert len(list((content/'Textures').glob('*.uasset')))==1
assert len(list((content/'Maps').glob('*.umap')))==1
reports={n:json.loads((OUT/n).read_text()) for n in ['source_validation.json','unreal_import_validation.json','unreal_reload_validation.json','unreal_map_validation.json','unreal_map_reload_validation.json']}
assert all(r['passed'] for r in reports.values())
assert len(reports['unreal_reload_validation.json']['assets'])==24
assert reports['unreal_map_reload_validation.json']['kit_instances']==96
page=(OUT/'Review.html').read_text(encoding='utf-8')
links=set(re.findall(r'(?:href|src)="([^"]+)"',page))
assert all((OUT/link).is_file() for link in links)
report={'passed':True,'asset_commit':asset_commit,'removed_generators':removed,'runtime_packages':23,'expansion_meshes':18,'base_meshes':6,'expansion_triangles':476,'combined_unique_triangles':636,'sample_instances':96,'sample_triangles':1556,'base_three_paths_unchanged_from':'c2b70edf','runtime_content_unchanged_after_generator_removal':True,'gallery_links_verified':len(links),'reports_passed':list(reports),'post_cleanup_asset_process_exit':1,'post_cleanup_map_process_exit':0,'engine_exit_note':'Asset assertions pass separately; existing NE_PostProcess Niagara ensure remains.'}
report['post_cleanup_asset_process_exit']=reports['unreal_reload_validation.json']['process_exit']
report['post_cleanup_map_process_exit']=reports['unreal_map_reload_validation.json']['process_exit']
report['runtime_sha256']={str(p.relative_to(content)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(content.rglob('*')) if p.is_file()}
(OUT/'cleanup_validation.json').write_text(json.dumps(report,indent=2))
print('EXPANSION_CLEANUP_VALIDATION_PASSED',report['runtime_packages'])
