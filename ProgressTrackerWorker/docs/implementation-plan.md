# Progress Tracker Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Worker and Unreal implementation run in parallel as explicitly requested by the user; the root agent integrates and commits once.

**Goal:** Build an independently movable Workers/D1 dashboard and the Unreal demo progress sender using one event contract.
**Architecture:** A dependency-light module Worker validates and stores events in D1; a static dashboard reads authenticated aggregates. Unreal emits best-effort events without retries, pending queues or error UI.
**Tech Stack:** JavaScript modules, Cloudflare Workers/D1, Node test runner + node:sqlite, Unreal Engine 5.7 C++.
**Spec:** [design.md](design.md)

## Global Constraints

- Worker files live only under ProgressTrackerWorker; Unreal files remain in TunaSweeper and Docs/save_persistence.md.
- No local Wrangler commands, Cloudflare mutations, GitHub creation, push or deployment.
- Cloudflare GitHub Builds may run its normal Wrangler deploy command with server-managed credentials.
- All UI text resolves through string keys. No player-supplied narrative, names or Steam IDs.
- No retry queue, error popup, inferred milestone record or rejection based on missing predecessors.
- One task commit includes implementation, tests, documentation and logs. No subagent commits.
- No access to ProductionPayload and no dependencies on QuestStudio.

## Review Focus

- Save loading must not manufacture new quest completion events; test hook location and attempted checkpoint persistence.
- A later milestone with missing earlier records must remain counted; test both endpoint acceptance and SQL aggregates.
- Cross-run and cross-build records must not repair one another's missing prerequisites; test composite grouping.
- Duplicate event ID with different contents must not silently replace data; test exact duplicate versus conflict.
- Unauthenticated API calls, oversized streaming bodies and hostile strings must fail safely; test bounded reads and safe DOM writes.

## Task 1: Event persistence and input contract (root)

**Files:** src/worker.js, src/events.js, src/database.js, migrations/0001_initial.sql, test/events.test.js, test/support/database.js, package.json, package-lock.json, .gitignore.
**Consumes:** POST /api/events JSON fields in design.md.
**Produces:** validateEvent(input), readEvent(request), storeEvent(db,event); API with 201 new, 200 exact duplicate, 409 ID conflict, 400 bad input, 413 oversized body, 503 DB unavailable.

- [x] Write endpoint tests for valid events, later stage only, duplicates/conflicts, invalid categories/IDs/time, unknown keys, 16 KiB body bound and failing DB.
- [x] Run Node tests to confirm expected failures before product code.
- [x] Implement strict validation, parameterized insert and bounded body reader. Event identity is global UUID; run grouping includes playerId/runId/buildId/dataset.
- [x] Run the SQL migration and endpoints against an in-memory SQLite adapter exercising real SQL. Migration is repeatable and includes useful aggregate indexes.

## Task 2: Statistics and prerequisites (root)

**Files:** src/statistics.js, src/catalog.js, test/statistics.test.js.
**Consumes:** stored event records and a checkpoint catalog with required predecessor IDs.
**Produces:** getSummary(db,filter), getRuns(db,filter,page), getRun(db,identity); authenticated routes /api/admin/summary, /api/admin/runs, /api/admin/run with query identity fields.

- [x] Write cases for A missing/B present, A arriving later, optional milestones, q3a/q3b branch, unknown milestones, multiple runs/builds and missing game start.
- [x] Define the denominator as distinct observed playerId within the filter; start count remains separate. First arrival time uses minimum playtimeSeconds per run and milestone.
- [x] Implement SQL aggregates and page limits. Missing prerequisites are derived from current observations; never insert inferred events.
- [x] Use actual quest IDs: demo_q1_water_intake_check, demo_q2_clear_water_screen, demo_q3a_repair_valve, demo_q3b_repair_bunker_pipe, demo_q4_todays_reward. Required dependencies: q2/q1, q3a/q2, q3b/q2, q4/q3a+q3b.
- [x] Validate catalog IDs, references and acyclic prerequisite relationships with tests.

## Task 3: Dashboard, authentication and portability (root)

**Files:** public/index.html, public/app.js, public/styles.css, public/strings/ko.json, src/auth.js, test/auth.test.js, test/structure.test.js, scripts/preview.mjs, wrangler.jsonc, README.md.
**Consumes:** administrator token from ADMIN_TOKEN secret and APIs from task 2.
**Produces:** a responsive Korean dashboard with login, build filter, stage table, run pagination/detail and missing-record labels.

- [x] Test missing/wrong/correct admin token and absent server configuration; use constant-time comparison or a cryptographic verification operation.
- [x] Build same-origin authenticated read requests; keep token only in memory. All API responses containing statistics are no-store.
- [x] Render external strings through textContent and all UI text through keys; cover empty/loading/error states and missing records.
- [x] Add minimal local preview server with sample data explicitly labeled as preview; this does not contact Cloudflare.
- [x] Provide GitHub Builds setup, manual D1 SQL initialization, DB binding, secret configuration, endpoint JSON example and Unreal endpoint setting instructions.
- [x] Copy the folder to a temporary directory and run tests there to prove it is standalone; inspect browser layout and interaction if browser tooling can reach preview.

## Task 4: Unreal best-effort sender (parallel Unreal agent)

**Files:** new Public/Subsystem/TunaSweeperProgressTrackerSubsystem.h and Private/Subsystem/TunaSweeperProgressTrackerSubsystem.cpp; GameInstance save flow/header and save structs; QuestSubsystem.cpp; AchievementLocationTrigger.cpp; DemoEndingActor.cpp; DefaultGame.ini; relevant automation tests; Docs/save_persistence.md.
**Consumes:** event contract in design.md and endpoint setting, no ADMIN_TOKEN.
**Produces:** stable installation GUID, per-save run GUID and attempted checkpoint IDs; HTTPS JSON events for start, actual quest reward claims, named location triggers and demo completion.

- [x] Pin pure event serialization/deduplication behavior with project-appropriate tests before implementation.
- [x] Create persistent installation ID and save-bound run ID; old saves receive a run ID without fabricating earlier milestones. Reuse GetCurrentActiveSlotTotalPlaySeconds for cumulative time and document its semantics.
- [x] Emit game.start after new save activation succeeds. Emit actual rewards directly in ClaimQuestReward, never from achievement restore.
- [x] Emit named location checkpoints once per run; ignore NAME_None. Do not invent physical placement IDs or change map assets.
- [x] Emit demo.complete after farewell/EndingSeen and before completed save deletion; preserve captured payload until HTTP completion.
- [x] Enforce demo/endpoint gating, HTTPS, no retries/queue/UI, finite timeout, orderly request cleanup and no raw identifiers in logs.
- [x] Build UE5.7 editor target, run relevant automation checks, and update persistence documentation. Notify root before launching Editor so the correct project instance is used.

## Task 5: Integration and completion (root + agent report)

- [x] Compare actual Unreal payloads with Worker validation and catalog IDs, UUID format, timing and deduplication semantics.
- [x] Run all Worker tests, independent-folder check and available browser checks. Check deploy binding configuration without executing Wrangler; complete production deployment remains user-owned.
- [x] Review complete changed files and git diff --check; verify no secrets, local paths or unrelated changes in the portable folder.
- [x] Gather Unreal build/test evidence and confirm the requested project editor is already running and leave it open.
- [x] Add task log with measured elapsed time; commit all task files once. Report copy location, test results, commit and actual deployment limitations.
