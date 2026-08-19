# Spline World Builder

UE 5.7 plugin for non-destructive spline-authored modular environment geometry.

## Current vertical slice

- Non-branching `ASplineWorldBuilderActor` chains with rigid HISM modules.
- Automatic free-end and spline-corner pieces.
- `ASplineWorldJunctionActor` graph nodes for End, Straight, Corner, T, and Cross junctions.
- Junction trimming so chain modules stop before an authored junction mesh.
- Public `DA_SWB_TestStoneWall` profile and generated test meshes/material.
- `TunaSweeper > World Building > Spline World Builder: Add Junction Test Set` editor command.

## Generated test assets

The editor module imports `Resources/SourceArt/T_SplineWorldBuilder_StoneBlocks_ImageGen.png`, then creates versioned internal texture, material, and test meshes under `/SplineWorldBuilder/Generated/Internal`. The editable test profile is stored at `/SplineWorldBuilder/Profiles/DA_SWB_TestStoneWall`.

Use `-SplineWorldBuilderRebuildAssets` to force regeneration. Add `-SplineWorldBuilderAssetGenerationQuit` for a headless generate-and-exit run.

## Mesh contract

- Forward direction is local `+X`.
- Ground is local `Z = 0`.
- The test junction kit uses 100 cm arms and a 50 cm wall thickness.
- The test straight module is 200 cm long.
- T junction local arms are `+X`, `-X`, and `+Y`.
- Cross junction local arms are `+X`, `-X`, `+Y`, and `-Y`.

PCG is intentionally optional. Core module allocation, junction classification, and deterministic rebuilds are implemented in C++ so the plugin does not require PCG to cook or run.
