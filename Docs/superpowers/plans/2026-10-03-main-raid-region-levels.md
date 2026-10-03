# Main Raid Region Levels Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and verify three distinct Main-only UE 5.7 raid-region maps with Landscapes, reusable props, and playable primary routes.

**Architecture:** A temporary editor-only C++ generator creates deterministic Landscapes and serialized level actors, then is removed after the generated assets pass fresh-process verification. A retained Python verifier reloads the maps through UE and writes a machine-readable report; a capture script produces review images. Full cook configuration references the three maps, while Demo exclusion remains directory-based.

**Tech Stack:** Unreal Engine 5.7 C++, `ALandscape::Import`, UE Editor Python, existing project and LoopRail assets, PowerShell build commands.

**Spec:** `Docs/superpowers/specs/2026-10-03-main-raid-region-levels-design.md`

## Global Constraints

- Store all maps under `/Game/MainRaid` and preserve every Demo configuration's `/Game/MainRaid` exclusion.
- Preserve unrelated dirty files and do not edit `BunkerMap`, placement anchors, placement IDs, or spawn JSON.
- Reuse existing assets; only the missing SSTO receives an in-map composite blockout.
- Remove the temporary generator, entry point, and generator-only dependencies before final verification.
- Implementation, generator cleanup, verification, task logging, and documentation are one task unit and one final commit.
- Do not claim final art or combat-balance approval.

## Review Focus

- A map file exists but reloads without a real Landscape or layer assignments.
- A Demo custom configuration accidentally includes `/Game/MainRaid`.
- A primary route contains excessive slope, missing ground collision, or an obstructed PlayerStart.
- Village LoopRail is open, too short, or intersects district blockers.
- Saved maps or references break after the temporary generator is removed.

---

### Task 1: Fresh-process verifier and red baseline

**Files:**
- Create: `Tools/MainRaidLevels/verify_maps.py`
- Create: `Tools/MainRaidLevels/run_verify.ps1`

**Interfaces:**
- Consumes: expected map names and actor labels from the design spec.
- Produces: `TunaSweeper/Saved/MainRaidLevels/verification.json` with a boolean `passed`, per-map structural results, route samples, collision results, and Map Check status.

- [x] Write the verifier to load each map and assert the required Landscape, layer material, PlayerStart, lighting, map-specific labels, and primary-route collision/clearance samples.
- [x] Run it before map generation and confirm it fails because the three maps are missing.

### Task 2: Temporary generator and serialized maps

**Files:**
- Temporarily create: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperMainRaidLevelGenerator.h`
- Temporarily create: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperMainRaidLevelGenerator.cpp`
- Temporarily modify: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperEditor.cpp`
- Temporarily modify: `TunaSweeper/Source/TunaSweeperEditor/TunaSweeperEditor.Build.cs`
- Create: `TunaSweeper/Content/MainRaid/RaidForest.umap`
- Create: `TunaSweeper/Content/MainRaid/RaidVillage.umap`
- Create: `TunaSweeper/Content/MainRaid/RaidPlains.umap`

**Interfaces:**
- Consumes: existing landscape material/layer infos, Nature meshes, building/facility meshes, and LoopRail classes.
- Produces: three standalone maps with deterministic actor labels used by Task 1.

- [x] Add a deferred command-line generator entry point and the minimal temporary `Landscape`, `Foliage`, and `LoopRail` dependencies.
- [x] Build `TunaSweeperEditor Win64 Development` and require exit code 0.
- [x] Run the generator once with the absolute `.uproject` path and require all three packages to save.
- [x] Run Task 1 verifier and require all three maps to pass structural and collision checks.

### Task 3: Cleanup, visual review, and cook boundary

**Files:**
- Delete: temporary generator files from Task 2.
- Restore generator changes from: `TunaSweeperEditor.cpp`, `TunaSweeperEditor.Build.cs`.
- Create: `Tools/MainRaidLevels/capture_maps.py`
- Create: `Tools/MainRaidLevels/run_capture.ps1`
- Modify: `TunaSweeper/Config/DefaultGame.ini`

**Interfaces:**
- Consumes: Task 2 map packages and Task 1 verifier.
- Produces: generator-independent assets, review PNGs, and Full cook references.

- [x] Remove every temporary generator entry point and dependency.
- [x] Rebuild the editor and require exit code 0.
- [x] Reload and verify all three maps again in a fresh process.
- [x] Capture and inspect one overview image per map; correct gross visual or collision issues and repeat checks if needed.
- [x] Add the three maps to the base Full `MapsToCook` list and statically verify every Demo custom config still excludes `/Game/MainRaid`.
- [x] Open `RaidForest` in Unreal Editor with the absolute `.uproject` path for the user's review.

### Task 4: Record, review, and commit

**Files:**
- Modify: `Docs/requests.md`
- Retain: the spec, plan, verifier/capture tools, map packages, and cook configuration.

**Interfaces:**
- Consumes: all verification evidence and final diff.
- Produces: one task commit and a clear handoff report.

- [x] Append the completed request to `Docs/requests.md` using the required Korean timestamp and elapsed-time heading without revealing restricted narrative data.
- [x] Run a final diff/status audit, build, verifier, and boundary checks.
- [x] Request a fresh whole-change review and resolve Critical/Important findings.
- [x] Commit implementation, cleanup, verification tools, documentation, and log together in one commit.
