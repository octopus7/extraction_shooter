# Stylized Water

`StylizedWater` is a self-contained UE 5.7 rendering plugin for calm anime-background water.

## User workflow

Do not place or subclass the generated internal `BP_` asset and do not assign the generated `MI_` asset manually.

1. Open `TunaSweeper > Rendering > Stylized Water` in the Level Editor menu.
2. Add a Calm Lake, Gentle Beach, or Flowing River.
3. Resize the generated actor and choose `Rebuild And Bake Terrain Depth` in Details.
4. Tune color, shore waves, foam, flow, distortion, and water-level properties on the placed actor.

`Depth Bake > Last Bake Result` reports the number of successful trace samples and the baked minimum/maximum depth. If it reports no hits or almost no depth range, check the water actor height, landscape collision response, and `Terrain Trace Channel`, then bake again.

`Enable Terrain-Following Runup` builds a second non-colliding procedural mesh on the sampled terrain. It lets the animated waterline and foam render above dry shoreline terrain instead of being hidden below it. That overlay uses a separate generated translucent material: `Shore Water Opacity` controls the pale, terrain-visible water film and `Shore Foam Opacity` controls the stronger white foam. Its signed-depth alpha fades continuously, so grid-cell edges remain invisible. `Terrain Overlay Offset` keeps the mesh slightly above the ground to avoid z-fighting. Rebuild and bake after sculpting the shoreline.

The default depth palette comes from the preserved ImageGen study at `Resources/SourceArt/WaterDepthPalette_ImageGen.png`. Its vertical average is stored as the deterministic `256x1` LUT `WaterDepthGradient_1D.png`, with an enlarged review image at `WaterDepthGradient_Preview.png`. The corresponding `T_WaterDepthGradient` asset is committed in plugin Content. `ImageGen Depth Gradient Influence` blends from the legacy three-color controls at `0` to the generated LUT at `1`.

The internal surface/shore materials, material instances, and Blueprint template are committed under `/StylizedWater/Generated/Internal`. The menu loads those saved assets and connects both materials. Startup generation and rebuild flags were retired on 2026-09-06; edit saved assets directly and restore missing files from Git/LFS.

## Rendering model

- A generated procedural grid samples signed terrain depth into vertex color R.
- A terrain-following shore overlay reuses the same baked depth so runup and foam can cross above the flat water plane.
- The masked Single Layer Water material uses that depth for shallow/mid/deep color, the behind-water composite tint, visual waterline motion, runup, and foam.
- The shore overlay uses a separate Default Lit translucent material with continuous depth coverage, a low-opacity shallow tint, and independently weighted foam.
- World-space procedural normals provide restrained flow and distortion.
- Small WPO waves are optional and require enough grid subdivisions.
- `WaterLevelOffset` is a visual waterline/depth offset. It does not alter collision or navigation.
