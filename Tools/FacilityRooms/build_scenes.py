"""Assemble the same level placements in editable Blender scenes for review."""
from pathlib import Path
import json
import math
import sys
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'TunaSweeper/SourceArt/Environment/FacilityRooms'
layout=json.loads((SOURCE/'level_layout.json').read_text(encoding='utf-8'))
selected=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []

def point(xyz):return Vector((xyz[0]/100,-xyz[1]/100,xyz[2]/100))

for level in layout['levels']:
    if selected and level['name'] not in selected:continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version=0
    library={}
    for family in ['Architecture','Utilities','Control']:
        with bpy.data.libraries.load(str(SOURCE/f'FacilityRooms_{family}.blend'),link=False) as (available,loaded):
            loaded.objects=[n for n in available.objects if n.startswith('SM_FacilityRooms_')]
        for obj in loaded.objects:library[obj.name]=obj
    scene=bpy.context.scene
    roofs=[];cutaway=[]
    for spec in level['placements']:
        original=library[spec['mesh']]
        obj=bpy.data.objects.new(spec['label'],original.data)
        scene.collection.objects.link(obj)
        obj.location=point(spec['location_cm']);obj.rotation_euler.z=-math.radians(spec['yaw_deg'])
        if spec.get('review_hidden'):roofs.append(obj)
        x,y,z=spec['location_cm'];width,depth=level['size_cm']
        if spec['zone'] in {'Architecture','AtticTimber'} and ('Wall' in spec['mesh'] or 'Gable' in spec['mesh']):
            if x==width/2 or y==-depth/2:cutaway.append(obj)
    # Loaded library objects are unlinked templates; their mesh datablocks remain shared.
    for obj in list(library.values()):bpy.data.objects.remove(obj,do_unlink=True)
    scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
    scene.render.resolution_x=1800;scene.render.resolution_y=1400;scene.render.resolution_percentage=100
    scene.view_settings.view_transform='AgX'
    world=bpy.data.worlds.new('FacilityReviewWorld');world.use_nodes=True
    world.node_tree.nodes['Background'].inputs[0].default_value=(.16,.20,.25,1)
    world.node_tree.nodes['Background'].inputs[1].default_value=.4;scene.world=world
    target=point(level['camera']['target_cm'])
    attic='Control' in level['name']
    forest=None
    if attic:
        spec=level['forest'];center=point(spec['location_cm'])
        mesh=bpy.data.meshes.new('ForestImpostorQuad')
        mesh.from_pydata([(0,4,-3),(0,-4,-3),(0,-4,3),(0,4,3)],[],[(0,1,2,3)])
        uv=mesh.uv_layers.new(name='UVMap')
        for loop,value in zip(uv.data,[(0,0),(1,0),(1,1),(0,1)]):loop.uv=value
        forest=bpy.data.objects.new('Exterior_ForestImpostor',mesh);scene.collection.objects.link(forest);forest.location=center
        forest.visible_shadow=False
        mat=bpy.data.materials.new('M_FacilityRooms_ForestImpostor');mat.use_nodes=True
        nodes=mat.node_tree.nodes;nodes.clear()
        out=nodes.new('ShaderNodeOutputMaterial');em=nodes.new('ShaderNodeEmission');tex=nodes.new('ShaderNodeTexImage')
        tex.image=bpy.data.images.load(str(SOURCE/'Textures/T_FacilityRooms_Forest.png'));tex.image.pack()
        tex.image.filepath='//Textures/T_FacilityRooms_Forest.png'
        mat.node_tree.links.new(tex.outputs['Color'],em.inputs['Color']);em.inputs['Strength'].default_value=.7
        mat.node_tree.links.new(em.outputs[0],out.inputs['Surface']);mesh.materials.append(mat)
        spec=level['window_light'];data=bpy.data.lights.new('WindowSunBeam','SPOT');data.energy=1800
        data.color=(1,.82,.57);data.spot_size=math.radians(spec['outer_cone_deg']*2);data.spot_blend=.15;data.shadow_soft_size=.05
        beam=bpy.data.objects.new('REVIEW_WindowSunBeam',data);scene.collection.objects.link(beam)
        beam.location=point(spec['location_cm']);beam.rotation_euler=(point(spec['target_cm'])-beam.location).to_track_quat('-Z','Y').to_euler()
    light_power=3600 if attic else 6500
    for label,offset,color,power in [('Key',(-6,5,11),(1,.78,.54) if attic else (1,.90,.76),light_power),
                                    ('Fill',(7,2,10),(.76,.86,1),light_power*.7),
                                    ('Rim',(0,-8,12),(1,.86,.69),light_power)]:
        data=bpy.data.lights.new(label,'AREA');data.energy=power;data.shape='DISK';data.size=8
        light=bpy.data.objects.new('REVIEW_'+label,data);scene.collection.objects.link(light)
        light.location=target+Vector(offset);light.rotation_euler=(target-light.location).to_track_quat('-Z','Y').to_euler()
        data.color=color
    for spec in level['placements']:
        if spec['mesh'].endswith(('AtticLamp','WallLamp')):
            data=bpy.data.lights.new('Practical','POINT');data.energy=35 if attic else 60;data.color=(1,.55,.22);data.shadow_soft_size=.22
            light=bpy.data.objects.new('REVIEW_'+spec['label'],data);scene.collection.objects.link(light)
            light.location=point(spec['location_cm'])+Vector((0,-.25,.3))
    data=bpy.data.cameras.new('FacilityCamera');camera=bpy.data.objects.new('REVIEW_Camera',data)
    scene.collection.objects.link(camera);camera.location=point(level['camera']['location_cm'])
    camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
    data.type='ORTHO';data.ortho_scale=11 if attic else 18;scene.camera=camera
    for obj in roofs:obj.hide_render=True
    for obj in cutaway:obj.hide_render=True
    if forest:forest.hide_render=True
    scene.render.filepath=str(SOURCE/'Previews'/(level['name']+'.png'));bpy.ops.render.render(write_still=True)
    for obj in cutaway:obj.hide_render=False
    # A direct overhead view makes floor layout and hatch openings inspectable.
    camera.location=target+Vector((0,0,25));camera.rotation_euler=(0,0,0)
    data.ortho_scale=8.4 if attic else 15.8
    scene.render.filepath=str(SOURCE/'Previews'/(level['name']+'_Top.png'));bpy.ops.render.render(write_still=True)
    if attic:
        for obj in roofs:obj.hide_render=False
        forest.hide_render=False
        spec=level['interior_camera'];camera.location=point(spec['location_cm'])
        camera.rotation_euler=(point(spec['target_cm'])-camera.location).to_track_quat('-Z','Y').to_euler()
        data.type='PERSP';data.angle=math.radians(spec['fov'])
        scene.render.filepath=str(SOURCE/'Previews'/(level['name']+'_Interior.png'));bpy.ops.render.render(write_still=True)
    else:
        camera.location=point(level['camera']['location_cm']);camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
        data.ortho_scale=18
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE/(level['name']+'.blend')))
    print('FACILITY_SCENE_RENDERED',level['name'],len(level['placements']))
