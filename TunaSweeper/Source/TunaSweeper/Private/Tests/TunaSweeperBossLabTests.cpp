#if WITH_DEV_AUTOMATION_TESTS
#include "BossLab/TunaSweeperBossDefinition.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossDefinitionRoundTrip, "TunaSweeper.BossLab.Definition.RoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaBossDefinitionRoundTrip::RunTest(const FString&)
{
	FTunaSweeperBossDefinition Original = TunaSweeperBossDefinition::MakeDefault();
	Original.Name = TEXT("보스 α");
	FName Error; FString Json;
	TestTrue(TEXT("Default serializes"), TunaSweeperBossDefinition::ToJson(Original, Json, Error));
	FTunaSweeperBossDefinition Loaded;
	TestTrue(TEXT("Definition imports"), TunaSweeperBossDefinition::FromJson(Json, Loaded, Error));
	TestEqual(TEXT("Identity survives file copy"), Loaded.BossId, Original.BossId);
	TestEqual(TEXT("Unicode name survives"), Loaded.Name, Original.Name);
	TestEqual(TEXT("Part count survives"), Loaded.Parts.Num(), Original.Parts.Num());
	TMap<int32, FTransform> Transforms;
	TestTrue(TEXT("Assembly transforms resolve"), TunaSweeperBossDefinition::BuildTransforms(Loaded, Transforms));
	TestEqual(TEXT("All parts are assembled"), Transforms.Num(), Original.Parts.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossDefinitionRejectsInvalid, "TunaSweeper.BossLab.Definition.RejectsInvalid", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaBossDefinitionRejectsInvalid::RunTest(const FString&)
{
	const auto Good = TunaSweeperBossDefinition::MakeDefault();
	FName Error;
	auto Bad = Good; Bad.Parts[1].InstanceId = Bad.Parts[0].InstanceId;
	TestFalse(TEXT("Duplicate part IDs rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.Parts[1].ParentId = Bad.Parts[1].InstanceId;
	TestFalse(TEXT("Cycles rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.Parts[1].ModuleId = TEXT("/Game/ArbitraryAsset");
	TestFalse(TEXT("Unknown modules rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.Parts[1].SocketIndex = Bad.Parts[2].SocketIndex;
	TestFalse(TEXT("Occupied socket rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.AttackInterval = 0.f;
	TestFalse(TEXT("Unbounded attack rate rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.Version = 99;
	TestFalse(TEXT("Unsupported version rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	Bad = Good; Bad.Name = FString::ChrN(65, TEXT('x'));
	TestFalse(TEXT("Oversized name rejected"), TunaSweeperBossDefinition::Validate(Bad, Error));
	FString Json; TunaSweeperBossDefinition::ToJson(Good, Json, Error);
	Bad = Good;
	TestFalse(TEXT("Malformed JSON rejected"), TunaSweeperBossDefinition::FromJson(TEXT("{\"parts\":["), Bad, Error));
	TestEqual(TEXT("Failed import preserves current identity"), Bad.BossId, Good.BossId);
	TestFalse(TEXT("Large payload rejected before parse"), TunaSweeperBossDefinition::FromJson(FString::ChrN(TunaSweeperBossDefinition::MaxFileBytes + 1, TEXT('x')), Bad, Error));
	Json.ReplaceInline(TEXT("\"version\": 1"), TEXT("\"version\": 1.5"));
	TestFalse(TEXT("Fractional integer rejected"), TunaSweeperBossDefinition::FromJson(Json, Bad, Error));
	Bad = Good;
	TunaSweeperBossDefinition::RemoveBranch(Bad, 1);
	TestEqual(TEXT("Core cannot be removed"), Bad.Parts.Num(), Good.Parts.Num());
	const int32 RemovedId = Bad.Parts[1].InstanceId;
	TunaSweeperBossDefinition::RemoveBranch(Bad, RemovedId);
	TestFalse(TEXT("Part removed"), Bad.Parts.ContainsByPredicate([RemovedId](const auto& P) { return P.InstanceId == RemovedId; }));
	TestTrue(TEXT("Remaining boss remains valid"), TunaSweeperBossDefinition::Validate(Bad, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossSlotFiles, "TunaSweeper.BossLab.Library.Files", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaBossSlotFiles::RunTest(const FString&)
{
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("BossLab-") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
	ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Directory, false, true); };
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UTunaSweeperBossLabSubsystem> Library(NewObject<UTunaSweeperBossLabSubsystem>(Instance.Get()));
	Library->SetLibraryDirectoryForTesting(Directory);
	FName Error;
	const auto Original = TunaSweeperBossDefinition::MakeDefault();
	TestFalse(TEXT("Negative slot rejected"), Library->SaveSlot(-1, Original, Error));
	TestFalse(TEXT("Slot limit enforced"), Library->SaveSlot(TunaSweeperBossDefinition::SlotCount, Original, Error));
	TestTrue(TEXT("Save slot"), Library->SaveSlot(0, Original, Error));
	FTunaSweeperBossDefinition Loaded;
	TestTrue(TEXT("Reload slot"), Library->LoadSlot(0, Loaded, Error));
	TestEqual(TEXT("Same boss"), Loaded.BossId, Original.BossId);
	auto Bad = Original; Bad.Parts.Reset();
	TestFalse(TEXT("Bad replacement rejected"), Library->SaveSlot(0, Bad, Error));
	TestTrue(TEXT("Good file preserved"), Library->LoadSlot(0, Loaded, Error));
	TestEqual(TEXT("Previous identity preserved"), Loaded.BossId, Original.BossId);
	TestEqual(TEXT("External file copy succeeds"), IFileManager::Get().Copy(*Library->GetSlotPath(1), *Library->GetSlotPath(0)), COPY_OK);
	TestTrue(TEXT("Copied file recognized without index"), Library->LoadSlot(1, Loaded, Error));
	FFileHelper::SaveStringToFile(TEXT("broken"), *Library->GetSlotPath(2));
	TestFalse(TEXT("Corrupt slot rejected"), Library->LoadSlot(2, Loaded, Error));
	TestEqual(TEXT("Corrupt read preserves output"), Loaded.BossId, Original.BossId);
	TestTrue(TEXT("Corrupt file is not destroyed"), Library->SlotExists(2));
	TestTrue(TEXT("Manual delete"), Library->DeleteSlot(1, Error));
	TestFalse(TEXT("Deleted slot is empty"), Library->SlotExists(1));
	Library->SetDraft(Original);
	Library->SetDirty(true);
	Library->SetSelectedSlot(3);
	Library->SetDevelopmentMode(false);
	auto Solo = Original; Solo.BossId = FGuid::NewGuid();
	Library->SetDraft(Solo);
	Library->SetSelectedSlot(1);
	Library->SetDirty(false);
	Library->SetDevelopmentMode(true);
	TestEqual(TEXT("Solo load does not discard edit draft"), Library->GetDraft().BossId, Original.BossId);
	TestTrue(TEXT("Unsaved edit marker restored"), Library->IsDirty());
	TestEqual(TEXT("Editor slot restored"), Library->GetSelectedSlot(), 3);
	return true;
}
#endif
