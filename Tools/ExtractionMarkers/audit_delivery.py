"""Read-only source/runtime scope and provenance audit; writes JSON evidence only."""
from pathlib import Path
import subprocess,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ExtractionMarkers'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
reuse=[
'TunaSweeper/Content/Environment/ModularInteriorExpansion/Materials/M_MI_ExpansionAmber.uasset',
'TunaSweeper/Content/Environment/ModularInteriorExpansion/Meshes/SM_MIE_EmergencyLight.uasset',
'TunaSweeper/Content/Environment/ModularInteriorPreview/Materials/M_MI_Steel.uasset',
'TunaSweeper/Content/Environment/ModularInteriorPreview/Textures/T_MI_Atlas.uasset',
'TunaSweeper/Content/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask.uasset',
'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Models/SM_MIE_EmergencyLight.fbx',
'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Models/SM_MIE_ZoneSign.fbx',
'TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures/T_MI_Atlas.png',
'TunaSweeper/SourceArt/Environment/ModularInteriorPreview/Textures/T_MI_DirtMask.png']
for p in reuse:assert (ROOT/p).read_bytes()==git('show','8bb53750:'+p),p
changed=git('diff','--name-only','4a1b45f8^').decode().splitlines()
allowed=('Tools/ExtractionMarkers/','TunaSweeper/SourceArt/Environment/ExtractionMarkers/','TunaSweeper/Content/Interaction/ExtractionMarkers/')
assert all(p in reuse or p=='Docs/requests.md' or p.startswith(allowed) for p in changed),changed
reports=['source_validation.json','unreal_import_validation.json','unreal_reload_validation.json','unreal_scene_validation.json','unreal_scene_reload_validation.json']
for n in reports:assert json.loads((OUT/n).read_text())['passed'],n
runtime=[p for folder in ['TunaSweeper/Content/Interaction/ExtractionMarkers','TunaSweeper/Content/Environment/ModularInteriorExpansion','TunaSweeper/Content/Environment/ModularInteriorPreview'] for p in (ROOT/folder).rglob('*') if p.is_file()]
hashes={p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in runtime}
if len(sys.argv)>1:
 commit=sys.argv[1]
 for p,h in hashes.items():assert hashlib.sha256(git('show',commit+':'+p)).hexdigest()==h,p
 assert not list((ROOT/'Tools/ExtractionMarkers').glob('*_once.py'))
 for p in (ROOT/'Tools/ExtractionMarkers').glob('*.py'):
  if p.name.startswith('verify_'):assert not any(x in p.read_text() for x in ('import_asset_tasks','save_loaded_asset','spawn_actor_from_class','save_current_level')),p
report={'passed':True,'reused_files_byte_identical_to':'8bb53750','reused_paths':reuse,'existing_game_maps_blueprints_source_preserved':True,'runtime_packages':len(runtime),'runtime_sha256':hashes,'reports_passed':reports,'cleanup_verified':len(sys.argv)>1}
(OUT/'delivery_validation.json').write_text(json.dumps(report,indent=2));print('EM_DELIVERY_PASSED',len(runtime))
