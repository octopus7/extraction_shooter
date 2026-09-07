"""Render an annotated catalogue from the saved Blender assets, without re-export.

This review scene normalizes individual display sizes; labels retain real cm.
The authored source scenes, FBX files and Unreal assets are not modified.
"""
import bpy, json, math, sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/InteriorAdditions'
bpy.ops.wm.read_factory_settings(use_empty=True)
entries=[];objects={}
for f in sorted((OUT/'Manifests').glob('*.json')):
    manifest=json.loads(f.read_text(encoding='utf-8'));entries.extend(manifest['assets'])
    with bpy.data.libraries.load(str(OUT/f"InteriorAdditions_{manifest['category']}.blend"),link=False) as (source,dest):
        dest.objects=[n for n in source.objects if n.startswith('SM_InteriorAdditions_')]
    for o in dest.objects:
        bpy.context.scene.collection.objects.link(o);objects[o.name]=o;o.location=(0,0,0);o.hide_render=True
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=True
scene.view_settings.view_transform='AgX'
scene.world=bpy.data.worlds.new('CatalogWorld');scene.world.use_nodes=True;scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.7
floor_mat=bpy.data.materials.new('CatalogCream');floor_mat.use_nodes=True;floor_mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.44,.38,.31,1);floor_mat.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value=.95
ink=bpy.data.materials.new('CatalogInk');ink.use_nodes=True;ink.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value=(.035,.024,.018,1)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.012));bpy.context.object.data.materials.append(floor_mat)
lights=[]
for name,loc,energy,size in [('Key',(-8,-6,14),2900,9),('Fill',(8,-1,10),2200,9),('Back',(0,12,14),3200,10)]:
    d=bpy.data.lights.new(name,'AREA');d.energy=energy;d.shape='DISK';d.size=size;o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=loc;lights.append(o)
d=bpy.data.cameras.new('CatalogCamera');cam=bpy.data.objects.new('CatalogCamera',d);scene.collection.objects.link(cam);d.type='ORTHO';scene.camera=cam

def render(label,subset,columns):
    texts=[];rows=math.ceil(len(subset)/columns);spacing=2.8;row_spacing=3.6
    for o in objects.values():o.hide_render=True
    for i,e in enumerate(subset):
        o=objects[e['name']];o.hide_render=False
        factor=1.65/(max(e['size_cm'])/100);o.scale=(factor,factor,factor);o.rotation_euler=(0,0,math.radians(-22))
        x=(i%columns-(columns-1)/2)*spacing;y=(rows-1-i//columns)*row_spacing
        o.location=(x,y,0)
        for line,text in enumerate([e['name'].replace('SM_InteriorAdditions_',''), ' x '.join(str(round(v)) for v in e['size_cm'])+' cm']):
            data=bpy.data.curves.new('Caption','FONT');data.body=text;data.align_x='CENTER';data.size=.135 if line==0 else .105;data.extrude=0
            obj=bpy.data.objects.new('Caption',data);scene.collection.objects.link(obj);obj.location=(x,y-1.02-line*.2,.015);obj.data.materials.append(ink);texts.append(obj)
    target=Vector((0,(rows-1)*row_spacing/2,.25));cam.location=target+Vector((0,-15,20));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
    for l in lights:l.rotation_euler=(target-l.location).to_track_quat('-Z','Y').to_euler()
    d.ortho_scale=columns*spacing+.4
    scene.render.resolution_x=columns*420;scene.render.resolution_y=rows*440+90;scene.render.resolution_percentage=100
    scene.render.filepath=str(OUT/'Previews'/f'{label}_Catalog.png');bpy.ops.render.render(write_still=True)
    for o in texts:bpy.data.objects.remove(o,do_unlink=True)

for category in ['Kitchen','BathroomLaundry','Furniture']:
    render(category,[e for e in entries if e['category']==category],4)
render('All',entries,6)
print('INTERIOR_CATALOG_RENDERED '+str(len(entries)))
