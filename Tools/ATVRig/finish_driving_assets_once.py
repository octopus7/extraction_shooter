"""One-off Physics Asset authoring and Blueprint refresh; remove after the asset commit."""
import unreal

unreal.load_module('TunaSweeperEditor')
generator_class = unreal.load_class(None, '/Script/TunaSweeperEditor.TunaSweeperATVPhysicsCommandlet')
assert generator_class
generator = unreal.new_object(generator_class)
assert generator.generate_asset() == 0
physics = unreal.load_asset('/Game/Meshes/Props/ATV/PA_ATV')
assert physics
bp = unreal.load_asset('/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA')
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
defaults = unreal.get_default_object(bp.generated_class())
mesh = defaults.get_editor_property('vehicle_mesh')
mesh.set_physics_asset(physics)
mesh.set_editor_property('relative_location', unreal.Vector(0,0,0))
mesh.set_anim_instance_class(unreal.TunaSweeperATVAnimInstance)
defaults.get_editor_property('chassis_collision').set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
unreal.log('ATV_DRIVING_ASSETS_CREATED')
