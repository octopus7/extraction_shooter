# Stylized Water

`StylizedWater` is a self-contained UE 5.7 rendering plugin for calm anime-background water.

## User workflow

Do not place or subclass the generated internal `BP_` asset and do not assign the generated `MI_` asset manually.

1. Open `TunaSweeper > Rendering > Stylized Water` in the Level Editor menu.
2. Add a Calm Lake, Gentle Beach, or Flowing River.
3. Resize the generated actor and choose `Rebuild And Bake Terrain Depth` in Details.
4. Tune color, shore waves, foam, flow, distortion, and water-level properties on the placed actor.

`Depth Bake > Last Bake Result` reports the number of successful trace samples and the baked minimum/maximum depth. If it reports no hits or almost no depth range, check the water actor height, landscape collision response, and `Terrain Trace Channel`, then bake again.

The default depth palette comes from the preserved ImageGen study at `Resources/SourceArt/WaterDepthPalette_ImageGen.png`. Its vertical average is stored as the deterministic `256x1` LUT `WaterDepthGradient_1D.png`, with an enlarged review image at `WaterDepthGradient_Preview.png`. The editor module turns that LUT into the internal `T_WaterDepthGradient` asset automatically. `ImageGen Depth Gradient Influence` blends from the legacy three-color controls at `0` to the generated LUT at `1`.

The plugin creates its internal material, material instance, and Blueprint template under `/StylizedWater/Generated/Internal`. The menu uses that template and connects its material automatically.

## Rendering model

- A generated procedural grid samples signed terrain depth into vertex color R.
- The masked Single Layer Water material uses that depth for shallow/mid/deep color, the behind-water composite tint, visual waterline motion, runup, and foam.
- World-space procedural normals provide restrained flow and distortion.
- Small WPO waves are optional and require enough grid subdivisions.
- `WaterLevelOffset` is a visual waterline/depth offset. It does not alter collision or navigation.
