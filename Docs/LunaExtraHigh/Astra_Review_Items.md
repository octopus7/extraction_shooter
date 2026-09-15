# Astra Review Items

## Scope

The demo ending dialogue and the STOVE review text must use the actual runtime data in the project. Design notes and older SSOT documents are not runtime sources for this change.

## Runtime data flow

1. `ATunaSweeperDemoEndingActor::ResumePendingEnding()` waits for the demo final quest `demo_q4_todays_reward` to reach `reward_completed`.
2. `StartEnding()` asks `UTunaSweeperScenarioSubsystem` to resolve the trigger `demo.ending.dinner` for `BunkerMap`.
3. The existing scenario loader reads the ending definition from `TunaSweeper/Content/Data/ScenarioDefinitions.json` and resolves all speaker and dialogue keys through `TunaSweeper/Content/Data/ScenarioTextStrings.csv`.
4. The resolved eight-line presentation is passed to `UTunaSweeperDialogueWidget`; no dialogue text is serialized on the ending Blueprint.
5. When the dialogue finishes, the scenario completion flag `dialogue.demo.ending.dinner` is saved. The existing farewell flow then marks `demo.ending.farewell_seen`, removes the completed demo save, and returns to the title.

The existing `ScenarioDefinitions` schema supports the required quest-state condition and line array, so no separate ending JSON file was added.

## Localization

The farewell card strings are system UI strings in `TunaSweeper/Content/Data/UITextStrings.csv`:

- `ui.demo_ending.farewell_title`
- `ui.demo_ending.return_to_title`

`TunaSweeperDemoFarewellWidget.cpp` resolves these keys through `TunaSweeperUiText::ResolveUiText` with an empty fallback. The Korean farewell phrases are not hardcoded in C++.

## Blueprint boundary

`TunaSweeper/Content/Interaction/DemoEnding/BP_DemoDinnerEnding.uasset` retains presentation-only values such as the illustration, camera, actor positions, fade timing, and BGM fade timing. The removed serialized `DinnerDialogue` property was cleared by resaving the Blueprint after the C++ property was removed.

## Review files

- Runtime scenario definition: `TunaSweeper/Content/Data/ScenarioDefinitions.json`
- Scenario localization: `TunaSweeper/Content/Data/ScenarioTextStrings.csv`
- System UI localization: `TunaSweeper/Content/Data/UITextStrings.csv`
- Ending runtime flow: `TunaSweeper/Source/TunaSweeper/Private/Scenario/TunaSweeperDemoEndingActor.cpp`
- Farewell UI: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperDemoFarewellWidget.cpp`
- Regression test: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperDemoEndingTests.cpp`
- STOVE review output: `Docs/Stove/TunaSweeper_Stove_Demo_Dialogue_Quest_Text.txt`

## Astra review checklist

- Confirm the ending is gated by `demo_q4_todays_reward = reward_completed` and `BunkerMap`.
- Confirm the eight lines alternate Luna/Mole and every line key exists in `ScenarioTextStrings.csv` for Korean, English, and Japanese.
- Confirm `one_shot` and `dialogue.demo.ending.dinner` prevent replay after the dialogue is completed.
- Confirm the farewell widget contains only string keys and no hardcoded Korean user-facing text.
- Confirm the Blueprint has no serialized `DinnerDialogue` property and only owns presentation configuration.
- Confirm the focused automation test `TunaSweeper.DemoEnding.AssetsAndInput` covers the data keys, Blueprint boundary, and farewell input behavior.
