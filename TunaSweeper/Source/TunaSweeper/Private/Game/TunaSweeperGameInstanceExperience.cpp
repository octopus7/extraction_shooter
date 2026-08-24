#include "TunaSweeperGameInstanceShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"

int32 UTunaSweeperGameInstance::GetCurrentExperienceLevel() const
{
	return GetExperienceLevelForTotal(TotalExperiencePoints);
}

int32 UTunaSweeperGameInstance::GetMaxExperienceLevel() const
{
	EnsureExperienceLevelTableLoaded();
	return FMath::Max(1, CachedExperienceForLevels.Num());
}

int32 UTunaSweeperGameInstance::GetExperienceLevelForTotal(int64 ExperiencePoints) const
{
	EnsureExperienceLevelTableLoaded();

	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	int32 Level = 1;
	for (int32 LevelIndex = 1; LevelIndex < CachedExperienceForLevels.Num(); ++LevelIndex)
	{
		if (ClampedExperience < CachedExperienceForLevels[LevelIndex])
		{
			break;
		}

		Level = LevelIndex + 1;
	}

	return Level;
}

int64 UTunaSweeperGameInstance::GetExperienceForLevel(int32 Level) const
{
	EnsureExperienceLevelTableLoaded();

	if (CachedExperienceForLevels.Num() <= 0)
	{
		return 0;
	}

	const int32 LevelIndex = FMath::Clamp(Level, 1, CachedExperienceForLevels.Num()) - 1;
	return CachedExperienceForLevels[LevelIndex];
}

int64 UTunaSweeperGameInstance::GetExperienceForNextLevel(int32 Level) const
{
	EnsureExperienceLevelTableLoaded();

	const int32 SafeLevel = FMath::Clamp(Level, 1, FMath::Max(1, CachedExperienceForLevels.Num()));
	if (SafeLevel >= CachedExperienceForLevels.Num())
	{
		return 0;
	}

	return FMath::Max<int64>(
		0,
		CachedExperienceForLevels[SafeLevel] - CachedExperienceForLevels[SafeLevel - 1]);
}

float UTunaSweeperGameInstance::GetExperienceProgressForTotal(int64 ExperiencePoints) const
{
	const int64 ClampedExperience = FMath::Max<int64>(0, ExperiencePoints);
	const int32 Level = GetExperienceLevelForTotal(ClampedExperience);
	if (Level >= GetMaxExperienceLevel())
	{
		return 1.0f;
	}

	const int64 LevelStartExperience = GetExperienceForLevel(Level);
	const int64 NextLevelExperience = GetExperienceForLevel(Level + 1);
	const int64 LevelSpan = FMath::Max<int64>(1, NextLevelExperience - LevelStartExperience);
	return static_cast<float>(ClampedExperience - LevelStartExperience) / static_cast<float>(LevelSpan);
}

FTunaSweeperExperienceLevelStatBonuses UTunaSweeperGameInstance::GetExperienceLevelStatBonuses(int32 Level) const
{
	EnsureExperienceLevelRewardsLoaded();

	FTunaSweeperExperienceLevelStatBonuses Bonuses;
	const int32 TargetLevel = FMath::Max(1, Level);
	for (const FTunaSweeperExperienceLevelReward& Reward : CachedExperienceLevelRewards)
	{
		if (Reward.Level > TargetLevel)
		{
			break;
		}

		Bonuses.MaxHealthBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxHealthIncrease);
		Bonuses.MaxFoodBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxFoodIncrease);
		Bonuses.MaxHydrationBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxHydrationIncrease);
		Bonuses.MaxStaminaBonus += TunaSweeperDataValues::ToRatioFloat(Reward.MaxStaminaIncrease);
		Bonuses.CarryStrengthBonus += FMath::Max(0.0f, Reward.CarryStrengthIncrease);
	}

	Bonuses.ClampNonNegative();
	return Bonuses;
}

FTunaSweeperExperienceLevelStatBonuses UTunaSweeperGameInstance::GetCurrentExperienceLevelStatBonuses() const
{
	return GetExperienceLevelStatBonuses(GetCurrentExperienceLevel());
}

void UTunaSweeperGameInstance::BeginRaidExperienceSession()
{
	EnsureInventoryStateInitialized();
	RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	PendingRaidExperiencePoints = 0;
	bRaidExperienceSessionActive = true;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
}

int32 UTunaSweeperGameInstance::AddRaidExperience(int32 ExperienceAmount)
{
	if (ExperienceAmount <= 0)
	{
		return 0;
	}

	EnsureInventoryStateInitialized();
	if (!bRaidExperienceSessionActive)
	{
		const UWorld* World = GetWorld();
		if (!World || !TunaSweeperBuildFlavor::IsRaidGameplayLevelName(FName(*World->GetMapName())))
		{
			return 0;
		}

		RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
		PendingRaidExperiencePoints = 0;
		bRaidExperienceSessionActive = true;
	}

	PendingRaidExperiencePoints += ExperienceAmount;
	return ExperienceAmount;
}

int32 UTunaSweeperGameInstance::AddRaidExperienceForItem(int32 ItemId, int32 Quantity)
{
	const int32 ExperienceValue = ResolveItemExperienceValue(ItemId);
	const int32 SafeQuantity = FMath::Max(0, Quantity);
	if (ExperienceValue <= 0 || SafeQuantity <= 0)
	{
		return 0;
	}

	return AddRaidExperience(ExperienceValue * SafeQuantity);
}

void UTunaSweeperGameInstance::ClearRaidExperienceGain()
{
	PendingRaidExperiencePoints = 0;
	RaidStartExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
}

bool UTunaSweeperGameInstance::CommitRaidExperienceGain(FTunaSweeperExperienceAnimationState& OutAnimationState)
{
	EnsureInventoryStateInitialized();

	const int64 StartExperience = bRaidExperienceSessionActive
		? FMath::Max<int64>(0, RaidStartExperiencePoints)
		: FMath::Max<int64>(0, TotalExperiencePoints);
	const int64 GainedExperience = FMath::Max<int64>(0, PendingRaidExperiencePoints);
	TotalExperiencePoints = FMath::Max<int64>(StartExperience, TotalExperiencePoints) + GainedExperience;
	OutAnimationState = BuildExperienceAnimationState(
		StartExperience,
		TotalExperiencePoints,
		GainedExperience);
	if (GainedExperience > 0)
	{
		PendingRaidExperienceAnimationState = OutAnimationState;
		bHasPendingRaidExperienceAnimationState = true;
		RefreshCarryWeightState();
		OnExperienceChanged.Broadcast();
	}
	else
	{
		PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
		bHasPendingRaidExperienceAnimationState = false;
	}

	PendingRaidExperiencePoints = 0;
	RaidStartExperiencePoints = TotalExperiencePoints;
	bRaidExperienceSessionActive = false;
	return GainedExperience > 0;
}

bool UTunaSweeperGameInstance::ConsumePendingRaidExperienceAnimationState(
	FTunaSweeperExperienceAnimationState& OutAnimationState)
{
	if (!bHasPendingRaidExperienceAnimationState)
	{
		OutAnimationState = FTunaSweeperExperienceAnimationState();
		return false;
	}

	OutAnimationState = PendingRaidExperienceAnimationState;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bHasPendingRaidExperienceAnimationState = false;
	return true;
}

int32 UTunaSweeperGameInstance::ResolveItemExperienceValue(int32 ItemId)
{
	if (ItemId == INDEX_NONE)
	{
		return 0;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition ItemDefinition;
	if (!ItemDataSubsystem || !ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
	{
		return 0;
	}

	return FMath::Max(0, ItemDefinition.ExperienceValue);
}

FTunaSweeperExperienceAnimationState UTunaSweeperGameInstance::BuildExperienceAnimationState(
	int64 StartExperiencePoints,
	int64 TargetExperiencePoints,
	int64 GainedExperiencePoints) const
{
	FTunaSweeperExperienceAnimationState AnimationState;
	AnimationState.StartExperiencePoints = FMath::Max<int64>(0, StartExperiencePoints);
	AnimationState.TargetExperiencePoints = FMath::Max<int64>(
		AnimationState.StartExperiencePoints,
		TargetExperiencePoints);
	AnimationState.GainedExperiencePoints = FMath::Max<int64>(
		0,
		FMath::Max<int64>(GainedExperiencePoints, AnimationState.TargetExperiencePoints - AnimationState.StartExperiencePoints));
	AnimationState.StartLevel = GetExperienceLevelForTotal(AnimationState.StartExperiencePoints);
	AnimationState.TargetLevel = GetExperienceLevelForTotal(AnimationState.TargetExperiencePoints);
	AnimationState.AnimationDurationSeconds = TunaSweeperExperience::RaidReturnAnimationDurationSeconds;
	return AnimationState;
}

void UTunaSweeperGameInstance::EnsureExperienceLevelTableLoaded() const
{
	if (bExperienceLevelTableLoaded)
	{
		return;
	}

	bExperienceLevelTableLoaded = true;
	if (!LoadExperienceLevelTableJson(CachedExperienceForLevels))
	{
		BuildDefaultExperienceLevelTable(CachedExperienceForLevels);
	}
}

bool UTunaSweeperGameInstance::LoadExperienceLevelTableJson(TArray<int64>& OutExperienceForLevels) const
{
	OutExperienceForLevels.Reset();

	const FString JsonPath = GetExperienceLevelTableJsonPath();
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *JsonPath))
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Could not load experience level table JSON: %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Could not parse experience level table JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* LevelValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		LevelValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			if (!RootObject->TryGetArrayField(TEXT("levels"), LevelValues))
			{
				RootObject->TryGetArrayField(TEXT("level_table"), LevelValues);
			}
		}
	}

	if (!LevelValues)
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level table JSON has no level array: %s"), *JsonPath);
		return false;
	}

	TMap<int32, int64> ExperienceByLevel;
	for (const TSharedPtr<FJsonValue>& LevelValue : *LevelValues)
	{
		const TSharedPtr<FJsonObject> LevelObject = LevelValue.IsValid() ? LevelValue->AsObject() : nullptr;
		int32 ParsedLevel = 1;
		int64 ParsedExperience = 0;
		if (TunaSweeperExperience::ParseLevelTableRow(LevelObject, ParsedLevel, ParsedExperience))
		{
			ExperienceByLevel.Add(ParsedLevel, ParsedExperience);
		}
	}

	if (!ExperienceByLevel.Contains(1))
	{
		ExperienceByLevel.Add(1, 0);
	}

	int32 MaxLevel = 1;
	for (const TPair<int32, int64>& LevelPair : ExperienceByLevel)
	{
		MaxLevel = FMath::Max(MaxLevel, LevelPair.Key);
	}

	if (MaxLevel <= 1)
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Experience level table JSON has no valid progression rows: %s"), *JsonPath);
		return false;
	}

	int64 PreviousExperience = 0;
	for (int32 Level = 1; Level <= MaxLevel; ++Level)
	{
		const int64* FoundExperience = ExperienceByLevel.Find(Level);
		if (!FoundExperience)
		{
			UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level table JSON is missing level %d: %s"), Level, *JsonPath);
			OutExperienceForLevels.Reset();
			return false;
		}

		const int64 NormalizedExperience = Level <= 1
			? 0
			: FMath::Max(*FoundExperience, PreviousExperience + 1);
		OutExperienceForLevels.Add(NormalizedExperience);
		PreviousExperience = NormalizedExperience;
	}

	return OutExperienceForLevels.Num() > 1;
}

void UTunaSweeperGameInstance::BuildDefaultExperienceLevelTable(TArray<int64>& OutExperienceForLevels) const
{
	OutExperienceForLevels.Reset();
	OutExperienceForLevels.Add(0);

	int64 TotalExperience = 0;
	for (int32 Level = 2; Level <= TunaSweeperExperience::DefaultMaxExperienceLevel; ++Level)
	{
		const int64 PreviousLevel = static_cast<int64>(Level - 1);
		TotalExperience += TunaSweeperExperience::BaseExperienceForNextLevel +
			((PreviousLevel - 1) * TunaSweeperExperience::ExperienceIncreasePerLevel);
		OutExperienceForLevels.Add(TotalExperience);
	}
}

FString UTunaSweeperGameInstance::GetExperienceLevelTableJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperExperience::LevelTableJsonRelativePath);
}

void UTunaSweeperGameInstance::EnsureExperienceLevelRewardsLoaded() const
{
	if (bExperienceLevelRewardsLoaded)
	{
		return;
	}

	bExperienceLevelRewardsLoaded = true;

	TArray<FTunaSweeperExperienceLevelReward> LoadedRewards;
	LoadExperienceLevelRewardsJson(LoadedRewards);

	TMap<int32, FTunaSweeperExperienceLevelReward> RewardsByLevel;
	for (FTunaSweeperExperienceLevelReward Reward : LoadedRewards)
	{
		Reward.Normalize();
		RewardsByLevel.Add(Reward.Level, Reward);
	}

	CachedExperienceLevelRewards.Reset();
	RewardsByLevel.GenerateValueArray(CachedExperienceLevelRewards);
	CachedExperienceLevelRewards.Sort([](
		const FTunaSweeperExperienceLevelReward& Left,
		const FTunaSweeperExperienceLevelReward& Right)
	{
		return Left.Level < Right.Level;
	});
}

bool UTunaSweeperGameInstance::LoadExperienceLevelRewardsJson(
	TArray<FTunaSweeperExperienceLevelReward>& OutRewards) const
{
	OutRewards.Reset();

	const FString JsonPath = GetExperienceLevelRewardsJsonPath();
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *JsonPath))
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Could not load experience level rewards JSON: %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Could not parse experience level rewards JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* RewardValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		RewardValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			if (!RootObject->TryGetArrayField(TEXT("level_rewards"), RewardValues))
			{
				RootObject->TryGetArrayField(TEXT("rewards"), RewardValues);
			}
		}
	}

	if (!RewardValues)
	{
		UE_LOG(LogTunaSweeperGameInstance, Error, TEXT("Experience level rewards JSON has no reward array: %s"), *JsonPath);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& RewardValue : *RewardValues)
	{
		const TSharedPtr<FJsonObject> RewardObject = RewardValue.IsValid() ? RewardValue->AsObject() : nullptr;
		FTunaSweeperExperienceLevelReward Reward;
		if (TunaSweeperExperience::ParseLevelReward(RewardObject, Reward))
		{
			OutRewards.Add(Reward);
		}
	}

	if (OutRewards.Num() <= 0)
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Experience level rewards JSON has no valid rows: %s"), *JsonPath);
		return false;
	}

	return true;
}

FString UTunaSweeperGameInstance::GetExperienceLevelRewardsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperExperience::LevelRewardsJsonRelativePath);
}

