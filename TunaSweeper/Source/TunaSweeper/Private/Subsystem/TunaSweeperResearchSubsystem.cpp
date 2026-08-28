#include "Subsystem/TunaSweeperResearchSubsystem.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperResearch, Log, All);

namespace
{
	ETunaSweeperResearchEffectType ParseEffectType(const FString& Value)
	{
		if (Value.Equals(TEXT("max_food"), ESearchCase::IgnoreCase)) return ETunaSweeperResearchEffectType::MaxFood;
		if (Value.Equals(TEXT("max_hydration"), ESearchCase::IgnoreCase)) return ETunaSweeperResearchEffectType::MaxHydration;
		if (Value.Equals(TEXT("max_stamina"), ESearchCase::IgnoreCase)) return ETunaSweeperResearchEffectType::MaxStamina;
		if (Value.Equals(TEXT("carry_strength"), ESearchCase::IgnoreCase)) return ETunaSweeperResearchEffectType::CarryStrength;
		return ETunaSweeperResearchEffectType::MaxHealth;
	}
}

void UTunaSweeperResearchSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SessionStartPlatformSeconds = FPlatformTime::Seconds();
	SessionStartUtcTicks = FDateTime::UtcNow().GetTicks();
	LastObservedUtcTicks = SessionStartUtcTicks;
	LoadResearchData(false);
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UTunaSweeperResearchSubsystem::TickResearch), 0.25f);
}

void UTunaSweeperResearchSubsystem::Deinitialize()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Super::Deinitialize();
}

bool UTunaSweeperResearchSubsystem::LoadResearchData(bool bForceReload)
{
	if (bResearchDataLoaded && !bForceReload) return true;
	Definitions.Reset();
	FString JsonText;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/StatResearchNodes.json"));
	if (!FFileHelper::LoadFileToString(JsonText, *Path))
	{
		UE_LOG(LogTunaSweeperResearch, Error, TEXT("Could not load research data: %s"), *Path);
		bResearchDataLoaded = false;
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Values;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Values))
	{
		UE_LOG(LogTunaSweeperResearch, Error, TEXT("Could not parse research data JSON: %s"), *Path);
		bResearchDataLoaded = false;
		return false;
	}
	TMap<int32, int32> NodesPerRow;
	for (const TSharedPtr<FJsonValue>& Value : Values)
	{
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid()) continue;
		FTunaSweeperResearchNodeDefinition Definition;
		Definition.NodeId = FName(*Object->GetStringField(TEXT("node_id")));
		double Number = 0.0;
		if (Object->TryGetNumberField(TEXT("row"), Number)) Definition.Row = FMath::RoundToInt(Number);
		if (Object->TryGetNumberField(TEXT("column"), Number)) Definition.Column = FMath::RoundToInt(Number);
		if (Object->TryGetNumberField(TEXT("required_applied_node_count"), Number)) Definition.RequiredAppliedNodeCount = FMath::Max(0, FMath::RoundToInt(Number));
		if (Object->TryGetNumberField(TEXT("duration_seconds"), Number)) Definition.DurationSeconds = FMath::Clamp(FMath::RoundToInt(Number), 1, 3600);
		Object->TryGetStringField(TEXT("display_name_ko"), Definition.DisplayNameKo);
		Object->TryGetStringField(TEXT("display_name_en"), Definition.DisplayNameEn);
		Object->TryGetStringField(TEXT("description_ko"), Definition.DescriptionKo);
		Object->TryGetStringField(TEXT("description_en"), Definition.DescriptionEn);
		FString IconPath;
		if (Object->TryGetStringField(TEXT("icon"), IconPath)) Definition.Icon = FSoftObjectPath(IconPath);
		const TArray<TSharedPtr<FJsonValue>>* ParentValues = nullptr;
		if (Object->TryGetArrayField(TEXT("parent_node_ids"), ParentValues))
		{
			for (const TSharedPtr<FJsonValue>& ParentValue : *ParentValues) Definition.ParentNodeIds.Add(FName(*ParentValue->AsString()));
		}
		const TArray<TSharedPtr<FJsonValue>>* EffectValues = nullptr;
		if (Object->TryGetArrayField(TEXT("effects"), EffectValues))
		{
			for (const TSharedPtr<FJsonValue>& EffectValue : *EffectValues)
			{
				const TSharedPtr<FJsonObject> EffectObject = EffectValue.IsValid() ? EffectValue->AsObject() : nullptr;
				if (!EffectObject.IsValid()) continue;
				FTunaSweeperResearchEffect Effect;
				Effect.Type = ParseEffectType(EffectObject->GetStringField(TEXT("type")));
				double EffectValueNumber = 0.0;
				EffectObject->TryGetNumberField(TEXT("value"), EffectValueNumber);
				Effect.Value = static_cast<float>(EffectValueNumber);
				Definition.Effects.Add(Effect);
			}
		}
		if (Definition.NodeId.IsNone() || Definitions.Contains(Definition.NodeId)) continue;
		int32& RowCount = NodesPerRow.FindOrAdd(Definition.Row);
		if (++RowCount > 3)
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research row %d has more than three nodes."), Definition.Row);
			Definitions.Reset();
			bResearchDataLoaded = false;
			return false;
		}
		Definitions.Add(Definition.NodeId, MoveTemp(Definition));
	}
	bResearchDataLoaded = Definitions.Num() > 0;
	return bResearchDataLoaded;
}

bool UTunaSweeperResearchSubsystem::EnsureResearchDataLoaded() const
{
	return bResearchDataLoaded || const_cast<UTunaSweeperResearchSubsystem*>(this)->LoadResearchData(false);
}

ETunaSweeperResearchNodeState UTunaSweeperResearchSubsystem::EvaluateNodeState(const FTunaSweeperResearchNodeDefinition& Definition, const FTunaSweeperActiveResearchSaveData** OutActive) const
{
	if (AppliedNodeIds.Contains(Definition.NodeId)) return ETunaSweeperResearchNodeState::Applied;
	for (const FTunaSweeperActiveResearchSaveData& Active : ActiveResearch)
	{
		if (Active.NodeId == Definition.NodeId)
		{
			if (OutActive) *OutActive = &Active;
			return Active.bTimerCompleted || GetEffectiveUtcTicks() >= Active.FinishUtcTicks
				? ETunaSweeperResearchNodeState::ReadyToClaim : ETunaSweeperResearchNodeState::Researching;
		}
	}
	return AppliedNodeIds.Num() >= Definition.RequiredAppliedNodeCount
		? ETunaSweeperResearchNodeState::Available : ETunaSweeperResearchNodeState::Locked;
}

bool UTunaSweeperResearchSubsystem::GetNodeView(FName NodeId, FTunaSweeperResearchNodeView& OutView) const
{
	if (!EnsureResearchDataLoaded()) return false;
	const FTunaSweeperResearchNodeDefinition* Definition = Definitions.Find(NodeId);
	if (!Definition) return false;
	const FTunaSweeperActiveResearchSaveData* Active = nullptr;
	OutView = FTunaSweeperResearchNodeView();
	OutView.NodeId = NodeId;
	OutView.DisplayName = ResolveLocalizedText(Definition->DisplayNameKo, Definition->DisplayNameEn);
	OutView.Description = ResolveLocalizedText(Definition->DescriptionKo, Definition->DescriptionEn);
	OutView.Row = Definition->Row;
	OutView.Column = Definition->Column;
	OutView.RequiredAppliedNodeCount = Definition->RequiredAppliedNodeCount;
	OutView.DurationSeconds = Definition->DurationSeconds;
	OutView.State = EvaluateNodeState(*Definition, &Active);
	if (Active)
	{
		const int64 RemainingTicks = FMath::Max<int64>(0, Active->FinishUtcTicks - GetEffectiveUtcTicks());
		OutView.RemainingSeconds = FMath::CeilToInt(static_cast<double>(RemainingTicks) / ETimespan::TicksPerSecond);
		OutView.Progress = 1.0f - FMath::Clamp(static_cast<float>(OutView.RemainingSeconds) / FMath::Max(1, Definition->DurationSeconds), 0.0f, 1.0f);
	}
	return true;
}

bool UTunaSweeperResearchSubsystem::GetAllNodeViews(TArray<FTunaSweeperResearchNodeView>& OutViews) const
{
	OutViews.Reset();
	if (!EnsureResearchDataLoaded()) return false;
	for (const TPair<FName, FTunaSweeperResearchNodeDefinition>& Pair : Definitions)
	{
		FTunaSweeperResearchNodeView View;
		if (GetNodeView(Pair.Key, View)) OutViews.Add(MoveTemp(View));
	}
	OutViews.Sort([](const FTunaSweeperResearchNodeView& A, const FTunaSweeperResearchNodeView& B)
	{
		return A.Row != B.Row ? A.Row < B.Row : A.Column < B.Column;
	});
	return true;
}

bool UTunaSweeperResearchSubsystem::TryStartResearch(FName NodeId)
{
	EnsureSaveStateLoaded();
	const FTunaSweeperResearchNodeDefinition* Definition = EnsureResearchDataLoaded() ? Definitions.Find(NodeId) : nullptr;
	if (!Definition || EvaluateNodeState(*Definition) != ETunaSweeperResearchNodeState::Available) return false;
	const int64 NowTicks = GetEffectiveUtcTicks();
	FTunaSweeperActiveResearchSaveData Active;
	Active.NodeId = NodeId;
	Active.StartUtcTicks = NowTicks;
	Active.FinishUtcTicks = NowTicks + static_cast<int64>(Definition->DurationSeconds) * ETimespan::TicksPerSecond;
	ActiveResearch.Add(Active);
	LastObservedUtcTicks = NowTicks;
	OnResearchStateChanged.Broadcast();
	RequestSaveGameState();
	return true;
}

bool UTunaSweeperResearchSubsystem::TryClaimResearch(FName NodeId)
{
	EnsureSaveStateLoaded();
	RefreshTemporalState(true);
	const int32 Index = ActiveResearch.IndexOfByPredicate([NodeId](const FTunaSweeperActiveResearchSaveData& Active) { return Active.NodeId == NodeId && Active.bTimerCompleted; });
	if (Index == INDEX_NONE) return false;
	ActiveResearch.RemoveAt(Index);
	AppliedNodeIds.Add(NodeId);
	LastObservedUtcTicks = GetEffectiveUtcTicks();
	OnResearchEffectsChanged.Broadcast();
	OnResearchStateChanged.Broadcast();
	RequestSaveGameState();
	return true;
}

FTunaSweeperResearchStatBonuses UTunaSweeperResearchSubsystem::GetAppliedStatBonuses() const
{
	FTunaSweeperResearchStatBonuses Result;
	if (!EnsureResearchDataLoaded()) return Result;
	for (const FName& NodeId : AppliedNodeIds)
	{
		const FTunaSweeperResearchNodeDefinition* Definition = Definitions.Find(NodeId);
		if (!Definition) continue;
		for (const FTunaSweeperResearchEffect& Effect : Definition->Effects)
		{
			switch (Effect.Type)
			{
			case ETunaSweeperResearchEffectType::MaxHealth: Result.MaxHealth += Effect.Value; break;
			case ETunaSweeperResearchEffectType::MaxFood: Result.MaxFood += Effect.Value; break;
			case ETunaSweeperResearchEffectType::MaxHydration: Result.MaxHydration += Effect.Value; break;
			case ETunaSweeperResearchEffectType::MaxStamina: Result.MaxStamina += Effect.Value; break;
			case ETunaSweeperResearchEffectType::CarryStrength: Result.CarryStrength += Effect.Value; break;
			}
		}
	}
	return Result;
}

void UTunaSweeperResearchSubsystem::ExportResearchProgressForSave(TArray<FName>& OutAppliedNodeIds, TArray<FTunaSweeperActiveResearchSaveData>& OutActiveResearch, int64& OutLastObservedUtcTicks) const
{
	OutAppliedNodeIds = AppliedNodeIds.Array();
	OutAppliedNodeIds.Sort(FNameLexicalLess());
	OutActiveResearch = ActiveResearch;
	OutActiveResearch.Sort([](const FTunaSweeperActiveResearchSaveData& A, const FTunaSweeperActiveResearchSaveData& B) { return A.NodeId.LexicalLess(B.NodeId); });
	OutLastObservedUtcTicks = FMath::Max(LastObservedUtcTicks, GetEffectiveUtcTicks());
}

void UTunaSweeperResearchSubsystem::LoadResearchProgressFromSave(const TArray<FName>& SavedAppliedNodeIds, const TArray<FTunaSweeperActiveResearchSaveData>& SavedActiveResearch, int64 SavedLastObservedUtcTicks)
{
	LoadResearchData(false);
	AppliedNodeIds.Reset();
	ActiveResearch.Reset();
	for (const FName& NodeId : SavedAppliedNodeIds) if (Definitions.Contains(NodeId)) AppliedNodeIds.Add(NodeId);
	TSet<FName> Seen;
	for (const FTunaSweeperActiveResearchSaveData& Saved : SavedActiveResearch)
	{
		if (!Definitions.Contains(Saved.NodeId) || AppliedNodeIds.Contains(Saved.NodeId) || Seen.Contains(Saved.NodeId)) continue;
		FTunaSweeperActiveResearchSaveData Active = Saved;
		Active.StartUtcTicks = FMath::Max<int64>(0, Active.StartUtcTicks);
		Active.FinishUtcTicks = FMath::Max(Active.StartUtcTicks, Active.FinishUtcTicks);
		ActiveResearch.Add(Active);
		Seen.Add(Active.NodeId);
	}
	LastObservedUtcTicks = FMath::Max(FDateTime::UtcNow().GetTicks(), SavedLastObservedUtcTicks);
	SessionStartPlatformSeconds = FPlatformTime::Seconds();
	SessionStartUtcTicks = LastObservedUtcTicks;
	bProgressLoaded = true;
	OnResearchEffectsChanged.Broadcast();
	OnResearchStateChanged.Broadcast();
}

void UTunaSweeperResearchSubsystem::ResetResearchProgressForNewGame()
{
	AppliedNodeIds.Reset();
	ActiveResearch.Reset();
	LastObservedUtcTicks = FDateTime::UtcNow().GetTicks();
	bProgressLoaded = true;
	OnResearchEffectsChanged.Broadcast();
	OnResearchStateChanged.Broadcast();
}

bool UTunaSweeperResearchSubsystem::TickResearch(float DeltaSeconds)
{
	if (bProgressLoaded) RefreshTemporalState(true);
	return true;
}

void UTunaSweeperResearchSubsystem::RefreshTemporalState(bool bSaveIfChanged)
{
	const int64 NowTicks = GetEffectiveUtcTicks();
	LastObservedUtcTicks = FMath::Max(LastObservedUtcTicks, NowTicks);
	bool bChanged = false;
	for (FTunaSweeperActiveResearchSaveData& Active : ActiveResearch)
	{
		if (!Active.bTimerCompleted && NowTicks >= Active.FinishUtcTicks)
		{
			Active.bTimerCompleted = true;
			bChanged = true;
		}
	}
	if (bChanged)
	{
		OnResearchStateChanged.Broadcast();
		if (bSaveIfChanged) RequestSaveGameState();
	}
}

int64 UTunaSweeperResearchSubsystem::GetEffectiveUtcTicks() const
{
	const int64 WallTicks = FDateTime::UtcNow().GetTicks();
	const int64 MonotonicTicks = SessionStartUtcTicks + static_cast<int64>((FPlatformTime::Seconds() - SessionStartPlatformSeconds) * ETimespan::TicksPerSecond);
	return FMath::Max3(WallTicks, MonotonicTicks, LastObservedUtcTicks);
}

void UTunaSweeperResearchSubsystem::EnsureSaveStateLoaded() const
{
	if (UTunaSweeperGameInstance* GameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance())) GameInstance->GetInventorySlots();
}

void UTunaSweeperResearchSubsystem::RequestSaveGameState() const
{
	if (UTunaSweeperGameInstance* GameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance())) GameInstance->SaveGameState();
}

FText UTunaSweeperResearchSubsystem::ResolveLocalizedText(const FString& Korean, const FString& English) const
{
	const FString Language = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();
	return FText::FromString(Language.Equals(TEXT("ko"), ESearchCase::IgnoreCase) || English.IsEmpty() ? Korean : English);
}
