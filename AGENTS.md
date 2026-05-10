# Agent Instructions

- The Unreal Engine project is `TunaSweeper/TunaSweeper.uproject`.
- Treat TunaSweeper as an Unreal Engine 5.7 project.
- Version check: `TunaSweeper.uproject` has `"EngineAssociation": "5.7"`, and both `TunaSweeper.Target.cs` and `TunaSweeperEditor.Target.cs` use `EngineIncludeOrderVersion.Unreal5_7`.
- Prefer UE 5.7-compatible APIs and build settings when editing C++ or project configuration.
- Record user instructions in `Docs/requests.md` with the current timestamp.
