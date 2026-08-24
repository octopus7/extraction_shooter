#include "QuestDatasetSwitcher.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogQuestDatasetSwitcher, Log, All);

namespace QuestDatasetSwitcher
{
	const TCHAR* GeneratedDataRelativePath = TEXT("Data/QuestDatasetGenerated");
	const TCHAR* ActiveDatasetFileName = TEXT("active-dataset.json");
	const TCHAR* QuestDefinitionsFileName = TEXT("QuestDefinitions.json");
	const TCHAR* QuestTextStringsFileName = TEXT("QuestTextStrings.csv");

	bool ReadRequiredString(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		FString& OutValue)
	{
		return JsonObject.IsValid() &&
			JsonObject->TryGetStringField(FieldName, OutValue) &&
			!OutValue.TrimStartAndEnd().IsEmpty();
	}
}

FString FQuestDatasetDescriptor::GetSaveNamespace() const
{
	switch (Kind)
	{
	case EQuestDatasetKind::Production:
		return TEXT("Production");
	case EQuestDatasetKind::Public:
	default:
		return TEXT("Public");
	}
}

FString FQuestDatasetDescriptor::GetDisplayName() const
{
	switch (Kind)
	{
	case EQuestDatasetKind::Production:
		return TEXT("Production");
	case EQuestDatasetKind::Public:
	default:
		return TEXT("Public");
	}
}

FQuestDatasetSwitcherModule& FQuestDatasetSwitcherModule::Get()
{
	return FModuleManager::LoadModuleChecked<FQuestDatasetSwitcherModule>(TEXT("QuestDatasetSwitcher"));
}

bool FQuestDatasetSwitcherModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded(TEXT("QuestDatasetSwitcher"));
}

void FQuestDatasetSwitcherModule::StartupModule()
{
	ReloadActiveDataset();
}

void FQuestDatasetSwitcherModule::ShutdownModule()
{
}

const FQuestDatasetDescriptor& FQuestDatasetSwitcherModule::GetActiveDataset() const
{
	return ActiveDataset;
}

bool FQuestDatasetSwitcherModule::ReloadActiveDataset()
{
	FQuestDatasetDescriptor ProductionDataset;
	if (TryLoadProductionDataset(ProductionDataset))
	{
		ActiveDataset = MoveTemp(ProductionDataset);
		UE_LOG(
			LogQuestDatasetSwitcher,
			Display,
			TEXT("Active quest dataset: %s (%s, revision %s)"),
			*ActiveDataset.GetDisplayName(),
			*ActiveDataset.DatasetId.ToString(),
			*ActiveDataset.DatasetRevision);
		return true;
	}

	ActiveDataset = MakePublicDataset();
	UE_LOG(LogQuestDatasetSwitcher, Display, TEXT("Active quest dataset: Public"));
	return true;
}

FQuestDatasetDescriptor FQuestDatasetSwitcherModule::MakePublicDataset() const
{
	FQuestDatasetDescriptor Descriptor;
	Descriptor.Kind = EQuestDatasetKind::Public;
	Descriptor.DatasetId = TEXT("public");
	Descriptor.SaveCompatibilityId = TEXT("public_v1");
	Descriptor.DatasetRevision = TEXT("legacy");
	Descriptor.QuestDefinitionsPath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("Data/QuestDefinitions.json"));
	Descriptor.QuestTextStringsPath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("Data/QuestTextStrings.csv"));
	return Descriptor;
}

bool FQuestDatasetSwitcherModule::TryLoadProductionDataset(
	FQuestDatasetDescriptor& OutDescriptor) const
{
	const FString GeneratedDataDirectory = FPaths::Combine(
		FPaths::ProjectContentDir(),
		QuestDatasetSwitcher::GeneratedDataRelativePath);
	const FString ActiveDatasetPath = FPaths::Combine(
		GeneratedDataDirectory,
		QuestDatasetSwitcher::ActiveDatasetFileName);

	if (!FPaths::FileExists(ActiveDatasetPath))
	{
		return false;
	}

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *ActiveDatasetPath))
	{
		UE_LOG(
			LogQuestDatasetSwitcher,
			Error,
			TEXT("Could not read active quest dataset marker: %s"),
			*ActiveDatasetPath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogQuestDatasetSwitcher, Error, TEXT("Active quest dataset marker is invalid JSON."));
		return false;
	}

	FString DatasetId;
	FString DatasetRevision;
	FString SaveCompatibilityId;
	if (!QuestDatasetSwitcher::ReadRequiredString(JsonObject, TEXT("dataset_id"), DatasetId) ||
		!QuestDatasetSwitcher::ReadRequiredString(JsonObject, TEXT("dataset_revision"), DatasetRevision) ||
		!QuestDatasetSwitcher::ReadRequiredString(JsonObject, TEXT("save_compatibility_id"), SaveCompatibilityId))
	{
		UE_LOG(LogQuestDatasetSwitcher, Error, TEXT("Active quest dataset marker is missing required fields."));
		return false;
	}

	if (DatasetId == TEXT("production"))
	{
		OutDescriptor.Kind = EQuestDatasetKind::Production;
	}
	else
	{
		UE_LOG(
			LogQuestDatasetSwitcher,
			Error,
			TEXT("Unsupported production quest dataset id: %s"),
			*DatasetId);
		return false;
	}

	OutDescriptor.DatasetId = FName(*DatasetId);
	OutDescriptor.DatasetRevision = DatasetRevision;
	OutDescriptor.SaveCompatibilityId = FName(*SaveCompatibilityId);
	OutDescriptor.QuestDefinitionsPath = FPaths::Combine(
		GeneratedDataDirectory,
		QuestDatasetSwitcher::QuestDefinitionsFileName);
	OutDescriptor.QuestTextStringsPath = FPaths::Combine(
		GeneratedDataDirectory,
		QuestDatasetSwitcher::QuestTextStringsFileName);

	if (!FPaths::FileExists(OutDescriptor.QuestDefinitionsPath) ||
		!FPaths::FileExists(OutDescriptor.QuestTextStringsPath))
	{
		UE_LOG(
			LogQuestDatasetSwitcher,
			Error,
			TEXT("Production quest dataset is incomplete; falling back to Public."));
		return false;
	}

	return true;
}

IMPLEMENT_MODULE(FQuestDatasetSwitcherModule, QuestDatasetSwitcher)
