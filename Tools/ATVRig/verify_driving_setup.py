"""Read-only validation of the saved ATV driving configuration."""
import unreal

bp = unreal.load_asset('/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA')
defaults = unreal.get_default_object(bp.generated_class())
assert isinstance(defaults, unreal.Pawn), 'ATV must be a Pawn for Chaos vehicle movement'
movement = defaults.get_editor_property('vehicle_movement')
assert isinstance(movement, unreal.ChaosWheeledVehicleMovementComponent)
assert len(movement.get_editor_property('wheel_setups')) == 4
mesh = defaults.get_editor_property('vehicle_mesh')
assert mesh.get_editor_property('physics_asset_override') is not None
assert mesh.get_editor_property('anim_class') is not None
mount = defaults.get_editor_property('mount_component')
assert mount.get_editor_property('engine_drive_sound')
assert mount.get_editor_property('engine_boost_sound')
unreal.log('ATV_DRIVING_SETUP_PASS')
