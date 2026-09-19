# Housing System Removal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the retired bunker housing placement system from runtime code, UI, input, persistence, quest rewards, data, and current-behavior documentation while preserving independently placed facility actors.

**Architecture:** Delete the housing runtime boundary first—subsystem, placement actors, management interaction, panel, camera/input mode—and remove all consumers in one compile-safe slice. Then remove the remaining save and quest schema so old serialized housing fields are ignored while unrelated save data continues to load. Finish by deleting housing-only data and current-behavior documentation, while leaving historical request/question/audit records intact.

**Tech Stack:** Unreal Engine 5.7, C++20/UHT reflection, UE Automation Tests, JSON/CSV gameplay data, PowerShell, Git.

**Spec:** `Docs/housing_removal_design.md`

## Global Constraints

- Preserve independently placed storage, shop, workbench, and piggy-bank actors and their own save state.
- Do not implement facility-actor unlocking or migrate `UnlockedHousingFacilityIds` into a replacement system.
- Do not publish quest data; `quest:push` is out of scope.
- Do not edit historical entries in `Docs/requests.md`, `Docs/questions.md`, or dated audit documents.
- Keep unrelated pre-existing worktree changes unstaged and unmodified. In particular, do not include the current mole, title, map, ATV asset, shallow-puddle, editor-generator, or unrelated documentation changes in housing commits.
- Execute the plan in an isolated Git worktree created from commit `94ccd2a1`. The current checkout now has overlapping uncommitted edits in `TunaSweeperPlayerController.cpp`, `TunaSweeperTopDownCharacter.h`, and `UITextStrings.csv`; do not edit or stage those current-checkout files.
- Commit the complete removal in the isolated branch. Do not merge or cherry-pick it into the dirty `main` checkout until the overlapping owner changes are committed and conflict resolution can be reviewed.
- Append the housing completion entry to the isolated worktree's `Docs/requests.md`; never stage a request-log change from the dirty current checkout.
- Treat `BarrierGateActor::Housing` as the physical gate enclosure; it is unrelated and must remain.
- Use UE 5.7-compatible APIs and build `TunaSweeperEditor Win64 Development`.

## Review Focus

- Removing optional housing save properties does not change the save-version policy; the existing Save suite must still pass after the fields disappear from reflection.
- `BunkerMap` cannot create a housing subsystem, housing area, management terminal, grid, preview, or placed housing facility because those reflected classes are absent.
- Independent storage, shop, workbench, and piggy-bank actor classes remain reflected and constructible after housing classes are removed.
- Player-controller and HUD reflection expose no housing entry points, while existing pause and interaction regression suites still pass.
- Quest definitions without housing rewards still load and initial quest behavior remains valid; legacy housing reward properties disappear from reflection and parsing.

---

### Task 1: Remove the runtime housing boundary and all live consumers

**Files:**
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Tests/TunaSweeperHousingSubsystemTests.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Subsystem/TunaSweeperHousingSubsystem.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperHousingSubsystem.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Housing/TunaSweeperHousingAreaActor.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Housing/TunaSweeperHousingAreaActor.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Housing/TunaSweeperHousingFacilityActor.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Housing/TunaSweeperHousingFacilityActor.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Housing/TunaSweeperHousingGridVisualActor.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Housing/TunaSweeperHousingGridVisualActor.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Housing/TunaSweeperNpcFacilityActor.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Housing/TunaSweeperNpcFacilityActor.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperHousingManagementActor.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperHousingManagementActor.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperHousingPanelWidget.h`
- Delete: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperHousingPanelWidget.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperInteractableComponent.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperInteractableComponent.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Subsystem/TunaSweeperInteractionSubsystem.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperInteractionSubsystem.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Player/TunaSweeperPlayerController.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Player/TunaSweeperPlayerController.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Player/TunaSweeperPlayerControllerPause.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperGameHudWidget.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetGameplay.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetHandlers.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetLayout.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetPanels.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetRefresh.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetShared.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Character/TunaSweeperTopDownCharacter.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterActions.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterInput.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterMovement.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterState.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterWeapon.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperBossTestGameMode.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Vehicle/TunaSweeperATVActor.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Vehicle/TunaSweeperVehicleMountComponent.cpp`
- Delete: `TunaSweeper/Content/Data/HousingFacilityDefinitions.json`

**Interfaces:**
- Consumes: `ETunaSweeperInteractionType`, `ATunaSweeperPlayerController`, `UTunaSweeperGameHudWidget`, and independent facility actor APIs already present in the project.
- Produces: a runtime with no housing subsystem/class entry point, no `HousingManagement` interaction type, no housing mode state, and no housing-only data file.

- [ ] **Step 1: Replace the disabled-subsystem test with a failing removal contract**

Replace the current header-dependent test with a reflection/data test that compiles both before and after class deletion:

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperPiggyBankActor.h"
#include "Interaction/TunaSweeperShopActor.h"
#include "Interaction/TunaSweeperStorageActor.h"
#include "Interaction/TunaSweeperWorkbenchActor.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperHousingRemovalContractTest,
	"TunaSweeper.Housing.RemovalContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperHousingRemovalContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestNull(TEXT("Housing subsystem class is absent"),
		FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperHousingSubsystem")));
	TestNull(TEXT("Housing management actor class is absent"),
		FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperHousingManagementActor")));
	TestNotNull(TEXT("Storage actor remains"), ATunaSweeperStorageActor::StaticClass());
	TestNotNull(TEXT("Shop actor remains"), ATunaSweeperShopActor::StaticClass());
	TestNotNull(TEXT("Workbench actor remains"), ATunaSweeperWorkbenchActor::StaticClass());
	TestNotNull(TEXT("Piggy-bank actor remains"), ATunaSweeperPiggyBankActor::StaticClass());
	const UEnum* InteractionEnum = StaticEnum<ETunaSweeperInteractionType>();
	TestTrue(TEXT("Interaction enum remains reflected"), InteractionEnum != nullptr);
	TestEqual(TEXT("HousingManagement interaction is absent"),
		InteractionEnum ? InteractionEnum->GetIndexByNameString(TEXT("HousingManagement")) : INDEX_NONE,
		INDEX_NONE);
	TestFalse(TEXT("Housing facility definitions are absent"),
		IFileManager::Get().FileExists(*FPaths::Combine(
			FPaths::ProjectContentDir(), TEXT("Data/HousingFacilityDefinitions.json"))));
	TestNull(TEXT("Player controller has no OpenHousingMode entry point"),
		ATunaSweeperPlayerController::StaticClass()->FindFunctionByName(TEXT("OpenHousingMode")));
	TestNull(TEXT("HUD has no housing context menu entry point"),
		UTunaSweeperGameHudWidget::StaticClass()->FindFunctionByName(TEXT("ShowHousingFacilityContextMenu")));
	return true;
}

#endif
```

- [ ] **Step 2: Build and run the new test to verify RED**

Run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' TunaSweeperEditor Win64 Development 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -WaitMutex -NoHotReloadFromIDE
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -unattended -nop4 -nosplash -NullRHI '-ExecCmds=Automation RunTests TunaSweeper.Housing.RemovalContract; Quit' '-TestExit=Automation Test Queue Empty' -log
```

Expected: build succeeds; the automation test fails because the subsystem and management actor classes, enum member, and JSON file still exist.

- [ ] **Step 3: Delete the housing runtime classes and data file**

Delete every file marked `Delete` above. Keep `TunaSweeperHousingTypes.h` temporarily because save and quest fields still require it until Task 2.

- [ ] **Step 4: Remove interaction dispatch and suppression**

Remove `HousingManagement` from `ETunaSweeperInteractionType`, its localized-name switch case, the housing-mode suppression helper, housing subsystem/management includes, `HandleHousingManagementInteraction`, and the dispatch switch branch. Preserve the numeric values of the remaining explicitly numbered enum entries.

- [ ] **Step 5: Remove player-controller housing mode and camera/input code**

Remove all public/private housing methods and fields from `TunaSweeperPlayerController.h`; remove the subsystem/area/camera includes, `TunaSweeperHousingCamera` constants, log category, delegate binding, input bindings, tick branch, context menu handler, camera implementation, open/start/commit/query functions, and housing-only guards from the `.cpp` files. Where a condition is `A || IsHousingModeOpen()`, remove only that term and preserve every other guard.

- [ ] **Step 6: Remove HUD housing widgets and state branches**

Remove the housing panel/context menu members, creation methods, callbacks, layout visibility branch, key handling, subsystem delegate wiring, and housing-mode HUD suppression. Do not change inventory, quest, map, memo, research, dialogue, pause, or difficulty UI behavior.

- [ ] **Step 7: Remove housing guards from character, boss-test, and ATV paths**

Remove only `IsHousingModeOpen()` predicates and `SetHousingModeVisualHidden` support. Preserve death, dialogue, inventory, pause, mount, movement, combat, and weapon conditions exactly as they are.

- [ ] **Step 8: Build and run the removal contract to verify GREEN**

Run the same build and test commands from Step 2.

Expected: build succeeds and `TunaSweeper.Housing.RemovalContract` reports `Success`.

- [ ] **Step 9: Commit the runtime removal**

Stage only Task 1 paths and inspect the cached diff before committing:

```powershell
git diff --cached --check
git commit -m "refactor(housing): remove runtime placement system"
```

### Task 2: Remove housing persistence and quest reward schema

**Files:**
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Tests/TunaSweeperHousingSubsystemTests.cpp`
- Delete: `TunaSweeper/Source/TunaSweeper/Public/Housing/TunaSweeperHousingTypes.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Inventory/TunaSweeperSaveGame.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Game/TunaSweeperGameInstance.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceSave.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceWorldState.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Public/Quest/TunaSweeperQuestTypes.h`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperQuestSubsystem.cpp`
- Modify: `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperQuestWidget.cpp`

**Interfaces:**
- Consumes: existing `UTunaSweeperSaveGame`, `UTunaSweeperGameInstance`, and `FTunaSweeperQuestRewardDefinition` reflection types.
- Produces: save and quest schemas with no housing fields or reward behavior; old serialized housing tags are ignored by UE property serialization.

- [ ] **Step 1: Extend the contract test with failing reflection assertions**

Add includes for `Inventory/TunaSweeperSaveGame.h`, `Quest/TunaSweeperQuestTypes.h`, and `UObject/UnrealType.h`, then assert:

```cpp
TestNull(TEXT("SaveGame has no HousingFacilities property"),
	FindFProperty<FProperty>(UTunaSweeperSaveGame::StaticClass(), TEXT("HousingFacilities")));
TestNull(TEXT("SaveGame has no UnlockedHousingFacilityIds property"),
	FindFProperty<FProperty>(UTunaSweeperSaveGame::StaticClass(), TEXT("UnlockedHousingFacilityIds")));
TestNull(TEXT("Quest rewards have no HousingFacilityUnlocks property"),
	FindFProperty<FProperty>(FTunaSweeperQuestRewardDefinition::StaticStruct(), TEXT("HousingFacilityUnlocks")));
TestNull(TEXT("Housing placement save struct is absent"),
	FindObject<UScriptStruct>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperHousingPlacedFacilitySaveData")));
```

- [ ] **Step 2: Build and run the test to verify RED**

Run the Task 1 build/test commands.

Expected: `RemovalContract` fails because all four reflected housing schema elements still exist.

- [ ] **Step 3: Remove housing fields and GameInstance APIs**

Remove `HousingFacilities`, `UnlockedHousingFacilityIds`, `GetHousingFacilities`, `SetHousingFacilities`, `IsHousingFacilityUnlocked`, `UnlockHousingFacility`, and `GetUnlockedHousingFacilityIds` from headers and implementations. Remove their load, completed-quest backfill, save, sort, and reset blocks from `TunaSweeperGameInstanceSave.cpp`. Do not change save version constants solely for removed optional properties; UE must ignore unknown tags from older saves.

- [ ] **Step 4: Remove quest reward parsing, granting, and presentation**

Remove `HousingFacilityUnlocks` from `FTunaSweeperQuestRewardDefinition`; remove parsing of both `housing_facilities` and `housing_facility_unlocks`; remove reward claiming via `UnlockHousingFacility`; remove housing reward display-name lookup from `TunaSweeperQuestWidget.cpp`. Keep coins, items, recipes, XP, and all other reward paths unchanged.

- [ ] **Step 5: Delete the final housing type header and verify no include remains**

Delete `TunaSweeperHousingTypes.h`, then run:

```powershell
rg -n -S 'Housing/TunaSweeperHousingTypes.h|FTunaSweeperHousing' TunaSweeper/Source/TunaSweeper
```

Expected: no matches.

- [ ] **Step 6: Build and run housing, save, and quest tests**

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' TunaSweeperEditor Win64 Development 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -WaitMutex -NoHotReloadFromIDE
foreach ($Filter in @('TunaSweeper.Housing', 'TunaSweeper.Save', 'TunaSweeper.Quest')) {
	& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -unattended -nop4 -nosplash -NullRHI "-ExecCmds=Automation RunTests $Filter; Quit" '-TestExit=Automation Test Queue Empty' -log
	if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: all discovered Housing, Save, and Quest tests report `Success`; no housing UHT symbol remains.

- [ ] **Step 7: Commit persistence and quest cleanup**

```powershell
git diff --cached --check
git commit -m "refactor(housing): remove save and quest schema"
```

### Task 3: Remove housing localization and current-behavior documentation

**Files:**
- Modify: `TunaSweeper/Content/Data/UITextStrings.csv`
- Delete: `Docs/bunker_internal_facilities.md`
- Modify: `Docs/interaction_system_flow.md`
- Modify: `Docs/level_transition_flow.md`
- Modify: `Docs/atv_mount.md`
- Modify: `Docs/save_persistence.md`
- Modify: `Docs/save_load_runtime_persistence_flow.md`
- Modify: `Docs/quest_system.md`
- Modify: `Docs/quest_dialogue_scenario_flow.md`
- Modify: `Docs/quest_and_runtime_actor_data_authoring_guide.md`
- Modify: `Docs/pause_menu.md`
- Modify: `Docs/housing_removal_design.md`

**Interfaces:**
- Consumes: the runtime/schema state produced by Tasks 1 and 2.
- Produces: localization and current-behavior documentation that no longer advertises housing; the design document becomes the completed-removal record.

- [ ] **Step 1: Remove housing-only localization rows**

First extend `TunaSweeperHousingSubsystemTests.cpp` to read `UITextStrings.csv` with `FFileHelper::LoadFileToString` and assert that it contains neither `ui.interaction.housing_management` nor `ui.housing.`. Run `TunaSweeper.Housing.RemovalContract` and confirm it fails on those rows. Then delete `ui.interaction.housing_management` and every `ui.housing.*` row from `UITextStrings.csv`. Preserve facility-independent workbench, storage, shop, piggy-bank, signal-bot, and supply-bot strings.

- [ ] **Step 2: Remove or rewrite current-behavior documentation**

Delete the housing-only `Docs/bunker_internal_facilities.md`. In the remaining listed documents, remove housing-mode flow, housing save fields, housing quest rewards, housing authoring instructions, housing-based interaction blocking, and housing-based ATV HUD/dismount blocking. Keep descriptions of independent facility actors and general world-progress state. Leave SSOT, story, Steam-scope, dated audits, and historical logs unchanged; their planned facility content will be revised with the later facility-actor unlock task rather than being silently deleted here.

- [ ] **Step 3: Mark the design complete**

Change the design status line to:

```markdown
> 상태: 구현 완료. 하우징 런타임·UI·입력·저장·퀘스트 보상 경로를 제거했으며 시설별 액터 언락은 별도 후속 작업이다.
```

- [ ] **Step 4: Verify active documentation and localization are clean**

```powershell
rg -n -S 'ui\.housing|interaction\.housing|housing_facilities|housing_facility_unlocks|하우징 모드|하우징 UI|벙커 하우징|HousingFacilities|UnlockedHousingFacilityIds|TunaSweeperHousingSubsystem' TunaSweeper/Content/Data/UITextStrings.csv Docs/interaction_system_flow.md Docs/level_transition_flow.md Docs/atv_mount.md Docs/save_persistence.md Docs/save_load_runtime_persistence_flow.md Docs/quest_system.md Docs/quest_dialogue_scenario_flow.md Docs/quest_and_runtime_actor_data_authoring_guide.md Docs/pause_menu.md
```

Expected: no matches. Historical logs and audits are intentionally excluded.

- [ ] **Step 5: Re-run the localization-aware removal contract**

Run `TunaSweeper.Housing.RemovalContract` with the command from Task 1 Step 2.

Expected: the test reports `Success`, including both localization-key absence assertions.

- [ ] **Step 6: Commit localization and documentation cleanup**

```powershell
git diff --cached --check
git commit -m "docs(housing): remove retired system references"
```

### Task 4: Final verification, worktree isolation, and completion record

**Files:**
- Modify: `Docs/requests.md` (append only; stage only the new housing hunk)

**Interfaces:**
- Consumes: all removal commits from Tasks 1–3.
- Produces: verified UE 5.7 build/test evidence and a scoped completion record without capturing unrelated worktree changes.

- [ ] **Step 1: Run targeted source and asset scans**

```powershell
rg -n -S 'TunaSweeperHousing|HousingManagement|HousingFacilityUnlocks|HousingFacilities|UnlockedHousingFacilityIds|IsHousingModeOpen|OpenHousingMode|housing_facilities|housing_facility_unlocks' TunaSweeper/Source TunaSweeper/Config TunaSweeper/Content -g '!TunaSweeper/Intermediate/**' -g '!TunaSweeper/Saved/**' -g '!TunaSweeper/Binaries/**'
rg -a -l -S 'TunaSweeperHousing|HousingManagement|HousingPanel' TunaSweeper/Content -g '*.uasset' -g '*.umap'
```

Expected: no matches. Do not broaden the scan to bare `Housing`, because `BarrierGateActor::Housing` is an unrelated physical enclosure.

- [ ] **Step 2: Build the complete editor target**

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' TunaSweeperEditor Win64 Development 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -WaitMutex -NoHotReloadFromIDE
```

Expected: `Result: Succeeded`. If unrelated pre-existing worktree code fails, record the exact unrelated compiler error and do not modify that feature to hide it.

- [ ] **Step 3: Run the regression suites**

```powershell
foreach ($Filter in @('TunaSweeper.Housing', 'TunaSweeper.Save', 'TunaSweeper.Quest', 'TunaSweeper.Pause', 'TunaSweeper.Interaction')) {
	& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -unattended -nop4 -nosplash -NullRHI "-ExecCmds=Automation RunTests $Filter; Quit" '-TestExit=Automation Test Queue Empty' -log
	if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
```

Expected: all discovered tests report `Success` and the command exits `0`.

- [ ] **Step 4: Open the explicit UE project after a successful build**

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject'
```

Expected: the UE 5.7 editor process opens or the existing process for this exact project remains active.

- [ ] **Step 5: Append and stage the isolated-worktree request log**

Append a timestamped entry to the isolated worktree's `Docs/requests.md` covering the removal scope, preserved independent facilities, no save migration, tests, build, and no quest publish. Stage only that isolated-worktree file; do not touch the current checkout's request log.

- [ ] **Step 6: Verify cached scope and commit the completion record**

```powershell
git diff --cached --check
git diff --cached --stat
git diff --cached -- Docs/requests.md
git commit -m "docs: record housing system removal"
git status --short
```

Expected: the commit contains only the new housing completion entry. The isolated worktree is clean after the commit; the current checkout still retains its unrelated pre-existing changes untouched.
