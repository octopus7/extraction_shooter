# Bunker pipe presentation

- The pipe rises from the floor and turns 90 degrees into the wall at 99 cm. The complete asset is 107.1 cm high, including its wall flange; placement scale is preserved.
- Both states share the same 636-triangle beveled square elbow geometry, couplings, brackets, and flanges. They differ only through the base-color texture panel and material.
- `SM_BunkerPipe_LowPoly_Normal` uses `M_BunkerPipe_Normal`; `SM_BunkerPipe_LowPoly_Damaged` uses `M_BunkerPipe_Damaged`. Both read `T_BunkerPipe_States`, respectively its left and right panels.
- Before `demo.water_intake.blocked_screen` is completed, the pipe is intact and has no repair interaction or water emission. After cleaning, damage and water appear. Repair immediately stops emission and replaces the actor with the intact pipe, preserving its complete transform.
- `P_BunkerPipe_Leak` contains one CPU sprite emitter: 14 particles/second, 0.52-second lifetime, velocity-aligned 1.5 x 9 cm tapered water tails, a narrow outward spray, downward acceleration, and alpha fade. It has no collision, lights, puddles, or secondary spray. The origin is local `(0, 11.6, 49.5)` cm and follows the user's placed actor rotation.
- The replacement meshes retain simple collision. Presentation derives from existing saved world progress; no save schema was added.
- The user's BunkerMap placement rotation is retained. No generator modifies the map.

Editable source and previews are in `TunaSweeper/SourceArt/Props/BunkerPipe`: `BunkerPipe.blend`, `SM_BunkerPipe.fbx`, the PNG atlas, Blender preview, and UE review capture. Runtime assets are in `/Game/Interaction/BunkerPipe`.

## Texture generation

Generated with the built-in image generation tool; no API CLI fallback was used. The saved original is `TunaSweeper/SourceArt/Props/BunkerPipe/Textures/T_BunkerPipe_States.png`.

Prompt:

> Create a production game texture atlas, a single flat 2D base-color/albedo bitmap, square 1024x1024. It will be UV mapped onto a stylized low-poly bunker plumbing pipe. EXACT layout: two equal vertical rectangular panels, left half healthy intact pipe surface, right half damaged leaking pipe surface. Panels extend edge to edge, zero margins, zero labels, no text. This is NOT a picture of a pipe, NOT a 3D object, NO perspective or cast shadows. Each panel is a flattened painted metal surface seen orthographically. Both panels have the same warm desaturated pale sage gray painted steel base with broad quiet hand-painted tonal variation, suited to a cozy stylized low-poly game. LEFT: intact smooth maintained enamel paint, a few broad subtle edge wear strokes, no cracks, no holes, no rust blooms. RIGHT: clearly damaged version of that same material: a distinctive dark jagged hairline fissure concentrated around the exact middle of this right panel (75% image width, 50% image height), medium-sized muted burnt orange rust patches around fissure, simple dark damp streak below fissure, restrained chipped paint, still mostly sage gray. Low-frequency clean painterly shapes only, no noisy tiny dots or photoreal grunge. Crack region occupies about one quarter of the panel width. Lighting-free diffuse color, no highlights baked as lighting, no ambient occlusion, no water drawn, no borders. Whole atlas is opaque. Intended for mapping one common angular elbow pipe mesh with identical UVs to either the left or right panel, keeping normal and broken geometry identical.
