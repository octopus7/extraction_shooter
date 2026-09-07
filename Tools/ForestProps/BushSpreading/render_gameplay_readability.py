"""C++ default gameplay-camera approximation, no runtime/BP overrides."""
from pathlib import Path
import bpy,math,json
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'SM_BushSpreading.blend'))
obj=bpy.data.objects['SM_BushSpreading'];scene=bpy.context.scene
for x,y,yaw,s in [(-2,-1.6,.5,.86),(0,-1.8,2.4,1.04),(2,-1.7,5.2,.93),(-2.3,0,1.8,1.12),(2.1,.2,4.1,.87),(-2,1.6,3.3,.92),(.1,1.7,6.1,1.06),(2.2,1.8,.8,1)]:
    dup=bpy.data.objects.new('PreviewInstance',obj.data);scene.collection.objects.link(dup)
    dup.location=(x,y,0);dup.rotation_euler.z=yaw;dup.scale=(s,s,s)
cam=scene.camera;target=Vector((0,0,.8))
cam.location=target+Vector((-6,0,12*math.sin(math.radians(60))))
cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='PERSP';cam.data.sensor_fit='HORIZONTAL';cam.data.angle=math.radians(70)
scene.render.resolution_x=1920;scene.render.resolution_y=1080;scene.render.resolution_percentage=100
scene.cycles.samples=16;scene.render.filepath=str(OUT/'Previews/gameplay_distance.png')
bpy.ops.render.render(write_still=True)
(OUT/'gameplay_preview_settings.json').write_text(json.dumps({'camera_distance_m':12,'pitch_degrees':-60,'horizontal_fov_degrees':70,'resolution':[1920,1080],'target_height_m':.8,'note':'C++ default camera approximation, not an in-game screenshot; excludes BP overrides, aiming and alternate camera modes.','source':'TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacter.cpp:44'},indent=2))
