# Stylized Water — mask rebuild

This experimental UE 5.7 plugin replaces the failed masked Single Layer Water surface and culled shore overlay. It uses a complete translucent surface and a pixel-sampled distance/depth texture. The runtime actor keeps its native class name only to support level migration; the old BP, MI, materials and palette are retired.

## Place and edit

1. Choose **TunaSweeper > Rendering > Stylized Water: Add Calm Lake / Gentle Beach / Flowing River**.
2. Move the native actor to the water height and set **Surface Size**. Keep pitch/roll at zero and use positive actor scale; yaw is supported.
3. Choose **Boundary Mask**. The supplied 1024² masks are starting shapes, not a terrain shoreline detector.
4. Adjust **Mask World Size**, **Mask UV Scale / Offset**, **Edge Feather**, **Shore Offset** and the color/flow controls. **Mask Resolution** reports dimensions and world cm per texel. Resolution itself is changed by importing a higher/lower resolution texture.
5. For a shore film that extends onto raised ground, click **Fit Surface To Terrain**. This stores a complete fitted mesh, with no cell removal. Refit after terrain changes, actor movement, size or height edits. Increasing **Grid Resolution** improves terrain conformity, not the mask silhouette.
6. **Use Painted Sky Reflection** enables the optional sky experiment. It is off by default. Adjust **Sky Height**, **Sky World Size**, **Sky World Anchor**, strength or the sky texture. No BP or MI assignment is needed.

The plugin reads saved materials; startup, placement and reconstruction never create or overwrite content packages. Missing base assets hide the water rather than generating replacement content.

## Mask contract

Use a linear texture: **sRGB off**, HDR/uncompressed data, trilinear filtering, mipmaps, clamp addressing. The supplied masks use RGBA16F source and HDR storage.

- R is signed shore distance: 0.5 is the static waterline, higher values are inside the water. Decode as `(R - 0.5) * 2 * MaskDistanceRange` cm.
- G is depth normalized over **Mask Depth Range** (default 700 cm). **Depth Color Range** controls the shallow-to-deep palette distance. **Terrain Depth Influence** blends G with explicitly fitted terrain depth.
- B is reserved; A is unused.
- Import the desired texture resolution. Match **Mask Distance Range** to the authored encoding (supplied masks: ±1000 cm).
- Mapping is XY in world centimeters, rotated by actor yaw or **World Mask Yaw**. World lock uses **World Mask Center**; otherwise the mask follows the actor. Custom **Mask World Size** decouples mask coverage from mesh size.
- Keep shoreline features inside the texture domain. A soft safety fade near the UV/mesh border prevents a hard rectangular edge. Beach and river endpoints taper at this domain; overlap/extend bodies deliberately.
- Mip filtering plus derivative-aware feathering protects distance views. A very low resolution image cannot recover detail it does not contain.

The outer film remains translucent; foam opacity is bounded. The mask defines coverage and runup while depth fade softens intersections with opaque geometry. Terrain fitting does not automatically author a mask. Large rocks/overhangs on the trace channel can raise the fitted grid; use a suitable terrain trace channel and inspect the result.

## Sky experiment and removal

Search **WATER_SKY_PARALLAX_EXPERIMENT**. The sky uses a reflected view ray intersecting a virtual horizontal sky plane, with a smooth finite-horizon limit. It has no Time input. Water ripples/shore animation are separate; set **Animation Speed = 0** for whole-water still comparisons.

The source image is hand-painted sky generated with built-in imagegen; its prompt is preserved in `Resources/SkyParallax/AnimeSky.prompt.txt`. Mirrored addressing is intentional: its generated edges are not a seamless tile.

- Base assets/code: `Content/MaskWater`, `Shaders/Private/MaskWater.ush`, the runtime actor, editor placement module.
- Optional assets/code: `Content/SkyParallax`, `Shaders/SkyParallax`, `Resources/SkyParallax`, marked actor defaults/properties/material-selection blocks.
- To disable: uncheck **Use Painted Sky Reflection** on actors. The selected base material has no sky asset or sky shader dependency.
- To remove: disable on all actors and save maps; remove the marked actor blocks and optional directories. Base material loading remains intact. Remove sky-specific checks from the render/asset tests.
- Review-only content/tests: `Content/Review`, `Source/StylizedWaterEditor/Private/Tests`. No gameplay systems or save data depend on these.

For the full replacement inventory, commit sequence, screenshots and integration notes, see `Docs/water_mask_rebuild_2026-09-06.md` in the repository root.
