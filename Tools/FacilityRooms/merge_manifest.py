import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/FacilityRooms'
families=[json.loads((SOURCE/'Manifests'/f'{name}.json').read_text(encoding='utf-8')) for name in ['Architecture','Utilities','Control']]
manifest={'axis_contract':families[0]['axis_contract'],'materials':families[0]['materials'],
          'texture':'Textures/T_FacilityRooms_Atlas.png','assets':[]}
for family in families:
    assert family['materials']==manifest['materials']
    manifest['assets'].extend(family['assets'])
assert len({a['name'] for a in manifest['assets']})==len(manifest['assets'])
manifest['total_triangles']=sum(a['triangles'] for a in manifest['assets'])
(SOURCE/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('FACILITY_MERGED',len(manifest['assets']),manifest['total_triangles'])
