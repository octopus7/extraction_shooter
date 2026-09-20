"""One-off creation of standalone animations only. Never saves shared skeletons or blueprints."""
import unreal,json
from pathlib import Path
ROOT=Path('D:/github/extraction_shooter')
SOURCE=ROOT/'TunaSweeper/SourceArt/Characters/LunaMk2/TitleMotions'
DEST='/Game/Characters/Player/LunaMk2/Animations/Title'
sk=unreal.load_asset('/Game/Characters/Player/LunaMk2/SKM_LunaMk2_Skeleton')
tools=unreal.AssetToolsHelpers.get_asset_tools()
for suffix in ['A','B','C','AtoB','BtoA']:
    name='AS_LunaMk2_Title_'+suffix
    data=json.loads((SOURCE/(name+'.json')).read_text())
    clip=unreal.load_asset(DEST+'/'+name) if unreal.EditorAssetLibrary.does_asset_exist(DEST+'/'+name) else None
    if not clip:
        factory=unreal.AnimSequenceFactory();factory.set_editor_property('target_skeleton',sk)
        clip=tools.create_asset(name,DEST,unreal.AnimSequence,factory)
    assert isinstance(clip,unreal.AnimSequence)
    ctl=clip.get_editor_property('controller')
    ctl.open_bracket('Author standalone title motion',False)
    ctl.set_frame_rate(unreal.FrameRate(data['fps'],1),False)
    ctl.set_number_of_frames(unreal.FrameNumber(data['frames']),False)
    ctl.remove_all_bone_tracks(False)
    for bone,keys in data['tracks'].items():
        assert ctl.add_bone_curve(bone,False),bone
        assert ctl.set_bone_track_keys(bone,[unreal.Vector(*v) for v in keys['p']],[unreal.Quat(*v) for v in keys['q']],[unreal.Vector(1,1,1)]*len(keys['p']),False),bone
    ctl.close_bracket(False)
    clip.set_editor_property('enable_root_motion',False)
    assert unreal.EditorAssetLibrary.save_loaded_asset(clip,False),name
    unreal.log('TITLE_MOTION_IMPORTED '+clip.get_path_name())
unreal.log('TITLE_MOTIONS_IMPORT_COMPLETE')
