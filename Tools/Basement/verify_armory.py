"""Read-only verification of the saved armory Blueprint and range placement."""
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[2]
BP='/Game/Blueprints/Debug/BP_DebugArmory'
bp=u.load_asset(BP)
assert isinstance(bp,u.Blueprint),'Missing BP_DebugArmory'
cdo=u.get_default_object(bp.generated_class())
assert isinstance(cdo,u.TunaSweeperDebugArmoryActor)
visual=cdo.get_editor_property('visual_mesh')
assert visual.static_mesh.get_path_name()=='/Game/Environment/Basement/Range/Meshes/SM_WeaponRack.SM_WeaponRack'
assert (visual.get_editor_property('relative_scale3d')-u.Vector(1,1,1)).length()<.001
assert str(visual.get_collision_profile_name())=='BlockAll'
component=cdo.get_interactable_component()
assert component.get_interaction_type()==u.TunaSweeperInteractionType.DEBUG_ARMORY_OPEN
assert str(component.get_editor_property('interaction_display_name_string_key'))=='ui.interaction.debug_armory_open'
assert component.get_editor_property('interaction_distance')==200
assert u.EditorLoadingAndSavingUtils.load_map('/Game/Environment/Basement/Maps/L_Basement')
found=[a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors() if a.get_actor_label()=='WeaponRack_001']
assert len(found)==1 and found[0].get_class()==bp.generated_class()
a=found[0]
assert a.get_attach_parent_actor().get_actor_label()=='ROOT_Range'
assert a.get_editor_property('visual_mesh').static_mesh==visual.static_mesh
assert a.get_interaction_type()==u.TunaSweeperInteractionType.DEBUG_ARMORY_OPEN
placement=json.loads((ROOT/'TunaSweeper/SourceArt/Environment/Basement/side_furniture_placement.json').read_text())
expected=next(p for p in placement['placements'] if p['label']=='WeaponRack_001')
assert (a.get_actor_location()-u.Vector(*expected['location_cm'])).length()<.1
assert abs((a.get_actor_rotation().yaw-expected['yaw_deg']+180)%360-180)<.1
report={'passed':True,'blueprint':BP,'parent':cdo.get_class().get_super_class().get_path_name() if hasattr(cdo.get_class(),'get_super_class') else '/Script/TunaSweeper.TunaSweeperDebugArmoryActor','mesh':visual.static_mesh.get_path_name(),'actor':'WeaponRack_001','parent_actor':'ROOT_Range','interaction_key':'ui.interaction.debug_armory_open','interaction_distance_cm':200,'location_cm':[a.get_actor_location().x,a.get_actor_location().y,a.get_actor_location().z]}
(ROOT/'TunaSweeper/SourceArt/Environment/Basement/armory_asset_validation.json').write_text(json.dumps(report,indent=2))
u.log('DEBUG_ARMORY_ASSET_VERIFIED')
