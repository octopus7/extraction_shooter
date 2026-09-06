"""Round-trip all exported FBX files in a separate Blender process."""
import bpy
import json
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/WaterIntakeDemo'
manifest=json.loads((OUT/'model_manifest.json').read_text(encoding='utf-8'))
report=[]
for entry in manifest['assets']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/(entry['name']+'.fbx')))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')]
    cols=[o for o in bpy.context.scene.objects if o.name.startswith('UCX_')]
    assert len(meshes)==1, entry['name']
    assert len(cols)==entry['collision_boxes'], entry['name']
    obj=meshes[0];obj.data.calc_loop_triangles()
    assert len(obj.data.loop_triangles)==entry['triangles'], entry['name']
    assert obj.data.uv_layers.active is not None
    assert all(m is not None for m in obj.data.materials)
    positions=[obj.matrix_world@Vector(p) for p in obj.bound_box]
    bounds=[min(p[i] for p in positions) for i in range(3)]+[max(p[i] for p in positions) for i in range(3)]
    expected=[v-entry['placement_blender_m'][i%3] for i,v in enumerate(entry['bounds_world_m'])]
    error=max(abs(a-b) for a,b in zip(bounds,expected))
    assert error<.0001,(entry['name'],bounds,expected)
    report.append({'name':entry['name'],'triangles':len(obj.data.loop_triangles),
                   'collision_meshes':len(cols),'max_bounds_error_m':error,'passed':True})
(OUT/'fbx_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('INTAKE_FBX_VALIDATION_PASSED '+str(len(report)))
