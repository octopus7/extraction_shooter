# Chain-link gate reset implementation plan

> **For agentic workers:** Use superpowers:subagent-driven-development to implement the scoped tasks below. Do not commit without a new user instruction.

**Goal:** Reset only explicitly selected, loaded rotating chain-link gates through a separate helper Cheat popup, persisting the reset in the current save slot.

**Architecture:** The helper submits a one-shot request to a dedicated versioned runtime mailbox. The game thread validates the cheat gate and current world/slot context, removes only the selected gate's unlock flag using the existing save path, then restores its closed/locked pose and interactions. The helper displays the authoritative response and refreshes the list. Automatic cheat activation never submits resets.

**Tech Stack:** UE 5.7 C++, Windows WPF .NET 10, existing process-memory bridge and save system.

**Spec:** User-approved design in this chat: persistent current-slot reset; separate helper popup; only user-requested progression categories may be implemented. Initially only rotating chain-link gates are authorized.

## Constraints and rulings

- Preserve all unrelated working edits; this task builds on the uncommitted gate and helper implementations in the shared project checkout. Work in place to keep those dependencies and assets available.
- No bulk reset, quest/item reset, arbitrary flag edit, automatic reset, or new progression category.
- All helper labels/status/errors use resource keys. No new Unreal widget is needed.
- Keep movement-control ABI compatible. New mailbox starts empty/inactive, is never saved, and is checked in package validation.
- Reset results follow the current slot's ordinary save/cloud behavior. Only control state remains memory-only.
- Only one valid standalone gameplay context may service the process mailbox. Context changes invalidate previously displayed rows and pending requests.
- Requests expire; helper timeouts must not become surprise resets on a later game tick. Multiple helper writers must serialize request publication. Reset requires explicit button action and confirmation.

## Review focus

- Save failure preserves gate flag, pose and interaction.
- World/slot/process changes reject stale selections; process identity includes start time.
- Closed, partially opening and fully open gates recover correctly; other gates and unrelated flags remain intact.
- Editor idle, disabled cheats, duplicate gate IDs and unsupported interface fail closed.
- Polling and reset never block WPF UI; closing the popup stops monitoring, and AUTO does not repeat resets.

## Tasks

- [x] Runtime: add transactional gate reset API and a dedicated game-thread mailbox subsystem; test isolated-slot persistence, rollback, replay/expiry/context rejection and gate/control reuse.
- [x] Helper: add resource-based popup, async list/status/reset client, pinned target identity, context-aware selection and reset confirmation; test fake-process protocol and UI lifecycle.
- [x] Integration: extend package guard for initially empty mailbox, update persistence and explicit-only policy, verify Editor/Shipping and helper builds, automation and popup rendering; remove disposable test generators, publish helper and log completion.

## Progress

- 2026-11-15 09:21:32: implementation started; user approved persisted resets and explicitly restricted future categories to user requests.
