import unreal as u
dest='/Game/Characters/NPC/Mole'
mesh=u.load_asset(dest+'/SKM_MoleDummy'); blend=u.load_asset(dest+'/BS_Mole_IdleTurn')
bp=u.load_asset('/Game/Characters/Mole/BP_Mole')
assert mesh and blend and bp
u.BlueprintEditorLibrary.compile_blueprint(bp)
def setup(comp):
    comp.set_skeletal_mesh_asset(mesh)
    comp.set_relative_rotation(u.Rotator(pitch=0,yaw=-90,roll=0),False,False)
    comp.set_visibility(True,True); comp.set_hidden_in_game(False,True)
    comp.override_animation_data(blend,True,True,0,1)
cdo=u.get_default_object(bp.generated_class())
cdo.set_editor_property('idle_turn_blend_space',blend)
setup(cdo.get_component_by_class(u.SkeletalMeshComponent))
subsys=u.get_engine_subsystem(u.SubobjectDataSubsystem)
for handle in subsys.k2_gather_subobject_data_for_blueprint(bp):
    data=u.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    obj=u.SubobjectDataBlueprintFunctionLibrary.get_object(data)
    if isinstance(obj,u.SkeletalMeshComponent): setup(obj)
u.BlueprintEditorLibrary.compile_blueprint(bp)
assert u.EditorAssetLibrary.save_loaded_asset(bp,False)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/BunkerMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
moles=[a for a in actors if isinstance(a,u.TunaSweeperMoleCompanionActor)]
assert moles,'No placed mole in BunkerMap'
for actor in moles:
    actor.set_editor_property('idle_turn_blend_space',blend)
    setup(actor.get_component_by_class(u.SkeletalMeshComponent))
    assert not actor.get_components_by_class(u.StaticMeshComponent),'Old static mesh still attached'
    print('PLACED_MOLE',actor.get_path_name(),actor.get_actor_location(),actor.get_actor_scale3d())
assert level.save_current_level()
print('BP_MOLE_CONNECTED',len(moles))
