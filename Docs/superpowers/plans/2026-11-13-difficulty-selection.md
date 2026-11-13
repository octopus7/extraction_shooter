# Difficulty selection implementation plan

**Goal:** Apply the approved six-language cream/mint difficulty mockup with localized descriptions, white grayscale label sprites, selected-card shine/waves and drifting brown tuna silhouettes.
**Architecture:** An authored WBP_DifficultySelection child inside WBP_IntroMenu owns the complete editable tree. UDifficultySelectionWidget updates existing controls only; the intro controller retains gameplay and navigation. UI materials animate the selected card and mask text sprites.
**Tech stack:** UE 5.7 UMG, C++, UI materials, project CSV localization.
**Spec:** User-approved output/imagegen/difficulty-selection-v3 and this chat's implementation request.

## Constraints
- Purpose-only paths/classes (latest user correction); model gpt-6-astra / medium in a source comment only.
- No runtime tree construction, no commits. Existing demo notice and difficulty adjustment semantics remain.
- Korean story copy: 스토리를 즐기기 위해 퀘스트 아이템이 확정 드랍 됩니다.
- Description text uses existing localization keys. Label sprites map to those same keys and all six cultures.

## Tasks
- [x] Prepare cropped white-on-black grayscale label sprites from approved language images and native-transparent tuna sprites.
- [x] Add authored widget presentation and connect to existing selection handlers; update six-language story strings.
- [x] Generate/import/save WBP and materials once in the editor, validate assets, then remove one-off generator.
- [x] Build UE 5.7 and run asset/localization/navigation checks; render all languages and selection states, inspect animations; reopen editor and log work.

## Review focus
- Russian/Japanese labels and descriptions fit without clipping.
- Selected graphics follow all three stages; unselected cards have no wave/shine.
- New game, existing-save difficulty adjustment, demo notice and busy/disabled controls retain behavior.
- Six language changes refresh text and sprites together; asset references survive cooking.
- Materials stay inside rounded cards; background widgets never intercept input.

## Progress
- Confirmed runtime source currently constructs difficulty tree; replacing that path with authored child WBP.
- Confirmed actual session model from turn_context metadata; latest user naming instruction overrides original model directories.
- Editor process check found no running UnrealEditor; no shared-editor closure needed.

- Final UE5.7 build succeeded after removal of one-off generator. Fresh-process AuthoredAssets, VisualAndNavigation, DefinitionsAndLocalizedTextData all passed. Six cultures and three selected states rendered; selected-card pixel changes and drifting fish verified.
- Corrected nested-template empty property overrides, UE widget variable collision, HLSL reserved keyword, and render fixture font/material warm-up. No commits.
- Story guaranteed-drop wording is a UI/localization change; gameplay drop logic was not modified.
