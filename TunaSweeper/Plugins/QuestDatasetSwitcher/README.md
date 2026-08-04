# Quest Dataset Switcher

This public plugin selects exactly one quest dataset and the matching full-save namespace.

| Switch value | Dataset id | Save namespace |
|---|---|---|
| `Public` | `public` | Existing public save slots |
| `ProductionDemo` | `production_demo` | `ProductionDemo` save slots |
| `ProductionRelease` | `production_release` | `ProductionRelease` save slots |

`ProductionPayload/` is a separate access-restricted Git repository. It is ignored by the public repository. The switch script materializes only the selected production dataset under the ignored project path `Content/Data/QuestDatasetGenerated/`.

```powershell
# Verify without changing the active dataset
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionDemo -VerifyOnly

# Activate one dataset
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset Public
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionDemo
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionRelease

# Check that production files cannot enter the public Git history
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\VerifyPublicSafety.ps1
```

Restart Unreal Editor after every switch. Public packaging must use a clean checkout that never contained `ProductionPayload/` or `QuestDatasetGenerated/`.
