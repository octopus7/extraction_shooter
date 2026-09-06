"""Render repository Blender reference meshes without changing their originals."""
import bpy, json, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved/References'
OUT.mkdir(parents=True, exist_ok=True)
report = []
files=['Bush_Combined.fbx'] if '--bush' in sys.argv else ['SM_GrassLow.blend', 'SM_Flower.blend', 'SM_SimpleTree.blend', 'Wood.blend', 'RockBasic.blend']
for filename in files:
    if filename.endswith('.fbx'):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(ROOT/'TunaSweeper/Saved/GrassLongCurved'/filename))
        for obj in list(bpy.context.scene.objects):
            if obj.name.startswith('UCX_'):bpy.data.objects.remove(obj,do_unlink=True)
        im=bpy.data.images.load(str(ROOT/'TunaSweeper/Saved/GrassLongCurved/Bush_BaseColor.tga'))
        for mat in bpy.data.materials:
            mat.use_nodes=True; bsdf=mat.node_tree.nodes.get('Principled BSDF')
            tex=mat.node_tree.nodes.new('ShaderNodeTexImage');tex.image=im
            mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color']);bsdf.inputs['Roughness'].default_value=.9
    else:
        bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Blender'/filename))
    scene=bpy.context.scene
    objects=[o for o in scene.objects if o.type=='MESH']
    report.append({'source':'/Game/Nature/Bush/Bush_Combined' if filename.endswith('.fbx') else 'Blender/'+filename,'objects':[{'name':o.name,'dimensions':list(o.dimensions),'polygons':len(o.data.polygons),'materials':[m.name if m else None for m in o.data.materials]} for o in objects]})
    for o in scene.objects:
        if o.type in ['LIGHT','CAMERA']: o.hide_render=True
    for im in bpy.data.images:
        if im.source=='FILE' and not im.packed_file:
            basename=im.filepath.replace('\\','/').rsplit('/',1)[-1]
            candidates=list((ROOT/'Blender').rglob(basename))
            if candidates:
                im.filepath=str(candidates[0]); im.reload()
    pts=[o.matrix_world@Vector(c) for o in objects for c in o.bound_box]
    lo=Vector(tuple(min(p[i] for p in pts) for i in range(3)))
    hi=Vector(tuple(max(p[i] for p in pts) for i in range(3)))
    center=(lo+hi)/2; size=max(hi-lo)
    data=bpy.data.cameras.new('ReferenceCamera'); cam=bpy.data.objects.new('ReferenceCamera',data); scene.collection.objects.link(cam)
    cam.location=center+Vector((1,-1.7,1.2))*size
    cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler()
    data.type='ORTHO'; data.ortho_scale=size*1.45; data.clip_end=size*100; scene.camera=cam
    scene.render.engine='CYCLES'; scene.cycles.samples=16
    scene.render.resolution_x=1000; scene.render.resolution_y=760; scene.render.resolution_percentage=100
    scene.world=bpy.data.worlds.new('ReferenceWorld'); scene.world.use_nodes=True
    scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.22,.25,.28,1)
    scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
    data=bpy.data.lights.new('ReferenceSun','SUN'); data.energy=2.2; data.angle=.2
    sun=bpy.data.objects.new('ReferenceSun',data); scene.collection.objects.link(sun); sun.rotation_euler=(.4,-.6,-.4)
    scene.view_settings.view_transform='Standard'
    scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(OUT/(Path(filename).stem+'.png'))
    bpy.ops.render.render(write_still=True)
(OUT/('bush_render_inventory.json' if '--bush' in sys.argv else 'reference_inventory.json')).write_text(json.dumps(report,indent=2),encoding='utf-8')
