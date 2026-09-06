# Concrete bunker automatic door

Blender 4.5 replacement kit for the existing four-panel FoldingCanopyGarageDoor. Textures were generated with ImageGen; the exact prompt is in `texture.prompt.txt`.

## Existing structure and dimensions

Read the existing BP_RaidBunkerGarageDoor class defaults and the native actor equations. Its actual opening is 200 cm wide, with four 45 cm upper panels, a 25 cm bottom panel embedded 10 cm into the floor, and 16 cm panel thickness. The 600 cm legacy OBJ files were prototypes scaled by the actor and are not the current opening dimensions. The fixed concrete frame is 270 × 45 × 230 cm. X is width, -Y is outdoors, Z is up.

The moving upper slabs remain gray metal with inset plates, while the jambs, lintel and optional shell are gray concrete. Separate rail recesses follow the real inward rail offset. The original four hinge pivots, moving lower slab and canopy equations are preserved. A white emissive bar follows the first hinge. Two 12 cm circular lamps sit on the solid outer strip of the right jamb: red above green.

## Connected Blueprint and manual setup reference

After the user's follow-up authorization, `/Game/Environment/Bunker/GarageDoor/BP_RaidBunkerGarageDoor` now uses this kit, including both motion indicators and authored mesh materials. The door's dimensions and motion settings are preserved. Level assets are unchanged. Imported assets live under `/Game/Environment/Bunker/GarageDoorConcrete`.

1. In the existing door's mesh fields, assign the corresponding `SM_GarageConcrete_FrameTop`, `FrameLeft`, `FrameRight`, `CanopyRailLeft`, `CanopyRailRight`, `LEDBar` and `LowerEmbeddedPanel` meshes.
2. Assign `SM_GarageConcrete_UpperPanel` to all four Upper Panel Meshes entries. Optional TemporaryWallLeft/Right and TemporaryRoof meshes are also supplied.
3. Enable **Use Authored Mesh Materials** to preserve the new concrete, metal, rubber, LED and lens material slots instead of applying the legacy single metal override.
4. Set **Motion Indicator Mesh** to `SM_GarageConcrete_MotionIndicator`. The native actor supplies both lamp components, placement and signals; no Blueprint event wiring is required.

Default lamp settings: diameter 12 cm, depth 4 cm, vertical spacing 18 cm, height ratio 0.65, emissive strength 8. Opening lights only green; closing lights only red. Fully open, fully closed and disabled/paused states have zero lens emission. Re-enabling a paused moving door resumes its previous direction. Reversing swaps the active lamp immediately. The white bar is continuously emissive. Emission is a material effect, not a separate dynamic point light.

The shared lens material uses Custom Primitive Data scalar index 0 for emission and vector indices 1–3 for linear RGB. Colors remain dimly visible when emission is zero. No new persisted save state is introduced.

## Reproduction and validation

Run Blender in background with `--python-exit-code 1 --python Tools/GarageDoorConcrete/build_garage.py`. FBX meshes are centered, measured in cm after import, with authored UVs, beveled edges and material slots. Export copies compensate for UE's Y-axis conversion; the review blend uses project axes directly. Four renders show closed, opening, open and closing poses. Optional shell meshes and studio props are hidden from the review/export as appropriate.

Build `TunaSweeperEditor Win64 Development`. Run automation test `TunaSweeper.GarageDoor.MotionIndicators` to check start, reversal, pause/resume, endpoints and manual endpoint changes in a real transient UE world. The test does not modify project assets.

Import with `run_unreal_import.ps1`; reload/audit with `run_unreal_import.ps1 -VerifyOnly`. These scripts save only the new kit folder and compare hashes of the existing garage assets. Existing editor/BP instances need an editor restart to load the rebuilt native class.

Verified import results: 12 meshes, 6 materials and 1 ImageGen atlas imported and reloaded in UE 5.7.4; centered bounds match the native contract within 0.1 cm. Material graph checks confirm LED emission and lens CPD indices with zero default lens emission. The initial import preserved all existing garage assets; the subsequent authorized connection changes only the door BP. Static meshes include simple collision; the actor disables collision on the moving/visual components as before, and always disables it on the indicators. Frame recesses are visual details within each jamb's simple collision hull.

The project currently raises a pre-existing Niagara `NE_PostProcess` typed-element registry ensure at engine startup. Asset validation passes, but import/reload commandlets exit with code 1 because of that engine error. The runner deliberately reports this separately instead of hiding the nonzero exit. Native motion automation passes.
