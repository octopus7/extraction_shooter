# Wardrobe UI verification

`Previews/` contains actual Slate renders of the wardrobe panel in Korean, English, and Japanese. The panel uses the existing native transparent 1024 × 1536 images in `SourceArt/Characters/LunaMk2/UIPreviews/Images/` through `/Game/UI/Wardrobe/T_UIOutfit_<ID>`.

Validation covers 1280 × 760 and 960 × 570 panel rendering, all six cards, selected preview, locked/equipped states, unlock/language refresh, rejected apply feedback, and reopening after failure. `validation.json` records the final 14-test wardrobe/outfit/save run. No asset generator or startup regeneration is retained.

Run the editor automation prefix `TunaSweeper.Wardrobe+TunaSweeper.Outfits+TunaSweeper.Save`. Add `-WardrobeUIPreview` with an active rendering backend to produce fresh captures under `Saved/WardrobePreview/`.
