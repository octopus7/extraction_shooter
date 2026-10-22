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
