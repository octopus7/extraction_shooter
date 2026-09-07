"""Fresh Blender process: verify FBX payload and packed .blend texture."""
from pathlib import Path
import bpy, bmesh, json, math
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
m=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LeafLitter.blend'))
assert bpy.data.images['T_LeafLitter_Palette'].packed_file
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models/SM_LeafLitter.fbx'))
objs=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(objs)==1
o=objs[0];me=o.data;me.calc_loop_triangles()
assert len(me.loop_triangles)==m['triangles']
assert len(me.materials)==1 and me.materials[0].name=='M_LeafLitter'
assert len(me.uv_layers)==1
deg_uv=0
for tri in me.loop_triangles:
    a,b,c=[me.uv_layers.active.data[i].uv for i in tri.loops]
    deg_uv+=abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))<1e-9
assert deg_uv==0
bm=bmesh.new();bm.from_mesh(me)
deg=sum(f.calc_area()<1e-10 for f in bm.faces);nonman=sum(not e.is_manifold for e in bm.edges)
assert deg==nonman==0
assert all(abs(f.normal.length-1)<1e-5 for f in bm.faces)
bm.free()
ps=[o.matrix_world@v.co for v in me.vertices]
bounds=[min(p[i] for p in ps) for i in range(3)]+[max(p[i] for p in ps) for i in range(3)]
err=max(abs(a-b) for a,b in zip(bounds,m['bounds_m']));assert err<.0001
assert o.location.length<.0001
report={'passed':True,'triangles':len(me.loop_triangles),'material_slots':1,'uv_channels':1,'degenerate_uv_triangles':deg_uv,'degenerate_faces':deg,'nonmanifold_edges':nonman,'unit_normals':True,'packed_blend_texture':True,'max_bounds_error_m':err,'pivot_m':list(o.location)}
(OUT/'fbx_validation.json').write_text(json.dumps(report,indent=2))
print('LEAFLITTER_FBX_PASSED',report)
