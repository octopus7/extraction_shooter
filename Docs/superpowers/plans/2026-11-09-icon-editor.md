# Global RGB Icon Editor Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development for the explicitly delegated widget task. Initial user override was implement and wait without tests/commits; follow-up authorization permits verification and the final task commit. Root implements storage/integration; Astra medium implements actual widget layout/assets.

**Goal:** A placeable actor opens an editable 32×32 RGB icon UI with global raw-file Confirm/Cancel persistence.

**Architecture:** Native GameInstance subsystem stores one account/flavor-global RGB file. Existing interaction routing and controller own modal lifecycle. Astra authors native widget behavior and saved WBP layout.

**Tech Stack:** Unreal Engine 5.7, C++, UMG Widget Blueprint, existing six-language CSV strings, Steam Auto-Cloud.

**Spec:** `Docs/superpowers/specs/2026-11-09-icon-editor-design.md`

## Global Constraints

- Preserve other conversations' pending changes. Follow-up authorization permits focused tests/PIE and the root-owned final task commit.
- Native widget classes never create widget trees; actual WBP assets must contain them.
- Widget sources: `UI/Astra`; model `gpt-6-astra`, reasoning `medium`; class suffix `GPT6Astra`; WBP suffix `_GPT6Astra`.
- User-facing text uses existing localized keys only. No project names/reasoning in widget class names.
- Read `Docs/unreal_editor_coordination.md` before editor operations; UE 5.7 only.

## Review Focus

- Cancel/Escape/travel leaves prior on-disk RGB bytes unchanged.
- I/O failure preserves prior file and keeps draft available.
- Invalid file length/read failure is reported without automatic overwrite.
- Slot changes/deletion cannot reset global icon; account/flavor separation persists.
- Modal blocks gameplay input, restores previous pause state, and canvas stays square while scaled.

### Task 1: Storage and gameplay integration (root)

**Files:** `Source/TunaSweeper/{Public,Private}/IconEditor/IconStorageSubsystem.*`, `IconEditorTerminal.*`; interaction enum/subsystem; player-controller header and modal/input implementation; `Docs/save_persistence.md`, `Docs/Steam/cloud_saves.md`.

**Interfaces:** Consume existing `GetSaveGameDirectory()` and the widget interface in the spec. Produce `UIconStorageSubsystem`, `AIconEditorTerminal`, controller Open/Close APIs.

- [x] Implement strict raw RGB load and staged replacement save, with no slot/savegame coupling.
- [x] Add placeable interaction actor and route interaction type to controller.
- [x] Integrate local-player modal gating, input restoration, Escape and teardown.
- [x] Document file format/path and required Steamworks rule.

### Task 2: Widget behavior and saved layout (Astra medium sub-agent)

**Files:** `Source/TunaSweeper/{Public,Private}/UI/Astra/Icon*.{h,cpp}`; `Content/UI/IconEditor/WBP_*_GPT6Astra.uasset`; optional `Content/Interaction/BP_IconEditorTerminal.uasset`; additive `Content/Data/UITextStrings.csv` entries. Temporary editor authoring code is removed after asset save.

**Interfaces:** Consume `UIconStorageSubsystem`; produce `UIconEditorGPT6Astra::InitializeEditor()`, `OnCloseRequested`, the WBP path in the spec.

- [x] Implement draft painting, nearest-neighbor preview, RGB controls, palette, eyedropper, fill, confirm/cancel and localized statuses.
- [x] Author concrete hierarchy and styles from the spec into real WBP assets.
- [x] Coordinate native compile/asset-authoring and later authorized test runs with root.
- [x] Remove one-off generator code after assets are saved; report exact assets and deferred checks.

### Task 3: Handoff (root)

- [x] Reconcile interfaces and record implementation state without executing tests.
- [x] Update this checklist and append only this task's request log with actual elapsed time.
- [x] Prepare asset handoff, no-test/no-commit status and external Cloud setting remaining; wait for user.

## Execution notes

- Root owns storage, interaction and controller wiring. `/root/icon_widget_astra` (`gpt-6-astra`, `medium`) owns widget code, six-language labels and real WBP authoring.
- Existing unrelated work is present in the shared checkout. Coordinate UE 5.7 compile/commandlet work with the active plains wave-defense conversation; do not stage/commit either task here.
- Runtime native objects compiled/linked during the shared authoring build; a temporary commandlet TObjectPtr deduction error was corrected before the next authoring build. No icon-editor test has run.
- API naming ruling: `LoadRgbIcon` avoids Windows' unrelated `LoadIcon` macro. Asset-authoring inspector width is 452 px for localization space; spec updated.
- Authoring build: `Saved/Logs/Build/WaveDefense-visual07.log` succeeded. The shared checkout required a module-qualified commandlet name: `-run=TunaSweeperEditor.IconEditorAuthor`. `Saved/Logs/IconEditor-Author-20261110-module.log` records result 0 and the three asset saves. Initial blank-factory BindWidget warnings preceded populated Blueprint compilation/save; no runtime/visual acceptance is claimed.
- Saved `WBP_IconCanvas_GPT6Astra` (22636 bytes), `WBP_IconEditor_GPT6Astra` (227626 bytes), `BP_IconEditorTerminal` (24512 bytes). Removed both temporary authoring commandlet sources. No startup hooks or generator-only dependencies were added.
- No tests, PIE, icon-render acceptance, package, or Steam synchronization execution. Cleanup rebuild is deferred to the next coordinated shared build. No commit/staging was performed. Steamworks exact filename rule still requires external publication.

## Authorized verification follow-up (2026-11-10)

- [x] Cleanup editor build succeeded: `Saved/Logs/Build/IconEditor-final.log`.
- [x] Both `TunaSweeper.IconEditor.Assets` and `TunaSweeper.IconEditor.IntegrationAndVisual` passed, zero failures/errors: `Saved/Logs/IconEditor-final-tests.log`, report `Saved/Automation/IconEditorReport/index.json`. Integration has seven non-failing existing/isolated-fixture warnings documented in `Docs/icon_editor.md`.
- [x] Real PIE terminal/controller modal, pointer interpolation, scaled eyedropper, RGB/fill, exact raw Confirm, Cancel/Escape, OS-locked-file read/replacement failure with preserved original and retry, slot/new-game independence and input/pause restoration covered using an isolated GUID save root.
- [x] Twelve six-language 1080p/720p renders saved in `Saved/IconEditor/Screenshots/`. Visual review caught premature Japanese/Chinese button wrapping; fixed the saved WBP, removed the one-off fix commandlet, rebuilt and reran both tests successfully.
- [x] Initial test fixture failure traced to animated startup HUD visibility; fixture now waits on the real closed condition rather than bypassing modal guards.
- Steamworks registration/publication and two-PC synchronization are user-owned; package/multiplayer/account-switch acceptance is not claimed. Root owns final task logging and commit. Earlier no-test execution notes above describe only the initial handoff.
