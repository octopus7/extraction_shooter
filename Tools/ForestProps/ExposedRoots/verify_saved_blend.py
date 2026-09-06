"""Freshly reopen the delivered blend and check that its render textures are embedded."""
import bpy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'ExposedRoots.blend'))
obj=bpy.data.objects['SM_ExposedRoots'];assert obj.location.length<1e-6
images=[]
for item in [obj,bpy.data.objects['PREVIEW_ONLY_ExistingWoodStump']]:
    for mat in item.data.materials:
        for node in mat.node_tree.nodes:
            if node.type=='TEX_IMAGE':
                assert node.image and node.image.packed_file,(item.name,node.image.name if node.image else None,'not packed')
                _=node.image.pixels[0] # Decoding packed pixels is lazy after opening a blend.
                assert node.image.has_data,(item.name,node.image.name,'pixels unavailable')
                images.append({'object':item.name,'image':node.image.name,'size':list(node.image.size),'packed':True})
assert len(obj.data.uv_layers)==2 and len(obj.data.materials)==1
(OUT/'blend_reload_validation.json').write_text(json.dumps({'passed':True,'images':images,'origin':list(obj.location)},indent=2),encoding='utf-8')
