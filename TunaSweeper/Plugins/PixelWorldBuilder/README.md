# Pixel World Builder (UE 5.7)

Editor-only PNG terrain and vegetation tool. Source RGB bytes are classification IDs: no texture import, sRGB conversion, color-distance matching or filtered color sampling occurs.

## Workflow

1. Open **Tools > PNG 월드 빌더 열기**.
2. Edit the temporary profile or select a saved PixelWorldProfile. **프로필 저장** creates a persistent DataAsset copy in the Content Browser; changes to a saved profile are saved with the normal asset save workflow.
3. Set **RGB 배치도 PNG** to an opaque, 8-bit RGB/RGBA PNG. Enter one rule for every RGB triplet used in the image, with R/G/B integers from 0 to 255. Disable antialiasing and color-profile conversion in the authoring application's export settings.
4. Set meters per pixel, map center, seed and rule heights. Optionally provide a same-sized, 16-bit grayscale PNG and its elevation range. The optional heightmap replaces color-rule elevations. Height smoothing never changes category RGBs.
5. Optionally set a Landscape material and one distinct Layer Info per color. All referenced layer names must exist in the material. Without layers, the material is simply assigned to the generated Landscape.
6. Add vegetation entries to each rule. Select either a static mesh (HISM instances) or an actor/BP class. Density is a target per 100 square meters; slope and spacing can reduce the realized count. Minimum spacing applies within each rule/entry, not between different species.
   For mesh instances, the material must have **Used with Instanced Static Meshes** enabled and saved. UE may enable it in memory while editing; save the material too when UE reports the missing usage flag.
7. **분석 / 미리보기** validates the input and shows pixel counts, terrain dimensions and planned vegetation count without modifying the level.
8. **새로 생성** creates a new batch in the current level. Select its root actor and use **선택한 생성 그룹 재생성** to replace only that batch. Ordinary Undo/Redo is supported. Save the level to preserve the result.

The image top maps to north (+X), image right maps to +Y, and one meter is 100 UE units. A pixel represents an area. Landscape component grids are rounded up to multiples of 63 quads while preserving the exact world rectangle; nearest-cell classification avoids inventing colors. Generated outputs use standard Engine/Landscape classes and do not need PCG.

## Scope and limits

- Generates a new Landscape; does not overwrite existing hand-authored terrain.
- Current editable level only. PIE, locked levels and World Partition are rejected before mutation.
- Maximum image axis: 2048 pixels; maximum Landscape components: 256; maximum palette: 64 rules.
- Maximum planned mesh instances: 50,000; actor plants: 2,000. Actor classes must be concrete, non-editor-only, non-Landscape actors.
- Any nonopaque pixel, unknown RGB, duplicate rule, invalid numeric setting, unavailable asset or incompatible layer causes validation failure. Failed validation leaves the old batch untouched.
- A root and its outputs share a unique generation tag. Rebuild removes only tagged outputs from that batch. Manual untagged actors remain untouched; manual edits to tagged generated outputs are replaced.
- UE actor duplication also copies these tags. If two roots in the level share a generation tag, regeneration is rejected without deleting either group. Use **새로 생성** to create independent batches.
- PNG paths are authoring inputs; keep source images in a durable project location. Generation is explicit, never on editor startup or during cooking.

## Verification

Run `Tools/PixelWorldBuilder/run_tests.ps1` from the repository, following `Docs/unreal_editor_coordination.md` first. The script refuses to run while this project's UE 5.7 editor is open. Reports are written under `TunaSweeper/Saved/Automation`.

The six tests use hand-defined PNG fixtures with non-primary RGB samples and sRGB/gAMA metadata, validate mapping and limits, and create temporary maps for generation, replacement, duplicate-group protection, Undo/Redo and save/reload checks. The round-trip map is removed afterward. Layer weights are read from the imported edit layer; the NullRHI suite does not validate GPU material rendering. Do not run the map-changing integration tests inside an editor with unsaved user work.

UE 5.7 Development Editor build and all six tests passed on 2026-10-23, with no test warnings or errors. Final report: `TunaSweeper/Saved/Automation/PixelWorldBuilder-final/index.json` (local, not tracked).

An editable 128×128 m sample map and profile are available under `/Game/Prototype/PixelWorldDemo`. Original PNG inputs, palette, verification report and rendered captures are documented in `TunaSweeper/SourceArt/PixelWorldDemo/README.md`.
