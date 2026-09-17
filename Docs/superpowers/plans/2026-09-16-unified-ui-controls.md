# Unified UI Controls Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development with independent file ownership and parallel agents as explicitly requested by the user.

**Goal:** Apply the approved pause-menu button language throughout game UI, compact settings into arrow selectors, and replace textual checkboxes with a softly filled rounded box and white drawn check.

**Architecture:** Native reusable style helpers and a painted check indicator serve existing UMG widgets. Settings use reusable single-row selectors. Adapt existing authored widget trees at runtime without one-off asset generators or changing gameplay/save semantics.

**Tech Stack:** Unreal Engine 5.7, C++, UMG, Slate, existing localized string keys.

**Spec:** User-approved scope in this conversation (2026-09-16); earlier UI review plus the explicit checkbox reference. Preserve comparison cards, semantic marker colors, independent toggle states, pending/apply/cancel and 15-second display rollback.

## Global Constraints

- All user-facing UI text resolves existing localization/string keys; root owns any string table additions.
- Preserve existing Docs/requests.md changes. No unrelated edits or publishing.
- User explicitly requested parallel agents; each worker owns disjoint files and does not launch other agents or build concurrently.
- Keep working checkout at the project's specified absolute path; use codex/unified-ui-controls branch so existing UE binaries/content remain usable.
- No asset-generation scripts or startup asset regeneration. Runtime UI composition is ordinary permanent UI code.

## Shared interfaces

Root provides `UI/TunaSweeperUIStyle.h`:
```cpp
namespace TunaSweeperUIStyle {
enum class EButtonRole : uint8 { Primary, Secondary, Danger, Icon, Tab };
void ApplyButton(UButton* Button, EButtonRole Role = EButtonRole::Primary, bool bSelected = false);
void ApplyLabel(UTextBlock* Label, int32 FontSize = 0);
void SetCheckButton(UWidgetTree* Tree, UButton* Button, UTextBlock* Label, bool bChecked);
}
```
Graphics worker provides `UI/TunaSweeperOptionRowWidget.h`: `UTunaSweeperOptionRowWidget`, `Configure(const FText&)`, `SetValue(const FText&)`, `SetStepEnabled(bool,bool)`, native multicast `OnStepRequested(int32 Delta)`.

### Task 1: Common controls (root)
- [x] Add shared button/label styling, painted check indicator, and checkbox composition helper.
- [x] Apply common styling to pause buttons while preserving focus/pressed animation and confirmation flow.
- [x] Coordinate new localized keys and integration fixes.

### Task 2: Graphics settings (graphics worker)
- [x] Compact preset, window mode, resolution, DLSS and FPS to one-line selectors in both authored and native trees.
- [x] Preserve unavailable modes, current custom values, pending state, applying/canceling and rollback.
- [x] Replace the four textual toggles with check controls. Restyle eleven quality selectors and actions.
- [x] Update meaningful graphics/UI tests for changed controls; root builds and runs them.

### Task 3: Title and language (title worker)
- [x] Style title actions, save/delete actions, difficulty actions, demo notice/credits and settings navigation appropriately; preserve comparison cards.
- [x] Compact interface/development language selectors, preserving language apply/cancel.
- [x] Replace developer textual/legacy checkbox presentation with shared check controls.

### Task 4: Game panel controls (panels worker)
- [x] Apply roles to selling, crafting/dismantling/registration, quest actions, research nodes, stack split, storage sort/filter, HUD tabs, memo and map controls.
- [x] Preserve list information, disabled states, rarity/marker colors and interactions; adapt compact geometry.

### Task 5: Integration and validation (root)
- [x] Review all diffs and update old style assertions without dropping behavior coverage.
- [x] Build Development Editor, run graphics/pause/title UI automation and render actual composed widgets.
- [x] Inspect rendered controls, fix layout problems, request independent final review, and revalidate changed areas.
- [x] Open TunaSweeper.uproject in the editor, append completed request record with real elapsed time.

## Progress / decisions

- Initial source status: only Docs/requests.md already modified. Runtime C++ and WBP widgets both present; no generated assets required by this design.
- Shared interfaces checked: graphics and title consume option rows; all workers consume style helper; only root edits helper and localized string files. Worker source ownership is disjoint.
- Ruling: reuse specified checkout on a new branch rather than copying large UE assets into a second checkout; this preserves the project's editor path and avoids concurrent binary/editor writes.
- Ruling: prior approved review is the design authorization. Proceed without repeating permission questions.
- Panel task and shared helpers independently reviewed by review_controls: no actionable findings. Runtime rendering remains required.
- Runtime ordering review: UE 5.7 InsertChildAt does not reorder live Slate children; graphics/title workers correcting this before integrated validation.

## Verification outcome

- UE 5.7 Development Editor build succeeded after integration and final layout fixes.
- Final automation: all nine tests passed (three Graphics, four PauseMenu, Title.ScreenAssetsAndTransitions, Controls.LocalizedPresentation). Log: TunaSweeper/Saved/Logs/UnifiedUI_Final.log.
- Actual authored UI rendered in Korean, English and Japanese. Checked selector alignment, faint rounded painted checks, long-label bounds, pending/cancel behavior and custom resolution/FPS round trips.
- Visual fixes: checkbox columns align; long option, quality, tab and checkbox labels scale down only when needed; no resolution digit grouping; obsolete language outline removed. Missing Japanese glyphs use the engine's already-staged DroidSansFallback.
- Independent review found the custom-value round-trip bug; fixed and regression-tested. Subsequent review confirmed the shared style/checkbox behavior and fallback staging.
- Gameplay-panel code was reviewed and compiled; every gameplay panel was not exercised interactively. No assets regenerated or packaging performed.