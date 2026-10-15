# Unreal Editor coordination

Read and follow these rules before checking, opening, reusing, or closing an editor for TunaSweeper. These are the detailed editor coordination rules linked from `AGENTS.md`.

## Identify the relevant editor

- Scope Unreal Editor detection and close/build coordination by executable path and engine version, not process name alone. Inspect the process executable path to distinguish UE 5.7 from UE 5.8; ignore UE 5.8 editors for this project. For UE 5.7, inspect the command line/project path when available and ignore editors opened for a different project. Only the editor for `D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject` is a confirmed match; do not treat an unknown version or project as a confirmed match or close it without identifying it. Apply this scope when checking whether an editor is open, reusing one, requesting closure, and verifying closure.

## Open and reuse

- When opening Unreal Editor for Computer Use, explicitly open `TunaSweeper/TunaSweeper.uproject` (absolute path: `D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject`). Reuse an existing editor for this project when available; do not launch Unreal Editor without specifying the project.
- After a build finishes and Unreal Editor can be launched, open the editor for `TunaSweeper/TunaSweeper.uproject` unless the user says not to or another conversation still needs it closed under the closure coordination rules below.
- When the user's entire message is exactly `빌드` or `build` (with no other text), do not add a request-log entry. Build the project, then open the Unreal Editor when it can be launched and no coordinated closed-editor work is pending.

## Coordinate closure and reopening

- Cross-conversation Unreal Editor coordination: when compilation, asset saving, or another task requires the shared project editor to close, first inspect the relevant conversations to identify which conversation opened or is using that editor. Ask that conversation to check its current work and, unless there is a concrete reason to keep the editor open, save the necessary changes and close it. The user authorizes sending these editor-coordination requests and replies between relevant project conversations without asking again for each message.
- The receiving conversation should check for active work and unsaved changes before closing the editor. If it must remain open, reply with the reason and when it can be released; otherwise close it safely and confirm. Do not force-terminate the editor or discard unsaved work. If the owner cannot be identified or saving/closing requires a user decision, ask the user with the specific reason.
- Wait for closure confirmation and verify that the relevant editor process has exited before starting work that needs it closed. Keep the editor closed while another conversation has pending work that requires closure; coordinate completion before reopening it so automatic post-build launches do not interrupt another task.
