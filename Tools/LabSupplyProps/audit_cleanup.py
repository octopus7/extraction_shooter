"""Check removed UE generators and byte-identical saved packages after cleanup."""
from pathlib import Path
import subprocess,json,sys,hashlib,re
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/LabSupplyProps'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
commit=sys.argv[1]
retired=['import_unreal_once.py','import_once_driver.py','build_map_once.py','capture_once.py']
assert all(not (Path(__file__).parent/f).exists() for f in retired)
runner=(Path(__file__).with_name('run_unreal.ps1')).read_text(encoding='utf-8-sig')
assert not any(x in runner for x in ('Import','MapBuild','Capture','once.py'))
packages=git('ls-tree','-r','--name-only',commit,'--','TunaSweeper/Content/Environment/LabSupplyProps').decode().splitlines()
assert len(packages)==15,len(packages)
verified=[]
for path in packages:
    expected=git('show',f'{commit}:{path}');actual=(ROOT/path).read_bytes();assert actual==expected,path
    verified.append({'path':path,'sha256':hashlib.sha256(actual).hexdigest()})
base=['Tools/ModularInteriorPreview','TunaSweeper/SourceArt/Environment/ModularInteriorPreview','TunaSweeper/Content/Environment/ModularInteriorPreview']
assert not git('diff','c2b70edf','--',*base)
review=(OUT/'Review.html').read_text(encoding='utf-8')
links=[p for p in re.findall(r'(?:src|href)="([^"]+)"',review) if not p.startswith(('#','http'))]
assert all((OUT/p).is_file() for p in links),[p for p in links if not (OUT/p).is_file()]
for name in ('source_validation.json','unreal_reload_validation.json','unreal_map_reload_validation.json','unreal_capture_validation.json'):
    assert json.loads((OUT/name).read_text(encoding='utf-8-sig'))['passed'],name
report={'passed':True,'asset_commit':commit,'removed_generators':retired,'runtime_packages_unchanged':verified,'base_byte_identical_to':'c2b70edf','review_links_checked':len(links),'startup_generation_entry_points':False}
(OUT/'cleanup_validation.json').write_text(json.dumps(report,indent=2))
print('LSP_CLEANUP_PASSED',len(packages),len(links))
