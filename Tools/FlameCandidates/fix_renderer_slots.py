import unreal
root='/Game/Effects/FlameProjectiles/'
for name in ['NS_FlameProjectile_ScrollLance','NS_FlameProjectile_FlipbookComet','NS_FlameProjectile_Helix']:
    system=unreal.load_asset(root+name)
    for emitter_name in ['ProjectileCore','FlameTail']:
        emitter=unreal.find_object(system,emitter_name)
        renderers=[unreal.find_object(emitter,'NiagaraMeshRendererProperties_'+str(i)) for i in range(30)]
        renderers=[r for r in renderers if r]
        assert len(renderers)==1
        renderer=renderers[0]
        meshes=[m for m in renderer.get_editor_property('meshes') if m.get_editor_property('mesh')]
        assert len(meshes)==1
        renderer.set_editor_property('meshes',meshes)
    unreal.EditorAssetLibrary.save_loaded_asset(system)
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,unreal.NiagaraActor):
        actor.get_component_by_class(unreal.NiagaraComponent).reinitialize_system()
print('RENDERER_SLOTS_FIXED')
