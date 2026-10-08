Anime foliage tree — web v06 port

Origin: astra-prochat-models/foliage_tree_web_v06 (user-provided local source).
scene.json, cards.json, meta.json and PNG textures are unchanged source assets.
The source geometry/meta version is 05; the billboard/wind runtime is v06.

Place /Game/Environment/AnimeTree/BP_AnimeFoliageTree in a level.
The Details panel's Tree category exposes:
  Wind Strength:   default 0.75, range 0.0–1.5 (75%, adjustable 0–150%).
  Leaf Card Scale: default 0.80, range 0.2–1.4.
  Leaf Density:    default 0.45, range 0.0–1.0 (45%, adjustable 0–100%).
Editor changes refresh immediately. At runtime use SetTreeParameters, or call
RefreshTree after setting the BP properties. Each actor has independent MIDs.
Preview wind in a realtime viewport or PIE; it uses material Time, no actor tick.

Source geometry: 39,876 trunk triangles; 2,880 cards / 5,760 leaf triangles.
Conversion: source metres (x,y,z) -> Unreal centimetres (z,x,y)*100.
Original UVs are retained. The trunk uses the supplied DX normal map, relief
strength 0.48, roughness map and UE lighting. The canopy retains the web's
tree-relative baked colors and does not relight as the camera/sun rotates.
The leaf material is masked, two-sided and unlit; alpha cutoff is 0.48.
The web gamma-space atlas/color product is converted to linear for UE output.
Exposure and tonemapping in the destination level still affect the final image.

Leaf mesh data layout (full-precision UVs; lightmap repacking disabled):
  UV0: atlas coordinates
  UV1: local pivot X/Y (cm)
  UV2: local pivot Z (cm), original per-card phase
  UV3: original in-plane offset X/Y (cm), including original random roll
  UV4: bunch index, frac(phase * 8.31) for stable density selection
  Vertex color: source canopy shade, encoded to the original RGB bytes

M_AnimeTree_Leaves contains the ported v06 shader in its WPO Custom node:
  angle = wind*.095*(sin(time*1.25+phase)+.42*sin(time*2.1+phase*1.7));
  q = rotate(originalOffset * leafScale, angle);
  sway = (localRight*2.3 + localUp*.46)*wind*sin(time*.82+bunch*1.39);
  world = pivotWorld + cameraRight*q.x + cameraUp*q.y + sway;
The right/up vectors retain actor scale. Rotation and translation affect the
pivots and sway. Density masks whole cards deterministically, preserving the
same subset as the control increases or decreases. Density 0 hides all leaves.

Leaves do not cast shadows. ShadowProxy uses the same mesh and controls but
StableShadowProxy=1: fixed tree-local right/up, independent of the view camera.
It is excluded from main/depth passes and casts the stable masked shadow.
Leaves and shadow bounds are enlarged for billboarding and wind.
Only the trunk collides (complex-as-simple). There is no gameplay/save state.

Scope: reusable environment BP, no Demo/channel-specific behavior and no
automatic placement in existing gameplay maps. This preserves source geometry;
there are no distance LODs or forest-scale performance guarantees. Masking the
canopy reduces visible density, not the submitted vertex count.

Validation: TunaSweeper.AnimeTree.Assets / Parameters / Rendering automation.
Rendering requires a real RHI and saves captures under Saved/AnimeTreeQA.
The one-off import commandlet is removed after asset validation; the committed
assets load normally and have no startup generation dependency.
