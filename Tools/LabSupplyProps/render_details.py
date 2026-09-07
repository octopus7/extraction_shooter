"""Render individual source-model proofs, never modify or regenerate UE assets."""
from pathlib import Path
import bpy,json,math
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'TunaSweeper/SourceArt/Environment/LabSupplyProps'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LabSupplyProps.blend'))
s=bpy.context.scene;s.render.resolution_x=640;s.render.resolution_y=640;s.cycles.samples=24
for c in bpy.data.collections:
    if c.name in ('Lab','Warehouse','Sheet','Scale reference'):c.hide_render=True;c.hide_viewport=True
s.node_tree.nodes.get('Flip').mute=True
cam=s.camera;cam.data.type='ORTHO'
collection=bpy.data.collections.new('Individual proof');s.collection.children.link(collection)
manifest=json.loads((OUT/'model_manifest.json').read_text())
for e in manifest['assets']:
    src=bpy.data.objects[e['name']];o=src.copy();o.data=src.data;collection.objects.link(o);o.hide_render=False;o.hide_set(False);o.location=(0,0,0);o.color=(.15,.2,0,1)
    z=e['dimensions_m'][2]/2;size=max(e['dimensions_m']);cam.location=(size*1.5,-size*2.6,z+size*1.6);cam.rotation_euler=(Vector((0,0,z))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=size*1.8
    s.render.filepath=str(OUT/'Previews'/('Model_'+e['key']+'.png'));bpy.ops.render.render(write_still=True)
    if e['key']=='Workbench':
        strength=bpy.data.materials['M_LSP_Surface'].node_tree.nodes['DirtStrength']
        for label,value in [('Base',0),('Dust',1.8)]:
            strength.inputs[1].default_value=value;s.render.filepath=str(OUT/'Previews'/('CloseWear_'+label+'.png'));bpy.ops.render.render(write_still=True)
        strength.inputs[1].default_value=.35
    bpy.data.objects.remove(o,do_unlink=True)
print('LSP_DETAIL_RENDERS_PASSED')
