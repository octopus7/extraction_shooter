#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

enum class EQuestDatasetKind : uint8
{
	Public,
	ProductionDemo,
	ProductionRelease
};

struct QUESTDATASETSWITCHER_API FQuestDatasetDescriptor
{
	EQuestDatasetKind Kind = EQuestDatasetKind::Public;
	FName DatasetId = TEXT("public");
	FName SaveCompatibilityId = TEXT("public_v1");
	FString DatasetRevision = TEXT("legacy");
	FString QuestDefinitionsPath;
	FString QuestTextStringsPath;

	bool IsPublic() const
	{
		return Kind == EQuestDatasetKind::Public;
	}

	FString GetSaveNamespace() const;
	FString GetDisplayName() const;
};

class QUESTDATASETSWITCHER_API FQuestDatasetSwitcherModule final : public IModuleInterface
{
public:
	static FQuestDatasetSwitcherModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	const FQuestDatasetDescriptor& GetActiveDataset() const;
	bool ReloadActiveDataset();

private:
	FQuestDatasetDescriptor MakePublicDataset() const;
	bool TryLoadProductionDataset(FQuestDatasetDescriptor& OutDescriptor) const;

	FQuestDatasetDescriptor ActiveDataset;
};
