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

Pre-existing shared saves remain in `SaveGames/Demo` or `SaveGames/FullGame`. They are not automatically assigned to a Steam account because they may belong to another store or user. To transfer an existing save, close the game, preserve a backup, and explicitly copy the desired complete save set into the intended account/flavor directory before its next launch. Never copy a running game's `.candidate` files.

## Required Steamworks configuration

These settings must be saved and published in **each app's Steam Cloud settings**. Code changes alone cannot enable the service in Steamworks.

| Field | Demo (5158070) | Full game (5137900) |
| --- | --- | --- |
| Root | `WinAppDataLocal` | `WinAppDataLocal` |
| Subdirectory | `TunaSweeper/Saved/SaveGames/Steam/{64BitSteamID}/Demo` | `TunaSweeper/Saved/SaveGames/Steam/{64BitSteamID}/FullGame` |
| Pattern, rule 1 | `*.sav` | `*.sav` |
| Pattern, rule 2 (same root/subdirectory) | `*.sav.previous` | `*.sav.previous` |
| Pattern, rule 3 (same root/subdirectory) | `GlobalIcon.rgb` | `GlobalIcon.rgb` |
| OS Restriction | `[Cross-Platform]` | `[Cross-Platform]` |
| Recursive (rules 1 and 2) | Enabled | Enabled |
| Recursive (rule 3) | Disabled | Disabled |
| Shared cloud APP ID | `0` | `0` |

All three rules are necessary: active/settings/achievement saves and `Backups/*.sav` match rule 1; recovery generations match rule 2; the slot-independent 32×32 RGB icon matches rule 3. `GlobalIcon.rgb` is exactly 3072 raw RGB8 bytes and is saved only when the player confirms the icon editor. Its `GlobalIcon.rgb.<guid>.candidate` files must remain local. The third rule is a required Steamworks configuration addition; adding this documentation does not publish that rule or verify live synchronization. Candidate files, corrupt archives, and deletion audit logs remain local. Avoid a broad `*` rule or syncing the entire `Saved` directory, which also contains machine-specific settings and logs.

Set both byte and file quotas before enabling Auto-Cloud. An initial allocation of 256 MiB and 200 files per user/app allows room for the current 30 retained timestamped backups plus active/settings/achievement and previous-generation files; verify byte usage with representative long-play saves before release. The two app IDs remain separate so a demo never replaces full-game progress.

The Windows paths above target Windows builds, including the Windows build under Proton. Native Linux/macOS builds would require matching path resolution and Steam Root Overrides before claiming cross-platform support.

`[Cross-Platform]` is Valve's recommended restriction even for a currently Windows-only game. It avoids locking uploaded files to Windows; it does not itself implement native Linux/macOS save paths or certify compatibility.

## 운영 작업 안내: Auto-Cloud 적용

### 클라이언트 코드의 역할

Auto-Cloud에는 별도의 클라우드 업로드·다운로드 코드가 필요 없다. 게임은 로컬 저장·불러오기를 담당하고 Steam 클라이언트가 실행 전과 종료 후 파일을 동기화한다. 게임 실행 중 매 저장마다 즉시 업로드되는 방식은 아니다.

현재 프로젝트에는 계정별 경로를 선택하는 `TunaSweeperSteamCloud.cpp`와 이를 사용하는 `TunaSweeperBuildFlavor::GetSaveGameDirectory()`가 이미 있다. 이 코드는 유지한다. Auto-Cloud가 로컬 저장 코드나 계정별 파일 분리를 대신하는 것은 아니다. Steamworks 설정만으로 실제 동기화가 확인된 것으로 간주하지 않는다.

### Steamworks 담당자 작업 순서

1. 대상 앱의 Steam Cloud 설정을 열고 위 표의 Demo/Full game을 구분한다.
2. 사용자당 용량과 파일 수를 지정한다. 위 256 MiB(268435456 bytes)/200 files는 프로젝트의 초기 제안값이며 Steam의 필수값이 아니다. 실제 최대 세이브·백업 크기로 여유를 확인한다.
3. Auto-Cloud 경로 규칙을 세 개 등록한다. 위 표의 Root, Subdirectory, Pattern, OS Restriction, Recursive를 사용한다. 기존 두 규칙이 등록되어 있다면 `GlobalIcon.rgb` 규칙을 추가한다. `{64BitSteamID}`는 실제 계정 번호로 치환해서 입력하지 않고 변수 그대로 등록한다.
4. Steam 배포 빌드를 실행해 실제 생성 경로를 위 표와 대조한다. 런타임 로그의 `Steam Auto-Cloud save directory:`도 확인할 수 있다. `LocalOnly`로 저장되면 계정 초기화부터 확인한다.
5. 이미 출시된 앱은 `Enable cloud support for developers only`로 먼저 시험한다. 이 모드는 해당 게임의 Developer Comp 라이선스를 가진 계정을 대상으로 한다.
6. Save와 Publish를 완료한다. 설정 반영을 위해 최대 10분 기다리거나 Steam 클라이언트를 재시작한다.
7. 아래 Validation 절차로 두 PC 간 왕복 저장·복원, 슬롯 삭제, 계정 전환을 확인한다. 개발자 전용 시험이 끝나면 해당 제한을 해제하고 다시 게시한다.

### 적용 범위와 완료 기준

- Demo와 FullGame은 경로 및 앱별 클라우드를 분리한다. 이번 설정에서 Shared cloud APP ID는 `0`을 유지한다.
- 그래픽 설정·로그·임시 파일을 포함하도록 동기화 범위를 넓히지 않는다.
- Dynamic Cloud Sync는 별도 기능이다. 실행 중 외부 파일 변경에 대응하는 구현과 검증 없이 활성화하지 않는다.
- 완료 기록에는 앱 ID, 게시 여부, 두 PC의 저장·복원 결과와 미검증 항목을 남긴다. 이 안내 문서 작성 자체는 Steamworks 설정 변경이나 실서비스 시험 완료를 의미하지 않는다.

## Validation

Local automation checks the packaged-only/channel gate, account separation, Demo/FullGame separation and unavailable/invalid-identity fallback. The build-flavor path test verifies that full-game saves resolve directly to `FullGame`.

After publishing the Steamworks settings, run the Steam console command `testappcloudpaths <AppId>` and inspect `%Steam Install%/logs/cloud_log.txt`. Save and exit on PC A; wait for Steam synchronization; launch on PC B with the same account; verify slots, selected slot, achievements and recovery files. Test slot deletion and account switching as well. Cloud-disabled/offline sessions must retain local saves. End test overrides with `testappcloudpaths 0`.

For developer-only Auto-Cloud testing, open `steam://open/console`, run `testappcloudpaths <AppId>` and `set_spew_level 4 4` on each test PC. After testing, reset both with `testappcloudpaths 0` and `set_spew_level 0 0`. Also save new progress on PC B, exit and wait for synchronization, then verify it on PC A. Preserve copies of test saves before exercising divergent offline progress; synchronization does not merge gameplay state inside save files.

Live two-machine synchronization requires Steamworks configuration and a Steam-distributed build; local automation does not verify that service behavior.

Source: [Valve Steam Cloud documentation](https://partner.steamgames.com/doc/features/cloud?l=english).
