# Wardrobe UI verification

`Previews/` contains actual Slate renders of the wardrobe panel in Korean, English, and Japanese. The panel uses the existing native transparent 1024 × 1536 images in `SourceArt/Characters/LunaMk2/UIPreviews/Images/` through `/Game/UI/Wardrobe/T_UIOutfit_<ID>`.

Validation covers 1280 × 760 and 960 × 570 panel rendering, all seven cards, selected preview, locked/equipped states, unlock/language refresh, rejected apply feedback, and reopening after failure. The raincoat renders also show the seventh card fully scrolled into view and its 2:3 preview. `validation.json` records the original six-outfit run; `raincoat_validation.json` records the seven-outfit verification. No asset generator or startup regeneration is retained.

`Previews/Raincoat_Korean.png`, `Raincoat_English.png`, and `Raincoat_Japanese_Small.png` show the raincoat selection. The earlier `Wardrobe_*` images retain the initial six-outfit layout.

Run the editor automation prefix `TunaSweeper.Wardrobe+TunaSweeper.Outfits+TunaSweeper.Save`. Add `-WardrobeUIPreview` with an active rendering backend to produce fresh captures under `Saved/WardrobePreview/`.
