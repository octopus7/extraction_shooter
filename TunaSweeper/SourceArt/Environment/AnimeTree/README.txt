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

Soft whole-canopy rim (Tree / Rim in the Blueprint defaults or actor Details):
  Rim Strength: 0-1, default 0.7; 0 restores the previous leaf colors.
  Rim Color: saturated yellow-green by default; use the color picker to adjust.
  Rim Width: 0.01-0.8, default 0.30; fraction of the canopy radius for the band.
  Rim Brightness: 0-4, default 1.5; keeps underlying leaf luminance variation.
  Rim Envelope Scale: 0.25-2, default 0.85; lower moves paint toward the center.
All tagged clumps share one fitted ellipsoid from their combined rest bounds.
The shader projects it along the camera ray and blends paint into outer leaves.
It does not use individual clump/card normals or restart at clump boundaries.
The middle of the crown retains the original gradient and clump shading.
This is broad soft paint, not a pixel-exact outline: deeply recessed perimeter
segments and separate protrusions do not get a constant screen-space rim width.
Use Envelope Scale/Width to fit a substantially edited or irregular canopy.
The frame updates on clump/actor transform edits and RefreshTree, with no actor
tick or extra rendering pass. Rest-card pivots keep paint steady under wind.
Bush A/B/C use the same actor/material and expose the same per-actor controls.
Previews/Rim_Off.png and Rim_On.png compare strength 0 and the default 0.7 in
the same UE view, with wind paused for a stable comparison.

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
Slots 4-15 are three float4 rows mapping local card pivots into the shared rim
ellipsoid. RimViewX/Y/Z in the actor-owned MID transform world camera rays into
the same normalized space, including actor rotation and nonuniform scaling.

M_AnimeTree_Leaves retains the v06 WPO wind/billboard behavior. Visible cards do
not cast shadows; one transient proxy per clump uses StableShadowProxy=1 with a
fixed local basis and the same density/size/wind. Proxies do not render in main
or depth passes. Only the trunk collides. There is no gameplay/save state.

Scope: environment BP, no Demo/channel branching or automatic map placement.
The old monolithic SM_AnimeTree_Leaves is retained as a source reference and is
not used by this Blueprint. No distance LODs are supplied; masking density does
not reduce vertex count. Clump count also increases draw calls and shadow work.

Validation: TunaSweeper.AnimeTree.Assets / Modular / Parameters / Rendering /
WholeCanopyRim. Rim checks cover shared coordinates, transformed/moved clumps,
controls, collapsed frames, and real top/oblique off/on renders with an unchanged
central crown. Saved/AnimeTreeQA/tree_15..18.png are off/on render pairs.
Rendering uses a real RHI; captures in Saved/AnimeTreeQA include front/side/back,
top, reversed gradient, zero-strength gradient and a moved clump. The one-off
asset commandlet and its dependency are removed before the final build/commit.
