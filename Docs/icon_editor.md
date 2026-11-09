# Global icon editor authoring

## Assets and placement

Place `AIconEditorTerminal` (or `/Game/Interaction/BP_IconEditorTerminal`) in the desired level. It inherits the project's usual interaction component and marker, using interaction type `IconEditor` (40) and localized `ui.icon_editor.title`. All instances open the same icon. The actor's default cube is a replaceable placeholder; set its mesh/transform in a derived Blueprint. This task does not place a terminal into an existing map.

The terminal's `EditorWidgetClass` defaults to `/Game/UI/IconEditor/WBP_IconEditor_GPT6Astra`. Edit this WBP in the UMG Designer to change the hierarchy, spacing, colors and controls. Preserve names/types used by `BindWidget` in `UI/Astra/IconEditorGPT6Astra.h`. Pixel drawing/input lives in the canvas class; layout and buttons are saved in WBP assets, with no runtime tree creation. Widget sources record the implementing model `gpt-6-astra` and reasoning effort `medium`.

The reusable canvas Blueprint is `/Game/UI/IconEditor/WBP_IconCanvas_GPT6Astra`; the main WBP contains editing and preview instances. The whole modal uses a uniform DownOnly ScaleBox, with a 1120×820 panel, 576×576 editing canvas and 128×128 preview. The temporary authoring commandlet was removed after saving these assets; ordinary editor startup does not regenerate them.

## Editing and persistence

The canvas is 32×32 opaque pixels. Drag with the brush to paint; sample with the eyedropper/right click, choose palette colors or enter RGB bytes (0–255), and use Fill for the whole draft. The preview uses the same draft. Confirm saves; Cancel/Escape discards. Closing because the owner/pawn disappears also discards. Standalone play pauses while the modal is open; network sessions only lock the local player's gameplay input.

The output is `<current account/flavor save directory>/GlobalIcon.rgb`, independent of slots: exactly 3072 bytes, RGB8, top-left row-major, no header/alpha. The initial canvas is black. Wrong-size/unreadable existing data is reported without being overwritten unless the player explicitly confirms a replacement. A write failure keeps the panel/draft available. See [persistence contract](save_persistence.md) for details.

## Cloud operations and task status

Add the exact filename `GlobalIcon.rgb` to both apps' existing Steam Auto-Cloud roots as specified in [cloud_saves.md](Steam/cloud_saves.md). Do not sync candidates. The user owns Steamworks registration/publication and live two-PC synchronization; those external steps have not been performed here.

The initial implementation-only instruction was superseded by authorization to finish verification and commit. On 2026-11-10, the UE 5.7 editor build passed after removing all one-off authoring code (Saved/Logs/Build/IconEditor-final.log). Both focused automation tests passed: TunaSweeper.IconEditor.Assets and TunaSweeper.IconEditor.IntegrationAndVisual (Saved/Logs/IconEditor-final-tests.log; report Saved/Automation/IconEditorReport/index.json).

The asset test checks all 39 required bindings, uniform scaling, terminal Blueprint and short button-label wrapping. The PIE test exercises the real terminal/controller/widget path, continuous native pointer strokes, scaled sampling, RGB/fill, exact Confirm bytes, Cancel/Escape, real Windows locked-file read/replacement failures and retry, candidate cleanup, new-game/slot-deletion independence, and input/pause restoration. All saves use a GUID directory under Saved/Automation/WaveDefense/IconEditor-*, never player saves. The fixture waits for the startup HUD's actual animated visibility to settle before opening a second modal.

Twelve captures cover all six languages at 1920×1080 and 1280×720 in Saved/IconEditor/Screenshots/icon_{0..5}_{1080,720}.png. Visual review found Japanese/Chinese button labels wrapping prematurely; the saved WBP now keeps these short labels on one line and fills the button content width. Final captures show the correction. Integration reported seven non-failing fixture/project warnings: four absent fresh-root achievement/cosmetic files, an existing duplicate ui.common.confirm key, DLSS PIE screen-percentage settings and the map's crowd-navigation warning. No packaged-build, multiplayer, Steam-account switching or live Steam sync acceptance is claimed.
