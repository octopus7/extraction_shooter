# Agent Instructions

- The Unreal Engine project is `TunaSweeper/TunaSweeper.uproject`.
- Treat TunaSweeper as an Unreal Engine 5.7 project.
- Version check: `TunaSweeper.uproject` has `"EngineAssociation": "5.7"`, and both `TunaSweeper.Target.cs` and `TunaSweeperEditor.Target.cs` use `EngineIncludeOrderVersion.Unreal5_7`.
- Prefer UE 5.7-compatible APIs and build settings when editing C++ or project configuration.
- Record user instructions in `Docs/requests.md` with the current timestamp and elapsed duration next to the timestamp. Do not include a timezone suffix in request log timestamps.
- Record user questions and their answers separately in `Docs/questions.md` with the current timestamp and elapsed duration next to the timestamp. Do not include a timezone suffix in question log timestamps. Do not duplicate questions in `Docs/requests.md`.
- For project-local transparent icon or icon-sheet generation, use `.codex/skills/icon-alpha-from-solid-bg`: generate one muted mid-value solid-background source image, derive matched black/white background images locally, then extract alpha from their difference.
