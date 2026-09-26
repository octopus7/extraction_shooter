#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BossLab/TunaSweeperBossDefinition.h"
#include "TunaSweeperBossLabSubsystem.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperBossLabSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	bool LoadSlot(int32 Index, FTunaSweeperBossDefinition& OutDefinition, FName& OutError) const;
	bool SaveSlot(int32 Index, const FTunaSweeperBossDefinition& Definition, FName& OutError);
	bool DeleteSlot(int32 Index, FName& OutError);
	bool SlotExists(int32 Index) const;
	FString GetSlotPath(int32 Index) const;
	FString GetLibraryDirectory() const;
	void EnterLab(bool bDevelopment);
	void SetDevelopmentMode(bool bDevelopment);
	FTunaSweeperBossDefinition& GetDraft() { return Draft; }
	const FTunaSweeperBossDefinition& GetDraft() const { return Draft; }
	void SetDraft(const FTunaSweeperBossDefinition& Definition) { Draft = Definition; }
	bool IsDevelopmentMode() const { return bDevelopmentMode; }
	int32 GetSelectedSlot() const { return SelectedSlot; }
	void SetSelectedSlot(int32 Index) { SelectedSlot = FMath::Clamp(Index, 0, TunaSweeperBossDefinition::SlotCount - 1); }
	bool IsDirty() const { return bDirty; }
	void SetDirty(bool bValue) { bDirty = bValue; }
	void SetLibraryDirectoryForTesting(const FString& Directory) { DirectoryOverride = Directory; }
private:
	FTunaSweeperBossDefinition Draft;
	FTunaSweeperBossDefinition DevelopmentDraft;
	FString DirectoryOverride;
	int32 SelectedSlot = 0;
	bool bDevelopmentMode = true;
	bool bDirty = false;
	bool bHasDevelopmentSnapshot = false;
	bool bDevelopmentDirty = false;
	int32 DevelopmentSlot = 0;
};
