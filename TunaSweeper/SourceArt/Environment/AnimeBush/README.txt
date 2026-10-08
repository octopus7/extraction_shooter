Anime bush variants — editable leaf-clump Blueprints

Assets in /Game/Environment/AnimeBush:
  BP_Bush_A: compact round mound, 5 small clumps.
  BP_Bush_B: low, laterally spreading mound, 6 small clumps.
  BP_Bush_C: slightly taller asymmetric mound, 5 small clumps.

Place the desired BP directly in a level. The origin is at ground level.
Each Blueprint derives directly from TunaSweeperAnimeTreeActor and reuses the
existing Round/Wide/Small card meshes, leaf atlas and material in AnimeTree.
These are independent layouts, not scaled copies of the entire tree Blueprint.
Existing trees and maps are unchanged.

In the Blueprint viewport, move/rotate/scale LeafClump_01 ... LeafClump_05/06.
Duplicate or delete a clump component to change the count. Keep AnimeTreeClump
in Component Tags; the actor discovers tagged components and manages their
materials and stable shadow proxies automatically. Do not edit transient
shadow components. Each actor owns separate dynamic material instances.

Defaults match the tree: Wind Strength 0.75, Leaf Card Scale 0.80, Leaf Density
0.45. The smaller component transforms reduce both the clump and its cards.
The same controls remain adjustable up and down, including zero density/wind.
GradientGuide is near the center of the bush; its +X axis points toward the
light color. Move/rotate/scale it to shape one continuous whole-bush gradient.
Gradient Width is 125 cm for A/C and 150 cm for B; Strength is 0.65.
See ../AnimeTree/README.txt for material, gradient and runtime RefreshTree rules.

SM_Bush_Stems is a shared low-detail, tapered branching mesh: 336 triangles,
about 57 cm high before each BP's adjustment. It stays mostly inside the leaves.
M_Bush_Stems has a calm brown StemColor, Roughness 0.92 and Specular 0.1.
There is no detail normal map or additional bark texture. StemColor can be
adjusted in a material instance. Bush stems and leaves have no collision and
do not affect navigation; these assets are decorative environment props.

Per variant: A/C use 1,440 cards (2,880 visible leaf triangles); B uses 1,728
cards (3,456 visible leaf triangles). Each clump also has a stable shadow proxy.
Mesh/texture assets are reused, but components still incur draw/shadow work.
No new distance LODs or foliage batching are supplied by these Blueprints.

Editable art source: Bush_Stems.blend; import source: SM_Bush_Stems.fbx.
The Blender scene uses centimetres and includes no generation scripts.
Previews/Bush_ABC.png is an actual UE GPU capture: A, B, C from left to right.

Validation: fresh Blueprint reload/spawn, editable tagged clumps, shared source
meshes, independent actor materials, parameter propagation, no duplicate shadow
proxies after refresh, no collision, and front/back/top GPU captures.
One-off asset-generation scripts are removed after validation. No startup
regeneration, C++ changes, Demo/channel branching, UI or save data was added.
