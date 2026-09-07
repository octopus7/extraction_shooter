"""Blender approximation of native default camera, not an in-game screenshot."""
import bpy
import json
import math
from pathlib import Path
from mathutils import Vector
from bpy_extras.object_utils import world_to_camera_view
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Memo/StorageDevice'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'SM_MemoStorageDevice.blend'))
scene=bpy.context.scene
obj=bpy.data.objects['SM_MemoStorageDevice']
for light in [o for o in scene.objects if o.type=='LIGHT']:
    light.hide_render=True
bpy.ops.object.light_add(type='SUN',rotation=(math.radians(25),math.radians(-35),math.radians(-20)))
bpy.context.object.data.energy=2
bpy.context.object.data.angle=math.radians(8)
cam=scene.camera
cam.data.type='PERSP'
cam.data.sensor_fit='HORIZONTAL'
cam.data.angle=math.radians(70)
cam.location=(-6,0,12*math.sin(math.radians(60)))
cam.rotation_euler=(Vector((0,0,0))-cam.location).to_track_quat('-Z','Y').to_euler()
scene.render.resolution_x=1920
scene.render.resolution_y=1080
scene.cycles.samples=16
rows=[]
for scale in [1,3]:
    obj.scale=(scale,)*3
    bpy.context.view_layer.update()
    points=[world_to_camera_view(scene,cam,obj.matrix_world@v.co) for v in obj.data.vertices]
    pixels=[(max(p.x for p in points)-min(p.x for p in points))*1920,
            (max(p.y for p in points)-min(p.y for p in points))*1080]
    rows.append({'uniform_scale':scale,'projected_pixel_size':pixels})
    scene.render.filepath=str(OUT/'Previews'/f'MemoDevice_Camera12m_Scale{scale}.png')
    bpy.ops.render.render(write_still=True)
(OUT/'camera_readability.json').write_text(json.dumps({'type':'Blender camera approximation; no UI marker, not an in-game capture',
    'camera_source':'TunaSweeperTopDownCharacter.cpp / .h native default: arm 1200 cm, pitch -60, horizontal FOV 70',
    'resolution':[1920,1080],'results':rows},indent=2),encoding='utf-8')
print('MEMO_CAMERA_CHECK_COMPLETE')
