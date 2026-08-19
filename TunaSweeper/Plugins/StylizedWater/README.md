# Stylized Water

`StylizedWater` is a self-contained UE 5.7 rendering plugin for calm anime-background water.

## User workflow

Do not place or subclass the generated internal `BP_` asset and do not assign the generated `MI_` asset manually.

1. Open `TunaSweeper > Rendering > Stylized Water` in the Level Editor menu.
2. Add a Calm Lake, Gentle Beach, or Flowing River.
3. Resize the generated actor and choose `Rebuild And Bake Terrain Depth` in Details.
4. Tune color, shore waves, foam, flow, distortion, and water-level properties on the placed actor.

The plugin creates its internal material, material instance, and Blueprint template under `/StylizedWater/Generated/Internal`. The menu uses that template and connects its material automatically.

## Rendering model

- A generated procedural grid samples signed terrain depth into vertex color R.
- The masked Single Layer Water material uses that depth for shallow/mid/deep color, visual waterline motion, runup, and foam.
- World-space procedural normals provide restrained flow and distortion.
- Small WPO waves are optional and require enough grid subdivisions.
- `WaterLevelOffset` is a visual waterline/depth offset. It does not alter collision or navigation.
