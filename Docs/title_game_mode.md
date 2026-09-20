# Title GameMode

`/Game/Core/BP_TitleGameMode` derives from `TunaSweeperGameMode` and retains
`TunaSweeperPlayerController` for the existing title menu and camera flow.
Its Default Pawn Class and Spectator Class are None.

`Config/DefaultEngine.ini` selects this mode with the `IntroMap` map prefix.
IntroMap has no World Settings GameMode override. Do not add a conflicting
override: UE gives the map override precedence over the prefix setting.
UE removes the PIE prefix before matching. Other gameplay maps continue using
their existing mode and pawn. `/Game/Core` is already included in always-cook
directories, so this config-referenced Blueprint is included in cooking.

This avoids resaving IntroMap and mixing its independent title-animation edits
into the GameMode change. No gameplay C++ changes or UI string additions are needed.

## Regression check

Run `Tools/TitleMotions/verify_title_game_mode.py` with a separate UE 5.7 editor's
`-ExecutePythonScript` option and the TunaSweeper project. The script runs real
PIE without saving assets, then exits that editor. It checks zero pawns, an
unpossessed local controller, the title presentation camera, a title menu in the
viewport, and the gameplay GameMode's retained default pawn.

Result: `TunaSweeper/Saved/Automation/TitleGameMode.json` (`status: passed`).
This is an editor PIE check, not a packaged-build acceptance test.
