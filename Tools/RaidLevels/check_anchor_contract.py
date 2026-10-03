"""Run in a fresh UE Python commandlet to verify saved anchor/plugin contracts."""
import unreal
import os

anchor_class = unreal.load_class(None, '/Script/RaidLevelRuntime.TunaSweeperRaidPlacementAnchor')
assert anchor_class, 'Anchor must load from RaidLevelRuntime without game-owned type'
assert anchor_class.get_outer().get_name() == '/Script/RaidLevelRuntime'
bp = unreal.load_object(None, '/RaidLevelKit/Placement/BP_RaidPlacementAnchor.BP_RaidPlacementAnchor')
assert bp, 'Saved plugin anchor Blueprint must load'
cls = bp.generated_class()
assert cls, 'Saved Blueprint must have a generated class'
assert isinstance(unreal.get_default_object(cls), unreal.TunaSweeperRaidPlacementAnchor), 'Saved Blueprint retains native anchor parent'
cdo = unreal.get_default_object(cls)
assert cdo.get_editor_property('placement_id') == 1
assert int(cdo.get_editor_property('anchor_kind').value) == 0
assert not cdo.get_editor_property('allow_duplicate_placement_id')
assert int(unreal.TunaSweeperRaidPlacementAnchorKind.AUTHORED_ACTOR.value) == 3
catalog = unreal.load_object(None, '/RaidLevelKit/Placement/DA_LootAnchorPreviews.DA_LootAnchorPreviews')
assert catalog, 'Saved preview catalog must load after native class/struct redirect'
rows = catalog.get_editor_property('preview_definitions')
assert len(rows) == 3
for row in rows:
    assert row.get_editor_property('preview_mesh').get_path_name().startswith('/RaidLevelKit/'), row
legacy = os.environ.get('RAID_ANCHOR_LEGACY_BP')
if legacy:
    legacy_bp = unreal.load_object(None, legacy)
    assert legacy_bp and isinstance(unreal.get_default_object(legacy_bp.generated_class()), unreal.TunaSweeperRaidPlacementAnchor)
    unreal.log('RAID_LEGACY_ANCHOR_REDIRECT_OK')
unreal.log('RAID_ANCHOR_CONTRACT_OK')
