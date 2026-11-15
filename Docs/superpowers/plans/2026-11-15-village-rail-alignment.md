# Village rail alignment and random start

**Goal:** At level BeginPlay, choose the train head distance once in the first half of its route. Align the saved village spline to the existing three station platforms.

**Design:** Authority chooses StartDistance in [0, TrackLength * 0.5] before ResetTrain. Clients use replicated distance. ResetTrain retains the selected distance; editor construction remains deterministic. No save data changes. Keep station transforms fixed and modify only the adjacent spline spans to provide level straight track at station height, with the existing 115cm centerline-to-platform edge (5cm vehicle gap). Preserve the surrounding route and other map actors.

**Constraints:** UE 5.7; preserve existing uncommitted map changes; coordinate editor closure; no commit requested. First-half range refers to the locomotive reference point, with trailing cars wrapping around the closed route.

- [x] Add BeginPlay regression coverage: first-half range, varied starts, reset stability and invalid track.
- [x] Observe failure, implement authority-only random choice, build and run LoopRail tests.
- [x] Back up current saved map; inspect spline points and fit station spans with continuous transitions.
- [x] Validate saved/reloaded map: parallel angle, 5cm boarding gap, ramp opposite track, preserved stations/other actors, closed route and transition geometry.
- [x] Remove one-off scripts, record results and reopen the editor after coordination.

Ruling: work in the shared checkout because the target map contains the user's current uncommitted layout; preserve and back up that state instead of using a stale worktree map. User's two-task instruction authorizes implementation directly.

Validation: regression failed on all 12 fixed starts before the change. UE 5.7 build succeeded and 7 LoopRail.Runtime tests passed. Saved-map reload validates all 133 actor transforms, all station angles 0 degrees and vehicle boarding gaps approximately 5cm. Initial reload comparison included transient FTransform addresses; numeric comparison corrected that verifier defect. Maximum grade remains 4.572 degrees and pre-existing maximum heading change remains 8.03 degrees/m.


Final: project rail interaction tests (2) and train camera test (1) passed; total 10 passing tests. One-off scripts removed, task recorded, editor reopened with RaidVillage. No commit requested or created.
