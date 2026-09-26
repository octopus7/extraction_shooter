# Melee inventory icons

Generated with the built-in image_gen tool. Final game icons are 256x256 RGBA PNGs in `../../Split/`. The existing material Crowbar icon is preserved; the new weapon uses `T_UIIcon_MeleeCrowbar.png`.

Each item has one generated solid-background source. The project `extract-alpha-from-solid-bg.ps1` script derived the matched black/white pair and difference alpha with `-OutputSize 256 -OpaqueDistance 45` and automatic border key detection. Checker images were visually inspected.

## WoodenClub prompt

Use case: stylized-concept. Create one square 1024x1024 inventory item icon for a survival game. A basic wooden club, single solid piece of medium warm brown hardwood, thick slightly irregular blunt head tapering into a narrower carved wooden handle, understated broad wood grain, mildly worn. Clearly a primitive wooden bludgeon, NOT a baseball bat: no baseball bat knob, no sporting proportions, no spikes, no straps. Realistic painterly 3D game inventory illustration with crisp detailed edges, clean controlled highlights, minimal tiny texture noise. Entire object isolated centered diagonally handle bottom-left and heavy head top-right, occupies 80% of canvas with generous padding. Soft upper-left light, no cast shadow. Use a perfectly flat solid background color #7A6B78 across the entire image. Background uniform with no checkerboard, texture, gradient, shadow, reflection, border, text, watermark. Do not use #7A6B78 anywhere inside the item. Keep item fully opaque and cleanly separated from background.

## MeleeCrowbar prompt

Use case: stylized-concept. Create one square 1024x1024 inventory item icon for a survival game. A single forged steel crowbar with a clearly hooked curved upper end and flattened prying tip, long strong hexagonal shaft, small flattened wedge at lower end. Dark charcoal steel with broad silver bevel highlights and restrained wear. No wood, no rubber grip, no extra items. Realistic painterly 3D game inventory illustration with crisp detailed edges, clean controlled highlights, minimal tiny texture noise. Entire object isolated centered diagonally straight lower end bottom-left and hooked end top-right; curved hook silhouette clearly readable, occupies 80% of canvas with generous padding. Soft upper-left light, no cast shadow. Use a perfectly flat solid background color #7A6B78 across the entire image. Background uniform with no checkerboard, texture, gradient, shadow, reflection, border, text, watermark. Do not use #7A6B78 anywhere inside the item. Keep item fully opaque and cleanly separated from background.
