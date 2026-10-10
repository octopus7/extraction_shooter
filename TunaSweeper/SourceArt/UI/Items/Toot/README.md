# Toot item icon

Generated with built-in imagegen using native transparency. No black/white background passes or alpha extraction.

- `Toot_generated.png`: unchanged generated original.
- `T_UIIcon_Toot.png`: 256x256 import source, resized with native alpha preserved.
- UE: `/Game/UI/Icons/T_UIIcon_Toot`.
- Consumer: item ID 6006 in ItemTable.json.

Prompt: Square inventory item icon. One charming comic puff of gas shaped like a small billowing cloud, slightly brown: light warm beige/tan with muted soft brown shaded lobes, not dark mud. Simple rounded cloud silhouette with 5-7 broad lobes and one tiny detached puff, clean readable cartoon game illustration, subtle dimensional shading, no microtexture or sparkles. Centered with comfortable margin, legible at 256x256. Represents a silly fart reward item. No face, no poop, no container, no text, no frame, no ground shadow. Truly transparent background, preserve native alpha.

Validation: source corner alpha is 0; UE fresh-process reload confirms 256x256, UI texture group, EditorIcon compression, alpha enabled, no mipmaps, item reference resolves. Importer saved the asset then exited with code 3 during editor shutdown; independent validation exited 0.
