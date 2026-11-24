#include "Subsystem/TunaSweeperSupplyDropSubsystem.h"
#include "Scenario/TrailerGameMode.h"

#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperRaidPlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Raid/TunaSweeperSupplyDropActor.h"
#include "Raid/TunaSweeperSupplyDropDataAsset.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperSupplyDrop, Log, All);

namespace
{
	struct FSupplyDropSettings
	{
		double ChancePercent = 0.0;
		double ArrivalDelaySeconds = 0.0;
		int32 ContainerDefinitionId = INDEX_NONE;
		TArray<int32> PresetIds;
	};

	bool ParseId(double Number, int32& OutId)
	{
		if (!(Number >= 1.0 && Number <= MAX_int32 && Number == FMath::FloorToDouble(Number)))
		{
			return false;
		}

		OutId = static_cast<int32>(Number);
		return true;
	}

	bool ReadSupplyDropSettings(FSupplyDropSettings& Settings)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/SupplyDrop.json"));
		FString Text;
		TSharedPtr<FJsonObject> Object;
		const TArray<TSharedPtr<FJsonValue>>* Presets = nullptr;
		double ContainerId = 0.0;
		if (!FFileHelper::LoadFileToString(Text, *Path) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) || !Object.IsValid() ||
			!Object->TryGetNumberField(TEXT("spawn_chance_percent"), Settings.ChancePercent) ||
			!(Settings.ChancePercent >= 0.0 && Settings.ChancePercent <= 100.0) ||
			!Object->TryGetNumberField(TEXT("arrival_delay_seconds"), Settings.ArrivalDelaySeconds) ||
			!(Settings.ArrivalDelaySeconds >= 0.0 && Settings.ArrivalDelaySeconds <= MAX_flt) ||
			!Object->TryGetNumberField(TEXT("container_definition_id"), ContainerId) ||
			!ParseId(ContainerId, Settings.ContainerDefinitionId) ||
			!Object->TryGetArrayField(TEXT("loot_preset_ids"), Presets) || Presets->IsEmpty())
		{
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Preset : *Presets)
		{
			double Number = 0.0;
			int32 Id = INDEX_NONE;
			if (!Preset.IsValid() || Preset->Type != EJson::Number ||
				!Preset->TryGetNumber(Number) || !ParseId(Number, Id) || Settings.PresetIds.Contains(Id))
			{
				return false;
			}
			Settings.PresetIds.Add(Id);
		}
		return true;
	}

	bool TraceGround(UWorld& World, const FVector& Center, const FCollisionQueryParams& Query, FVector& Ground)
	{
		FHitResult Hit;
		if (!World.LineTraceSingleByChannel(Hit, Center + FVector(0, 0, 175), Center - FVector(0, 0, 470),
			ECC_Pawn, Query) || Hit.ImpactNormal.Z < 0.85 || !Hit.Component.IsValid() ||
			Hit.Component->GetCollisionObjectType() != ECC_WorldStatic)
		{
			return false;
		}

		Ground = Hit.ImpactPoint;
		return true;
	}

	bool FindLandingSurface(UWorld& World, const FVector& Candidate, const FQuat& Facing, const FBox& ContainerBounds,
		const FCollisionQueryParams& Query, FVector& Landing)
	{
		if (!TraceGround(World, Candidate, Query, Landing))
		{
			return false;
		}

		const FVector Extent = ContainerBounds.GetExtent();
		double Highest = Landing.Z;
		for (const double X : {-Extent.X, Extent.X})
		{
			for (const double Y : {-Extent.Y, Extent.Y})
			{
				FVector Foot;
				if (!TraceGround(World, Landing + Facing.RotateVector(FVector(X, Y, 0)), Query, Foot) ||
					FMath::Abs(Foot.Z - Landing.Z) > 16.0)
				{
					return false;
				}
				Highest = FMath::Max(Highest, Foot.Z);
			}
		}
		Landing.Z = Highest - ContainerBounds.Min.Z + 2.0;
		return true;
	}

	bool CanDescend(UWorld& World, const FVector& Release, const FVector& Landing, const FQuat& Facing,
		const FBox& MeshBounds, const FCollisionQueryParams& Query)
	{
		const FVector Center = Facing.RotateVector(MeshBounds.GetCenter());
		return !World.SweepTestByChannel(Release + Center, Landing + Center, Facing, ECC_Pawn,
			FCollisionShape::MakeBox(MeshBounds.GetExtent()), Query);
	}

	bool FindDropPath(UWorld& World, const FVector& Origin, const APawn& Player,
		const UTunaSweeperSupplyDropDataAsset& Assets, FTransform& ReleaseTransform, FVector& Landing)
	{
		const FBox ContainerBounds = Assets.ContainerMesh->GetBoundingBox();
		const FBox ChuteBounds = Assets.ParachuteMesh->GetBoundingBox().ExpandBy(FVector(24, 24, 14));
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(SupplyDrop), false, &Player);
		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World);
		FNavLocation NavOrigin;
		const FVector PlayerLocation = Player.GetActorLocation();
		const FVector NavProjectionExtent(150, 150, 230);
		const bool bHasNavOrigin = Navigation &&
			(Navigation->ProjectPointToNavigation(PlayerLocation, NavOrigin, NavProjectionExtent) ||
				(!Origin.Equals(PlayerLocation) && Navigation->ProjectPointToNavigation(Origin, NavOrigin, NavProjectionExtent)));

		constexpr int32 CandidateCount = 36;
		constexpr double ReleaseHeight = 720.0;
		constexpr double DriftDistance = 350.0;
		for (int32 Index = 0; Index < CandidateCount; ++Index)
		{
			const FRotator Heading(0.0, FMath::FRandRange(-180.0, 180.0), 0.0);
			const FVector Direction = Heading.Vector();
			const bool bTryNearby = Index < 24;
			FVector Candidate = Origin + Direction * FMath::FRandRange(
				bTryNearby ? 460.0 : 400.0, bTryNearby ? 1000.0 : 2400.0);
			FNavLocation NavCandidate;
			if (bHasNavOrigin)
			{
				if (!Navigation->ProjectPointToNavigation(Candidate, NavCandidate, FVector(85, 85, 430)))
				{
					continue;
				}
				Candidate = NavCandidate.Location;
			}

			const FQuat Facing = Heading.Quaternion();
			if (!FindLandingSurface(World, Candidate, Facing, ContainerBounds, Query, Landing) ||
				(bHasNavOrigin && FMath::Abs(Candidate.Z - Landing.Z) > 50.0))
			{
				continue;
			}

			const FVector Release = Landing + Direction * DriftDistance + FVector(0, 0, ReleaseHeight);
			if (!CanDescend(World, Release, Landing, Facing, ContainerBounds, Query) ||
				!CanDescend(World, Release, Landing, Facing, ChuteBounds, Query))
			{
				continue;
			}

			if (bHasNavOrigin)
			{
				const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
					&World, NavOrigin.Location, NavCandidate.Location);
				if (!Path || !Path->IsValid() || Path->IsPartial())
				{
					continue;
				}
			}
			else if (World.LineTraceTestByChannel(Origin, Landing + FVector(0, 0, 95), ECC_Pawn, Query))
			{
				continue;
			}

			ReleaseTransform = FTransform(Facing, Release);
			return true;
		}
		return false;
	}

	bool FindDropPathNearEntry(UWorld& World, const APawn& Player, const UTunaSweeperSupplyDropDataAsset& Assets,
		FTransform& ReleaseTransform, FVector& Landing)
	{
		const APlayerStart* NearestStart = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<APlayerStart> It(&World); It; ++It)
		{
			if (It->IsA<ATunaSweeperRaidPlayerStart>())
			{
				continue;
			}
			const double Distance = FVector::DistSquared(Player.GetActorLocation(), It->GetActorLocation());
			if (Distance < NearestDistance)
			{
				NearestStart = *It;
				NearestDistance = Distance;
			}
		}

		return NearestStart && FindDropPath(World, NearestStart->GetActorLocation(), Player, Assets, ReleaseTransform, Landing);
	}
}

bool UTunaSweeperSupplyDropSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTunaSweeperSupplyDropSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bRolledForWorld || InWorld.GetAuthGameMode<ATrailerGameMode>() || InWorld.GetNetMode() == NM_Client ||
		!TunaSweeperBuildFlavor::IsRaidGameplayLevelName(FName(*InWorld.GetMapName())))
	{
		return;
	}
	bRolledForWorld = true;

	FSupplyDropSettings Settings;
	if (!ReadSupplyDropSettings(Settings))
	{
		UE_LOG(LogTunaSweeperSupplyDrop, Warning, TEXT("Invalid Content/Data/SupplyDrop.json"));
		return;
	}
	if (Settings.ChancePercent == 0.0 ||
		(Settings.ChancePercent < 100.0 && FMath::FRand() * 100.0 >= Settings.ChancePercent))
	{
		return;
	}

	ContainerDefinitionId = Settings.ContainerDefinitionId;
	ContentsId = Settings.PresetIds[FMath::RandHelper(Settings.PresetIds.Num())];
	InWorld.GetTimerManager().SetTimer(SpawnTimer, this, &UTunaSweeperSupplyDropSubsystem::TrySpawnDrop,
		2.0f, true, FMath::Max(0.01f, static_cast<float>(Settings.ArrivalDelaySeconds)));
}

void UTunaSweeperSupplyDropSubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	InWorld.GetTimerManager().ClearTimer(SpawnTimer);
	Super::OnWorldEndPlay(InWorld);
}

const ATunaSweeperSupplyDropActor* UTunaSweeperSupplyDropSubsystem::GetDescendingDrop() const
{
	const auto* Drop = ActiveDrop.Get();
	return Drop && Drop->IsDescending() ? Drop : nullptr;
}

void UTunaSweeperSupplyDropSubsystem::TrySpawnDrop()
{
	UWorld& World = GetWorldRef();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(&World, 0);
	if (!IsValid(Player))
	{
		return;
	}

	UTunaSweeperGameInstance* Game = World.GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperSupplyDropDataAsset* Assets = Game ? Game->SupplyDropDataAsset.LoadSynchronous() : nullptr;
	if (!Assets || !Assets->ContainerMesh || !Assets->ParachuteMesh)
	{
		UE_LOG(LogTunaSweeperSupplyDrop, Warning, TEXT("Supply drop is missing its presentation data asset or meshes."));
		World.GetTimerManager().ClearTimer(SpawnTimer);
		return;
	}

	UTunaSweeperItemDataSubsystem* ItemData = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperLootContainerDefinition Definition;
	FTunaSweeperLootContainerContents Contents;
	if (!ItemData || !ItemData->TryGetLootContainerDefinition(ContainerDefinitionId, Definition) ||
		!ItemData->TryGetLootContainerContents(ContentsId, Contents))
	{
		UE_LOG(LogTunaSweeperSupplyDrop, Warning, TEXT("Unknown supply drop definition (%d) or preset (%d)."),
			ContainerDefinitionId, ContentsId);
		World.GetTimerManager().ClearTimer(SpawnTimer);
		return;
	}

	FTransform ReleaseTransform;
	FVector LandingLocation;
	if (FindDropPath(World, Player->GetActorLocation(), *Player, *Assets, ReleaseTransform, LandingLocation) ||
		FindDropPathNearEntry(World, *Player, *Assets, ReleaseTransform, LandingLocation))
	{
		if (ATunaSweeperSupplyDropActor* Drop = World.SpawnActorDeferred<ATunaSweeperSupplyDropActor>(
			ATunaSweeperSupplyDropActor::StaticClass(), ReleaseTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
		{
			Drop->ConfigureDrop(*Assets, ContainerDefinitionId, ContentsId, LandingLocation);
			Drop->FinishSpawning(ReleaseTransform);
			ActiveDrop = Drop;
			UE_LOG(LogTunaSweeperSupplyDrop, Log, TEXT("Released supply drop with preset %d in %s at %s."),
				ContentsId, *World.GetMapName(), *LandingLocation.ToCompactString());
			World.GetTimerManager().ClearTimer(SpawnTimer);
		}
	}
}
