# Print Maps

Run `node Tools/PrintMaps/generate_maps.mjs` from the repository root to create the three region sheets under `Docs/PrintMaps/`. The generator writes editable SVG files and A4 landscape PNGs at 300 dpi. It uses ImageMagick (`magick`) to rasterize SVG.

The sheets are coordinate maps, not game screenshots. World `+X` is up and `+Y` is right, matching `UTunaSweeperMapDefinition`. Marker coordinates in `print_maps.json` were curated from saved actor surveys and the current Elephant Lake placement record. Confirmed drops use fixed content rows whose drop chance is 100%; unknown or chance-based loot is excluded. Region 2 includes the surveyed loop-rail path.
