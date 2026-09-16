# Blank keycap press animation

- Generated with the built-in image generation tool on 2026-09-16.
- Final PNG: `KeycapPress_4x4.png`, 1024 × 1024 RGBA, 4 columns × 4 rows, 256 × 256 per frame.
- Playback: row-major, 16 frames over 1.3 seconds, looping. The ivory keycap has no lettering or pattern.
- UE asset: `/Game/UI/Dialogue/T_KeycapPress_4x4.T_KeycapPress_4x4` (UI compression, no mipmaps).
- Source: `KeycapPress_4x4_Source.png`. The initial generated cutout had background artifacts; the final source was repaired by image generation to an opaque muted background before alpha extraction.
- Alpha: project `icon-alpha-from-solid-bg` script, inferred key `#5C7C84`, transparent distance 14, opaque distance 42. Matched black/white extraction sources are retained for inspection.
- Packing: each generated frame was translated to a common horizontal center and footing baseline, without redrawing its keycap. Translations are recorded in `frame_alignment.json`.
- `KeycapPress_Preview.gif` previews the final frame order against a dark background. The checker and black/white debug sheets are the pre-alignment extraction outputs.

## Final image-generation edit prompt

Repair this sprite sheet background. Keep exactly the same sixteen ivory square blank keycaps, their 4x4 grid, shading and sequential press poses. Replace EVERY background pixel and stray speckle outside the keys with completely opaque perfectly flat solid muted blue-gray RGB(94,127,134), hex #5E7F86. The entire image must be fully opaque, including background. No cutout processing. No dark areas, white artifacts, splatters, checker pattern, noise, gradients or shadows in the background. Crisp key silhouettes. Preserve16frames in uniform4x4 equalcells on squarecanvas; preserve fixed camera/view and matching key positions. Bottom row keycap top should return to original first-row raised height relative to cell, with visible dark switch stem, to complete a loop. No labels, text, logos or marks. This is an opaque color-background production source sheet.
