# SSTO Lander Implementation Plan

> **For agentic workers:** Use focused parallel agents for model authoring and runtime implementation; the parent owns integration and final verification.

**Goal:** Place a tilted, enterable SSTO at the center of RaidPlains with paired ladder interaction and the existing boundary dissolve.

**Architecture:** Separate upper shell meshes from the walkable interior. Pair two local transfer interactables and use the existing vertical occlusion component with compatible colored material instances.

**Tech Stack:** Blender 4.5, UE 5.7 C++, Unreal Python, static FBX/GLB assets.

**Spec:** `Docs/superpowers/specs/2026-10-03-ssto-lander-design.md`

## Constraints and review focus

- Preserve existing map and unrelated work. Apply only the current SSTO change after checking the latest saved map.
- UI text resolves existing string-table keys; add localized entry/exit keys.
- Blocked destination, invalid pairs and non-player callers must not transfer.
- Capsule placement uses actual scaled height and avoids the hull; nearby opposite-floor interactions must not be selected.
- Keep the current dissolve behavior and default material behavior; floor/props remain visible.
- No additional persisted state; no narrative data changes.
- Remove one-off generators and include validation/logging in one final commit.

## Tasks

- [x] Model and validate the multipart ship, interior, ladder and landing gear. Export common-origin FBX parts and a GLB with a part/material/endpoint manifest.
- [x] Add `TunaSweeperLadderTransferActor` under Public/Private Interaction, enum/dispatch support and localized keys. Add meaningful automation tests for invalid/blocked endpoints and actual entry/exit roundtrips.
- [x] Extend `TunaSweeperVerticalOcclusionRevealComponent` with an opt-in compatible source-material mode, preserving the existing mask and restoring source materials correctly. Test the new option and default behavior.
- [x] Build `TunaSweeperEditor Win64 Development`; run the relevant automation tests.
- [x] Import into `/Game/MainRaid/SSTO`, create material instances from the existing reveal shader, place the ship at the actual terrain center and configure paired endpoints. Use separate static collision that leaves the cabin hollow.
- [x] Reload saved assets in a fresh UE process; check mesh references, passage dimensions, support/capsule clearance, entry/exit, palette, dissolve and Map Check. Render exterior and interior review images.
- [x] Remove generators, repeat affected checks, open the editor with the explicit project, update the request log and make one task commit.
