"""Read-only validation of saved ATV damage assets and Blueprint references."""
import unreal

bp = unreal.load_asset('/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA')
defaults = unreal.get_default_object(bp.generated_class())
assert defaults.get_editor_property('max_durability') > 0
parts = defaults.get_editor_property('detached_part_meshes')
assert len(parts) == 3
for mesh, bone in zip(parts, ('wheel_FL', 'wheel_RR', 'handlebar')):
    assert isinstance(mesh, unreal.StaticMesh)
    assert mesh.get_name() == 'SM_ATV_Debris_' + bone
    bounds = mesh.get_bounds()
    assert 10 < bounds.sphere_radius < 100, (bone, bounds)
    for slot in mesh.get_editor_property('static_materials'):
        assert slot.material_interface is not None
for name in ('light_damage_smoke', 'heavy_damage_smoke'):
    component = defaults.get_editor_property(name)
    system = component.get_editor_property('asset')
    assert isinstance(system, unreal.NiagaraSystem), name
    assert system.get_path_name().startswith('/Game/Effects/ATV/')
unreal.log('ATV_DAMAGE_ASSETS_PASS max_durability=' + str(defaults.get_editor_property('max_durability')))
