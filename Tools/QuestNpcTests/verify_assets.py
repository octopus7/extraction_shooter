"""Read-only validation of the authored NPC asset; does not create or save assets."""
import unreal as u

path = '/Game/Characters/BabyBoar/'
bp = u.load_asset(path + 'BP_BabyBoar')
mesh = u.load_asset(path + 'SM_BabyBoar')
material = u.load_asset(path + 'M_BabyBoar')
texture = u.load_asset(path + 'T_BabyBoarPalette')
assert all((bp, mesh, material, texture))
defaults = u.get_default_object(bp.generated_class())
assert defaults.get_editor_property('body_mesh').static_mesh == mesh
assert defaults.get_editor_property('head_mesh').static_mesh is None
assert defaults.get_editor_property('presence_quest_id') > 0
assert defaults.get_editor_property('presence_role') == u.TunaSweeperQuestNpcPresenceRole.FIELD_OFFER
assert str(defaults.get_editor_property('quest_provider_id')) == 'provider.baby_boar'
editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.get_default_object(u.StaticMeshEditorSubsystem)
assert len(mesh.static_materials) == 1
assert mesh.static_materials[0].material_interface == material
assert editor.get_num_uv_channels(mesh, 0) == 1
assert u.MaterialEditingLibrary.get_material_property_input_node(material, u.MaterialProperty.MP_BASE_COLOR).texture == texture
bounds = mesh.get_bounds()
dimensions = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
assert max(abs(a-b) for a,b in zip(dimensions, [64,24,46])) < .1
assert abs(bounds.origin.z - bounds.box_extent.z) < .1
u.log('QUEST_NPC_ASSETS_PASS: BP, palette UV, one material, bounds and defaults verified')
