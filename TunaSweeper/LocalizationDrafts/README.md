# Additional text translations

This directory holds first-pass translations for Simplified Chinese (`zh-Hans`), Traditional Chinese (`zh-Hant`), and Russian (`ru`). Each CSV corresponds to a string-key CSV in `Content/Data` and keeps its `string_key` rows in the same order. The original Korean, English, and Japanese CSVs remain the source of truth.

| File | Rows |
| --- | ---: |
| `DifficultyTextStrings.csv` | 6 |
| `ItemNameStrings.csv` | 112 |
| `MemoTextStrings.csv` | 40 |
| `QuestTextStrings.csv` | 68 |
| `ScenarioTextStrings.csv` | 12 |
| `UITextStrings.csv` | 498 |

All 736 rows have non-empty translations. The new languages are not wired into the runtime yet. The text has been checked against the existing Korean and English entries for game terminology, formatting arguments, and tutorial markup. It still needs native-speaker proofreading and an in-game layout pass before release. The Traditional Chinese column uses script conversion from the Simplified Chinese translation as its starting point, so regional wording also needs review.

When the source CSVs change, update the matching rows here by `string_key`. The source `UITextStrings.csv` currently contains `ui.common.confirm` twice; the duplicate rows are preserved here to keep exact row alignment.
