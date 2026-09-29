# Localization

The interface supports English (`en`), Korean (`ko`), Japanese (`ja`), Simplified Chinese (`zh-Hans`), Traditional Chinese (`zh-Hant`), and Russian (`ru`). Language names in the selector use their native spelling.

The six string-key CSVs in `TunaSweeper/Content/Data` retain their `string_key,ko,en,ja` schema. The matching CSVs in `TunaSweeper/Content/Data/Translations` supply `string_key,zh-Hans,zh-Hant,ru`. These are runtime data, replacing the former `LocalizationDrafts` directory. Update both the source and its translations when changing a string key. UI labels must continue to resolve through keys.

Each loader reads the translation file beside its source CSV in a `Translations` subdirectory. This keeps quest and scenario translations attached to the selected narrative pack. A missing translation file, key, or language cell falls back to that source row's English text. Malformed files and conflicting duplicate keys fail loading. The existing identical `ui.common.confirm` duplicate is accepted.

The `Data` directory is already staged with the game; the packaging internationalization preset includes all six cultures. The existing NanumSquareRound font with the engine's DroidSansFallback covers all characters in the prepared translations. Native-speaker proofreading and visual layout review remain advisable before release; the Traditional Chinese draft began with script conversion from Simplified Chinese.
