# True-ending tuna fill implementation plan

> **For agentic workers:** Use superpowers:subagent-driven-development. Do not commit unless the user requests a commit.

**Goal:** Add one explicit helper Cheat action that fills the current slot's ten-slot raid warehouse with item 3004, ten per slot, for normal true-ending eligibility testing.

**Architecture:** Extend the existing transient request mailbox to version 2 without layout changes. Operation 3 with RowKey 0 fills tuna; result 14 acknowledges the committed fill. Gate operations retain their existing behavior. The helper supports gate reads/resets on versions 1/2 but requires v2 for tuna fill. All context, expiry, PID identity, disabled-cheat and duplicate-request checks remain active.

**Tech Stack:** UE 5.7 C++, existing gameplay save path, WPF .NET 10 helper.

**Spec:** User explicitly requested tuna filling for true-ending tests. The runtime condition requires ten distinct item UIDs of item 3004, quantity 10 each. Add no ending flag, arbitrary inventory cheat or other progression operation.

## Constraints

- Display a confirmation identifying the process, current map and slot, stating that raid warehouse contents are replaced and saved. No actual user save is modified by development verification.
- Preserve carried/base inventory, equipment, quests, XP and ending flags. Replace only warehouse contents and keep any open warehouse UI coherent.
- Persist all changes once through the existing slot save; restore runtime item map, warehouse/open-container state and dirty flags on failure.
- No startup grant side effects or extra save during cheat preparation. Mark successful warehouse fill initialized so starter logic cannot alter it.
- Use resource keys for helper text. AUTO never invokes this action. Disable duplicate submissions and reject stale/expired contexts.
- Keep movement ABI unchanged; update mailbox version package validation while retaining the all-zero runtime-state guard.
- Work in the shared project checkout and preserve unrelated question/request records and other conversations' edits.

## Tasks

- [x] Runtime: transactional GameInstance fill API and operation 3 handling; isolated-save tests for true-ending eligibility, exact UID/stack shape, save reload, rollback, existing/open warehouse, unrelated state, replay and stale/disabled requests.
- [x] Helper: Cheat button, explicit confirmation and async pinned-target request, v1 compatibility and v2 capability check; fake-process tests and layout check.
- [x] Integration: package guard/version tests, documentation and explicit-only policy; Editor/Shipping/helper build and relevant regressions; publish helper, record task, reopen coordinated editor.

## Progress

- 2026-11-15 13:47:09: inspected eligibility and persistence code; implementation started. Existing warehouse replacement is disclosed in the user-facing confirmation; no live save mutation is part of this task.
