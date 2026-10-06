# Save editing tool design

Approved direction: the user requested implementation on 2026-10-06 using a .NET 10 console wrapper around a project-specific UE 5.7 commandlet. Human CLI documentation and agent-facing commandlet documentation are separate deliverables.

## Contract

- `TunaSweeperSaveTool` accepts a UTF-8 JSON request file and writes a JSON response file. Protocol version 1 has catalog, list, inspect, validate, add, preset-export, and preset-apply operations.
- Explicit save path, expected Demo/Main flavor, and slot are required for save operations. Reading never recovers or modifies files. No guessing of Steam/Stove identities.
- Edits default to preview; commit must be explicitly true. Work on a deserialized copy, validate all requested changes, retain a unique original backup, then use the existing fail-closed save writer. Reject corrupt/unsupported saves and stale expected hashes. No gameplay save schema changes.
- Add weapons, ammo, and armor using project item definitions and stack limits. Loaded ammo and attachments are optional; enforce compatibility. Update acquisition history without granting quest/achievement events.
- Versioned JSON equipment presets replace the entire equipment set. Unspecified slots become empty. Move previous equipment to inventory, preserve attachments and other save state, and reject insufficient capacity atomically. Export current equipment as a reusable preset. Ship no actual loadout preset.
- Inspect exposes the serialized save snapshot plus enriched inventory/equipment/storage and metadata; catalog exposes IDs, string keys, localized names and limits. JSON is the machine interface.
- CLI supplies help, readable output, JSON output, explicit engine/project configuration, and noninteractive commands. User-visible copy resolves keys from the existing UITextStrings.csv and ItemNameStrings.csv workflow.
- Direct commandlet use needs only request/response files and the editor build. Document protocol, examples, error codes, preview/commit, backups, and offline usage independently of the CLI.

## Validation and limits

Use synthetic saves in a dedicated Saved/Automation subdirectory. Cover real save serialization, stack splitting, capacity rollback, invalid IDs/ammo/presets, equipment replacement/export, unrelated-state preservation, CRC failures, expected-hash mismatch, and CLI invocation. Reuse shared runtime constants and data subsystem; do not initialize the gameplay GameInstance or run gameplay save migration. Require current version for editing; older supported versions remain inspectable. Stop game/PIE before editing; an external writer can overwrite results. Engine startup cost is accepted.

## Workspace

Implement in the existing feature branch with narrow staged paths; preserve all pre-existing unrelated modifications. The user's explicit implementation instruction is the execution authorization. Implementation, verification, documentation and task log form one commit.
