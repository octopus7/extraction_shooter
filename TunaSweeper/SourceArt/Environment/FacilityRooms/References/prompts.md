# Facility rooms — built-in ImageGen prompts

## Basement_Reference

```text
Use case: stylized-concept
Asset type: 3D game environment modeling reference sheet
Primary request: an underground industrial utility facility reached by climbing down a ladder, with modular concrete walls, a boiler, diesel generator, water pump skid, materials storage and an enclosed ladder access room leading upward.
Composition: one large roofless isometric cutaway of a practical 12m x 10m basement, plus four clearly separated prop closeups along bottom: horizontal boiler, generator, twin blue water pumps, loaded materials rack. Generous central walking aisle and distinct equipment zones. Ladder landing in a small partitioned room. Concrete modular panels have visible regular joints. No ceiling or roof, no overhead beams blocking top-down camera.
Style: polished believable semi-realistic hard surface game environment, modern industrial architecture with restrained near-future control panels, readable chunky functional forms suitable for Blender modeling.
Materials: worn gray concrete, desaturated teal painted machinery, dark iron, brushed steel pipes, blue water pumps, muted yellow safety accents, small warm practical worklights. Modest broad wear, no speckled texture.
Constraints: physically grounded utility installation, realistic human proportions, clean logical paths, no people, no weapons, no text, no watermark, no exterior scenery.
```

## ControlRoom_Reference

```text
Use case: stylized-concept
Asset type: 3D game environment modeling reference
Primary request: a small second-floor command and communications room, 6m x 6m, with situation monitors on the far wall, two operator desks, radio communications rack, radio console with microphone, compact chairs and a ladder landing entrance from the floor below.
Composition: roofless isometric cutaway viewed from front above; room layout clear and cozy, separate enlarged closeup strip of desk monitor, radio rack and handset console. Ladder opening at one corner with protective railing, central aisle clear.
Style: polished semi-realistic modern industrial game environment, restrained near-future electronics, ordinary physical screens and solid consoles, no holograms or space station.
Materials: gray modular concrete panel walls, muted teal and ivory equipment, graphite monitor bezels, steel accents, dark cyan situation displays with simple maps and amber status indicators, warm desk lighting, moderate maintained wear.
Constraints: no ceiling or overhead beams, fully visible room footprint, no people, no weapons, no tiny illegible text or logos, no watermark, no exterior scenery.
```

## T_FacilityRooms_Atlas

```text
Use case: stylized-concept
Asset type: base-color texture atlas for a Blender and Unreal industrial facility modular kit
Primary request: exactly 4 by 4 equal square texture tiles, perfectly aligned edge to edge with no gutters. Flat orthographic albedo-only materials, no objects.
Tiles in row-major order: row 1 gray poured concrete; dark gray industrial rubberized floor; brushed steel; graphite iron. Row 2 muted desaturated teal painted metal; warm ivory painted metal; subdued brown rust; black rubber. Row 3 diagonal muted yellow and charcoal safety stripes; worn plywood; dark teal status monitor interface with a sparse technical map and simple cyan bars; warm amber luminous panel. Row 4 blue enamel water pipe; faded red valve paint; dark control panel face with sparse vents; cream technical paper.
Diffuse neutral albedo capture, subtle broad surface variation, each tile independent uniform scale, light wear, no busy microdetail. Concrete quiet not gritty. Screen tile mostly dark teal with readable bold cyan lines.
Constraints: no 3D objects, no spheres, no bevels, no shadows, no gutter, no labels, no words, no numbers, no logos or watermark. Exact 4x4 layout.
```
