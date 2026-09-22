# EOS Steam Key Coop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Add a UE 5.7 two-player, serverless matchmaking foundation where Steam users authenticate through EOS, the host publishes an eight-digit code, and a helper joins the host listen server by entering that code.

**Architecture:** A GameInstance subsystem owns EOS/UE online session state and exposes Blueprint-callable host/join/leave operations. A platform-auth adapter isolates Steam ticket acquisition and EOS Connect login. UE OnlineSubsystemEOS and OnlineSubsystemUtils provide session search, join, and resolved travel; the host remains the listen server.

**Tech Stack:** Unreal Engine 5.7 C++, OnlineSubsystem, OnlineSubsystemUtils, OnlineSubsystemEOS, OnlineSubsystemSteam, EOS Connect, Steam Web API ticket flow, Unreal Automation Tests.

**Spec:** docs/superpowers/specs/2026-09-22-eos-steam-key-coop-design.md

## Global Constraints

- Support host 1 + helper 1 only; advertise exactly two public connections.
- Use UE 5.7-compatible APIs and keep DefaultPlatformService=Steam as the existing platform default.
- Never store Steam tickets, EOS tokens, client secrets, or raw credentials in source, logs, lobby attributes, or save data.
- Use UTunaSweeperTextSubsystem keys for all user-facing labels and errors.
- EOS failure must leave offline single-player available.
- Keep implementation, cleanup, verification, and task logging in one final task commit.

## Review Focus

- Eight-digit values, including leading zeroes, must round-trip through generation, validation, and session search.
- Late or duplicate online callbacks must not complete an older operation or fire completion twice.
- A matching code with a full room or incompatible protocol must be rejected before travel.
- EOS/Steam unavailable or unauthenticated must disable only online coop.
- Missing or invalid EOS configuration must fail without exposing credential values.

---

### Task 1: Online configuration and module boundaries

**Files:**
- Modify TunaSweeper/TunaSweeper.uproject
- Modify TunaSweeper/Source/TunaSweeper/TunaSweeper.Build.cs
- Modify TunaSweeper/Config/DefaultEngine.ini and custom NoStore configs as needed
- Create Docs/eos_steam_coop_setup.md

**Interfaces:**
- Add OnlineSubsystemEOS plugin and OnlineSubsystemUtils dependency.
- Document EOS ProductId, SandboxId, DeploymentId, ClientId, and ClientSecret injection without tracked secrets.

- [ ] Add plugin/module declarations while retaining Steam as the default platform.
- [ ] Add empty/default-safe EOS config sections and document environment-specific injection.
- [ ] Document Steam Web API ticket provider registration, EOS Connect mapping, sandbox separation, and two-process launch.
- [ ] Verify with rg that required declarations exist and no credential literal is tracked.
- [ ] Commit: git add ... && git commit -m "build: add EOS coop configuration boundary"

### Task 2: Pure code/session contracts and tests

**Files:**
- Create TunaSweeper/Source/TunaSweeper/Public/Online/TunaSweeperOnlineCoopTypes.h
- Create TunaSweeper/Source/TunaSweeper/Private/Online/TunaSweeperOnlineCoopTypes.cpp
- Create TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperOnlineCoopTests.cpp

**Interfaces:**
- State enum: Offline, Initializing, Authenticating, Ready, Creating, Hosting, Searching, Joining, Connected, Failed.
- Error enum: None, SubsystemUnavailable, AuthenticationFailed, InvalidInviteCode, SessionCreateFailed, SessionSearchFailed, CodeNotFound, RoomFull, VersionMismatch, JoinFailed, ResolveConnectStringFailed, TravelFailed.
- Settings struct: InviteCodeLength=8, MaxPlayers=2, ProtocolVersion, SessionName.
- Pure helpers: GenerateInviteCode, IsValidInviteCode, NormalizeInviteCode, and a testable session metadata matcher.

- [ ] Write failing automation tests for leading zeroes, invalid lengths/chars, generated range, two-player session settings, exact code match, protocol mismatch, full-room rejection, and duplicate completion guard.
- [ ] Run the focused editor test filter and confirm failure before implementation.
- [ ] Implement fixed eight-character formatting and stable FName metadata keys; never log raw codes.
- [ ] Run focused tests and expect PASS.
- [ ] Commit: git add online type/test files && git commit -m "test: define coop code and session contracts"

### Task 3: Steam-to-EOS authentication adapter

**Files:**
- Create TunaSweeper/Source/TunaSweeper/Public/Online/TunaSweeperPlatformAuthAdapter.h
- Create TunaSweeper/Source/TunaSweeper/Private/Online/TunaSweeperPlatformAuthAdapter.cpp
- Modify TunaSweeper/Source/TunaSweeper/TunaSweeper.Build.cs for conditional EOS/Steam seams.

**Interfaces:**
- BeginLogin(callback), IsLoggedIn(), GetProductUserId(), Logout()
- Callback returns success and ETunaSweeperOnlineCoopError.

- [ ] Add fake-ticket/fake-connect seams for editor tests without live credentials.
- [ ] Acquire the Steam Web API session ticket under Win64/Steam guards and pass the actual byte count to EOS.
- [ ] Call EOS Connect login, handle continuance/invalid-user results explicitly, and only mark Ready with a valid ProductUserId.
- [ ] Return safe subsystem/auth errors without blocking offline startup.
- [ ] Run adapter tests and a compile-only build; commit feat: add Steam EOS coop auth adapter.

### Task 4: GameInstance online coop subsystem

**Files:**
- Create TunaSweeper/Source/TunaSweeper/Public/Online/TunaSweeperOnlineCoopSubsystem.h
- Create TunaSweeper/Source/TunaSweeper/Private/Online/TunaSweeperOnlineCoopSubsystem.cpp

**Interfaces:**
- Blueprint-callable InitializeOnlineCoop, CreateHostSession, JoinSessionByInviteCode, LeaveSession.
- Blueprint-pure GetOnlineCoopState and GetCurrentInviteCode.
- Dynamic delegates for state/error and invite-code-ready.

- [ ] Add subsystem tests for operation generations, one-shot completion, cleanup on failure, and code invalidation.
- [ ] Resolve configured platform/auth/session interfaces without breaking GameInstance startup.
- [ ] Generate a code and create an advertised two-player session with join-in-progress disabled and code/protocol metadata.
- [ ] Normalize/validate helper input, search, exact-filter, reject full/version-mismatch results, resolve connect string, then ClientTravel.
- [ ] Clear delegates and destroy the session on leave; ignore stale/duplicate callbacks.
- [ ] Run focused tests and runtime compile; commit feat: add EOS code based coop subsystem.

### Task 5: Localized UI integration and manual checklist

**Files:**
- Modify TunaSweeper/Content/Data/UITextStrings.csv
- Modify existing title/intro menu UI files or create a focused online coop widget pair
- Create Docs/eos_steam_coop_manual_test.md

**Interfaces:**
- Bind host/join/status controls to subsystem delegates.
- Add keys ui.coop.host, ui.coop.join_code, ui.coop.join, ui.coop.status.*, and ui.coop.error.*.
- Format the code with a localized pattern key and the subsystem-provided code.

- [ ] Add Korean/English/Japanese rows for host, join, code label, waiting, connected, invalid, not found, full, version mismatch, service unavailable, and auth failure.
- [ ] Bind controls without hardcoded labels; disable online controls only when online services are unavailable.
- [ ] Document credentials, two-process launch, success/failure cases, and cleanup.
- [ ] Run localization/editor tests and uniqueness checks.
- [ ] Commit feat: expose localized coop host and join flow.

### Task 6: Build, verification, and single task commit

**Files:**
- Modify Docs/requests.md
- Modify Docs/save_persistence.md only if persisted state is added
- Remove any temporary generator or runtime test entry points

- [ ] Run focused automation, localization checks, and rg checks for hardcoded copy or credential literals.
- [ ] Build the UE 5.7 editor/game target and record the exact result; report missing EOS binaries instead of claiming success.
- [ ] Run the two-client EOS/Steam manual validation when credentials exist; otherwise validate offline fallback and mark live validation pending.
- [ ] Review the final diff for secrets, tickets, tokens, generated artifacts, and preserved offline behavior.
- [ ] Append the required Korean timestamp/elapsed-time request entry to Docs/requests.md.
- [ ] Commit all implementation, cleanup, verification, and logging together in one final task commit, then report worktree, commits, and limitations.

