# Free-play endings implementation plan

**Goal:** Placeable native ending actor and real Blueprint, with two replaceable stories and a reusable save slot after either ending.
**Architecture:** Reuse existing interaction, dialogue and fade widgets. An editable presentation data asset contains localization keys; no new widget classes or widget trees. A separate ending-save transaction records completion without invoking demo save retirement.
**Tech Stack:** UE 5.7 C++, Blueprint, existing six-language CSV localization.
**Spec:** User request in this chat, 2026-11-09. Placeholder story and presentation decisions delegated to implementation.

## Contract and choices
- Normal prompt `엔딩?!`; true prompt `엔딩!?`.
- True ending requires all 10 shared raid-storage slots to contain item 3004, quantity 10 each (100 total), consistent with the chest's full-green indicator. Never consume these items.
- Normal story: 잠깐의 평온. True story: 백 개의 내일. Separate editable data assets reference existing localization keys so replacement requires no runtime-code change.
- Fade to black, existing typewriter dialogue, completion save, title map `/Game/Maps/IntroMap`. Pause world during presentation; unblock on teardown or failure. Save failure shows a localized retry line and prevents title travel.
- Use distinct `ending.freeplay.normal_seen` / `ending.freeplay.true_seen` flags, never `demo.ending.farewell_seen`. Carry inventory and storage remain; raid XP is settled once. Continue uses existing save selection and base entry.
- Do not change existing demo ending, unrelated pending work, map placements, achievements, or the restricted narrative repository. Placeholder ending text is public demo-authored content.
- Work in this existing UE workspace; no worktree copy of large assets. Follow-up instruction explicitly authorizes committing this ending task.

## Review focus
Missing/invalid slots and item UIDs; fewer than 100 tuna or mixed contents; save failure/retry without duplication; repeated endings and save reload; destruction or travel while UI owns input/pause.

## Tasks
- [x] Add contract automation and observe missing native class failure.
- [x] Add `TunaSweeperEndingActor`, presentation data asset, interaction type and dispatch; live prompt refresh from inventory events.
- [x] Add `IsRaidStorageReadyForTrueEnding` and transactional `SaveFreePlayEnding` in a dedicated GameInstance source file; disk tests under isolated RaidStorageAutomation path.
- [x] Add localized placeholder stories and generate actual actor BP plus two data assets; remove one-off generator.
- [x] Rebuild, run ending/storage/save regression tests, inspect actual assets/presentation and perform independent review.
- [x] Update persistence/usage docs and task log; preserve unrelated edits and report validation limits.

## Results
- Contract test failed before class implementation; passed afterward.
- Independent review found an empty marker class. A new BP marker assertion reproduced the failure; native default fixed and asset reload passes.
- Fixed literal newline escapes and validated missing story keys with TryGetTextByKey. Render inspection found title overflow in the NPC tab; titles now use their own card.
- Final UE 5.7 build succeeded. Render-enabled final suite: 21/21 (FreePlayEnding, RaidStorage, Save, UI.Localization). Normal/true Korean dialogue screenshots inspected.
- One-off asset commandlet removed and rebuilt. Shared WaveDefense compile error was reported to its owner and fixed there.
- Validation limits: no full gameplay-map interaction-to-title manual playthrough. Pause/input teardown assessed by code review; refused-save retry tested through the deterministic existing save guard, with underlying disk failure covered by existing safe-save regression.
- Commit only this task per the follow-up instruction; unrelated pending edits preserved. Editor remains closed for coordinated WaveDefense work.
