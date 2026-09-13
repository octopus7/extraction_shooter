#include "Subsystem/TunaSweeperEffectCandidateSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaEffectCandidates, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarCollectEffectCandidates(
		TEXT("tuna.FX.CollectCandidates"), 0,
		TEXT("Record observed combat effect asset requests. No asset loading or warmup. 0=off, 1=on."));

	UTunaSweeperEffectCandidateSubsystem* FindCollector(const UWorld* World)
	{
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<UTunaSweeperEffectCandidateSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorld DumpCandidates(
		TEXT("tuna.FX.DumpCandidates"), TEXT("Export this game instance's observed candidates to Saved/EffectWarmup."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (const auto* Collector = FindCollector(World))
			{
				Collector->ExportCandidates();
			}
			else
			{
				UE_LOG(LogTunaEffectCandidates, Warning, TEXT("Start PIE or a game before exporting candidates."));
			}
		}));

	FAutoConsoleCommandWithWorld ResetCandidates(
		TEXT("tuna.FX.ResetCandidates"), TEXT("Clear this game instance's in-memory candidates."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (auto* Collector = FindCollector(World))
			{
				Collector->ResetCandidates();
			}
		}));
}

void UTunaSweeperEffectCandidateSubsystem::Record(
	const UObject* Context, const FSoftObjectPath& AssetPath, const TCHAR* Source)
{
	if (!IsInGameThread() || CVarCollectEffectCandidates.GetValueOnGameThread() == 0 || !Context)
	{
		return;
	}
	const UWorld* World = Context->GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (auto* Collector = FindCollector(World))
	{
		Collector->AddCandidate(AssetPath, Source);
	}
}

void UTunaSweeperEffectCandidateSubsystem::RecordObject(
	const UObject* Context, const UObject* Asset, const TCHAR* Source)
{
	if (!IsInGameThread() || CVarCollectEffectCandidates.GetValueOnGameThread() == 0)
	{
		return;
	}
	// Runtime MIDs cannot be preloaded by path. Keep their authored parent instead.
	while (const auto* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Asset))
	{
		Asset = DynamicMaterial->Parent;
	}
	if (Asset && Asset->IsAsset())
	{
		Record(Context, FSoftObjectPath(Asset), Source);
	}
}

void UTunaSweeperEffectCandidateSubsystem::AddCandidate(const FSoftObjectPath& AssetPath, const FString& Source)
{
	const FString PackageName = AssetPath.GetLongPackageName();
	if (!AssetPath.IsValid() || !AssetPath.GetSubPathString().IsEmpty() ||
		!FPackageName::IsValidLongPackageName(PackageName) || PackageName == TEXT("/Engine/Transient") ||
		PackageName.StartsWith(TEXT("/Temp/")) || PackageName.StartsWith(TEXT("/Memory/")) ||
		PackageName.StartsWith(TEXT("/Script/")))
	{
		return;
	}
	auto& Candidate = Candidates.FindOrAdd(AssetPath);
	++Candidate.RequestCount;
	Candidate.Sources.Add(Source);
}

FString UTunaSweeperEffectCandidateSubsystem::ToJson() const
{
	auto Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schemaVersion"), 1);
	Root->SetStringField(TEXT("collectionType"), TEXT("observed-asset-requests"));
	TArray<FSoftObjectPath> Paths;
	Candidates.GetKeys(Paths);
	Paths.Sort([](const FSoftObjectPath& A, const FSoftObjectPath& B) { return A.ToString() < B.ToString(); });
	TArray<TSharedPtr<FJsonValue>> Entries;
	for (const FSoftObjectPath& Path : Paths)
	{
		const auto& Candidate = Candidates.FindChecked(Path);
		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("assetPath"), Path.ToString());
		Entry->SetNumberField(TEXT("requestCount"), static_cast<double>(Candidate.RequestCount));
		TArray<FString> Sources = Candidate.Sources.Array();
		Sources.Sort();
		TArray<TSharedPtr<FJsonValue>> JsonSources;
		for (const FString& Source : Sources)
		{
			JsonSources.Add(MakeShared<FJsonValueString>(Source));
		}
		Entry->SetArrayField(TEXT("sources"), JsonSources);
		Entries.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Root->SetArrayField(TEXT("candidates"), Entries);
	FString Json;
	FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
	return Json;
}

bool UTunaSweeperEffectCandidateSubsystem::ExportCandidates() const
{
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("EffectWarmup"));
	const FString Path = FPaths::CreateTempFilename(*Directory, TEXT("Candidates-"), TEXT(".json"));
	if (!IFileManager::Get().MakeDirectory(*Directory, true) ||
		!FFileHelper::SaveStringToFile(ToJson(), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTunaEffectCandidates, Error, TEXT("Could not export candidates: %s"), *Path);
		return false;
	}
	UE_LOG(LogTunaEffectCandidates, Display, TEXT("Exported %d candidates: %s"), Candidates.Num(), *FPaths::ConvertRelativePathToFull(Path));
	return true;
}

void UTunaSweeperEffectCandidateSubsystem::Deinitialize()
{
	if (!Candidates.IsEmpty())
	{
		ExportCandidates();
	}
	Candidates.Reset();
	Super::Deinitialize();
}
