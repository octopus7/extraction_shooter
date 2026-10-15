# Quest facility validation

`verify_assets.py` is a read-only UE 5.7 Python verifier. Use a dedicated editor after coordinating closure of the shared project editor. It loads RaidVillage and BunkerMap, refuses PIE/dirty maps, and does not save packages.

Run it using the project editor's `-ExecutePythonScript` option with an absolute path and `/Engine/Maps/Entry` as the startup map. The script waits for map collision registration between loads and exits an unattended editor after writing the result. Do not run it in a user's unsaved session.

Checks: mesh dimensions/UV/material count, palette filtering, Blueprint mesh transforms/roles, token UI texture compression, unique placements, upright orientation, ground contact, clear front approach, repair item and matching vehicle world-progress gate. Output: `TunaSweeper/Saved/QuestFacilitiesWork/asset_validation.json` and `QUEST_FACILITIES_VERIFY_PASS`.

The source models are under `TunaSweeper/SourceArt/QuestFacilities`. Both Blueprint visuals are replaceable components. One-off creation/import/placement scripts are removed after validation; reopening the project never regenerates assets.
