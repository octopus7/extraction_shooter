# Guided Rocket implementation plan

## Goal and approved design
Create an independent UE 5.7 C++ runtime plugin for small, visually readable guided rockets. A rocket BP selects a DA for speed, turn rate, guidance delay/duration/cone, fuse lifetime, collision radius and optional explosion damage. Meshes, trail and explosion presentation are editable in BP. Include a low-poly test mesh, simple effects, example BP/DA and demonstration map.

Guidance turns by a maximum number of degrees per second toward one supplied target, without interception prediction. Leaving the forward cone, losing the target or reaching the guidance deadline permanently ends guidance. Fuse expiry explodes at the current position; blocking impact also explodes. No target reacquisition. Default sample damage is zero for visual testing; optional damage uses engine damage dispatch and a BP filter hook.

## Architecture and constraints
- Runtime module `TunaGuidedRocket`: data asset, pure flight rules, rocket actor and lightweight mesh effect actor. No dependency on TunaSweeper classes or /Game assets.
- Sample content under `/TunaGuidedRocket/Examples`; +X mesh forward, cm, seconds, degrees per second.
- BP exposed Niagara slots allow replacing simple mesh effects; simple mesh effects work without external content.
- Fixed maximum simulation step for consistent turns and swept collision, with fuse time clipping.
- Flight is temporary runtime state, no new persistence. No player-facing UI strings.
- Work on `codex/guided-rocket-plugin` in the current checkout, preserving existing unrelated changes. User explicitly requested starting implementation after design approval; execute here without further design handoffs.
- All implementation, generated assets, generator removal, validation and request log belong in one task commit per AGENTS.md.

## Tasks
- [x] Add runtime module and meaningful automation coverage: bounded angular turn, early sidestep miss, guidance loss, invalid DA inputs, fuse and exactly-once explosion.
- [x] Implement DA, movement and actor lifecycle, owner immunity, optional damage filter, editable presentation.
- [x] Generate low-poly mesh/materials, example DA/BP and test map using temporary editor tooling.
- [x] Build and run automation, validate assets, remove generation tooling and rebuild/revalidate.
- [x] Review final changes, open project editor, record request and commit task files only.

## Review focus
Zero/invalid speed and lifetime must not create immortal actors; target loss must not restart guidance; large frame deltas must not overshoot fuse or obstacles; owner and instigator must be ignored; attached effects must stop and explosion dispatch must happen only once.

## Execution record
- Started 2026-10-03 00:34:13. Existing working tree contains unrelated map, loot and log edits; preserve them.


- Verification: initial turn/window tests failed against stubs; implemented flight rules passed. PhysicsBody regression failed before collision response fix. Added trigger/solid-cover damage and duplicate-effect assertions; all five automation tests passed in an otherwise empty UE 5.7 project after generator removal (GuidedRocket-AfterCleanup.log, exit 0).
- Sample assets saved; camera adjusted to frame the full path. Ten-second fixed-step sample game run completed with exit 0 and no warnings/errors (GuidedRocket-MapSmoke.log).
- Final editor build succeeded after removing the temporary generator module and its dependencies. Opened TunaSweeper.uproject with L_GuidedRocketRange.
- Review: fixed missing PhysicsBody collision and trigger volumes incorrectly shielding damage. Kept the plugin independent of host factions with a BP damage filter.
- Limitation: Computer Use permission to inspect Unreal Editor was not granted, so visual appearance has not been reviewed in the viewport. No multiplayer replication or existing AI integration was requested.
