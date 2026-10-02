# LoopRail texture detail and passenger camera implementation plan

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans for scoped implementation. Complete validation, generator removal and task logging before one task commit.

**Goal:** Refine the train with bespoke UV textures, widen the ridden player's camera while moving, and make adjacent cars walkable through articulated connections.
**Architecture:** Preserve vehicle dimensions and engine character movement bases; add a replaceable connection mesh and moving collision deck between every adjacent pair. Detailed source art exports into /LoopRail. The game camera consumes LoopRail speed through a one-way game-to-plugin dependency and composes a multiplier with the active camera mode.
**Tech Stack:** UE 5.7 C++, Unreal Editor Python import, Blender headless, authored UV texture atlases.
**Spec:** User-approved in-chat modeling division and 2026-10-03 instruction to start, allow more faces, emphasize unique island-aligned texture contents and moving/stopped camera distance.

## Global constraints

- Runtime LoopRail dependencies remain Core, CoreUObject, Engine; no /Game or TunaSweeper references in plugin assets.
- Vehicles remain 900x220cm, origin at rail-centre surface, +X forward, +Z up; floor top60cm, aisle90cm, 8 rows x2 seats. Seat centres x=-297.5+85*row,y=+-70cm.
- One decorative locomotive chimney; roof remains separate/optional. Four vehicles by default, configurable existing count.
- Geometry supports silhouette. Individually allocated UV islands carry panel seams, fasteners, vents, wood grain and restrained wear; no uniformly coloured substitute for textured detail.
- Initial art ceilings: locomotive12,000 triangles; carriage including roof16,000. Validate actual counts and retain low complexity where hidden.
- Camera affects only local rider. Default maximum moving distance multiplier1.6; stopped/offboard restore current mode's original target smoothly. Existing FOV, aim offset and camera mode selection remain compatible.
- No gameplay UI or persisted state is added. No modifications to unrelated current-workspace changes.
- Temporary generators/import scripts live under ignored Saved; remove them after asset verification. Keep editable blend, exports, textures and reusable verification evidence.

## Review focus

- Station dwell, manual stop, departure and disembarkation restore or widen camera as expected.
- Camera mode changes while riding compose distance instead of restoring a stale cached value.
- Seat/wall collision does not seal aisle or end entrances, and mesh floor does not compete with moving floor.
- UV coordinates, material references and texture colour spaces survive UE import.
- Saved preview map and independent plugin host still load without game dependencies.

## Tasks

- [x] Camera: real train/character regression, initial failure, three camera modes, moving/stopped/offboard behavior and input lock. Connection floors preserve the wider view.
- [x] Art: locomotive, reusable carriage, detachable roof and connection; unique UV islands and surface-specific atlases; scale, UV, collision and triangle validation.
- [x] Connections (follow-up request): open carriage end rails; 100cm deck with 92cm clear guardrail width; endpoints follow car transforms with 20cm overlap. Native character traverses both ways without falling on straight stationary and curved moving routes at 600cm/s, including locomotive rear platform.
- [x] Integration: saved FBX/collision and texture materials at plugin paths; exact source triangle counts and texture/UV checks; preview map retained.
- [x] Validation: UE 5.7.4 Editor build; project automation 11/11; independent plugin host nine functional tests (visual skipped under NullRHI); six D3D12 captures inspected. Fresh-process asset reload verifies all four meshes.
- [x] Cleanup and review: removed all one-off generators/importers, updated READMEs, independent review found no outstanding issues. Final build/tests rerun after cleanup.
- [x] Delivery: completion log recorded; project editor opened on LoopRailPreview; all task changes prepared for the single task commit.

## Verification evidence

- `TunaSweeper/Saved/LoopRailDetail/Build-VerifiedFinal.log`: build succeeded.
- `Tests-Project-Final.log`: 11 successes, no failures; moving connection tests use the actual movement base and record zero falling frames.
- `Tests-Standalone-Final.log`: nine functional/asset checks pass without the game module; visual test explicitly skips NullRHI.
- `SourceArt/LoopRail/validation.json`, `fbx_validation.json`, `unreal_validation.json`: source/FBX/UE counts and dimensions match; 43,536 UV corners match; all 256 source parts closed with outward normals.
- `Saved/LoopRail/Preview`: Train, Locomotive, Carriage, Roof, Connection and ConnectionCurve captures. Captures now submit deferred material shader jobs before waiting, so the optional roof uses its surface material.
- Actual multi-client network riding remains outside this local validation.
