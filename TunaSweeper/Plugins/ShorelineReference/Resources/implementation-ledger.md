# Shoreline Reference island implementation ledger

Request start: 2026-10-22 23:31:44.
Scope: first isolated deserted-island test scene and calm white shoreline band.

- Ruling: use a new content-only ShorelineReference plugin, with embedded Custom HLSL and native static-mesh actors. The user's new isolation requirement supersedes the earlier plan's shared StylizedWater actor/material modifications. Existing water is untouched; cost is an intentionally independent study rather than immediate integration.
- Ruling: create a new deserted-island test map because no existing island level was found in the project. This matches the requested test scene and keeps existing gameplay maps separate.
- Ruling: work in the current project in a completely new plugin directory; no shared C++/build output or existing asset is edited. A second checkout would duplicate this large UE project's content and invalidate the required shared editor/project path without improving asset isolation.
- Preflight: shoreline consumes world centimeters and the island's independent R distance/G depth map. EXR V points toward negative source Y. UE FBX import was measured to mirror source Y, so final shader mask V points toward positive UE world Y. Noise UV uses world X/Y separately. No runtime generated assets or startup generators.
- Verification contract written before material/map creation: required isolated assets, no /Game or /StylizedWater dependencies, linear 1024-square mask, flat noncolliding water, save/reload preservation.
- Red check failed for the missing material before creation. GPU pixels then exposed disconnected unnamed output pins; graph assertions now cover the Custom RGB/alpha path and terrain vertex color. UE vertex-vs-height comparison caught the FBX Y reflection and the corrected maximum error was 0.0000474cm.
- Review corrections: map-local Engine.GameModeBase; dedicated-process guards on validators; capture completion is separate from pixel-validation success; no prior or stale capture is accepted as a fresh result.
- Visual setting: manual exposure compensation +2.5, no physical camera exposure, bloom or motion blur; close review camera sees a local shoreline segment, wide view sees the whole island.
- Environment: another conversation introduced an unbuilt PixelWorldBuilder module during testing. The affected validation process used the engine's temporary DisablePlugins command-line option; no other plugin file or project configuration was changed by this task.
- Final visual correction: deep-water alpha and color finish transitioning at 200cm so the finite seabed and its clamped data do not expose a rectangular border. The shoreline formula remains unchanged.
- Final validation: actual D3D12 SM6 masks have 0 pixels outside the shallow shoreline, exact repeat at frozen time, 18,593 changed pixels between t=0 and t=1, and no shader compilation errors. All 240 frames exist, with all 239 consecutive pairs changing. Video probe confirms 1280x800, 12fps, 20s, 240 frames; full video decode passed.
- Cleanup: the temporary asset generator was removed after GPU validation. A fresh UE5.7 process then passed asset/dependency/graph/hash/axis/NoCollision checks and exited normally at 2026-10-23 00:11:36. The source-model agent also removed its temporary creation/validation scripts and confirmed no embedded Blender script remains.
- Handoff: no GUI editor was opened; shared editor remains closed for another conversation's pending UE5.7 plugin build. No C++ source or existing project asset was modified by this task.

## Irregular fading tail — 2026-10-23 00:35:30

- Request: soften the artificial, uniform gradient with additional pattern while reusing the existing texture.
- Implementation: sample T_ShoreDissolve once more with rotated, offset, scaled UVs, then subtract soft holes only from the fading interior. Preserve the bright front and scalloped outer contour. Defaults: TailBreakupStrength=0.85, TailBreakupScale=2.6; strength 0 restores the previous shader mask exactly.
- Scope: only M_ShorelineReference changed among saved Content assets. Existing textures, island, map, water plugins, C++ and project settings are unchanged.
- Validation: fresh D3D12/SM6 images passed pixel analysis. 4,778 mask pixels changed, 1,192 tail pixels were removed, bright-front mean error is 0.333/255, and breakup-off vs original mask error is 0. All 240 frames / 239 adjacent pairs move; no foam appears outside shallow water and shader errors are 0. Independent visual/code review found no blocking issue. Full encoded video decode and duration/frame-count probe passed.
- Capture process returned exit 1 despite complete capture/report files and a normal-shutdown log without Python/shader errors; capture success is established by fresh-file checks and separate pixel/video validation, not that process status. Final reload verification exited 0.
- Validation-tool correction: D3D12 texture startup can temporarily expose default-texture dimensions. verify_assets.py now executes UE 5.7's Editor.AsyncAssetCompilationFinishAll before checking actual texture dimensions; the 1024-square assertion remains intact.
- Cleanup: deleted the one-off material updater, then passed a fresh D3D12 asset reload, embedded-source hash, parameter, texture, dependency, axis, NoCollision and saved-file-invariance verification at 2026-10-23 00:47:51. Final process exit and project-editor absence were confirmed before releasing UE to the PixelWorldBuilder conversation. No startup regeneration remains.
