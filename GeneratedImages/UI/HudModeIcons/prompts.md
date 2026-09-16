# HUD 컬러 아이콘

Built-in image generation; 3 columns × 2 rows. Inventory, quest, map / memo, research, empty.

## Processing and use

- Final solid-background source: `HudModeIcons_Source.png`, 1536×1024.
- Project alpha-extraction script: automatic border key `#7B6975`, TransparentDistance 18, OpaqueDistance 45. Matched black/white debug images and checker preview are retained.
- Full-resolution transparent sheet: `HudModeIcons_Transparent.png`.
- Runtime sheet: `HudModeIcons_Atlas.png`, 384×256, 128×128 per cell; high-quality bicubic resize of the alpha sheet for smoother 36px HUD images.
- UE UI texture: `/Game/UI/Icons/T_UI_Mode_ColorAtlas`; the HUD selects cells with UV regions and keeps the icon tint white to preserve generated colors.

## Generation prompt

Use case: stylized-concept. Asset type: one coherent game HUD icon sprite sheet for TunaSweeper, a cozy stylized extraction game with teal UI. Create a landscape 3-column by 2-row sheet (1536x1024 preferred), six perfectly equal square cells, no visible grid. Exactly FIVE colorful opaque icons: row 1 left a compact olive-green canvas backpack with warm tan straps (inventory); row 1 center a warm ivory quest parchment with a small orange-red exclamation-shaped emblem and brass seal (quests); row 1 right an unfolded cream map with aqua water, green land and one coral location pin (map). Row 2 left a small blue hardcover notebook with a yellow pencil (memos); row 2 center a simple lavender and brass laboratory microscope (research); row 2 right entirely empty background. Each icon centered in its cell, same visual footprint, occupies 68% of cell width and height, at least 16% empty margin all sides. Friendly clean hand-painted 3D illustration, simple solid shapes, restrained two-tone shading, softly rounded edges, clear recognizable silhouette at 32 pixels. Rich distinct colors, not monochrome, not white glyphs. No tiny detail, no glitter, no grain, no outlines around cells, no letters, no words, no numbers, no labels, no border or watermark. Perfectly flat uniform solid background #7A6B78 throughout the entire sheet, including margins and empty cell, no background shadows, gradients or texture. Do not use background color #7A6B78 anywhere inside icons. No cast shadows or glow outside objects. This background will be removed by a local alpha-extraction process; keep object edges crisp.

## Background cleanup edit

Edit this sprite sheet ONLY to replace the entire background and all surrounding glow with a perfectly uniform solid #7A6B78. Keep exactly the same five illustrated objects, their colors, positions, silhouettes, and 1536x1024 sheet layout. Remove EVERY blurred halo, vignette, atmospheric shading, cast shadow and spotlight outside the object silhouettes. All empty space including the whole lower-right sixth cell must be the exact same flat #7A6B78 color, edge to edge. No gradient anywhere on the background. Do not create new objects or add a backdrop behind individual icons. Background is a technical solid matte for alpha extraction, not an artistic scene.
