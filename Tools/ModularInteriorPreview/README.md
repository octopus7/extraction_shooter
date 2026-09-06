# Modular industrial interior / minimum eight

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
| Ceiling | 200x200x20cm, underside lower XY corner, Z=0..20 | 12 | 1 | 1 |
| Beam | 200x20x20cm, upper centerline start, Z=-20..0 | 12 | 1 | 1 |
| LightBar | 150x12x8cm, upper centerline start; underside emissive | 12 | 2 | 0 |

Doorway opening is X=100..300cm and Z=0..240cm. The 10cm flush painted-metal frame is part of the watertight U mesh, with three separate collision hulls leaving the opening clear. Closed leaf pivot is at doorway-relative `(102,6,2)` cm, leaving 2cm clearances on all four edges. Its hinge axis is on the front thickness corner, so the 8cm thick leaf clears the jamb during the -68-degree opening. Sample leaf yaw=-68 degrees is a fixed review pose; no hinge hardware geometry or animation is included. Existing concrete automatic-door opening width is also 200cm, but its 270x45x230cm frame and motion setup are different; these assets do not replace it.

Room interior is exactly 600x600cm, floor Z=0, ceiling underside Z=300. A 20cm threshold crosses the doorway wall, made from two scaled floor/ceiling instances. Corridor footprint is `(100,620),(500,620),(500,1020),(900,1020),(900,1420),(100,1420)` cm; both legs have 400cm clear width. Coplanar mating endcaps are buried inside joints; there is no exposed coplanar trim or decal stacking. Open door sweep and circulation are preview-only.

## Shared surfaces and wear

Built-in ImageGen produced both images; exact prompts are in `texture.prompt.txt` and `dirt.prompt.txt`. Originals are retained. Runtime copies are mechanically resized only (Blender image.scale); no local repainting or fabricated normal maps. Atlas: 2048x2048 sRGB, four quadrants (concrete / painted panel / floor / door). Fixed mild wear, inset panel lines and screw marks are baked color detail. Dirt mask: 1024x1024, grayscale, non-color/linear, grayscale compression in UE. The mask mixes BaseColor toward dust color, Roughness toward .94, and Metallic toward zero. All surfaces stay Opaque. No decals, alpha blending, normal maps, per-module texture sets or triplanar projection.

UV0 is the shared atlas; UV1 (`DirtUV`) is a planar 4m coverage map. UE generates lightmap UV2, preserving dirt UV1. Atlas regions have gutters; dirt offsets vary by instance. Mask tile edges are ImageGen-authored, not mathematically periodic; faint wrap transitions can remain in strong coverage. Small length adapters compress their authored UVs proportionally; restrained broad concrete texture makes this inconspicuous in the reviewed sample. Avoid using severe adapter scaling as the default wall run.

UE Custom Primitive Data: 0 DirtStrength (0 clean / .65 weak default / 1.8 strong), 1 DirtScale (default 1; larger = smaller stains), 2 DirtOffsetU, 3 DirtOffsetV. DirtColor, DustRoughness, DustMetallic are material parameters. There are two texture reads per opaque surface; LED uses constant emission with zero texture reads. Setting strength to zero preserves the mask sample cost in this simple shared shader. No measured PBR, shader instruction count or FPS improvement is claimed.

Light / Dark / Managed are material parameter variants on the same meshes. Managed adds a teal shader band at world height 105..130cm without geometry. UE concrete variants are material instances; `apply_preview_look.py` applies review look, dirt amount/size, and ceiling visibility without saving. No threefold mesh duplication.

## Scene and performance accounting

Eight unique meshes total **184 triangles**. Full sample: **90 instances / 1,224 triangles**, including 23 floors, 23 ceilings, 17 walls, 8 corners, 1 doorway, 1 leaf, 12 beams and 5 light bars. Ceiling-hidden views show 67 kit instances / 948 triangles. Reference mannequin and cameras/lights are excluded from kit counts. All meshes use LOD0, no Nanite, no subdivisions or bevel segments. Counts are geometric budgets, not FPS measurements. The individual components remain separate actors for convenient review; no draw-call batching performance is claimed.

Five emissive bar meshes and five actual local lights are separate costs. Blender uses area lights and one overview-only softbox; UE uses five movable rect lights. These are preview lighting choices; broad real-time rect-light shadows/Lumen may cost more than this very small mesh budget. The LED housing has no collision. Structural meshes use authored simple UCX hulls, never complex-as-simple.

Blender previews: three material looks in overview and eye height; clean/weak/strong dirt in matching views; a project top-down camera view; FBX reload proof render. Eye camera is 160cm. Temporary reference is 176cm high / 68cm wide, matching native player capsule. Gameplay top-down source settings: arm 1500cm, pitch -88 degrees, yaw 0, horizontal FOV70, target at capsule-center Z=88cm. Blender play frame uses a horizontal film flip to match UE handedness (+X up, +Y right). Ceiling is hidden only for top-down/overview review. This is not a claim of an implemented runtime roof-hiding feature.

## Rebuild and audit

1. Blender 4.5: `blender --factory-startup -b --python-exit-code 1 --python Tools/ModularInteriorPreview/build_interior.py`. Set `MI_SKIP_RENDER=1` only for geometry iteration.
2. Fresh process: `blender --factory-startup -b --python-exit-code 1 --python Tools/ModularInteriorPreview/validate_source.py`. Checks manifold/outward normals, nondegenerate triangles and UVs, signed bounds/units, pivots, material slots, simple collision and assembly joins. Reloaded FBX replaces sample meshes for proof render. `source_validation.json` is the measured report.
3. First local commit contains verified source, FBX, images and previews.
4. Build TunaSweeperEditor Win64 Development with UE5.7. `run_unreal.ps1`, then `run_unreal.ps1 -VerifyOnly`, imports and audits persisted asset data in a fresh process. `run_unreal.ps1 -Map`, then `-Map -VerifyOnly` creates/reloads the dedicated map and checks transforms, primitive-data values and actual player Blueprint camera settings. Existing editor bootstrap completion state must already be present when starting the full editor; the worktree copies its current matching host checkout's RunOnce section to Saved config.
5. Second local commit contains UE assets/map, import tools and UE verification reports. No push.

The existing project may emit a pre-existing Niagara NE_PostProcess typed-element registry ensure at commandlet startup. Runner exit codes and asset assertions are reported separately; a successful JSON audit does not erase an engine error exit.
