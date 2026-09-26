# Backpack image sources

All concept and texture images were made with the built-in image generation tool. Each backpack has its own generated texture source and its own final 512×512 texture. No runtime texture is shared between backpacks.

## Final concept brief

Five cute, simple, rounded backpack game items: an ivory egg with large tan spots; a cream-yellow chicken with a small beak, red comb and short wings; a green dinosaur carried piggyback with its belly against the wearer, its head tilted skyward and its sharp teeth clenched in a funny closed grin; an olive military backpack; and a charcoal carbon-frame hard case with rounded corners, orange latches and 30% more height than the initial carbon bag. The egg, chicken and dinosaur have no external auxiliary pockets. Avoid pink, clouds, hearts, glitter, noisy microdetail and intricate hardware. Use a flat muted solid background for the project alpha-extraction workflow.

The final concept is `References/BackpackConcept.png`. The matched black and white images and checker preview were derived from that single source using `.codex/skills/icon-alpha-from-solid-bg/scripts/extract-alpha-from-solid-bg.ps1`. Individual icons were cropped from the five main connected components and fitted inside 256×256 canvases with padding.

## Texture prompts

Each `UV/*_PaintGuide.png` was supplied as an exact-layout edit reference. The generated result is retained as `References/*_Texture_ImageGen.png` and resized to 512×512 for the specified density. These are image-generated painted surfaces on continuous UV islands.

Common constraints: preserve the square canvas, island location, shape, orientation, size, color identity and empty space. Paint only within existing islands. Do not enlarge, rearrange, merge, split or delete islands. Use broad low-contrast hand-painted material variation. Avoid outlines, edge shadows, perspective, 3D rendering, labels, grain, tiny fabric patterns, glitter, pink and clouds.

- **Egg:** two ivory shell halves with four large sparse soft tan spots on each half; ochre leather straps. No pockets or painted face.
- **Chicken:** clean creamy golden body and wings; red comb, ochre beak and straps, dark eyes. Eyes and beak are modeled separately. No pocket or pouch painting.
- **Dinosaur:** clean muted green skin and straps; ochre back bumps; ivory clenched teeth; dark eyes. No additional face or pocket painted onto the body.
- **Military:** muted olive canvas and sand webbing; gentle broad material variation. No camouflage, logos, tiny seams or grime.
- **CarbonFrame:** graphite rigid shell panels, dark carbon frame strips and orange rigid latches. No soft fabric folds, front pouches or carbon weave pattern.

## Density and validation

The previous 1254px shared image measured approximately 616.525px/m. The final target is half that, approximately 308.263px/m, normalized per UV island. `UV/density_target.json` records the measurement. Each final texture has a separate material and image datablock, and is packed into `Backpacks.blend`.

`Tools/Backpacks/verify_source.py` reloads the saved Blender file and all five FBX files, checks geometry and UV areas, positive-area UV intersections, island density, per-item texture identity and dimensions. `Tools/Backpacks/verify_unreal.py` reloads saved UE assets, checks triangle counts, dimensions, UV channels, UI icon settings and five separate material/texture references. The one-off model and import generators are removed after validation.

The UI importer saved the first icon but the normal editor subsequently crashed during UI startup. The final five icons and model assets were imported using a Python commandlet with the project's UI texture settings, then validated in a separate process.

## Runtime capacity check

The previous runtime and load-time preservation limits capped inventory at 100. Both the required maximum and default setting are now 120. `TunaSweeper.Inventory.EquipmentData.LaserAndStartingLoadout` was extended to equip item 5011 while retaining a legacy setting of 100, place an item in slot 119, serialize and deserialize its UID and quantity in memory, and resolve 120 slots from the restored equipment. It failed before the cap change and passed afterward. The final UE 5.7 editor build and fresh-process asset reload also passed. Logs are in `TunaSweeper/Saved/BackpackWork/`.
