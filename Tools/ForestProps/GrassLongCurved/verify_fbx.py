"""Separate-process source and FBX round-trip audit."""
import bpy, bmesh, json, math
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved'
manifest=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(OUT/'GrassLongCurved.blend'))
assert bpy.context.scene.unit_settings.scale_length==1
image=bpy.data.images.get('T_GrassLongCurved_BaseColor');assert image and image.packed_file and list(image.size)==[128,128]
assert all(abs(image.pixels[i]-1)<1e-6 for i in range(3,len(image.pixels),4))
report={'packed_texture':True,'opaque_pixels':True,'lods':[]}
for entry in manifest['lods']:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(OUT/'Models'/(entry['name']+'.fbx')))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(meshes)==1
    obj=meshes[0];mesh=obj.data;mesh.calc_loop_triangles()
    assert len(mesh.loop_triangles)==entry['triangles']
    assert len(mesh.materials)==1 and mesh.materials[0]
    assert all(t.area>1e-9 for t in mesh.loop_triangles)
    assert all(abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
    assert mesh.uv_layers.active and all(math.isfinite(c) and 0<=c<=1 for uv in mesh.uv_layers.active.data for c in uv.uv)
    for t in mesh.loop_triangles:
        a,b,c=[mesh.uv_layers.active.data[i].uv for i in t.loops]
        assert abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))>1e-9
    pts=[obj.matrix_world@v.co for v in mesh.vertices]
    bounds=[min(p[i] for p in pts) for i in range(3)]+[max(p[i] for p in pts) for i in range(3)]
    error=max(abs(a-b) for a,b in zip(bounds,entry['bounds_m']));assert error<1e-5
    assert obj.location.length<1e-6 and abs(bounds[2])<1e-7
    report['lods'].append({'name':entry['name'],'triangles':len(mesh.loop_triangles),'bounds_error_m':error,'uv_valid':True,'normals_valid':True,'degenerate_faces':0,'pivot_ground_zero':True,'passed':True})
report['passed']=True
(OUT/'fbx_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('GRASS_FBX_RELOAD_PASSED')
