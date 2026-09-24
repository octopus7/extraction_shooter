"""Read-only authored-asset checks; visual motion/PIE acceptance is separate."""
import unreal as u

ROOT = '/Game/FX/Waterfall'


def verify():
    required = {
        'M_Waterfall_Curtain': u.Material,
        'M_Waterfall_Foam': u.Material,
        'M_Waterfall_Spray': u.Material,
        'M_Waterfall_Mist': u.Material,
        'M_Waterfall_TestRock': u.Material,
        'M_Waterfall_TestGround': u.Material,
        'MI_Waterfall_UV': u.MaterialInstanceConstant,
        'SM_Waterfall_UV': u.StaticMesh,
        'NS_Waterfall_Stream_UV': u.NiagaraSystem,
        'NS_Waterfall_Splash_Mist': u.NiagaraSystem,
        'T_Waterfall_Flow': u.Texture2D,
        'T_Waterfall_FoamBreakup': u.Texture2D,
    }
    loaded = {}
    for name, cls in required.items():
        asset = u.load_asset(ROOT + '/' + name)
        assert isinstance(asset, cls), 'Missing or wrong asset: ' + name
        loaded[name] = asset
    assert loaded['MI_Waterfall_UV'].get_editor_property('parent') == loaded['M_Waterfall_Curtain']
    assert loaded['M_Waterfall_Curtain'].get_editor_property('two_sided')
    assert loaded['M_Waterfall_Curtain'].get_editor_property('blend_mode') == u.BlendMode.BLEND_TRANSLUCENT
    for material, texture in [('M_Waterfall_Curtain', 'T_Waterfall_Flow'), ('M_Waterfall_Foam', 'T_Waterfall_FoamBreakup')]:
        assert loaded[texture] in u.MaterialEditingLibrary.get_used_textures(loaded[material]), material + ' is not using its generated texture'
        assert not loaded[texture].get_editor_property('srgb'), texture + ' must be linear mask data'
        assert loaded[texture].get_editor_property('mip_gen_settings') != u.TextureMipGenSettings.TMGS_NO_MIPMAPS, texture + ' needs mipmaps for world-space rendering'
        assert loaded[texture].get_editor_property('address_x') == u.TextureAddress.TA_WRAP
        assert loaded[texture].get_editor_property('address_y') == u.TextureAddress.TA_WRAP
        assert loaded[texture].get_editor_property('power_of_two_mode') == u.TexturePowerOfTwoSetting.RESIZE_TO_SPECIFIC_RESOLUTION
        assert loaded[texture].get_editor_property('resize_during_build_x') == 1024
        assert loaded[texture].get_editor_property('resize_during_build_y') == 1024
        assert not loaded[texture].get_editor_property('never_stream')
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    if not world or world.get_name() != 'L_WaterfallTest':
        world = u.EditorLoadingAndSavingUtils.load_map(ROOT + '/L_WaterfallTest')
    assert world, 'Test level failed to load'
    placed = {a.get_actor_label(): a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()}
    curtain = placed['Waterfall_Curtain']
    origin, extent = curtain.get_actor_bounds(False)
    assert extent.x >= 130 and extent.z >= 200 and extent.y < 30, 'Water curtain must span width and height, not be a narrow creek ribbon'
    # Both generated-texture layers subtract time from V: increasing V must point down.
    desc = loaded['SM_Waterfall_UV'].get_static_mesh_description(0)
    top, bottom = [], []
    for i in range(desc.get_vertex_instance_count()):
        instance = u.VertexInstanceID(i)
        uv = desc.get_vertex_instance_uv(instance, 0)
        point = desc.get_vertex_position(desc.get_vertex_instance_vertex(instance))
        height = u.MathLibrary.transform_location(curtain.get_actor_transform(), point).z
        (top if uv.y < 0.5 else bottom).append(height)
    assert top and bottom and min(top) > max(bottom) + 400, 'Positive V must flow down the curtain'
    assert placed['Waterfall_Review_Camera'].get_actor_location().y < -500
    foam_component = placed['Waterfall_Foam'].get_component_by_class(u.StaticMeshComponent)
    assert foam_component.get_material(0) == loaded['M_Waterfall_Foam']
    assert foam_component.is_visible() and not foam_component.get_editor_property('hidden_in_game'), 'Keep the foam surface visible beneath the splash'
    for label, name in [('Waterfall_Stream', 'NS_Waterfall_Stream_UV'), ('Waterfall_Impact', 'NS_Waterfall_Splash_Mist')]:
        component = placed[label].get_component_by_class(u.NiagaraComponent)
        assert component.get_asset() == loaded[name]
        assert component.get_editor_property('auto_activate'), label + ' must start automatically'
        # Niagara asset-editor recompilation can stop the existing preview instance.
        # Start an independent simulation without changing or saving authored settings.
        component.reinitialize_system()
        component.advance_simulation(600, 1.0 / 60.0)
        assert component.is_active(), label + ' stopped within ten seconds'
        params = u.NiagaraSimCacheCreateParameters()
        params.set_editor_property('attribute_capture_mode', u.NiagaraSimCacheAttributeCaptureMode.ALL)
        target_cache = u.NiagaraSimCache()
        cache = u.NiagaraSimCacheFunctionLibrary.capture_niagara_sim_cache_immediate(target_cache, params, component, True, 1.0 / 60.0)
        assert cache, 'Unable to capture particles: ' + label
        expected = {'FallingWater'} if label == 'Waterfall_Stream' else {'ImpactDroplets', 'DriftingMist'}
        assert {str(e) for e in cache.get_emitter_names()} == expected, 'Missing or unexpected emitters: ' + label
        for emitter in cache.get_emitter_names():
            points = cache.read_position_attribute('Position', emitter, False, 0)
            velocities = cache.read_vector_attribute('Velocity', emitter, 0)
            assert len(points) >= 20, str(emitter) + ' has too few live particles after ten seconds'
            assert len(velocities) == len(points), 'Missing velocity samples: ' + str(emitter)
            width = max(p.x for p in points) - min(p.x for p in points)
            mean_vz = sum(v.z for v in velocities) / len(velocities)
            u.log('WATERFALL_PARTICLES emitter=%s count=%d width=%.1f mean_vz=%.1f' % (emitter, len(points), width, mean_vz))
            assert width > 180, str(emitter) + ' is concentrated at the center instead of across the waterfall'
            if str(emitter) == 'FallingWater':
                assert mean_vz < -400, 'Stream particles are not falling at the authored speed'
            elif str(emitter) == 'DriftingMist':
                assert mean_vz > 5, 'Mist particles are not rising'
                assert len(points) >= 180, 'Impact mist is too sparse to veil the pool center'
            elif str(emitter) == 'ImpactDroplets':
                assert len(points) >= 280, 'Impact spray has not been intensified'
    registry = u.AssetRegistryHelpers.get_asset_registry()
    options = u.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    for name in [*required, 'L_WaterfallTest']:
        dependencies = registry.get_dependencies(ROOT + '/' + name, options)
        assert not any(str(d).startswith('/CascadeToNiagaraConverter') for d in dependencies), 'Generator-only dependency: ' + name
    u.log('WATERFALL_ASSET_CHECKS_PASSED (visual motion and PIE require separate inspection)')


verify()
