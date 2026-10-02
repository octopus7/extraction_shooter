# Main Raid Region Levels Design

## Purpose

Create three independently loadable UE 5.7 main-game raid levels: `/Game/MainRaid/RaidForest`, `/Game/MainRaid/RaidVillage`, and `/Game/MainRaid/RaidPlains`. Each level must contain an actual painted Landscape, a playable primary route, a PlayerStart, lighting, collision, and a recognizable blockout made primarily from existing project assets.

## Package boundary

- Store every map and any map-specific generated asset under `/Game/MainRaid` so all Demo custom configurations continue to exclude them through `DirectoriesToNeverCook=(Path="/Game/MainRaid")`.
- Add all three maps to the Full build's `MapsToCook` list in `Config/DefaultGame.ini`.
- Do not change Demo map lists, Demo content, `BunkerMap`, placement anchors, placement IDs, or spawn JSON.
- Do not add story text or unreleased narrative data to public documents or assets.

## Shared technical layout

- Use a deterministic 8 x 8 component Landscape with one 63-quad subsection per component and 100 cm XY scale, yielding a 504 m square terrain.
- Reuse `/Game/Materials/Landscape/M_LandScape` and the four existing layer-info assets in `/Game/RaidMap_sharedassets`: `Grass`, `GrassDark`, `Dirt`, and `Rock`.
- Use deterministic height and weight functions so props can be placed at the same evaluated ground height without a runtime generator.
- Keep clear route corridors below the project's practical walkable slope and keep PlayerStart clear of blockers.
- Reuse existing environment meshes through static-mesh or hierarchical-instanced components. A temporary editor generator may create the maps, but no completed generator, startup hook, command-line entry point, or generator-only module dependency may remain.
- Each map has four saved `BlockingVolume` actors in the `MapBoundary` folder. North/south follow +/-X and east/west follow +/-Y. Their inner faces are at +/-250 m, 2 m inside the Landscape edge, with 10 m thickness and overlapping corners. The walls extend at least 50 m below the lowest terrain and 100 m above the highest terrain. Use the standard `InvisibleWall` collision profile and hide the brushes in game.

## RaidForest

- Primary route runs generally south to north, following the project convention that north is +X.
- A distant northern destination silhouette must remain visible from the southern start clearing.
- Terrain uses wooded ridges, a lower main trail, one secondary loop, and reserved cliff-facility space without selecting a lift or zipline solution.
- Reuse simple trees, pine trees, forest ground props, logs/stumps, and rock meshes. Clear a wide movement corridor and sightline around the main route.

## RaidVillage

- Build three visually separate rural districts around a broad central basin.
- Put the starting/extraction overlook on higher ground and connect it to all districts with walkable terrain corridors.
- Reuse the LoopRail runtime actors for a closed circular route with three stations and one train; keep the railway on a flattened ring and leave safe clearance around the track.
- Reuse the open wooden shed, modular wood wall/roof pieces, crates, pallets, facility props, gate/barrier props, trees, and rocks. Final road and building art are outside this blockout scope.

## RaidPlains

- Use broad, dry, low-density terrain with long sightlines, sparse rock/vegetation clusters, and fewer points of interest than the other maps.
- Reserve and mark an SSTO landing site. Because no dedicated SSTO asset exists, assemble one clearly labeled in-map blockout from existing engine/project meshes; do not create a new reusable art asset.
- Keep the main route from the western start to the landing site walkable and visually legible.

## Verification

- A fresh editor process must load each map and confirm: one real Landscape with components, the expected material and layer infos, PlayerStart, lighting, map-specific landmark actors, reusable-prop references, and no missing referenced assets.
- Run Map Check and structural checks for every map.
- Sample the primary routes against the deterministic terrain, confirm height continuity and slope limits, and use collision traces to ensure ground support and route clearance.
- Verify saved boundary brush geometry and Pawn collision with outward capsule sweeps along all four sides and through the corners, at both terrain height and an elevated height. Include an interior sweep that must remain clear.
- Capture at least one review image per map after reloading the saved maps and inspect it for gross placement, clipping, lighting, and visibility issues.
- Rebuild after removing the temporary generator, then rerun the verifier to prove the saved assets do not depend on it.

## Out of scope

- Final environment art, combat encounters, loot/enemy/memo spawn data, quest progression, final rail art, finalized cliff transport, boss identity, and final balance.
- Making all three maps selectable through the current single-active-raid runtime flow.
