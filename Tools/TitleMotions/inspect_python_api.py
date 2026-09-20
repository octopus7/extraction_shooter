import unreal
for cls in [unreal.AnimSequence,unreal.AnimSequenceFactory,unreal.AnimationDataController]:
    print(cls.__name__, [n for n in dir(cls) if any(s in n for s in ['controller','model','bone','frame','skeleton','initialize'])])
for name in ['initialize_model','set_frame_rate','set_number_of_frames','add_bone_track','set_bone_track_keys','notify_populated']:
    fn=getattr(unreal.AnimationDataController,name,None)
    print(name,getattr(fn,'__doc__',None))
