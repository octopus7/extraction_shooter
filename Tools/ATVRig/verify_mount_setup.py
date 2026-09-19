"""Read-only validation of the ATV interaction defaults in a fresh editor."""
import unreal

atv_class = unreal.load_class(None, '/Script/TunaSweeper.TunaSweeperATVActor')
assert atv_class, 'ATV must have a placeable actor with the existing interaction component'
defaults = unreal.get_default_object(atv_class)
mount = defaults.get_editor_property('mount_component')
assert isinstance(mount, unreal.TunaSweeperInteractableComponent)
assert mount.get_interaction_type() == unreal.TunaSweeperInteractionType.VEHICLE_MOUNT
assert str(mount.get_editor_property('interaction_display_name_string_key')) == 'ui.vehicle.mount'
assert abs(mount.get_editor_property('dismount_hint_delay') - 1.5) < 0.001
assert defaults.get_editor_property('vehicle_mesh').get_skinned_asset() is not None
for name in ('engine_start_sound', 'engine_idle_sound', 'engine_stop_sound'):
    assert mount.get_editor_property(name), name
unreal.log('ATV_MOUNT_SETUP_PASS')

bp = unreal.load_asset('/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA')
assert isinstance(bp, unreal.Blueprint), 'BP_ATV_TypeA must be a saved Blueprint asset'
bp_defaults = unreal.get_default_object(bp.generated_class())
assert isinstance(bp_defaults, unreal.TunaSweeperATVActor), 'Blueprint must inherit the ATV actor'
bp_mount = bp_defaults.get_editor_property('mount_component')
assert isinstance(bp_mount, unreal.TunaSweeperVehicleMountComponent)
assert bp_mount.get_interaction_type() == unreal.TunaSweeperInteractionType.VEHICLE_MOUNT
assert str(bp_mount.get_attach_socket_name()) == 'seat'
assert bp_defaults.get_editor_property('vehicle_mesh').get_skinned_asset() == defaults.get_editor_property('vehicle_mesh').get_skinned_asset()
assert bp_defaults.get_editor_property('chassis_collision') is not None
for name in ('engine_start_sound', 'engine_idle_sound', 'engine_stop_sound'):
    assert bp_mount.get_editor_property(name) == mount.get_editor_property(name), name
unreal.log('ATV_BLUEPRINT_RELOAD_PASS')
