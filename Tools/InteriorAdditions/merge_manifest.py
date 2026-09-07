"""Collect independently authored Blender categories into the import manifest."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/InteriorAdditions'
manifest={'materials':{},'assets':[],'axis_contract':'X width, -Y front, Z up; floor-center pivot, dimensions centimetres. Existing X-facing assets are reoriented to match this contract.'}
for category in ['Kitchen','BathroomLaundry','Furniture']:
    part=json.loads((OUT/'Manifests'/f'{category}.json').read_text(encoding='utf-8'))
    for key,spec in part['materials'].items():
        assert key not in manifest['materials'] or manifest['materials'][key]==spec
        manifest['materials'][key]=spec
    manifest['assets'].extend(part['assets'])
assert len(manifest['assets'])==29
assert len({e['name'] for e in manifest['assets']})==29
assert all((OUT/'Models'/f"{e['name']}.fbx").exists() for e in manifest['assets'])
manifest['total_triangles']=sum(e['triangles'] for e in manifest['assets'])
manifest['provenance']={'modeling':'Original Blender script geometry; no existing geometry reused','reference_images':'Built-in image_gen using the user screenshots and inspected existing appearance','base_color':'Built-in image_gen 4x4 atlas, native image retained','scale_audits':'References/existing_unreal_mesh_bounds.json and existing_bunker_component_scales.json'}
(OUT/'model_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print(f"Merged {len(manifest['assets'])} meshes; {manifest['total_triangles']} triangles")
