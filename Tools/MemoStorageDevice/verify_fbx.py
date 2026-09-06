"""Independent fresh-process roundtrip: units, axes, normals, UVs and slots."""
import bpy
import bmesh
import json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Memo/StorageDevice'
spec=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/f"{spec['name']}.fbx"))
models=[o for o in bpy.context.scene.objects if o.type=='MESH' and not o.name.startswith('UCX_')]
cols=[o for o in bpy.context.scene.objects if o.name.startswith('UCX_')]
assert len(models)==1 and len(cols)==1
obj=models[0]
obj.data.calc_loop_triangles()
assert len(obj.data.loop_triangles)==spec['triangles']
assert [m.name for m in obj.data.materials]==list(spec['materials'])
assert len(obj.data.uv_layers)==1
assert all(-.0001<=c<=1.0001 for uv in obj.data.uv_layers.active.data for c in uv.uv)
positions=[obj.matrix_world@Vector(p) for p in obj.bound_box]
bounds=[min(p[i] for p in positions)*100 for i in range(3)]+[max(p[i] for p in positions)*100 for i in range(3)]
error=max(abs(a-b) for a,b in zip(bounds,spec['bounds_cm']))
assert error<.001,(bounds,spec['bounds_cm'])
assert obj.location.length<.00001
assert all(t.area>1e-12 for t in obj.data.loop_triangles)
bm=bmesh.new();bm.from_mesh(obj.data)
assert all(e.is_manifold for e in bm.edges)
# Each disconnected closed solid must have positive signed volume (outward winding).
pending=set(bm.verts);volumes=[]
while pending:
    found={pending.pop()};queue=list(found)
    while queue:
        v=queue.pop()
        for e in v.link_edges:
            other=e.other_vert(v)
            if other not in found:
                found.add(other);pending.discard(other);queue.append(other)
    faces={f for v in found for f in v.link_faces}
    volume=sum(f.verts[0].co.dot(f.verts[1].co.cross(f.verts[2].co))/6 for f in faces)
    assert volume>0,volume
    volumes.append(volume)
bm.free()
report={'passed':True,'bounds_cm':bounds,'max_bounds_error_cm':error,'triangles':len(obj.data.loop_triangles),
        'vertices':len(obj.data.vertices),'uv_channels':1,'slots':[m.name for m in obj.data.materials],
        'outward_closed_solids':len(volumes),'collision_boxes':len(cols),'pivot':[float(v) for v in obj.location]}
(OUT/'fbx_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('MEMO_FBX_VALIDATION_PASSED')
