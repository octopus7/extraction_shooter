# Quest food and prop item icons

Generated with the built-in image generation tool, native transparent backgrounds. No recoloring or alpha extraction was used. Source PNGs are the generated originals (1254 × 1254 RGBA).

Shared prompt: square game inventory item icon; polished hand-painted anime illustration, rounded silhouette, restrained warm dark brown outlines, broad cel-shaded color areas, little microtexture; isolated centered object, transparent background, no frame, text, watermark or external cast shadow.

- `T_UIIcon_SweetPotato.png`: one plump purple-red sweet potato diagonally positioned, a smaller cut piece showing golden yellow flesh in front.
- `T_UIIcon_DogFood.png`: one small sealed blue pet food pouch, with a simple friendly dog face pictogram and brown kibble illustrated near bottom, no lettering.
- `T_UIIcon_CatFood.png`: one small sealed coral-orange pet food pouch, with a simple friendly cat face pictogram and small fish pictogram, no lettering.
- `T_UIIcon_FermentedFuel.png`: one squat amber glass fuel bottle filled with golden brown fermented liquid, sturdy stopper and a small simple green leaf symbol label, no lettering.
- `T_UIIcon_ToadJar.png`: one clear ventilated glass jar with perforated metal lid, containing a tiny friendly olive-green live toad sitting inside, recognizable broad head and squat body.
- `T_UIIcon_RabbitBattery.png`: one chunky upright compact rectangular battery with two metal terminals, cream and teal casing, a very clear white rabbit face pictogram with long ears on front, no lettering.
- `T_UIIcon_ModelBoat.png`: one small handmade wooden model boat, warm brown hull, simple cream cabin and short mast, viewed from elevated three-quarter angle, no water.

Imported to `/Game/UI/Icons` through the existing TunaSweeper UI texture import path. UI texture group, EditorIcon compression, no mipmaps, sRGB enabled. Reload validation: `food_icons_validation.json` (7 textures). PNG alpha ranges were verified as 0–255.

For command-line imports, use `-ExecCmds=QUIT_EDITOR` instead of the early import-quit flag, which can exit before editor initialization finishes.
