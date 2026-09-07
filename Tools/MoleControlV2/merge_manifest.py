import json
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/MoleControlV2'
manifest={'axis_contract':'Blender X width/-Y front/Z up -> UE X width/+Y front/Z up; cm',
          'materials':{},'assets':[],'texture':'Textures/T_MoleControlV2_Atlas.png'}
for name in ('Shell','Console','Decor'):
    data=json.loads((SOURCE/'Manifests'/f'{name}.json').read_text(encoding='utf-8'))
    for key,spec in data['materials'].items():
        if key in manifest['materials']:assert manifest['materials'][key]==spec
        manifest['materials'][key]=spec
    manifest['assets'].extend(data['assets'])
assert len({a['name'] for a in manifest['assets']})==len(manifest['assets'])
manifest['total_triangles']=sum(a['triangles'] for a in manifest['assets'])
(SOURCE/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('MOLE_V2_MERGED',len(manifest['assets']),manifest['total_triangles'])
