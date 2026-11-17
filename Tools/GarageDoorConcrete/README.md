# Concrete garage door

The raid entrance uses `/Game/Environment/Bunker/GarageDoor/BP_RaidBunkerGarageDoor`. The Blender source, FBX meshes, PBR maps, UV guides and pose renders are under `TunaSweeper/SourceArt/Environment/GarageDoorConcrete`. Imported assets are under `/Game/Environment/Bunker/GarageDoorConcrete`.

## Painting the door

Open `GarageDoorConcrete.blend`. The four closed leaves are selected, and the Base Color image is active on each paint material. The metal UV guide arranges the twelve front plates as they appear on the closed door, with the top leaf in the first row and the lowest folding leaf in the fourth. Paint across corresponding rows to make a continuous marking or work on a single leaf to localize a scratch. Each plate, return, fastener and rail has its own islands within the metal set. The two signal lenses share one indicator mesh and receive their colors from the actor.

| UV0 texture set | Size | Principal exterior density | Triangle coverage | Guide |
| --- | --- | --- | --- | --- |
| Frame | 2048 × 2048 | 691 px/m | 72.50% | [Frame UV](../../TunaSweeper/SourceArt/Environment/GarageDoorConcrete/Previews/GarageConcrete_FrameUV.png) |
| Metal | 2048 × 2048 | 1,080 px/m | 72.88% | [Metal UV](../../TunaSweeper/SourceArt/Environment/GarageDoorConcrete/Previews/GarageConcrete_MetalUV.png) |

Each island has an eight-pixel edge margin at full resolution. Each set has an sRGB Base Color, a linear tangent-space Normal and a linear ORM texture with ambient occlusion in red, roughness in green and metallic in blue. Blender uses +Y normal maps. Unreal imports them with Flip Green Channel and generates lightmap UVs in channel 1. The optional side walls and roof use the original concrete atlas.

The cast frame has chipped edges, runoff at the lintel and moss around the exposed joint and feet. The ochre stripe crosses the painted faces, with iron oxide at exposed lips, short trails below selected fasteners and trolley scratches on the lowest folding leaf. The shared indicator backing and folded rain hood are a separate mesh.

## Blueprint assembly

The clear width is 200 cm. Each of the four upper leaves is 45 cm tall and 16 cm deep. The 25 cm lower panel extends 10 cm below the floor when closed. The fixed portal is 270 × 45 × 230 cm. X spans the opening, local −Y faces outside and Z points up.

The existing `UpperPanelMeshes` entries use `SM_GarageConcrete_UpperPanel`, `UpperPanel02`, `UpperPanel03` and `UpperPanel04` in descending order. Their centered origins, bounds and hinge placements match the native dimensions. `Use Authored Mesh Materials` is enabled. The LED follows the first hinge. The noncolliding `SignalBackplate` is attached to `ClosingIndicator` at local (0, −1.5, −9) cm.

The lamps use Custom Primitive Data index 0 for emissive strength and indices 1–3 for RGB. Opening illuminates green, closing illuminates red and stationary states set both emissive strengths to zero.

## Rebuilding and verifying

1. For a procedural rebake, set `GARAGE_BAKE_PYTHON` to a Python executable with NumPy and Pillow. The default executable is `python`. Run Blender 4.5 in background with `--python-exit-code 1 --python Tools/GarageDoorConcrete/build_garage.py`. This regenerates the paint textures, UV guides, FBX meshes, packed blend and four pose renders from the surface swatches. Keep any manually painted PNG edits before rebaking.
2. With UE 5.7, run `Tools/GarageDoorConcrete/run_unreal_import.ps1` to import the kit. Reimport preserves existing collision shapes. Newly created folding leaves inherit the first leaf's collision shape.
3. Run `Tools/GarageDoorConcrete/run_unreal_import.ps1 -VerifyOnly` for a fresh-process audit of dimensions, actual render section materials, PBR connections and serialized UV0 triangles across each complete paint set. Results are in `unreal_reload_validation.json`.
4. The placed door in `DemoBoxRaidMap` and a newly constructed Blueprint instance were checked at closed, partially open, open and reclosed poses. See `blueprint_reload_validation.json` and `blueprint_connection_validation.json`. The native automation test `TunaSweeper.GarageDoor.MotionIndicators` covers reversal, pause/resume and the endpoint signals.
