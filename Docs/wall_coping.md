# Wall Coping spline modules

## Assets and source

- Designer actor: `/Game/Environment/Architecture/WallCoping/BP_WallCopingSpline`
- Static mesh: `/Game/Environment/Architecture/WallCoping/SM_WallCoping_Straight`
- Material: `/Game/Environment/Architecture/WallCoping/M_WallCoping_WarmSandstone`
- Editable generator parameters: `TunaSweeper/SourceArt/Environment/WallCoping/wall_coping_parameters.json`
- Deterministic generator: `TunaSweeperWallCopingSetup.cpp`

Regenerate the assets with UE 5.7 using
`-TunaSweeperWallCopingSetup -TunaSweeperWallCopingSetupQuit`.

## Mesh contract

The nominal block is `50 x 38 x 14 cm`. Local `+X` is repeat length, local `+Y`
is wall width, and local `+Z` is up, matching the project direction and centimeter
conventions. The pivot is at the bottom center. This lets the spline sit directly
on the wall top and keeps width/height scaling anchored to that support surface.

The 96-triangle block has broad planar bevel bands, a restrained asymmetric top
and side silhouette, one UV channel, authored face normals/tangents, one warm
sandstone material slot, and one box collision. Nanite is disabled. Both end caps
use the same closed cross-section; zero-gap repetition therefore remains sealed.
The end bevel deliberately forms a visible seam between blocks.

The material blends two warm sandstone colors with `PerInstanceRandom` and uses
high roughness. HISM instances therefore vary without extra mesh assets or actors.

## Placement

Place `BP_WallCopingSpline`, move its spline points to the top centerline of a
wall, and edit the following groups in Details:

- **Module:** mesh/material, mesh-bounds or explicit module length, width and height scale.
- **Placement:** gap, `Centered` or `StartAligned`, and short-spline behavior.
- **Orientation:** horizontal Yaw-only placement by default; optional spline pitch/roll.
- **Variation:** deterministic seed and small Y/Z scale or Yaw ranges. X scale is always 1.

`Centered` fits as many whole blocks as possible and divides the remainder between
the ends. `StartAligned` puts the first end plane at spline distance zero. A spline
shorter than one module receives one centered block by default. Rebuild occurs in
`OnConstruction`; the actor does not Tick and all blocks live in one HISM component.

## Corners and future modules

Smooth exterior curves are approximated by rigid tangent-aligned blocks. Curvature
tighter than the module length can show wedge gaps on the outside or overlap on the
inside. A sharp 90-degree spline point has the same limitation because the straight
blocks are intentionally not bent or X-scaled.

The intended extension is optional corner and end-cap meshes plus trim distances,
similar to the generic `SplineWorldBuilder` profile/junction system. Until those
meshes are authored, designers should use a small corner radius, terminate two
coping actors at the corner, or accept/cover the local overlap. Product Raid and
Bunker levels are not modified by asset generation or tests.
