import bpy,bmesh,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'BushRound.blend'))
objects=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert len(objects)==1
o=objects[0];assert o.name=='SM_BushRound' and o.location.length==0
assert tuple(o.scale)==(1,1,1) and o.rotation_euler.to_quaternion().angle<1e-6
m=o.data;assert len(m.uv_layers)==2 and len(m.materials)==1
tex=[n.image for n in m.materials[0].node_tree.nodes if n.type=='TEX_IMAGE'];assert len(tex)==1 and tex[0].packed_file
assert all(a>.999 for a in list(tex[0].pixels)[3::4])
bm=bmesh.new();bm.from_mesh(m)
todo=set(bm.faces);volumes=[]
while todo:
    faces={todo.pop()};stack=list(faces)
    while stack:
        f=stack.pop()
        for e in f.edges:
            assert e.is_manifold
            for other in e.link_faces:
                if other in todo:todo.remove(other);faces.add(other);stack.append(other)
    volumes.append(sum(f.verts[0].co.dot(f.verts[1].co.cross(f.verts[2].co))/6 for f in faces))
assert min(volumes)>0,volumes
bm.free()
paths=[OUT/'BushRound.blend',OUT/'Models/SM_BushRound.fbx',OUT/'Textures/T_BushRound_Palette.png']
report={'saved_blend_reopened':True,'mesh_objects':1,'packed_textures':1,'texture_alpha_all_one':True,'closed_components':len(volumes),'all_component_normals_outward':True,'minimum_component_volume_m3':min(volumes),'sha256':{str(p.relative_to(OUT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'passed':True}
(OUT/'source_reload_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('BUSHROUND_BLEND_RELOAD_PASS')
