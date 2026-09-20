"""Render the saved actions as a continuous C -> A -> B -> A preview."""
import bpy,json
from pathlib import Path
ROOT=Path('D:/github/extraction_shooter')
OUT=ROOT/'TunaSweeper/SourceArt/Characters/LunaMk2/TitleMotions'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'LunaMk2_TitleMotions.blend'))
scene=bpy.context.scene;rig=bpy.data.objects['SK_LunaMk2']
scene.render.resolution_x=480;scene.render.resolution_y=600
scene.cycles.samples=8;scene.cycles.use_denoising=True
frames=ROOT/'TunaSweeper/Saved/TitleMotions/VideoFrames';frames.mkdir(parents=True,exist_ok=True)
frame_index=0;timeline=[]
for suffix,duration in [('C',3)]:
    rig.animation_data.action=bpy.data.actions['AS_LunaMk2_Title_'+suffix]
    timeline.append({'clip':suffix,'start_seconds':frame_index/15,'duration':duration})
    for f in range(round(duration*15)):
        scene.frame_set(f*2+1)
        scene.render.filepath=str(frames/f'{frame_index:04d}.png')
        bpy.ops.render.render(write_still=True);frame_index+=1

print('TITLE_MOTION_PREVIEW_FRAMES_COMPLETE',frame_index)
