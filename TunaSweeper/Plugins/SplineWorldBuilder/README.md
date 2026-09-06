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

Test meshes, texture and material are committed under `/SplineWorldBuilder/Generated/Internal`. The editable profile is `/SplineWorldBuilder/Profiles/DA_SWB_TestStoneWall`. The placement menu loads that saved profile.

Startup generation, the rebuild menu and generation flags were retired on 2026-09-06. Edit the saved assets directly or restore missing files from Git/LFS. Source art remains available for future authoring.

## Mesh contract

- Forward direction is local `+X`.
- Ground is local `Z = 0`.
- The test junction kit uses 100 cm arms and a 50 cm wall thickness.
- The test straight module is 200 cm long.
- T junction local arms are `+X`, `-X`, and `+Y`.
- Cross junction local arms are `+X`, `-X`, `+Y`, and `-Y`.

PCG is intentionally optional. Core module allocation, junction classification, and deterministic rebuilds are implemented in C++ so the plugin does not require PCG to cook or run.

## TunaSweeper wall coping specialization

`AWallCopingSplineActor` is the rigid, no-X-scaling specialization used by
`/Game/Environment/Architecture/WallCoping/BP_WallCopingSpline`. It uses one HISM,
supports centered or start-aligned whole blocks, defaults to horizontal yaw-only
orientation, and exposes deterministic non-length variation. See
`Docs/wall_coping.md` for designer usage and corner limitations.
