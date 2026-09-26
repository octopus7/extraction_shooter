#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "Combat/TunaSweeperBurnTypes.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperResearch, Log, All);

namespace
{
	bool ParseEffectType(const FString& Value, ETunaSweeperResearchEffectType& OutType)
	{
		if (Value.Equals(TEXT("max_health"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::MaxHealth; return true; }
		if (Value.Equals(TEXT("max_food"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::MaxFood; return true; }
		if (Value.Equals(TEXT("max_hydration"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::MaxHydration; return true; }
		if (Value.Equals(TEXT("max_stamina"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::MaxStamina; return true; }
		if (Value.Equals(TEXT("carry_strength"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::CarryStrength; return true; }
		if (Value.Equals(TEXT("weapon_burn_ticks"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::WeaponBurnTicks; return true; }
		if (Value.Equals(TEXT("ammo_burn_ticks"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::AmmoBurnTicks; return true; }
		if (Value.Equals(TEXT("weapon_burn_damage"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::WeaponBurnDamage; return true; }
		if (Value.Equals(TEXT("ammo_burn_damage"), ESearchCase::IgnoreCase)) { OutType = ETunaSweeperResearchEffectType::AmmoBurnDamage; return true; }
		return false;
	}

	bool TryGetIntegerField(const FJsonObject& Object, const TCHAR* Field, int32 Minimum, int32 Maximum, int32& OutValue)
	{
		double Number = 0.0;
		if (!Object.TryGetNumberField(Field, Number) || !FMath::IsFinite(Number) ||
			Number < Minimum || Number > Maximum || FMath::FloorToDouble(Number) != Number)
		{
			return false;
		}
		OutValue = static_cast<int32>(Number);
		return true;
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
	bResearchEffectsNotificationPending = false;
	bResearchStateNotificationPending = false;
	Super::Deinitialize();
}

bool UTunaSweeperResearchSubsystem::LoadResearchData(bool bForceReload)
{
	if (bResearchDataLoaded && !bForceReload) return true;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/StatResearchNodes.json"));
	return LoadResearchDataFromFile(Path);
}

bool UTunaSweeperResearchSubsystem::LoadResearchDataFromFile(const FString& Path)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *Path))
	{
		UE_LOG(LogTunaSweeperResearch, Error, TEXT("Could not load research data: %s"), *Path);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Values;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Values) || Values.IsEmpty())
	{
		UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research data must be a nonempty JSON array: %s"), *Path);
		return false;
	}
	TMap<FName, FTunaSweeperResearchNodeDefinition> ParsedDefinitions;
	TSet<FIntPoint> OccupiedPositions;
	TMap<int32, int32> NodesPerRow;
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		const TSharedPtr<FJsonValue>& Value = Values[Index];
		const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!Object.IsValid())
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research entry %d is not an object: %s"), Index, *Path);
			return false;
		}
		FTunaSweeperResearchNodeDefinition Definition;
		FString NodeIdString;
		if (!Object->TryGetStringField(TEXT("node_id"), NodeIdString)) return false;
		Definition.NodeId = FName(*NodeIdString.TrimStartAndEnd());
		if (Definition.NodeId.IsNone() || ParsedDefinitions.Contains(Definition.NodeId) ||
			!TryGetIntegerField(*Object, TEXT("row"), 0, MAX_int32, Definition.Row) ||
			!TryGetIntegerField(*Object, TEXT("column"), 0, 2, Definition.Column) ||
			!TryGetIntegerField(*Object, TEXT("required_applied_node_count"), 0, Values.Num() - 1, Definition.RequiredAppliedNodeCount) ||
			!TryGetIntegerField(*Object, TEXT("duration_seconds"), 1, 3600, Definition.DurationSeconds) ||
			OccupiedPositions.Contains(FIntPoint(Definition.Row, Definition.Column)))
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research entry %d has an invalid ID, position, prerequisite count, or duration: %s"), Index, *Path);
			return false;
		}
		FString DisplayNameStringKey;
		FString DescriptionStringKey;
		if (!Object->TryGetStringField(TEXT("display_name_string_key"), DisplayNameStringKey) ||
			!Object->TryGetStringField(TEXT("description_string_key"), DescriptionStringKey)) return false;
		Definition.DisplayNameStringKey = FName(*DisplayNameStringKey.TrimStartAndEnd());
		Definition.DescriptionStringKey = FName(*DescriptionStringKey.TrimStartAndEnd());
		if (Definition.DisplayNameStringKey.IsNone() || Definition.DescriptionStringKey.IsNone())
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research node %s has an empty display-name or description string key."), *Definition.NodeId.ToString());
			return false;
		}
		FString IconPath;
		if (Object->TryGetStringField(TEXT("icon"), IconPath)) Definition.Icon = FSoftObjectPath(IconPath.TrimStartAndEnd());
		const TArray<TSharedPtr<FJsonValue>>* ParentValues = nullptr;
		if (Object->HasField(TEXT("parent_node_ids")) && !Object->TryGetArrayField(TEXT("parent_node_ids"), ParentValues)) return false;
		if (ParentValues)
		{
			for (const TSharedPtr<FJsonValue>& ParentValue : *ParentValues)
			{
				FString ParentId;
				if (!ParentValue.IsValid() || !ParentValue->TryGetString(ParentId) || ParentId.TrimStartAndEnd().IsEmpty()) return false;
				Definition.ParentNodeIds.Add(FName(*ParentId));
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* EffectValues = nullptr;
		if (!Object->TryGetArrayField(TEXT("effects"), EffectValues) || !EffectValues || EffectValues->IsEmpty()) return false;
		for (const TSharedPtr<FJsonValue>& EffectValue : *EffectValues)
		{
			const TSharedPtr<FJsonObject> EffectObject = EffectValue.IsValid() ? EffectValue->AsObject() : nullptr;
			FString EffectTypeName;
			double EffectValueNumber = 0.0;
			FTunaSweeperResearchEffect Effect;
			if (!EffectObject.IsValid() || !EffectObject->TryGetStringField(TEXT("type"), EffectTypeName) ||
				!ParseEffectType(EffectTypeName, Effect.Type) ||
				!EffectObject->TryGetNumberField(TEXT("value"), EffectValueNumber) ||
				!FMath::IsFinite(EffectValueNumber) || EffectValueNumber < 0.0 ||
				!FMath::IsFinite(static_cast<float>(EffectValueNumber)))
			{
				UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research node %s has an invalid effect: %s"), *Definition.NodeId.ToString(), *Path);
				return false;
			}
			Effect.Value = static_cast<float>(EffectValueNumber);
			FString TargetTypeTag;
			if (EffectObject->TryGetStringField(TEXT("target_type_tag"), TargetTypeTag))
			{
				Effect.TargetTypeTag = FName(*TargetTypeTag.TrimStartAndEnd());
			}
			Definition.Effects.Add(Effect);
		}
		int32& RowCount = NodesPerRow.FindOrAdd(Definition.Row);
		if (++RowCount > 3)
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research row %d has more than three nodes."), Definition.Row);
			return false;
		}
		OccupiedPositions.Add(FIntPoint(Definition.Row, Definition.Column));
		ParsedDefinitions.Add(Definition.NodeId, MoveTemp(Definition));
	}
	TArray<const FTunaSweeperResearchNodeDefinition*> OrderedNodes;
	for (const TPair<FName, FTunaSweeperResearchNodeDefinition>& Pair : ParsedDefinitions)
	{
		OrderedNodes.Add(&Pair.Value);
		for (const FName& ParentId : Pair.Value.ParentNodeIds)
		{
			if (ParentId == Pair.Key || !ParsedDefinitions.Contains(ParentId))
			{
				UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research node %s references an invalid parent node."), *Pair.Key.ToString());
				return false;
			}
		}
	}
	OrderedNodes.Sort([](const FTunaSweeperResearchNodeDefinition& A, const FTunaSweeperResearchNodeDefinition& B)
	{
		return A.RequiredAppliedNodeCount < B.RequiredAppliedNodeCount;
	});
	for (int32 AvailableClaims = 0; AvailableClaims < OrderedNodes.Num(); ++AvailableClaims)
	{
		if (OrderedNodes[AvailableClaims]->RequiredAppliedNodeCount > AvailableClaims)
		{
			UE_LOG(LogTunaSweeperResearch, Error, TEXT("Research data contains unreachable required node counts: %s"), *Path);
			return false;
		}
	}
	Definitions = MoveTemp(ParsedDefinitions);
	bResearchDataLoaded = true;
	return true;
}

bool UTunaSweeperResearchSubsystem::EnsureResearchDataLoaded() const
{
	return bResearchDataLoaded || const_cast<UTunaSweeperResearchSubsystem*>(this)->LoadResearchData(false);
}

int32 UTunaSweeperResearchSubsystem::GetKnownAppliedNodeCount() const
{
	int32 Count = 0;
	for (const FName& NodeId : AppliedNodeIds)
	{
		if (Definitions.Contains(NodeId)) ++Count;
	}
	return Count;
}

int32 UTunaSweeperResearchSubsystem::GetAppliedNodeCount() const
{
	if (!EnsureResearchDataLoaded()) return 0;
	return GetKnownAppliedNodeCount();
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
	return GetKnownAppliedNodeCount() >= Definition.RequiredAppliedNodeCount
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
	OutView.DisplayName = ResolveLocalizedText(Definition->DisplayNameStringKey);
	OutView.Description = ResolveLocalizedText(Definition->DescriptionStringKey);
	OutView.Icon = Definition->Icon;
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
	if (!EnsureResearchDataLoaded() || !Definitions.Contains(NodeId)) return false;
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
			default: break;
			}
		}
	}
	return Result;
}

FTunaSweeperResearchBurnBonuses UTunaSweeperResearchSubsystem::GetAppliedBurnBonuses(FName WeaponTypeTag, FName AmmoTypeTag) const
{
	EnsureSaveStateLoaded();
	FTunaSweeperResearchBurnBonuses Result;
	if (!EnsureResearchDataLoaded()) return Result;
	for (const FName& NodeId : AppliedNodeIds)
	{
		const FTunaSweeperResearchNodeDefinition* Definition = Definitions.Find(NodeId);
		if (!Definition) continue;
		for (const FTunaSweeperResearchEffect& Effect : Definition->Effects)
		{
			const bool bWeaponEffect = Effect.Type == ETunaSweeperResearchEffectType::WeaponBurnTicks ||
				Effect.Type == ETunaSweeperResearchEffectType::WeaponBurnDamage;
			const bool bAmmoEffect = Effect.Type == ETunaSweeperResearchEffectType::AmmoBurnTicks ||
				Effect.Type == ETunaSweeperResearchEffectType::AmmoBurnDamage;
			if ((!bWeaponEffect && !bAmmoEffect) || !FMath::IsFinite(Effect.Value)) continue;
			const FName SourceTypeTag = bWeaponEffect ? WeaponTypeTag : AmmoTypeTag;
			if (SourceTypeTag.IsNone() || (!Effect.TargetTypeTag.IsNone() && Effect.TargetTypeTag != SourceTypeTag)) continue;
			if (Effect.Type == ETunaSweeperResearchEffectType::WeaponBurnTicks || Effect.Type == ETunaSweeperResearchEffectType::AmmoBurnTicks)
			{
				Result.AdditionalTickCount = FMath::Min(FTunaSweeperBurnSpec::MaxTickCount, Result.AdditionalTickCount + FMath::RoundToInt(FMath::Clamp(Effect.Value, 0.0f, static_cast<float>(FTunaSweeperBurnSpec::MaxTickCount))));
			}
			else
			{
				// Authored damage bonuses use the shared integer ratio convention: 2500 adds 0.25x.
				const int32 RatioBonus = FMath::RoundToInt(FMath::Clamp(Effect.Value, 0.0f, 1000000.0f));
				Result.DamageMultiplier = FMath::Min(100.0f, Result.DamageMultiplier + TunaSweeperDataValues::ToRatioFloat(RatioBonus));
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

void UTunaSweeperResearchSubsystem::LoadResearchProgressFromSave(
	const TArray<FName>& SavedAppliedNodeIds,
	const TArray<FTunaSweeperActiveResearchSaveData>& SavedActiveResearch,
	int64 SavedLastObservedUtcTicks,
	ETunaSweeperResearchNotificationMode NotificationMode)
{
	LoadResearchData(false);
	AppliedNodeIds.Reset();
	ActiveResearch.Reset();
	for (const FName& NodeId : SavedAppliedNodeIds) if (!NodeId.IsNone()) AppliedNodeIds.Add(NodeId);
	TSet<FName> Seen;
	for (const FTunaSweeperActiveResearchSaveData& Saved : SavedActiveResearch)
	{
		if (Saved.NodeId.IsNone() || AppliedNodeIds.Contains(Saved.NodeId) || Seen.Contains(Saved.NodeId)) continue;
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
	NotifyResearchProgressChanged(NotificationMode);
}

void UTunaSweeperResearchSubsystem::ResetResearchProgressForNewGame(
	ETunaSweeperResearchNotificationMode NotificationMode)
{
	AppliedNodeIds.Reset();
	ActiveResearch.Reset();
	SessionStartPlatformSeconds = FPlatformTime::Seconds();
	SessionStartUtcTicks = FDateTime::UtcNow().GetTicks();
	LastObservedUtcTicks = SessionStartUtcTicks;
	bProgressLoaded = true;
	NotifyResearchProgressChanged(NotificationMode);
}

void UTunaSweeperResearchSubsystem::NotifyResearchProgressChanged(
	ETunaSweeperResearchNotificationMode NotificationMode)
{
	bResearchEffectsNotificationPending = true;
	bResearchStateNotificationPending = true;
	if (NotificationMode == ETunaSweeperResearchNotificationMode::Immediate)
	{
		FlushDeferredResearchNotifications();
	}
}

void UTunaSweeperResearchSubsystem::FlushDeferredResearchNotifications()
{
	const bool bBroadcastEffectsChanged = bResearchEffectsNotificationPending;
	const bool bBroadcastStateChanged = bResearchStateNotificationPending;
	bResearchEffectsNotificationPending = false;
	bResearchStateNotificationPending = false;

	if (bBroadcastEffectsChanged)
	{
		OnResearchEffectsChanged.Broadcast();
	}
	if (bBroadcastStateChanged)
	{
		OnResearchStateChanged.Broadcast();
	}
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

FText UTunaSweeperResearchSubsystem::ResolveLocalizedText(FName StringKey) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(StringKey, FText::GetEmpty())
		: FText::FromName(StringKey);
}
