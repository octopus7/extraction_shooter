# Quest Dataset Switcher

This public plugin selects exactly one quest dataset and the matching full-save namespace.

| Switch value | Dataset id | Save namespace |
|---|---|---|
| `Public` | `public` | Existing public save slots |
| `ProductionDemo` | `production_demo` | `ProductionDemo` save slots |
| `ProductionRelease` | `production_release` | `ProductionRelease` save slots |

`ProductionPayload/` is a separate access-restricted Git repository. It is ignored by the public repository. The switch script materializes only the selected production dataset under the ignored project path `Content/Data/QuestDatasetGenerated/`.

## Editor UI

Open `Quest Dataset Switcher` in the `Data Tools` section of Unreal Editor's top-level `TunaSweeper` menu. Select Public, Production Demo, or Production Release and press Apply. The panel shows the loaded runtime dataset separately from the dataset materialized on disk.

Apply is disabled during PIE/Simulate/Standalone, Cook/Package, editor Build, Hot Reload, and Live Coding compilation. A successful change intentionally does not hot-reload quest data. Restart Unreal Editor immediately when the panel shows that a restart is required; the matching save namespace becomes active on that restart.

The command-line scripts remain available for automation and recovery:

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

Restart Unreal Editor after every dataset change. Public packaging must use a clean checkout that never contained `ProductionPayload/` or `QuestDatasetGenerated/`.
