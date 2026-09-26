# Wardrobe UI verification

`Previews/` contains actual Slate renders of the wardrobe panel in Korean, English, and Japanese. The panel uses the existing native transparent 1024 × 1536 images in `SourceArt/Characters/LunaMk2/UIPreviews/Images/` through `/Game/UI/Wardrobe/T_UIOutfit_<ID>`.

The wardrobe now uses the full viewport, without a framed window, preview panel, or card backplates. A full-screen translucent scrim separates the controls from the game. The list and preview share available width, and the portrait grows into available height while preserving its 2:3 aspect.

The automation suite renders the actual HUD Canvas attachment at 960 × 540, 1280 × 720, 1920 × 1080, and 2560 × 1080 in Korean, English, and Japanese. It checks viewport coverage, the eighth card, selected preview, action bounds, locked/equipped states, unlock/language refresh, rejected apply feedback, and reopening after failure. `validation.json` records the original six-outfit run; `raincoat_validation.json` records the seven-outfit verification. No asset generator or startup regeneration is retained.

`Previews/Raincoat_Korean.png`, `Raincoat_English.png`, and `Raincoat_Japanese_Small.png` show the raincoat selection. The earlier `Wardrobe_*` images retain the initial six-outfit layout.

`scifi_suit_validation.json` records the eight-outfit full-screen result: 14 automation tests passed after generator cleanup, followed by a successful UI recheck after fixing localized name wrapping. `Previews/SciFiSuit_Fullscreen_Korean.png`, `SciFiSuit_Fullscreen_English_Ultrawide.png`, and `SciFiSuit_Fullscreen_Japanese_Small.png` show the final layout.

Run the editor automation prefix `TunaSweeper.Wardrobe+TunaSweeper.Outfits+TunaSweeper.Save`. Add `-WardrobeUIPreview` with an active rendering backend to produce fresh captures under `Saved/WardrobePreview/Panel_<language>_<width>x<height>_SciFiSuit.png`.
