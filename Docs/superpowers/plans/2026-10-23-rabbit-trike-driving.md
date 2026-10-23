# Rabbit Trike Driving Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans for inline implementation and a fresh final review.

**Goal:** Replace rabbit-trike strafing with bounded steering, forward travel and stationary aimed firing.

**Architecture:** Replace only the wheeled character's movement subobject with a component that delegates unchanged for delivery robots. It converts grounded path velocity/input into forward motion, leaving collision/navigation to CharacterMovement. The AI yields facing to active trike driving; the existing trike fire override enforces stationary aligned shots. Wheel presentation derives steering and roll from actual movement.

**Tech Stack:** Unreal Engine 5.7 C++, native automation tests.

**Spec:** `Docs/superpowers/specs/2026-10-23-rabbit-trike-driving-design.md`

## Global Constraints

- UE 5.7; bTricycle only; retain profile yaw limits and attack tolerance.
- No localization, persistence, spawn/placement or model changes.
- Existing shared checkout is used for the installed UE project and its build products; stage only this task's files/hunks. User implementation authorization and per-task commit preference apply.
- One implementation/validation/documentation/task-log commit; questions and unrelated request records excluded.

## Review Focus

- A lateral or rearward goal must remain reachable without sideways drive or instantaneous yaw.
- Acceleration-based wander and direct path velocity must both steer correctly.
- Stopping, aborted paths and death must release driving state without stranded aim or firing on the move.
- Saved BP_RabbitTrike must instantiate the new movement component; delivery robot BPs must remain unaffected.
- Front wheel yaw must compose with wheel roll and stay stable across yaw wrap and stationary frames.

## Task 1: Trike driving and stationary fire

**Files:** Base enemy constructor declaration/definition; wheeled character header/implementation; new `AI/TunaSweeperWheeledMovementComponent.h/.cpp`; AI controller; `Private/Tests/TunaSweeperWheeledEnemyTests.cpp`; robot verification README.

**Interfaces:** Wheeled character `bool IsDriving() const`; movement component `bool HasSteeringInput() const`, `CalcVelocity(...)` and `PhysicsRotation(...)` overrides. Existing pawn/profile/nav interfaces remain unchanged.

- [x] Add behavioral automation asserting bounded yaw, forward-only velocity, sharp-turn slowdown, eventual forward progress, front-wheel steering, moving/misaligned fire blocking and stopped facing resumption. Run against the original movement and observe the expected failures.
- [x] Add the trike-only movement subobject and guarded velocity/input conversion; preserve delivery behavior and CharacterMovement collision handling.
- [x] Yield AI facing while the trike is driving, including pending navigation/input; enforce stop-and-face fire checks in the trike override.
- [x] Compose front steering with roll and preserve distance-based wheel animation.
- [x] Build UE 5.7 with a unique log and run Wheeled and Combat automation. Exercise acceleration input, saved robot BPs and a collision/navigation scenario. Expected: no regressions and all new behavior checks pass.
- [x] Obtain a fresh review, resolve substantive findings, update documentation/task log, commit only related changes and reopen the project editor when coordination permits.

## Execution ledger

- Start: 2026-10-23 20:33:26. Baseline HEAD 0ccd11d2; target C++ source clean. Existing shoreline edits and pending logs are unrelated.
- Ruling: continue directly from the user's implementation approval; no repeated design/plan approval. Work in the existing checkout to keep the explicitly coordinated UE project/build path.
- Editor: no UE 5.7 editor process at initial inspection; shoreline conversation notified of build/test work.
- RED: original code, existing presentation test passed; new steering/fire test failed on eight behavior assertions. `Saved/Automation/TrikeDriving/Red/index.json`.
- First GREEN run: 48/49 passed, including real ground/input/collision travel through the saved rabbit BP. Remaining fire assertion had an invalid fixture: AI possession had already stopped the robot. Restore actual moving velocity after possession, then rerun.
- Review check: combat target retention uses line of fire, not body yaw. Steering away does not itself lose the existing target; perception behavior remains unchanged. Existing solo reposition lease is five seconds.
- Fresh review found two important cases. ReviewRed2 reproduced both: stopped Wander→Suspicious/Alerted facing remained locked; an oblique wall reintroduced lateral movement after velocity integration. Restrict the wander lock to Unaware and suppress unwalkable grounded trike wall sliding; keep walkable-surface response.
- Ruling: exact physical tire kinematics remain out of scope because the approved game steering deliberately permits stationary pivots; this does not relax forward-only drive or collision constraints.
- Final verification: UE 5.7 Editor build succeeded. `Automation RunTests TunaSweeper.Enemy.Wheeled+TunaSweeper.Combat` passed 49/49, zero warnings/failures/not-run. `Saved/Automation/TrikeDriving/Final/index.json` records the run. Both important review findings reproduced in ReviewRed2 and passed in Final; no deferred code-review findings.
