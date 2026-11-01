# Localization

The six officially supported interface languages, in settings display order, are English (en), Japanese (ja), Korean (ko), Simplified Chinese (zh-Hans), Traditional Chinese (zh-Hant), and Russian (ru). All six have equal support; there is no base/additional language distinction. Brazilian Portuguese is removed.

The six string-key CSVs in TunaSweeper/Content/Data use the unified schema `string_key,ko,en,ja,zh-Hans,zh-Hant,ru`. Each row contains all six languages. No separate Translations CSVs are loaded. UI labels continue to resolve by string key. Quest and scenario strings are loaded from the selected narrative pack using the same schema.

An empty translation cell falls back to that row's English text. All current public string rows have values for all six languages. When adding strings, supply all six translations and preserve the source placeholders, markup and meaningful line breaks. Quest packaging validation requires all six language cells. Settings display order is independent of CSV column order; enum values 0–5 remain unchanged.

The Data directory is staged with the game. The internationalization preset supplies the supported cultures. Font coverage uses NanumSquareRound, DroidSansFallback for CJK and Roboto for accented Latin glyphs. Native-speaker proofreading and visual layout review remain advisable.
