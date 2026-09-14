import unreal

root = '/Game/Effects/FlameProjectiles/'
names = ['NS_FlameProjectile_ScrollLance', 'NS_FlameProjectile_FlipbookComet', 'NS_FlameProjectile_Helix']
systems = [unreal.load_asset(root + name) for name in names]
assert all(systems), 'Generate all three Niagara systems first'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.new_level('/Game/Effects/FlameProjectiles/L_FlameProjectile_Comparison')
for index, (name, system) in enumerate(zip(names, systems)):
    z = 460 - index * 170
    actor = actors.spawn_actor_from_class(unreal.NiagaraActor, unreal.Vector(110, 0, z))
    actor.set_actor_label(name)
    comp = actor.get_component_by_class(unreal.NiagaraComponent)
    comp.set_asset(system)
    comp.set_auto_activate(True)
    comp.activate(True)
    text = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-270, 0, z+55), unreal.Rotator(pitch=0,yaw=90,roll=0))
    text.set_actor_label('Label_'+str(index+1))
    tc=text.get_component_by_class(unreal.TextRenderComponent)
    tc.set_text(['01  SCROLL LANCE', '02  FLIPBOOK COMET', '03  FIRE HELIX'][index])
    tc.set_world_size(20)
    tc.set_text_render_color(unreal.Color(190,210,240,255))
camera = actors.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(-80,1100,330), unreal.Rotator(pitch=0,yaw=-90,roll=0))
camera.set_actor_label('ComparisonCamera')
cc = camera.get_component_by_class(unreal.CameraComponent)
cc.set_field_of_view(40)
cc.set_editor_property('aspect_ratio', 1.7777778)
cc.set_editor_property('constrain_aspect_ratio', True)
pp = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector())
pp.set_editor_property('unbound', True)
settings = pp.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_method', True)
settings.set_editor_property('auto_exposure_method', unreal.AutoExposureMethod.AEM_MANUAL)
settings.set_editor_property('override_auto_exposure_bias', True)
settings.set_editor_property('auto_exposure_bias', 8.0)
settings.set_editor_property('override_bloom_intensity', True)
settings.set_editor_property('bloom_intensity', 0.25)
pp.set_editor_property('settings', settings)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-80,850,320), unreal.Rotator(pitch=0,yaw=-90,roll=0))
levels.save_current_level()
unreal.EditorAssetLibrary.save_directory(root)
print('FLAME_PREVIEW_READY')
