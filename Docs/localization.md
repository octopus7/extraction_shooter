# Localization

The six officially supported interface languages, in settings display order, are English (en), Japanese (ja), Korean (ko), Simplified Chinese (zh-Hans), Traditional Chinese (zh-Hant), and Russian (ru). All six have equal support; there is no base/additional language distinction. Brazilian Portuguese is removed.

The six string-key CSVs in TunaSweeper/Content/Data use the unified schema `string_key,ko,en,ja,zh-Hans,zh-Hant,ru`. Each row contains all six languages. No separate Translations CSVs are loaded. UI labels continue to resolve by string key. Quest and scenario strings are loaded from the selected narrative pack using the same schema.

An empty translation cell falls back to that row's English text. All current public string rows have values for all six languages. When adding strings, supply all six translations and preserve the source placeholders, markup and meaningful line breaks. Quest packaging validation requires all six language cells. Settings display order is independent of CSV column order; enum values 0–5 remain unchanged.

The Data directory is staged with the game. The internationalization preset supplies the supported cultures. The shared `/Game/UI/Fonts/F_TunaSweeperUI` composite font selects a font face for the active culture.

- English and Russian use Nunito Bold (700).
- Korean uses Jua Regular.
- Japanese uses Zen Maru Gothic Bold.
- Simplified Chinese uses ZCOOL KuaiLe Regular.
- Traditional Chinese uses LXGW Marker Gothic Regular.

Each family uses its selected face for both the Regular and Bold slots. Droid Sans Fallback supplies glyphs missing from the active face and the default Nunito face. The sources are from the [Google Fonts distribution](https://github.com/google/fonts/tree/83c62e30d87daa70216dab6b984d00d20d96d585/ofl). The Nunito face was created by pinning the variable font to weight 700. The five OFL notices ship under `Content/Licenses`.

Use Korean as the semantic source and check gameplay code when a label's meaning depends on its arguments or behavior. Keep item, NPC and facility names consistent between names, descriptions, objectives and UI. The player home is 기지 (base), not the internal Bunker identifier. Runtime placeholders and rich-text tags must stay intact. Descriptive CLI metavariables in help text are translatable display text; preserve command names, flags and angle-bracket structure while localizing those descriptions.
