import unreal
root='/Game/Effects/LavaGround/'
assets=unreal.EditorAssetLibrary.list_assets(root,recursive=True,include_folder=False)
assert len(assets)==20, assets
for path in assets:
    assert unreal.load_asset(path),path
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
    if not isinstance(a,unreal.NiagaraActor):continue
    c=a.get_component_by_class(unreal.NiagaraComponent)
    name='NS_LavaImpactBurst' if '01_' in a.get_actor_label() else 'NS_LavaGround'
    a.set_is_temporarily_hidden_in_editor(False)
    c.set_asset(unreal.load_asset(root+name));c.set_paused(False)
    c.reinitialize_system();c.advance_simulation(840,1/60)
    assert not c.is_active(),name+' remained active after 14 seconds'
    c.set_asset(unreal.load_asset(root+name+'_PreviewLoop'));c.reinitialize_system()
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.EditorAssetLibrary.save_directory(root)
print('VALIDATED: 20 assets load, both standalone systems finish, preview restored')
