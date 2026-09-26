# Steam Cloud saves

TunaSweeper uses Steam Auto-Cloud. The Steam client synchronizes files before launch and after exit; the game continues to use its CRC-verified local save transactions. No remote-storage upload loop or Dynamic Cloud Sync is enabled.

## Runtime directories

Only a packaged build whose `TunaSweeper.Distribution/DistributionChannel` is `Steam` selects the Steam directory. The account comes from the Steam OnlineSubsystem identity (64-bit Steam ID), including Steam offline mode when the SDK still exposes the account. The selected directory is fixed for the process lifetime.

Windows packaged paths:

```text
%LOCALAPPDATA%/TunaSweeper/Saved/SaveGames/Steam/<64BitSteamID>/Demo/
%LOCALAPPDATA%/TunaSweeper/Saved/SaveGames/Steam/<64BitSteamID>/FullGame/
```

Slots, last-selected-slot settings, achievements, previous generations and timestamped backups all use this directory. Editor sessions and other stores do not write into Steam account directories. If Steam identity is unavailable, the game uses `SaveGames/Steam/LocalOnly/<Demo|FullGame>/` and logs a warning. That fallback is deliberately excluded from Auto-Cloud and is not automatically imported into another account. Restart through Steam to retry identity initialization.

Pre-existing shared saves remain in `SaveGames/Demo` or `SaveGames/FullGame`; legacy `Main` is renamed to `FullGame` when the local full-game directory is resolved. They are not automatically assigned to a Steam account because they may belong to another store or user. To transfer an existing save, close the game, preserve a backup, and explicitly copy the desired complete save set into the intended account/flavor directory before its next launch. Never copy a running game's `.candidate` files.

## Required Steamworks configuration

These settings must be saved and published in **each app's Steam Cloud settings**. Code changes alone cannot enable the service in Steamworks.

| Field | Demo (5158070) | Full game (5137900) |
| --- | --- | --- |
| Root | `WinAppDataLocal` | `WinAppDataLocal` |
| Subdirectory | `TunaSweeper/Saved/SaveGames/Steam/{64BitSteamID}/Demo` | `TunaSweeper/Saved/SaveGames/Steam/{64BitSteamID}/FullGame` |
| Pattern, rule 1 | `*.sav` | `*.sav` |
| Pattern, rule 2 (same root/subdirectory) | `*.sav.previous` | `*.sav.previous` |
| OS | Windows | Windows |
| Recursive (both rules) | Enabled | Enabled |
| Shared cloud APP ID | `0` | `0` |

Both rules are necessary: active/settings/achievement saves and `Backups/*.sav` match rule 1; recovery generations match rule 2. Candidate files, corrupt archives, and deletion audit logs remain local. Avoid a broad `*` rule or syncing the entire `Saved` directory, which also contains machine-specific settings and logs.

Set both byte and file quotas before enabling Auto-Cloud. An initial allocation of 256 MiB and 200 files per user/app allows room for the current 30 retained timestamped backups plus active/settings/achievement and previous-generation files; verify byte usage with representative long-play saves before release. The two app IDs remain separate so a demo never replaces full-game progress.

The Windows paths above target Windows builds, including the Windows build under Proton. Native Linux/macOS builds would require matching path resolution and Steam Root Overrides before claiming cross-platform support.

## Validation

Local automation checks the packaged-only/channel gate, account separation, Demo/FullGame separation and unavailable/invalid-identity fallback. FullGame migration tests cover preservation of save/recovery files, idempotence, and an existing destination.

After publishing the Steamworks settings, run the Steam console command `testappcloudpaths <AppId>` and inspect `%Steam Install%/logs/cloud_log.txt`. Save and exit on PC A; wait for Steam synchronization; launch on PC B with the same account; verify slots, selected slot, achievements and recovery files. Test slot deletion and account switching as well. Cloud-disabled/offline sessions must retain local saves. End test overrides with `testappcloudpaths 0`.

Live two-machine synchronization requires Steamworks configuration and a Steam-distributed build; local automation does not verify that service behavior.

Source: [Valve Steam Cloud documentation](https://partner.steamgames.com/doc/features/cloud?l=english).
