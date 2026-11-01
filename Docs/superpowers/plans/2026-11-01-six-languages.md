# Six-language unification implementation plan

**Goal:** Support Korean, Japanese, English, Simplified Chinese, Traditional Chinese and Russian equally; remove Brazilian Portuguese and separate translation files.

**Architecture:** Each existing string-key CSV contains `string_key,ko,en,ja,zh-Hans,zh-Hant,ru`. Preserve existing enum values and translations. Load every language directly from the row. Keep English fallback for empty translations and existing authoring validation rules.

**Tech stack:** UE 5.7 C++, UTF-8 CSV, PowerShell validation.

**Spec:** User's six-language requirement and explicit instruction to proceed in this chat.

## Work and validation

- [x] Update packaging-validator fixtures to seven columns and verify rejection before implementation.
- [x] Merge the six public CSV pairs by string key; verify all original retained-language cells survive. Remove Portuguese label and separate CSVs.
- [x] Update runtime loaders, language selection, types, localization tests and packaging validation. Preserve saved enum values 0–5; unsupported saved codes follow existing fallback.
- [x] Update localization/save documentation; run data checks, PowerShell regressions, UE 5.7 build and localization automation when available.
- [x] Review diff, record task and commit only related changes, excluding question records.

## Review focus

Check duplicate keys, orphan translation keys, quoted/multiline CSV cells, locale fallback, and authoring/packaging consumers of the old four-column schema. Do not copy private narrative data into public files or synchronize QuestStudio data.

Validation: UE 5.7 Editor build succeeded; all 6 localization automation tests and 15 project Node regressions passed. Packaging validator and build-flavor regressions passed. Settings order: English, Japanese, Korean, Simplified Chinese, Traditional Chinese, Russian.
