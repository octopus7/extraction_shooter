# Rail station lubrication and travel implementation plan

**Goal:** Use station countdown monuments for lubrication and unlock forward-only travel after any successful oil use.

**Architecture:** Attach project interaction components to existing LoopRail station monuments. Store each station's lubrication in existing transactional world-progress records; derive route-wide travel availability from any completed record, independently of quests. The reusable train plugin exposes an authoritative arrival operation; the project owns localization, inventory and screen fades.

**Tech Stack:** Unreal Engine 5.7 C++, existing UMG screen fade, JSON quest data.

**Spec:** Current user request in this conversation, 2026-10-17 05:43:34.

## Constraints and review focus
- One oil per station, never consume twice or unlock on failed payment/save.
- Three stations use existing track direction, including wraparound; no destination menu.
- Missing train, disabled station, blocked destination, mounted/dead player and overlapping travel fail safely.
- Fade out/in uses 0.2 seconds; release input and fade on cancellation/end play.
- Persist lubrication, not transient train pose; existing saves default to locked.
- Localized UI keys only. Keep private quest data in its own repository.
- Preserve unrelated map/document edits. Single task commit after validation and cleanup.

## Implementation
- [x] Add train arrival API and tests for stationary pose, ETA, dwell and resumed circulation.
- [x] Add station interaction component with lubrication and travel modes, transactional payment and route-wide saved unlock.
- [x] Attach both interactions to station monument pivots through the existing project rail subsystem; route input through interaction subsystem.
- [x] Connect lubrication quest objectives to persistent station records in local narrative data.
- [x] Add localized labels and tests for payment, shared unlock, restored records, direction and invalid transfer.
- [x] Build UE 5.7, run focused automation and inspect actual Village station configuration with a temporary read-only editor script.
- [x] Remove temporary automation scripts, update persistence/request logs, commit only task files and reopen editor after coordination.
