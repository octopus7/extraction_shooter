# PNG World Builder implementation plan

**Goal:** Build a UE 5.7 editor tool that reads stored PNG RGB integers and generates Landscape terrain and vegetation from editable color rules.

**Architecture:** A self-contained PixelWorldBuilder editor plugin owns a DataAsset profile, CPU image/terrain planning, transactional world generation, and a localized Slate panel. Generated worlds use ordinary Landscape, actor and HISM objects; no plugin runtime module or PCG dependency is required.

**Spec:** The user's accepted architecture and raw RGB clarification in this conversation. Implementation was explicitly requested on 2026-10-22 at 23:53:00.

## Constraints and decisions

- UE 5.7; original 8-bit RGB/RGBA PNG samples are integers, without sRGB-to-linear conversion or texture sampling.
- Image top is north (+X), image right is +Y, one meter is 100 cm. Profile origin is the map center at base elevation.
- Color rules provide elevation, optional Landscape layer info, and independent vegetation entries (mesh instances or actor classes). An optional 16-bit grayscale PNG supplies elevation instead of color heights.
- Generate a new Landscape in the current non-partitioned editor level. Never modify an unrelated Landscape. World Partition is rejected explicitly in this first version.
- Generation is bounded by image/grid/instance limits and validates assets, unknown colors, alpha, numeric values and layer/material compatibility before changing the world.
- Regeneration replaces only a selected tool-owned batch, after the replacement has been prepared. Undo/Redo and save/reopen are covered by integration tests.
- All tool text and property labels resolve string-table keys. Existing maps, unrelated staged changes and other conversations' work remain untouched.
- Work in the existing shared checkout: plugin source is isolated by directory; shared editor closure is coordinated. One commit contains implementation, tests, documentation and the request log, as required by AGENTS.md.

## Tasks

- [x] Write automation tests and minimal interfaces; observe failures for raw RGB, invalid input, orientation/extent and deterministic vegetation.
- [x] Implement profile validation and CPU generation plan in PixelWorldBuilderCore. Use native 8-bit image access, nearest-cell category sampling, valid Landscape component dimensions and deterministic area-based scatter.
- [x] Implement transactional generation, selected-batch replacement and cleanup in PixelWorldBuilderService. Validate real Landscape dimensions, instances, ownership, failed-regeneration preservation and Undo/Redo.
- [x] Implement keyed editor UI, editable/savable profiles and analysis output. Register a Tools menu entry and provide a usable default profile without startup asset generation.
- [x] Build TunaSweeperEditor with UE 5.7, run plugin automation tests, review the completed feature and document usage. Include the request log in the single task commit.

## Review focus

- PNG sRGB/gAMA metadata and non-primary channel values must preserve stored integers.
- Non-square images and edge samples must preserve map orientation and physical extent.
- Unknown colors, duplicate colors, transparent pixels, invalid assets and huge inputs must fail before world mutation.
- Seeded scatter must use world area, honor slope/spacing and remain stable across rebuilds.
- A failed rebuild must preserve the old batch; unrelated actors and manual content must survive replacement and Undo.

## Progress

- Source inspection confirmed UE 5.7 PNG decoding preserves native 8-bit values and FLinearColor conversion must be avoided.
- Editor closure was coordinated with the ShorelineReference, aquatic-plant and quest conversations. The final active editor was released at 00:11:36 on 2026-10-23.
- Initial stub implementation produced four expected automation failures; the implementation passed those tests, including actual map save/reload.
- Independent review identified copied batch tags after UE actor duplication. A regression test reproduced deletion of both roots; regeneration now rejects duplicate roots before any mutation and the regression passes.
- Final UE 5.7 build succeeded. All six automation tests passed with zero warnings/errors in `TunaSweeper/Saved/Automation/PixelWorldBuilder-final/index.json`.
- Layer verification targets the imported edit layer. The final GPU merge is asynchronous and unavailable with NullRHI; the suite does not claim visual material validation.
