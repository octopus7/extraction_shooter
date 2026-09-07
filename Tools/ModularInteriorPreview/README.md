# Modular industrial interior / six roofless modules

Source: `TunaSweeper/SourceArt/Environment/ModularInteriorPreview`. Runtime destination: `/Game/Environment/ModularInteriorPreview`. This is an isolated look-development kit and map, with no door gameplay, save data or changes to game maps/Blueprints.

## Assembly contract (meters in Blender, centimeters in UE)

Project north = +X, east = +Y, up = +Z. Blender export copies mirror Y and reverse winding to compensate UE FBX handedness; the authored .blend retains project coordinates. Every FBX is at origin with unit object scale. Do not add another import scale or rotation.

The 50cm planning grid applies to primary dimensions; wall thickness is explicitly 20cm. **Indoor finish faces**, rather than wall centerlines, define clear dimensions. Wall local X runs from 0 to 2m; local Y=0 is the indoor face, thickness extends toward +Y. Its pivot is the lower start of that indoor face. Standard wall dimensions are 200x20x300cm.

An InsideCorner's pivot is the intersection of its two indoor finish faces, at floor level. Its footprint is the L polygon `(-20,-20),(100,-20),(100,0),(0,0),(0,100),(-20,100)` cm. A convex room corner consumes 100cm on each adjacent indoor wall run. Straight 200cm walls fill the remaining 400cm of a 600cm room edge. For a concave L-corridor corner, rotate the same mesh toward the solid quadrant and shift its pivot by 20cm along both arms. Its exposed outer edges consume 120cm each; the adjacent straight run is shortened accordingly. The manifest records all exact transforms. Small terminal wall pieces use X scale (80cm, 100cm or 180cm lengths) rather than extra meshes. Thickness and height never scale. These are explicit thickness/end-run adapters, not a claim that every pivot is on the 50cm grid.

| Model | Size / origin | Triangles | Slots | Simple hulls |
|---|---|---:|---:|---:|
| Floor | 200x200x20cm, top lower XY corner, Z=-20..0 | 12 | 1 | 1 |
| Wall | 200x20x300cm, indoor lower start | 12 | 1 | 1 |
| InsideCorner | 120x120x300cm L, finish-face intersection | 20 | 1 | 2 |
| Doorway | 400x20x300cm, indoor lower left; fixed frame integrated | 92 | 2 | 3 |
| DoorLeaf | 196x8x236cm, lower hinge corner on front face, depth 0..8cm | 12 | 1 | 1 |
| LightBar | 150x8x12cm, rear lower start on wall; front and top emissive | 12 | 2 | 0 |

Doorway opening is X=100..300cm and Z=0..240cm. The 10cm flush painted-metal frame is part of the watertight U mesh, with three separate collision hulls leaving the opening clear. Closed leaf pivot is at doorway-relative `(102,6,2)` cm, leaving 2cm clearances on all four edges. Its hinge axis is on the front thickness corner, so the 8cm thick leaf clears the jamb during the -68-degree opening. Sample leaf yaw=-68 degrees is a fixed review pose; no hinge hardware geometry or animation is included. Existing concrete automatic-door opening width is also 200cm, but its 270x45x230cm frame and motion setup are different; these assets do not replace it.

Room interior is exactly 600x600cm, floor Z=0, wall top Z=300, open above. A 20cm threshold crosses the doorway wall, made from two scaled floor instances. Corridor footprint is `(100,620),(500,620),(500,1020),(900,1020),(900,1420),(100,1420)` cm; both legs have 400cm clear width. Coplanar mating endcaps are buried inside joints; there is no exposed coplanar trim or decal stacking. Open door sweep and circulation are preview-only.

## Shared surfaces and wear

Built-in ImageGen produced both images; exact prompts are in `texture.prompt.txt` and `dirt.prompt.txt`. Originals are retained. Runtime copies are mechanically resized only (Blender image.scale); no local repainting or fabricated normal maps. Atlas: 2048x2048 sRGB, four quadrants (concrete / painted panel / floor / door). Fixed mild wear, inset panel lines and screw marks are baked color detail. Dirt mask: 1024x1024, grayscale, non-color/linear, grayscale compression in UE. The mask mixes BaseColor toward dust color, Roughness toward .94, and Metallic toward zero. All surfaces stay Opaque. No decals, alpha blending, normal maps, per-module texture sets or triplanar projection.

UV0 is the shared atlas; UV1 (`DirtUV`) is a planar 4m coverage map. UE generates lightmap UV2, preserving dirt UV1. Atlas regions have gutters; dirt offsets vary by instance. Mask tile edges are ImageGen-authored, not mathematically periodic; faint wrap transitions can remain in strong coverage. Small length adapters compress their authored UVs proportionally; restrained broad concrete texture makes this inconspicuous in the reviewed sample. Avoid using severe adapter scaling as the default wall run.

UE Custom Primitive Data: 0 DirtStrength (0 clean / .65 weak default / 1.8 strong), 1 DirtScale (default 1; larger = smaller stains), 2 DirtOffsetU, 3 DirtOffsetV. DirtColor, DustRoughness, DustMetallic are material parameters. There are two texture reads per opaque surface; LED uses constant emission with zero texture reads. Setting strength to zero preserves the mask sample cost in this simple shared shader. No measured PBR, shader instruction count or FPS improvement is claimed.

Light / Dark / Managed are material parameter variants on the same meshes. Managed adds a teal shader band at world height 105..130cm without geometry. UE concrete variants are material instances; `apply_preview_look.py` applies review look, dirt amount/size without saving. No threefold mesh duplication.

## Scene and performance accounting

Six unique meshes total **160 triangles**. Full sample: **55 instances / 804 triangles**, including 23 floors, 17 walls, 8 corners, 1 doorway, 1 leaf and 5 wall-mounted light bars. Ceiling and Beam are removed from source files, FBXs, UE assets and the saved map. Reference geometry and cameras/lights are excluded. All meshes use LOD0, no Nanite, subdivisions or bevel segments. Counts are geometric budgets, not FPS measurements. The individual components remain separate actors for convenient review; no draw-call batching performance is claimed.

Five emissive bar meshes and five actual local lights are separate costs. Blender uses area lights and one softbox for roofless review; UE uses five movable rect lights. These are preview lighting choices; broad real-time rect-light shadows/Lumen may cost more than this very small mesh budget. The LED housing has no collision. FBX carries authored simple UCX boxes. The UE importer stores the same measured shapes as cheaper analytic box primitives (eight boxes across the six unique meshes), with SimpleAndComplex explicitly selected. It verifies every box center and dimension. Complex-as-simple is not used.

Primary review uses room and corridor gameplay-camera views; material and dirt comparisons default to top-down, with overviews and supplementary eye-height views. Eye camera is 160cm. Temporary reference is 176cm high / 68cm wide, matching native player capsule. Gameplay top-down settings: arm 1500cm, pitch -88 degrees, yaw 0, horizontal FOV70, target at capsule-center Z=88cm. Blender play frames use a horizontal film flip to match UE handedness (+X up, +Y right). UE_PlayCamera.png and UE_CorridorPlayCamera.png are editor viewport captures through matching camera actors. These are camera-matched asset previews, not a PIE gameplay session. Preview camera exposure compensation is -1.5 EV to retain atlas detail near wall lights; player settings are unchanged. Walls remain full-height; their top edges and doorway header still occlude narrow strips near the boundary. The room center and corridor circulation remain visible.

## Rebuild and audit

1. Blender 4.5: `blender --factory-startup -b --python-exit-code 1 --python Tools/ModularInteriorPreview/build_interior.py`. Set `MI_SKIP_RENDER=1` only for geometry iteration.
2. Fresh process: `blender --factory-startup -b --python-exit-code 1 --python Tools/ModularInteriorPreview/validate_source.py`. Checks manifold/outward normals, nondegenerate triangles and UVs, signed bounds/units, pivots, material slots, simple collision and assembly joins. Reloaded FBX replaces sample meshes for proof render. `source_validation.json` is the measured report.
3. Existing UE5.7 editor binaries are sufficient. Run `verify_unreal.ps1` for persisted asset checks and `verify_unreal.ps1 -Map` for map transforms, wall lights, cameras, CPD and simple-collision traces. These tools only load assets and write JSON reports; they cannot import, spawn, delete or save UE assets. The map audit uses full-editor ticks and needs the existing editor bootstrap RunOnce completion state in local Saved config.
4. One-off UE generation and screenshot code, its entry points and import-only runner flags were removed immediately after asset commit `473c3fae`. There is no startup regeneration path or project plugin dependency added by this kit. The reusable Blender source tool remains; UE assets are maintained through normal editor workflows. The following cleanup commit retains only read-only UE audits and the unsaved material-look helper.

UE 5.7.4 final asset import/reload assertions passed; the existing Niagara NE_PostProcess typed-element registry ensure makes these commandlets exit 1. Full-editor map creation/reload exits 0. The map runner uses console bootstrap and lets the editor tick 30 frames after loading, so Chaos registration finishes before collision queries. Fresh-process rays verify the wall, jamb and floor block, while the open doorway passes. No PIE/game instance is started during the audit. See `VALIDATION.md` and the four UE JSON reports. Runner exit codes and asset assertions are reported separately; a successful JSON audit does not erase an engine error exit.
