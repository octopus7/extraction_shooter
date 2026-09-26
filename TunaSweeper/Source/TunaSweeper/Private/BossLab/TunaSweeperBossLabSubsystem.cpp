#include "BossLab/TunaSweeperBossLabSubsystem.h"

#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"

void UTunaSweeperBossLabSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Draft = TunaSweeperBossDefinition::MakeDefault();
}

FString UTunaSweeperBossLabSubsystem::GetLibraryDirectory() const
{
	return DirectoryOverride.IsEmpty() ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BossLab"), TunaSweeperBuildFlavor::IsDemo() ? TEXT("Demo") : TEXT("Main")) : DirectoryOverride;
}

FString UTunaSweeperBossLabSubsystem::GetSlotPath(int32 Index) const
{
	return Index >= 0 && Index < TunaSweeperBossDefinition::SlotCount ? FPaths::Combine(GetLibraryDirectory(), FString::Printf(TEXT("Slot%02d.boss.json"), Index + 1)) : FString();
}

bool UTunaSweeperBossLabSubsystem::SlotExists(int32 Index) const
{
	const FString Path = GetSlotPath(Index);
	return !Path.IsEmpty() && IFileManager::Get().FileExists(*Path);
}

bool UTunaSweeperBossLabSubsystem::LoadSlot(int32 Index, FTunaSweeperBossDefinition& OutDefinition, FName& OutError) const
{
	const FString Path = GetSlotPath(Index);
	OutError = TEXT("ui.boss_lab.error.slot");
	if (Path.IsEmpty()) return false;
	const int64 Size = IFileManager::Get().FileSize(*Path);
	OutError = TEXT("ui.boss_lab.error.missing");
	if (Size < 0) return false;
	OutError = TEXT("ui.boss_lab.error.limit");
	if (Size > TunaSweeperBossDefinition::MaxFileBytes) return false;
	FString Json;
	OutError = TEXT("ui.boss_lab.error.read");
	if (!FFileHelper::LoadFileToString(Json, *Path)) return false;
	return TunaSweeperBossDefinition::FromJson(Json, OutDefinition, OutError);
}

bool UTunaSweeperBossLabSubsystem::SaveSlot(int32 Index, const FTunaSweeperBossDefinition& Definition, FName& OutError)
{
	const FString Path = GetSlotPath(Index);
	OutError = TEXT("ui.boss_lab.error.slot");
	if (Path.IsEmpty()) return false;
	FString Json;
	if (!TunaSweeperBossDefinition::ToJson(Definition, Json, OutError)) return false;
	OutError = TEXT("ui.boss_lab.error.write");
	auto& Files = IFileManager::Get();
	if (!Files.MakeDirectory(*GetLibraryDirectory(), true)) return false;
	const FString Candidate = Path + TEXT(".candidate");
	const FString Previous = Path + TEXT(".previous");
	if (!FFileHelper::SaveStringToFile(Json, *Candidate, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
	FString VerifyJson; FTunaSweeperBossDefinition Verify; FName VerifyError;
	if (!FFileHelper::LoadFileToString(VerifyJson, *Candidate) || !TunaSweeperBossDefinition::FromJson(VerifyJson, Verify, VerifyError) || Verify.BossId != Definition.BossId)
	{
		Files.Delete(*Candidate); return false;
	}
	const bool bHadFile = Files.FileExists(*Path);
	if (bHadFile && Files.Copy(*Previous, *Path, true, true) != COPY_OK) { Files.Delete(*Candidate); return false; }
	if (!Files.Move(*Path, *Candidate, true, true))
	{
		if (bHadFile && !Files.FileExists(*Path)) Files.Copy(*Path, *Previous, true, true);
		Files.Delete(*Candidate); return false;
	}
	OutError = NAME_None;
	return true;
}

bool UTunaSweeperBossLabSubsystem::DeleteSlot(int32 Index, FName& OutError)
{
	const FString Path = GetSlotPath(Index);
	OutError = TEXT("ui.boss_lab.error.slot");
	if (Path.IsEmpty()) return false;
	OutError = TEXT("ui.boss_lab.error.delete");
	auto& Files = IFileManager::Get();
	if (!Files.Delete(*Path, false, false)) return false;
	Files.Delete(*(Path + TEXT(".previous")), false, false);
	Files.Delete(*(Path + TEXT(".candidate")), false, false);
	OutError = NAME_None;
	return true;
}

void UTunaSweeperBossLabSubsystem::SetDevelopmentMode(bool bDevelopment)
{
	if (bDevelopment == bDevelopmentMode) return;
	if (!bDevelopment)
	{
		DevelopmentDraft = Draft;
		DevelopmentSlot = SelectedSlot;
		bDevelopmentDirty = bDirty;
		bHasDevelopmentSnapshot = true;
		bDirty = false;
	}
	else if (bHasDevelopmentSnapshot)
	{
		Draft = DevelopmentDraft;
		SelectedSlot = DevelopmentSlot;
		bDirty = bDevelopmentDirty;
		bHasDevelopmentSnapshot = false;
	}
	bDevelopmentMode = bDevelopment;
}

void UTunaSweeperBossLabSubsystem::EnterLab(bool bDevelopment)
{
	SetDevelopmentMode(bDevelopment);
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/BossCombatTestMap")), true, TEXT("game=/Script/TunaSweeper.TunaSweeperBossLabGameMode"));
}
