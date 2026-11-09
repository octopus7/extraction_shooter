# 32×32 global RGB icon editor

## Requested result and execution limits

An in-game interaction actor opens a 32×32 RGB pixel editor. Confirm saves the draft globally across save slots; Cancel/Escape closes and discards it. A saved Widget Blueprint owns the editable UI hierarchy. The user delegates concrete widget layout/asset work to a `gpt-6-astra` sub-agent with `medium` reasoning. The initial handoff deferred tests/commits; the user subsequently authorized local verification and a final task commit.

## Implementation choices

Use the existing UE 5.7 interaction routing and player-controller modal lifecycle. Use a separate GameInstance subsystem for file I/O, with no changes to slot serialization. A single icon is the initial scope. RGB means opaque RGB8, row-major from top left, interleaved R/G/B, exactly 3072 bytes with no header/compression/alpha. The default unsaved canvas is black. Store `GlobalIcon.rgb` directly in `TunaSweeperBuildFlavor::GetSaveGameDirectory()` so existing account/store/demo/full isolation remains intact. New game, slot selection and slot deletion must not reset this file.

Read into a draft when opening. Missing file is a normal new canvas. Invalid length/read failure leaves the file untouched and shows a localized warning. Confirm writes a sibling `.candidate`, closes/flushed data, and replaces the destination; failure leaves the modal/draft open and the existing saved file intact. Cancel, Escape, travel and teardown never save implicitly. No disk write per brush stroke.

The project already uses Steam Auto-Cloud; document an additional exact `GlobalIcon.rgb` rule for each app's existing account/flavor directory. Exclude `.candidate`. Steamworks publication and cross-machine synchronization remain external pending work; do not claim they were changed or tested.

## Layout delegated to Astra

Dark slate centered modal (nominal 1120×820 within a 1920×1080 design canvas), cool cyan accent, restrained borders, readable existing project font. Header with title, 32×32 RGB descriptor, and global-save description. Main body: left 576×576 editing canvas (18 screen units per pixel), right 452 px inspector (expanded from the initial approximate 300 px to give translated labels and RGB controls room). Inspector includes 128×128 nearest-neighbor preview, selected-color swatch, RGB integer controls (0–255), 12 preset swatches, brush/eyedropper controls and fill. Footer: status/help on left, Cancel then emphasized Confirm on right. Whole modal scales down uniformly for smaller screens while preserving square canvas. Labels use `ui.icon_editor.*` string keys in all six supported languages. Preserve localization-friendly widths and wrapping.

Left drag draws continuous single-pixel strokes; eyedropper samples an existing pixel (right click is also acceptable). Fill recolors the complete draft only. No transparency, undo history, image import/export picker or slot-specific icon bank in this scope.

All panels, controls and hierarchy are serialized in actual WBP assets; no runtime ConstructWidget/AddChild tree creation. A custom canvas widget may draw pixels/grid and handle pointer input, but must not create hierarchy. Widget sources live under `UI/Astra`, classes end `GPT6Astra`, source comments record actual model `gpt-6-astra` and reasoning `medium`, WBP names end `_GPT6Astra`. An editor-only one-off asset authoring commandlet may construct/save assets; remove it and its entry points after use. Asset-authoring compilation and the subsequently authorized focused PIE/automation tests are permitted.

## Integration interfaces

- `UIconStorageSubsystem` in `Public/IconEditor/IconStorageSubsystem.h`: `EIconLoadResult LoadRgbIcon(TArray<uint8>& OutPixels) const`, `bool TrySaveIcon(const TArray<uint8>& Pixels) const`, `FString GetIconFilePath() const`; enum values `Missing`, `Loaded`, `Invalid`, `ReadFailed`; constants `Width=32`, `Height=32`, `ByteCount=3072`. The method avoids the Win32 `LoadIcon` macro name.
- `UIconEditorGPT6Astra` in `Public/UI/Astra/IconEditorGPT6Astra.h`: public `bool InitializeEditor()` and dynamic multicast `OnCloseRequested` (no arguments). Successful Confirm and Cancel/Escape broadcast it. Controller owns removal/input restoration. Initialize runs after AddToViewport so BindWidget references exist.
- `/Game/UI/IconEditor/WBP_IconEditor_GPT6Astra.WBP_IconEditor_GPT6Astra_C` is the controller's default asset.
- `AIconEditorTerminal` in `Public/IconEditor/IconEditorTerminal.h`, `bool OpenFor(APawn*)`; optional placeable BP `/Game/Interaction/BP_IconEditorTerminal`. No map placement requested.
- `ATunaSweeperPlayerController::OpenIconEditorPanel(AIconEditorTerminal*)` and `CloseIconEditorPanel()` integrate modal blocking, input/focus and teardown.

## Alternatives considered

Runtime-built UMG hierarchy violates the editable-asset requirement. A Blueprint-only editor duplicates pixel/file logic and makes raw I/O harder to maintain. Chosen split: native behavior/storage and a saved, designer-editable WBP hierarchy. Separate Steam RemoteStorage uploads would duplicate the existing Auto-Cloud lifecycle, so retain its account path and add the narrow filename rule.

## Acceptance limits

Local build, saved hierarchy/bindings, native pointer input/scaled sampling, Confirm/Cancel/Escape and failure recovery, slot/new-game independence, controller restoration and six-language 1080p/720p renders were verified in the follow-up. See the plan and authoring guide for logs and coverage. Packaged/multiplayer/account-switch and Steam two-PC acceptance are not claimed; the user owns Steamworks rule registration/publication.
