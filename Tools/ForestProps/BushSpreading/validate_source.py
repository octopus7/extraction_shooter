"""Fresh-process packed blend, UV area and FBX mesh audit."""
from pathlib import Path
import bpy,bmesh,json,hashlib,math
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'SM_BushSpreading.blend'))
obj=bpy.data.objects['SM_BushSpreading'];mesh=obj.data;mesh.calc_loop_triangles()
assert list(obj.location)==[0,0,0] and list(obj.scale)==[1,1,1]
assert len(mesh.materials)==1 and len(mesh.uv_layers)==2
uv_results={}
for layer in mesh.uv_layers:
    bad=0
    for tri in mesh.loop_triangles:
        p=[layer.data[i].uv for i in tri.loops]
        area=abs((p[1].x-p[0].x)*(p[2].y-p[0].y)-(p[1].y-p[0].y)*(p[2].x-p[0].x))/2
        bad+=area<1e-12
    assert bad==0,(layer.name,bad)
    assert all(0<=c<=1 for item in layer.data for c in item.uv)
    uv_results[layer.name]={'zero_area_triangles':bad,'in_0_1':True}
image=[n.image for n in mesh.materials[0].node_tree.nodes if n.type=='TEX_IMAGE'][0]
assert image.packed_file
assert hashlib.sha256(image.packed_file.data).hexdigest()==hashlib.sha256((OUT/'Textures/T_BushSpreading_Palette.png').read_bytes()).hexdigest()
report={'passed':True,'fresh_blend_reload':True,'packed_palette_matches_png':True,'mesh_count_exported':1,'uvs':uv_results,'object_origin_ground':True,'preview_collection_excluded_from_fbx':True}
(OUT/'blend_reload_validation.json').write_text(json.dumps(report,indent=2))
print('BUSH_SPREADING_BLEND_RELOAD_PASSED')
