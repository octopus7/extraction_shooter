# Ethanol facility validation

Run `verify_assets.py` with UE 5.7 `-ExecutePythonScript=<absolute script path>` and the TunaSweeper project, using `/Engine/Maps/Entry` as the startup map. Coordinate editor closure first. The script refuses PIE or dirty maps, loads RaidVillage, and never saves packages. An unattended invocation exits after validation.

Checks include silo dimensions (280 × 270 × 350 cm), one material, UVs and collision, replaceable BP mesh, repair requirements, production recipe and localized keys, unique placement, upright orientation, ground contact and front approach clearance. Output: `TunaSweeper/Saved/EthanolWork/asset_validation.json` and `ETHANOL_VERIFY_PASS`.

Runtime automation: `TunaSweeper.Interaction.PowerFacility`, `TunaSweeper.Interaction.EthanolFacility`, `TunaSweeper.Quest.WorldProgress`, `TunaSweeper.Quest.ItemSubmission`, `TunaSweeper.Shop.QuestUnlock`.

Model sources are under `TunaSweeper/SourceArt/QuestFacilities/Ethanol`. The BP uses a regular low-poly, texture-driven silo with one opaque material. Creation/import/placement generators are temporary and are removed after validation; startup never regenerates these assets.
