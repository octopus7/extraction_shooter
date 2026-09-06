"""Fresh Blender FBX and packed .blend reload audit."""
import bpy,bmesh,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse'
spec=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models/SM_BushSparse.fbx'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(objects)==1
o=objects[0];m=o.data;m.calc_loop_triangles()
coords=[o.matrix_world@v.co for v in m.vertices]
bounds=[min(v[i] for v in coords) for i in range(3)]+[max(v[i] for v in coords) for i in range(3)]
error=max(abs(a-b) for a,b in zip(bounds,spec['bounds_m']));assert error<1e-5
assert len(m.loop_triangles)==spec['triangles'];assert len(m.uv_layers)==2;assert len(m.materials)==1
assert all(math.isfinite(c) for v in coords for c in v)
assert all(abs(p.normal.length-1)<1e-4 for p in m.polygons)
uv_areas=[]
for layer in m.uv_layers:
    areas=[]
    for tri in m.loop_triangles:
        a,b,c=[layer.data[i].uv for i in tri.loops]
        areas.append(abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))/2)
    assert min(areas)>1e-12,(layer.name,min(areas))
    assert all(0<=c<=1 for d in layer.data for c in d.uv)
    uv_areas.append({'name':layer.name,'minimum_triangle_uv_area':min(areas)})
bm=bmesh.new();bm.from_mesh(m);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
assert all(f.calc_area()>1e-10 for f in bm.faces);assert all(e.is_manifold for e in bm.edges)
volume=bm.calc_volume(signed=True);assert volume>0
bm.free()
result={'fbx_meshes':1,'triangles':len(m.loop_triangles),'bounds_m':bounds,'max_bounds_error_m':error,'material_slots':1,'uv_checks':uv_areas,'closed_after_weld':True,'signed_volume_m3':volume,'unit_normals':True,'degenerate_faces':0}
bpy.ops.wm.open_mainfile(filepath=str(OUT/'BushSparse.blend'))
obj=bpy.data.objects['SM_BushSparse'];assert len(obj.data.polygons)==spec['triangles']
tex=bpy.data.images['T_BushSparse_Palette'];assert tex.packed_file
assert tuple(obj.location)==(0,0,0) and tuple(obj.scale)==(1,1,1)
result.update({'packed_blend_texture':True,'blend_mesh_reloaded':True,'passed':True})
(OUT/'fbx_validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print('BUSH_SPARSE_FBX_RELOAD_PASSED')
