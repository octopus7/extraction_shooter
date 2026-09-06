"""Fresh Blender process: reload FBX and check geometry, normals, UVs, texture."""
import bpy
import json
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/GrassDenseShort'
m=json.loads((OUT/'model_manifest.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(OUT/'Models/SM_GrassDenseShort.fbx'))
objects=[o for o in bpy.context.selected_objects if o.type=='MESH']
assert len(objects)==1
o=objects[0];mesh=o.data;mesh.calc_loop_triangles()
coords=[o.matrix_world@v.co for v in mesh.vertices]
bounds=[min(v[i] for v in coords) for i in range(3)]+[max(v[i] for v in coords) for i in range(3)]
error=max(abs(a-b) for a,b in zip(bounds,m['bounds_m']))
assert error<1e-5,(bounds,m['bounds_m'])
assert len(mesh.loop_triangles)==m['triangles']
assert len(mesh.materials)==1 and len(mesh.uv_layers)==1
assert all(p.area>1e-10 and abs(p.normal.length-1)<1e-5 for p in mesh.polygons)
assert all(0<u.uv.x<1 and 0<u.uv.y<1 for u in mesh.uv_layers[0].data)
assert o.location.length<1e-5
image_nodes=[n for n in mesh.materials[0].node_tree.nodes if n.type=='TEX_IMAGE']
assert any(n.image and n.image.size[0]==64 for n in image_nodes)
result={'passed':True,'mesh_count':len(objects),'triangles':len(mesh.loop_triangles),
        'bounds_m':bounds,'bounds_max_error_m':error,'uv_channels':len(mesh.uv_layers),
        'materials':len(mesh.materials),'normals_unit_length':True,'degenerate_faces':0,
        'embedded_texture_loaded':True,'ground_pivot':True}
(OUT/'fbx_reload_validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))
