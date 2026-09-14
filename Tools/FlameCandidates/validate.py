import unreal
root='/Game/Effects/FlameProjectiles/'
paths=unreal.EditorAssetLibrary.list_assets(root,recursive=True,include_folder=False)
assert len(paths)==16, f'Expected 16 assets including preview map, got {len(paths)}'
for path in paths:
    assert unreal.load_asset(path), path
for name in ['NS_FlameProjectile_ScrollLance','NS_FlameProjectile_FlipbookComet','NS_FlameProjectile_Helix']:
    system=unreal.load_asset(root+name)
    for emitter_name in ['ProjectileCore','FlameTail','TrailingEmbers']:
        emitter=unreal.find_object(system,emitter_name)
        assert emitter, emitter_name
        if emitter_name!='TrailingEmbers':
            renderer=next(r for i in range(30) if (r:=unreal.find_object(emitter,'NiagaraMeshRendererProperties_'+str(i))))
            meshes=renderer.get_editor_property('meshes')
            assert len(meshes)==1 and meshes[0].get_editor_property('mesh'), name
            mesh=meshes[0].get_editor_property('mesh')
            assert mesh.get_num_lods()>0
            assert mesh.get_material(0), mesh.get_path_name()
    print('VALIDATED '+name+' : core mesh + tail mesh + ember emitter')
atlas=unreal.load_asset(root+'T_FlameComet_Flipbook4x4')
assert atlas.blueprint_get_size_x()==512 and atlas.blueprint_get_size_y()==512
print('VALIDATED 16 saved assets; 512x512 atlas / 4x4 frames')
