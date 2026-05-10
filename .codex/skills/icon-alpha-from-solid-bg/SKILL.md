---
name: icon-alpha-from-solid-bg
description: Project-local workflow for making transparent icon sheets and item icons by generating one flat mid-value, low-to-moderate saturation solid-background source image, deriving matched black and white background images locally, then calculating the alpha channel from the black/white difference. Use for TunaSweeper transparent icon generation, sprite sheets, inventory item icons, or any request to remove icon backgrounds without chroma-key fringing.
---

# Icon Alpha From Solid Background

Use this project-local skill whenever creating transparent icons or icon sheets for TunaSweeper.

## Required Workflow

1. Generate exactly one source image on a flat solid background.
2. Choose a background color with medium value and low-to-moderate saturation.
   - Prefer muted colors such as `#5E7F86`, `#756F5C`, `#6C7480`, `#7A6B78`, or `#637760`.
   - Avoid high-saturation chroma keys such as magenta, neon green, pure blue, pure red, or pure cyan.
   - Avoid colors that appear inside the icon subjects.
3. Do not generate black and white versions independently.
4. Run `scripts/extract-alpha-from-solid-bg.ps1` on the one source image.
5. Keep the generated black/white pair as debug sources when useful, and use the alpha PNG as the final asset.

Prompt the source image like this:

```text
Use a perfectly flat solid background color <#RRGGBB> across the entire image.
The background must be uniform with no checkerboard, texture, gradient, shadow, reflection, border, text, or watermark.
Do not use <#RRGGBB> anywhere inside the items.
Keep every item opaque, cleanly separated from the background, with crisp edges and enough padding.
```

## Script Usage

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\.codex\skills\icon-alpha-from-solid-bg\scripts\extract-alpha-from-solid-bg.ps1 `
  -BasePath .\GeneratedImages\ItemIcons\Source.png `
  -OutputPath .\GeneratedImages\ItemIcons\IconSheet_Transparent.png `
  -OutputSize 1024
```

The script:

- estimates the solid key color from the image border unless `-KeyColor "#RRGGBB"` is provided;
- computes an alpha matte from distance to that source background color;
- derives matched black and white background images from the same source;
- calculates alpha from the derived pair using `alpha = 255 - average(white - black)`;
- writes a `Format32bppArgb` PNG;
- optionally writes black, white, and checker preview outputs.

Useful parameters:

```powershell
-BlackPath <path>       # override black debug output path
-WhitePath <path>       # override white debug output path
-PreviewPath <path>     # override checker preview path
-KeyColor "#RRGGBB"     # use when border auto-detection is wrong
-OutputSize 1024        # resize square output; omit or use 0 to preserve source size
-TransparentDistance 14 # smaller removes more near-background pixels
-OpaqueDistance 88      # smaller makes edges harder; larger keeps softer edges
```

## Validation

After running, inspect the checker preview. Corners and empty areas should show only the checker background. If a colored halo remains:

- rerun with a less saturated source background color;
- provide `-KeyColor` explicitly;
- lower `-TransparentDistance` slightly for stricter background removal;
- reduce `-OpaqueDistance` for harder edges.

