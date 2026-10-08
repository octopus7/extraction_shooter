Anime foliage tree — modular canopy / web v06 port

Origin: astra-prochat-models/foliage_tree_web_v06 (user-provided local source).
scene.json, cards.json, meta.json and original PNG textures remain unchanged references.
The source geometry/meta version is 05; the billboard/wind runtime is v06.

Place /Game/Environment/AnimeTree/BP_AnimeFoliageTree in a level.
Open the Blueprint's viewport to shape the canopy:
  LeafClump_01 ... LeafClump_10 are editable StaticMesh components.
  Duplicate/delete these components to change clump count, and use ordinary
  move/rotate/scale gizmos to edit their placement. No fixed clump-count limit.
  Round, Wide and Small meshes each contain 288 cards. Their pivots sit at the
  source ellipsoid centers. The ten default components reuse these three assets.
  Component Tags must contain AnimeTreeClump (retained by duplication).
  A newly added ordinary StaticMesh component can participate with that tag
  and one of the SM_AnimeTree_Clump* meshes. Ordinary meshes lack the card UVs.
  Shadows are generated automatically and follow each clump's transform.
  Default center spread: X 210 cm, Y 270 cm; source X 44 cm, Y 291 cm.
  Components are used for direct BP editing. Mesh assets and MIDs are shared;
  this is not a custom ISM/HISM batching implementation.

Tree controls (Class Defaults or a placed actor's Details):
  Wind Strength:   default 0.75, range 0.0–1.5.
  Leaf Card Scale: default 0.80, range 0.2–1.4.
  Leaf Density:    default 0.45, range 0.0–1.0.

One whole-canopy gradient:
  Select GradientGuide. Its local +X axis points toward Gradient Light Color.
  Move it to shift the ramp; rotate it to change the direction. Its X scale
  stretches the ramp. Gradient Width is the dark-to-light distance in cm
  before guide scaling; default 400, minimum 1. The guide origin is midpoint.
  Gradient Strength: 0–1, default 0.65; 0 restores the ungraded clump shading.
  Gradient Exponent: 0.1–4, default 1; bends the eased ramp's response.
  Gradient Dark/Light Color multiply the underlying linear leaf color.
  Clump Shading Strength: 0–1, default 0.7; controls original baked volume shade
  independently of the global ramp. 0 is useful for inspecting the global ramp.
  All clumps sample the same guide, never their individual mesh bounds. Adding
  or moving a clump does not restart the gradient. A collapsed guide gives the
  same midpoint to every card instead of invalid shader values.

Shared silhouette rim (Tree / Rim in the Blueprint defaults or actor Details):
  Rim Strength: 0-1, default 0.7; 0 disables paint and extra leaf mask draws.
  Rim Color: saturated yellow-green by default; independent for each tree.
  Rim Width Pixels: 1-128, default 28; interior band in render pixels before
    temporal upscaling. This replaces the old radius-relative Rim Width.
  Rim Brightness: 0-4, default 1.5; retains underlying leaf luminance variation.
The old card-pivot ellipsoid implementation, Rim Envelope Scale, RimView rows
and CPD slots 4-15 are retired. The leaf material is restored to its pre-rim
version; whole-tree GradientGuide, card wind/density/size and shading remain.

CanopyRim is an early-loading runtime module with one world subsystem. Each
registered tree receives one stencil ID (16-255), shared by all of its visible
leaf components. Shadow proxies and trunks do not enter the mask. Existing
placement/cover outlines use 1-3. Released IDs are reused; beyond 240 registered
active trees per world, additional trees keep their normal colors without rim.
Disable Rim Strength on distant decorative trees if that limit is relevant.

UE's existing full-resolution CustomDepth pass supplies actual billboard/WPO
wind and alpha coverage. The extra shared mask and distance buffers are each
ceil(render width/4) by ceil(render height/4): 1/16 of the source pixel count.
There are no per-tree render targets or SceneCapture components. Small holes
are closed in the mask, then an ID-aware boundary distance controls soft paint.
The full-resolution composite tests leaf ID and scene depth, so filled gaps,
foreground objects and the ground are never colored. It runs before DOF and
TSR/TAA. Every frame follows the current wind-deformed silhouette.

Console controls:
  r.CanopyRim 0/1: disable/enable screen passes (leaf mask registration remains).
  r.CanopyRim.GapRadius 0-8: small-gap closing radius in mask texels; default 3.
  r.CanopyRim.Debug 0/1/2/3: normal / source IDs / closed mask / paint weight.
Limits: large openings can retain an inner rim; gap closing smooths very fine
concavities. A single ID/depth layer represents the frontmost visible leaf,
not complete hidden crowns. Screen-space width changes relative to tree size
when zooming. Desktop SM5/SM6 deferred rendering is supported; mobile is skipped.
Exact GPU cost and final artistic/TSR quality require target-scene evaluation.

Bush A/B/C share this actor/material and the same per-actor rim controls.
Previews/Rim_Off.png and Rim_On.png are actual UE captures at strength 0/1,
wind paused. SharedMask.png shows the processed mask for two overlapping trees;
SharedRim_TwoTrees.png uses deliberately distinct test colors to show separation.

Editor changes refresh immediately. Runtime SetTreeParameters applies the
wind/size/density controls; call RefreshTree after changing other BP properties,
adding/removing clumps or replacing their meshes. Clump/guide transforms update
on transform notifications; there is no per-frame actor tick. A whole actor
transform leaves the gradient attached to the tree. Each tree owns independent
visible/shadow MIDs. Preview wind in a realtime viewport or PIE.

Coordinate conversion: source metres (x,y,z) -> Unreal centimetres (z,x,y)*100.
Trunk: unchanged 39,876 triangles and original UVs. The active material now uses
T_AnimeTree_TrunkStylizedBaseColor, a 2048px atlas baked from image-generated
bark_stylized_source.png. Broad warm brown planes replace the photographic grain.
The original 15-branch skeleton guides the painted grain; nearby projections
blend at forks so roots and branches have no abrupt projection boundaries.
Normal mapping is disabled; BarkRoughness is 0.92 and BarkSpecular is 0.1.
Geometry normals and UE scene lighting still shade the trunk. The original bark,
normal and roughness textures are retained as references, not active inputs.
trunk_stylized.blend contains editable mesh/UVs, projection attributes/material,
packed source and baked images, and neutral preview lighting. The shader bakes
color only. See bark_stylized_prompt.txt for both built-in imagegen prompts and
projection details. No startup asset generator is needed.
Leaves: masked, two-sided, unlit, alpha cutoff 0.48. The source
atlas and baked vertex colors are multiplied in gamma space then converted to
linear. The whole-tree color multiplier is applied after that conversion.
Exposure and tonemapping in the destination level still affect the final image.

Clump mesh data (full-precision UVs; lightmap repacking disabled):
  UV0: atlas coordinates
  UV1: clump-local rest card pivot X/Y (cm)
  UV2: clump-local rest card pivot Z (cm), original per-card phase
  UV3: original in-plane card offset X/Y (cm), including original random roll
  UV4: source bunch index, frac(phase * 8.31) for stable density selection
  Vertex color: source shade, encoded to original RGB bytes
Custom Primitive Data slots 0–3 are reserved for the tree's gradient equation:
  t = saturate(0.5 + dot(restCardPivot, CPD.xyz) + CPD.w)
  t = pow(t*t*(3-2*t), GradientExponent)
  final = clumpColor * lerp(1, lerp(darkColor, lightColor, t), GradientStrength)
Coefficients map each component into the same GradientGuide space relative to
TreeRoot. Using rest pivots avoids color swimming with billboarding and wind.

M_AnimeTree_Leaves retains the v06 WPO wind/billboard behavior. Visible cards do
not cast shadows; one transient proxy per clump uses StableShadowProxy=1 with a
fixed local basis and the same density/size/wind. Proxies do not render in main
or depth passes. Only the trunk collides. There is no gameplay/save state.

Scope: environment BP, no Demo/channel branching or automatic map placement.
The old monolithic SM_AnimeTree_Leaves is retained as a source reference and is
not used by this Blueprint. No distance LODs are supplied; masking density does
not reduce vertex count. Clump count also increases draw calls and shadow work.

Validation: TunaSweeper.AnimeTree.Assets / Modular / Parameters / Rendering /
SharedMask / MaskRegistry. SharedMask verifies per-tree IDs, clump grouping,
no trunk/proxy participation, strength/density off and quarter-size rounding.
MaskRegistry covers exhaustion, stable IDs and reuse. Real-RHI Rendering checks
front/side/back/top, edits, gradient reversal, outer paint with unchanged crown
center, independent overlapping colors, foreground occlusion and moving masks.
Saved/AnimeTreeQA/tree_15..18.png: off/on pairs; 19..23: overlapping trees/debug
masks; 24..25: foreground blocker; 26..27: wind-deformed mask over time.
No asset generator, startup regeneration, game UI, persistence or Demo/channel
branching is introduced. The user's RaidMap is not modified by this task.
