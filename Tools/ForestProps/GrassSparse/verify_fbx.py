import bpy, json, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse'
m=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/(m['name']+'.fbx')))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(objects)==1
o=objects[0];me=o.data;me.calc_loop_triangles();assert len(me.loop_triangles)==m['triangles']
points=[o.matrix_world@v.co for v in me.vertices]
b=[min(v[i] for v in points) for i in range(3)]+[max(v[i] for v in points) for i in range(3)]
err=max(abs(a-b) for a,b in zip(b,m['bounds_m']));assert err<1e-5
assert o.location.length<1e-6 and len(me.uv_layers)==1
assert len(me.materials)==1 and me.materials[0].name==m['materials'][0]
assert all(p.area>1e-10 and abs(p.normal.length-1)<1e-5 for p in me.polygons)
assert all(math.isfinite(c) and 0<=c<=1 for l in me.uv_layers[0].data for c in l.uv)
report={'passed':True,'fresh_process':True,'triangles':len(me.loop_triangles),'bounds_error_m':err,'pivot':list(o.location),'uv_channels':len(me.uv_layers),'material_slots':len(me.materials),'degenerate_faces':0,'normals_finite_unit_length':True}
# Prove the native file is self-contained too.
bpy.ops.wm.open_mainfile(filepath=str(OUT/'GrassSparse.blend'))
assert bpy.data.images[m['texture']].packed_file
assert bpy.data.objects[m['name']].location.length==0
report['blend_texture_packed']=True
(OUT/'fbx_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print('GRASS_SPARSE_FBX_PASS')
