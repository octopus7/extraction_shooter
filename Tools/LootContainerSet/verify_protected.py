"""Read-only proof that pre-existing gameplay content/source was preserved."""
from pathlib import Path
import json,subprocess,hashlib
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/LootContainerSet'
BASE='c1b408f8'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
protected=['TunaSweeper/Source','TunaSweeper/Config','TunaSweeper/Content/Data','TunaSweeper/Content/Interaction/BP_LootContainer.uasset']
changed=git('diff','--name-only',BASE,'--',*protected).splitlines()
assert not changed,changed
existing_content_changes=git('diff','--name-only','--diff-filter=MDRT',BASE,'--','TunaSweeper/Content').splitlines()
assert not existing_content_changes,existing_content_changes
files=['TunaSweeper/Content/Data/LootContainerTable.json','TunaSweeper/Content/Interaction/BP_LootContainer.uasset','TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperLootContainerActor.cpp','TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperLootContainerActor.h']
report={'passed':True,'base_commit':BASE,'protected_paths':protected,'changed_existing_content':existing_content_changes,'sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in files}}
(OUT/'protected_assets_validation.json').write_text(json.dumps(report,indent=2))
print('LOOT_PROTECTED_ASSETS_PASSED')
