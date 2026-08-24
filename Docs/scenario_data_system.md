# Data-Driven Scenario System

## Purpose

`UTunaSweeperScenarioSubsystem` owns automatic dialogue scenario selection. Player code emits a runtime trigger and supplies the current level; it does not contain build-specific dialogue lines, speaker names, completion flags, or narrative conditions.

The subsystem loads exactly one complete scenario pack for the active build flavor:

- Demo definitions: `TunaSweeper/Content/Data/ScenarioDefinitions.json`
- Demo localized text: `TunaSweeper/Content/Data/ScenarioTextStrings.csv`
- Main definitions: active Main payload `Data/ScenarioDefinitions.json`
- Main localized text: active Main payload `Data/ScenarioTextStrings.csv`

Main files are staged only for Main packaging. Demo preparation removes the transient Main staging directory, so Main scenario text does not enter Demo packages.

## Runtime Flow

1. A caller emits a trigger such as `level_entered`, `quest_state_changed`, or `interaction.mole`.
2. The scenario subsystem filters the active pack by trigger and normalized level name.
3. It evaluates required completed flags, blocked completed flags, optional quest-state requirements, and one-shot completion.
4. Eligible definitions are considered by descending `priority`; the first valid definition is resolved into localized dialogue lines.
5. The player controller waits for `start_delay_seconds`, if any, and passes the resolved lines and completion flag to the common dialogue widget.
6. Finishing the dialogue stores the completion flag in the active save slot.

`interaction.mole` requests a forced replay. A forced replay ignores only the scenario's own one-shot completion check; level, required flags, blocked flags, and quest-state conditions still apply. If the completion flag was already stored, replay does not write it again.

## Definition Schema

```json
{
  "schema_version": 1,
  "build_flavor": "demo",
  "scenarios": [
    {
      "scenario_id": "scenario.demo.example",
      "triggers": ["level_entered", "interaction.mole"],
      "level_name": "BunkerMap",
      "required_completed_flags": [],
      "blocked_completed_flags": [],
      "required_quest_states": [
        { "quest_id": "quest.example", "state": "accepted" }
      ],
      "completion_flag": "dialogue.demo.example",
      "one_shot": true,
      "priority": 100,
      "start_delay_seconds": 0.25,
      "lines": [
        {
          "speaker_name_string_key": "scenario.demo.speaker.luna",
          "dialogue_text_string_key": "scenario.demo.example.line1",
          "use_camera_focus": false,
          "camera_focus_location": [0, 0, 0],
          "camera_blend_seconds": 0.75
        }
      ]
    }
  ]
}
```

Supported quest states are `available`, `accepted`, `reward_available`, and `reward_completed`. `completed` is accepted as an alias for `reward_completed`.

Every referenced speaker and dialogue key must exist in that flavor's `ScenarioTextStrings.csv`, whose header is `string_key,ko,en,ja`. Missing keys invalidate the resolved presentation instead of falling back to hardcoded narrative text.

## Initial Demo/Main Branch

Demo starts directly in `BunkerMap`. Its level-entry scenario has no opening requirement and completes `dialogue.demo.toilet_intro`.

Main new games route to `OpeningScenarioMap` from the Main manifest. The opening presentation requests bunker travel; successful bunker entry stores `scenario.opening.awakening`. The Main level-entry scenario requires that flag and completes `dialogue.main.bunker_intro`. Later Main starts with `scenario.opening.awakening` already stored route directly to `BunkerMap`, while `dialogue.main.bunker_intro` prevents the first dialogue from replaying automatically.

This keeps eligibility and consumption separate:

- `scenario.opening.awakening`: the Main opening route reached the bunker successfully.
- `dialogue.main.bunker_intro`: the Main first bunker dialogue was consumed.
- `dialogue.demo.toilet_intro`: the Demo first bunker dialogue was consumed.

## Authoring and Validation

Changing dialogue, ordering, flags, delay, priority, camera focus, or the supported conditions does not require recompiling C++.

Run the flavor-data checks after edits:

```powershell
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode PrepareDemo
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode PrepareMain
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode Clean
```

Do not leave `Content/Data/MainPayloadStaged` in the public worktree after validation.
