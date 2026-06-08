#include "AI/TunaSweeperRollingBomberSpawner.h"

#include "AI/TunaSweeperRollingBomber.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

namespace TunaSweeperRollingBomberSpawner
{
	const TCHAR* DefaultRollingBomberClassPath = TEXT("/Script/TunaSweeper.TunaSweeperRollingBomber");
	const TCHAR* DefaultLaunchSoundPath =
		TEXT("/Game/Audio/SFX/SFX_RollingBomberSpawnerLaunch_FM.SFX_RollingBomberSpawnerLaunch_FM");
	const TCHAR* DefaultSpawnerMaterialPath =
		TEXT("/Game/Interaction/M_RollingBomberSpawner_Mechanic.M_RollingBomberSpawner_Mechanic");
	const TCHAR* FallbackVertexColorMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");
	constexpr int32 ProceduralSampleRate = 48000;
	constexpr int32 ProceduralChannelCount = 1;
	constexpr float ProceduralSoundDurationSeconds = 0.42f;
	constexpr float VisualScale = 0.25f;
	constexpr float LaunchSpeedScale = 0.5f;
	constexpr float LaunchPitchScale = 0.6f;
	constexpr float PillarHeight = 132.0f * VisualScale;
	constexpr float PillarHalfExtent = 34.0f * VisualScale;
	constexpr float HexHeadBottomZ = 108.0f * VisualScale;
	constexpr float HexHeadTopZ = 208.0f * VisualScale;
	constexpr float HexHeadRadius = 88.0f * VisualScale;
	constexpr float LaunchPointHeight = 220.0f * VisualScale;
	constexpr float FallbackSpawnHeight = 140.0f * VisualScale;
	constexpr int32 VerticalSegments = 18;

	float CalculateEnvelope(float TimeSeconds, float DurationSeconds)
	{
		constexpr float AttackSeconds = 0.018f;
		constexpr float ReleaseSeconds = 0.16f;
		const float AttackAlpha = FMath::Clamp(TimeSeconds / AttackSeconds, 0.0f, 1.0f);
		const float ReleaseAlpha = FMath::Clamp((DurationSeconds - TimeSeconds) / ReleaseSeconds, 0.0f, 1.0f);
		return FMath::Min(AttackAlpha, ReleaseAlpha);
	}

	void AddQuad(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FLinearColor& ColorA,
		const FLinearColor& ColorB,
		const FLinearColor& ColorC,
		const FLinearColor& ColorD,
		const FVector2D& UvA,
		const FVector2D& UvB,
		const FVector2D& UvC,
		const FVector2D& UvD)
	{
		const int32 BaseIndex = Vertices.Num();
		const FVector Normal = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
		const FVector TangentVector = (B - A).GetSafeNormal();
		const FProcMeshTangent Tangent(TangentVector, false);

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Vertices.Add(D);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		UVs.Add(UvA);
		UVs.Add(UvB);
		UVs.Add(UvC);
		UVs.Add(UvD);
		VertexColors.Add(ColorA);
		VertexColors.Add(ColorB);
		VertexColors.Add(ColorC);
		VertexColors.Add(ColorD);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);

		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 3);
		Triangles.Add(BaseIndex + 2);
	}

	void AddTriangle(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FLinearColor& Color,
		const FVector2D& UvA,
		const FVector2D& UvB,
		const FVector2D& UvC)
	{
		const int32 BaseIndex = Vertices.Num();
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		const FVector TangentVector = (B - A).GetSafeNormal();
		const FProcMeshTangent Tangent(TangentVector, false);

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		UVs.Add(UvA);
		UVs.Add(UvB);
		UVs.Add(UvC);
		VertexColors.Add(Color);
		VertexColors.Add(Color);
		VertexColors.Add(Color);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 1);
	}
}

ATunaSweeperRollingBomberSpawner::ATunaSweeperRollingBomberSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PillarMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(SceneRoot);
	PillarMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PillarMesh->SetCollisionObjectType(ECC_WorldDynamic);
	PillarMesh->SetCollisionResponseToAllChannels(ECR_Block);

	HexHeadMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexHeadMesh"));
	HexHeadMesh->SetupAttachment(SceneRoot);
	HexHeadMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HexHeadMesh->SetCollisionObjectType(ECC_WorldDynamic);
	HexHeadMesh->SetCollisionResponseToAllChannels(ECR_Block);

	LaunchPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LaunchPoint"));
	LaunchPoint->SetupAttachment(SceneRoot);
	LaunchPoint->SetRelativeLocation(FVector(0.0f, 0.0f, TunaSweeperRollingBomberSpawner::LaunchPointHeight));

	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubject"));

	RollingBomberClass = TSoftClassPtr<ATunaSweeperRollingBomber>(
		FSoftObjectPath(TunaSweeperRollingBomberSpawner::DefaultRollingBomberClassPath));
	LaunchSound = TSoftObjectPtr<USoundBase>(
		FSoftObjectPath(TunaSweeperRollingBomberSpawner::DefaultLaunchSoundPath));
	SpawnerMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TunaSweeperRollingBomberSpawner::DefaultSpawnerMaterialPath));

	BuildSpawnerMeshes();
}

void ATunaSweeperRollingBomberSpawner::BeginPlay()
{
	Super::BeginPlay();

	bSpawnerDestroyed = false;
	bSpawnerActivated = false;
	CurrentHealth = FMath::Max(1.0f, MaxHealth);
	InitialSpawnCount = FMath::Max(1, InitialSpawnCount);
	MaxSpawnCount = FMath::Max(InitialSpawnCount, MaxSpawnCount);
	WaveIntervalSeconds = FMath::Max(0.01f, WaveIntervalSeconds);
	SpawnIntervalSeconds = FMath::Max(0.01f, SpawnIntervalSeconds);
	ActivationCheckIntervalSeconds = FMath::Max(0.01f, ActivationCheckIntervalSeconds);
	CurrentWaveSpawnCount = FMath::Clamp(InitialSpawnCount, 1, MaxSpawnCount);
	BuildSpawnerMeshes();
	ApplySpawnerMaterial();

	if (IsPlayerWithinActivationRange())
	{
		ActivateSpawner();
	}
	else
	{
		StartActivationCheck();
	}
}

void ATunaSweeperRollingBomberSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	ActiveProceduralLaunchSounds.Reset();

	Super::EndPlay(EndPlayReason);
}

float ATunaSweeperRollingBomberSpawner::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bSpawnerDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		if (EventInstigator && EventInstigator->IsPlayerController())
		{
			if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
			{
				TunaGameInstance->AddRaidExperience(ExperienceValue);
			}
		}
		DestroySpawner();
	}

	return AppliedDamage;
}

void ATunaSweeperRollingBomberSpawner::ConfigureSpawnerDefaults(
	const TSoftClassPtr<ATunaSweeperRollingBomber>& InRollingBomberClass,
	const TSoftObjectPtr<USoundBase>& InLaunchSound,
	int32 InInitialSpawnCount,
	int32 InMaxSpawnCount,
	float InWaveIntervalSeconds,
	float InSpawnIntervalSeconds,
	float InLaunchSpeedMin,
	float InLaunchSpeedMax,
	float InLaunchPitchMinDegrees,
	float InLaunchPitchMaxDegrees,
	float InMaxHealth,
	int32 InExperienceValue)
{
	if (!InRollingBomberClass.IsNull())
	{
		RollingBomberClass = InRollingBomberClass;
	}
	if (!InLaunchSound.IsNull())
	{
		LaunchSound = InLaunchSound;
	}

	InitialSpawnCount = FMath::Max(1, InInitialSpawnCount);
	MaxSpawnCount = FMath::Max(InitialSpawnCount, InMaxSpawnCount);
	WaveIntervalSeconds = FMath::Max(0.01f, InWaveIntervalSeconds);
	SpawnIntervalSeconds = FMath::Max(0.01f, InSpawnIntervalSeconds);
	LaunchSpeedMin = FMath::Max(0.0f, InLaunchSpeedMin * TunaSweeperRollingBomberSpawner::LaunchSpeedScale);
	LaunchSpeedMax = FMath::Max(
		LaunchSpeedMin,
		InLaunchSpeedMax * TunaSweeperRollingBomberSpawner::LaunchSpeedScale);
	LaunchPitchMinDegrees = FMath::Clamp(
		InLaunchPitchMinDegrees * TunaSweeperRollingBomberSpawner::LaunchPitchScale,
		0.0f,
		89.0f);
	LaunchPitchMaxDegrees = FMath::Clamp(
		InLaunchPitchMaxDegrees * TunaSweeperRollingBomberSpawner::LaunchPitchScale,
		LaunchPitchMinDegrees,
		89.0f);
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	ExperienceValue = FMath::Max(0, InExperienceValue);
	CurrentWaveSpawnCount = FMath::Clamp(InitialSpawnCount, 1, MaxSpawnCount);
}

void ATunaSweeperRollingBomberSpawner::StartActivationCheck()
{
	if (bSpawnerDestroyed || bSpawnerActivated)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		ActivationTimerHandle,
		this,
		&ATunaSweeperRollingBomberSpawner::CheckActivationRange,
		ActivationCheckIntervalSeconds,
		true,
		0.0f);
}

void ATunaSweeperRollingBomberSpawner::CheckActivationRange()
{
	if (bSpawnerDestroyed)
	{
		StopSpawning();
		return;
	}

	if (bSpawnerActivated)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ActivationTimerHandle);
		}
		return;
	}

	if (IsPlayerWithinActivationRange())
	{
		ActivateSpawner();
	}
}

void ATunaSweeperRollingBomberSpawner::ActivateSpawner()
{
	if (bSpawnerDestroyed || bSpawnerActivated)
	{
		return;
	}

	bSpawnerActivated = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
	}

	StartWave();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			WaveTimerHandle,
			this,
			&ATunaSweeperRollingBomberSpawner::StartWave,
			WaveIntervalSeconds,
			true,
			WaveIntervalSeconds);
	}
}

void ATunaSweeperRollingBomberSpawner::StartWave()
{
	if (bSpawnerDestroyed || !bSpawnerActivated || PendingSpawnCount > 0)
	{
		return;
	}

	PendingSpawnCount = FMath::Clamp(CurrentWaveSpawnCount, 1, MaxSpawnCount);
	CurrentWaveSpawnCount = FMath::Clamp(CurrentWaveSpawnCount * 2, 1, MaxSpawnCount);
	SpawnNextQueuedRollingBomber();
}

void ATunaSweeperRollingBomberSpawner::SpawnNextQueuedRollingBomber()
{
	if (bSpawnerDestroyed || !bSpawnerActivated)
	{
		StopSpawning();
		return;
	}

	UWorld* World = GetWorld();
	if (!World || PendingSpawnCount <= 0)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(BurstTimerHandle);
		}
		return;
	}

	TSubclassOf<ATunaSweeperRollingBomber> LoadedRollingBomberClass = RollingBomberClass.LoadSynchronous();
	if (!LoadedRollingBomberClass)
	{
		LoadedRollingBomberClass = ATunaSweeperRollingBomber::StaticClass();
	}

	float LaunchYawDegrees = 0.0f;
	const FVector LaunchVelocity = BuildLaunchVelocity(LaunchYawDegrees);
	const FVector SpawnLocation = LaunchPoint
		? LaunchPoint->GetComponentLocation()
		: GetActorLocation() + FVector(0.0f, 0.0f, TunaSweeperRollingBomberSpawner::FallbackSpawnHeight);
	const FRotator SpawnRotation(0.0f, LaunchYawDegrees, 0.0f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperRollingBomber* SpawnedBomber = World->SpawnActor<ATunaSweeperRollingBomber>(
		LoadedRollingBomberClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
	if (SpawnedBomber)
	{
		SpawnedBomber->LaunchFromSpawner(LaunchVelocity);
		PlayLaunchSound();
	}

	--PendingSpawnCount;
	if (PendingSpawnCount > 0)
	{
		World->GetTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&ATunaSweeperRollingBomberSpawner::SpawnNextQueuedRollingBomber,
			SpawnIntervalSeconds,
			false);
	}
	else
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
}

void ATunaSweeperRollingBomberSpawner::StopSpawning()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActivationTimerHandle);
		World->GetTimerManager().ClearTimer(WaveTimerHandle);
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}

	PendingSpawnCount = 0;
}

void ATunaSweeperRollingBomberSpawner::DestroySpawner()
{
	if (bSpawnerDestroyed)
	{
		return;
	}

	bSpawnerDestroyed = true;
	StopSpawning();
	SetActorEnableCollision(false);
	Destroy();
}

ATunaSweeperTopDownCharacter* ATunaSweeperRollingBomberSpawner::ResolvePlayerTarget() const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerPawn);
	if (!PlayerCharacter || PlayerCharacter->IsDead())
	{
		return nullptr;
	}

	return PlayerCharacter;
}

bool ATunaSweeperRollingBomberSpawner::IsPlayerWithinActivationRange() const
{
	const ATunaSweeperTopDownCharacter* PlayerCharacter = ResolvePlayerTarget();
	if (!PlayerCharacter)
	{
		return false;
	}

	return FVector::DistSquared2D(PlayerCharacter->GetActorLocation(), GetActorLocation()) <=
		FMath::Square(FMath::Max(0.0f, ActivationRangeCm));
}

void ATunaSweeperRollingBomberSpawner::BuildSpawnerMeshes()
{
	BuildPillarMesh();
	BuildHexHeadMesh();
}

void ATunaSweeperRollingBomberSpawner::BuildPillarMesh()
{
	if (!PillarMesh)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	constexpr float HalfExtent = TunaSweeperRollingBomberSpawner::PillarHalfExtent;
	constexpr float Height = TunaSweeperRollingBomberSpawner::PillarHeight;
	const FVector FaceCorners[4][4] =
	{
		{ FVector(HalfExtent, -HalfExtent, 0.0f), FVector(HalfExtent, HalfExtent, 0.0f), FVector(HalfExtent, HalfExtent, Height), FVector(HalfExtent, -HalfExtent, Height) },
		{ FVector(-HalfExtent, HalfExtent, 0.0f), FVector(-HalfExtent, -HalfExtent, 0.0f), FVector(-HalfExtent, -HalfExtent, Height), FVector(-HalfExtent, HalfExtent, Height) },
		{ FVector(HalfExtent, HalfExtent, 0.0f), FVector(-HalfExtent, HalfExtent, 0.0f), FVector(-HalfExtent, HalfExtent, Height), FVector(HalfExtent, HalfExtent, Height) },
		{ FVector(-HalfExtent, -HalfExtent, 0.0f), FVector(HalfExtent, -HalfExtent, 0.0f), FVector(HalfExtent, -HalfExtent, Height), FVector(-HalfExtent, -HalfExtent, Height) }
	};

	for (int32 FaceIndex = 0; FaceIndex < 4; ++FaceIndex)
	{
		const FVector BottomLeft = FaceCorners[FaceIndex][0];
		const FVector BottomRight = FaceCorners[FaceIndex][1];
		const FVector TopRight = FaceCorners[FaceIndex][2];
		const FVector TopLeft = FaceCorners[FaceIndex][3];
		for (int32 SegmentIndex = 0; SegmentIndex < TunaSweeperRollingBomberSpawner::VerticalSegments; ++SegmentIndex)
		{
			const float V0 = static_cast<float>(SegmentIndex) / TunaSweeperRollingBomberSpawner::VerticalSegments;
			const float V1 = static_cast<float>(SegmentIndex + 1) / TunaSweeperRollingBomberSpawner::VerticalSegments;
			const FVector A = FMath::Lerp(BottomLeft, TopLeft, V0);
			const FVector B = FMath::Lerp(BottomRight, TopRight, V0);
			const FVector C = FMath::Lerp(BottomRight, TopRight, V1);
			const FVector D = FMath::Lerp(BottomLeft, TopLeft, V1);
			const FLinearColor ColorA = ResolveMechanicalBandColor(0.0f, V0, V0, static_cast<float>(FaceIndex));
			const FLinearColor ColorB = ResolveMechanicalBandColor(1.0f, V0, V0, static_cast<float>(FaceIndex));
			const FLinearColor ColorC = ResolveMechanicalBandColor(1.0f, V1, V1, static_cast<float>(FaceIndex));
			const FLinearColor ColorD = ResolveMechanicalBandColor(0.0f, V1, V1, static_cast<float>(FaceIndex));

			TunaSweeperRollingBomberSpawner::AddQuad(
				Vertices,
				Triangles,
				Normals,
				UVs,
				VertexColors,
				Tangents,
				A,
				B,
				C,
				D,
				ColorA,
				ColorB,
				ColorC,
				ColorD,
				FVector2D(0.0f, V0),
				FVector2D(1.0f, V0),
				FVector2D(1.0f, V1),
				FVector2D(0.0f, V1));
		}
	}

	const FLinearColor TopColor = ResolveMechanicalBandColor(0.5f, 1.0f, 1.0f, 4.0f);
	const FLinearColor BottomColor = ResolveMechanicalBandColor(0.5f, 0.0f, 0.0f, 5.0f);
	TunaSweeperRollingBomberSpawner::AddQuad(
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		FVector(-HalfExtent, -HalfExtent, Height),
		FVector(HalfExtent, -HalfExtent, Height),
		FVector(HalfExtent, HalfExtent, Height),
		FVector(-HalfExtent, HalfExtent, Height),
		TopColor,
		TopColor,
		TopColor,
		TopColor,
		FVector2D(0.0f, 0.0f),
		FVector2D(1.0f, 0.0f),
		FVector2D(1.0f, 1.0f),
		FVector2D(0.0f, 1.0f));
	TunaSweeperRollingBomberSpawner::AddQuad(
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		FVector(-HalfExtent, HalfExtent, 0.0f),
		FVector(HalfExtent, HalfExtent, 0.0f),
		FVector(HalfExtent, -HalfExtent, 0.0f),
		FVector(-HalfExtent, -HalfExtent, 0.0f),
		BottomColor,
		BottomColor,
		BottomColor,
		BottomColor,
		FVector2D(0.0f, 0.0f),
		FVector2D(1.0f, 0.0f),
		FVector2D(1.0f, 1.0f),
		FVector2D(0.0f, 1.0f));

	PillarMesh->ClearAllMeshSections();
	PillarMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}

void ATunaSweeperRollingBomberSpawner::BuildHexHeadMesh()
{
	if (!HexHeadMesh)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	constexpr float Radius = TunaSweeperRollingBomberSpawner::HexHeadRadius;
	constexpr float BottomZ = TunaSweeperRollingBomberSpawner::HexHeadBottomZ;
	constexpr float TopZ = TunaSweeperRollingBomberSpawner::HexHeadTopZ;
	constexpr float Height = TopZ - BottomZ;
	TArray<FVector> BottomRing;
	TArray<FVector> TopRing;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float Angle = UE_PI / 6.0f + static_cast<float>(Index) * UE_PI / 3.0f;
		const float X = FMath::Cos(Angle) * Radius;
		const float Y = FMath::Sin(Angle) * Radius;
		BottomRing.Add(FVector(X, Y, BottomZ));
		TopRing.Add(FVector(X, Y, TopZ));
	}

	for (int32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
	{
		const int32 NextIndex = (FaceIndex + 1) % 6;
		for (int32 SegmentIndex = 0; SegmentIndex < TunaSweeperRollingBomberSpawner::VerticalSegments; ++SegmentIndex)
		{
			const float V0 = static_cast<float>(SegmentIndex) / TunaSweeperRollingBomberSpawner::VerticalSegments;
			const float V1 = static_cast<float>(SegmentIndex + 1) / TunaSweeperRollingBomberSpawner::VerticalSegments;
			const float HeightAlpha0 = (FMath::Lerp(BottomZ, TopZ, V0) - BottomZ) / Height;
			const float HeightAlpha1 = (FMath::Lerp(BottomZ, TopZ, V1) - BottomZ) / Height;
			const FVector A = FMath::Lerp(BottomRing[FaceIndex], TopRing[FaceIndex], V0);
			const FVector B = FMath::Lerp(BottomRing[NextIndex], TopRing[NextIndex], V0);
			const FVector C = FMath::Lerp(BottomRing[NextIndex], TopRing[NextIndex], V1);
			const FVector D = FMath::Lerp(BottomRing[FaceIndex], TopRing[FaceIndex], V1);
			const FLinearColor ColorA = ResolveMechanicalBandColor(0.0f, V0, HeightAlpha0, static_cast<float>(FaceIndex));
			const FLinearColor ColorB = ResolveMechanicalBandColor(1.0f, V0, HeightAlpha0, static_cast<float>(FaceIndex));
			const FLinearColor ColorC = ResolveMechanicalBandColor(1.0f, V1, HeightAlpha1, static_cast<float>(FaceIndex));
			const FLinearColor ColorD = ResolveMechanicalBandColor(0.0f, V1, HeightAlpha1, static_cast<float>(FaceIndex));

			TunaSweeperRollingBomberSpawner::AddQuad(
				Vertices,
				Triangles,
				Normals,
				UVs,
				VertexColors,
				Tangents,
				A,
				B,
				C,
				D,
				ColorA,
				ColorB,
				ColorC,
				ColorD,
				FVector2D(static_cast<float>(FaceIndex) / 6.0f, V0),
				FVector2D(static_cast<float>(FaceIndex + 1) / 6.0f, V0),
				FVector2D(static_cast<float>(FaceIndex + 1) / 6.0f, V1),
				FVector2D(static_cast<float>(FaceIndex) / 6.0f, V1));
		}
	}

	const FVector CenterTop(0.0f, 0.0f, TopZ);
	const FVector CenterBottom(0.0f, 0.0f, BottomZ);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const int32 NextIndex = (Index + 1) % 6;
		const FLinearColor TopColor = ResolveMechanicalBandColor(0.5f, 1.0f, 1.0f, static_cast<float>(Index));
		const FLinearColor BottomColor = ResolveMechanicalBandColor(0.5f, 0.0f, 0.0f, static_cast<float>(Index));
		TunaSweeperRollingBomberSpawner::AddTriangle(
			Vertices,
			Triangles,
			Normals,
			UVs,
			VertexColors,
			Tangents,
			CenterTop,
			TopRing[Index],
			TopRing[NextIndex],
			TopColor,
			FVector2D(0.5f, 0.5f),
			FVector2D(0.0f, 0.0f),
			FVector2D(1.0f, 0.0f));
		TunaSweeperRollingBomberSpawner::AddTriangle(
			Vertices,
			Triangles,
			Normals,
			UVs,
			VertexColors,
			Tangents,
			CenterBottom,
			BottomRing[NextIndex],
			BottomRing[Index],
			BottomColor,
			FVector2D(0.5f, 0.5f),
			FVector2D(1.0f, 0.0f),
			FVector2D(0.0f, 0.0f));
	}

	HexHeadMesh->ClearAllMeshSections();
	HexHeadMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}

void ATunaSweeperRollingBomberSpawner::ApplySpawnerMaterial()
{
	UMaterialInterface* LoadedMaterial = SpawnerMaterial.LoadSynchronous();
	if (!LoadedMaterial)
	{
		LoadedMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TunaSweeperRollingBomberSpawner::FallbackVertexColorMaterialPath);
	}

	if (PillarMesh && LoadedMaterial)
	{
		PillarMesh->SetMaterial(0, LoadedMaterial);
	}
	if (HexHeadMesh && LoadedMaterial)
	{
		HexHeadMesh->SetMaterial(0, LoadedMaterial);
	}
}

FLinearColor ATunaSweeperRollingBomberSpawner::ResolveMechanicalBandColor(
	float U,
	float V,
	float HeightAlpha,
	float FaceIndex) const
{
	const float Diagonal = FMath::Frac(U + HeightAlpha * 0.82f + FaceIndex * 0.13f);
	const bool bBlackHorizontalBand =
		(HeightAlpha > 0.22f && HeightAlpha < 0.28f) ||
		(HeightAlpha > 0.56f && HeightAlpha < 0.64f) ||
		(HeightAlpha > 0.82f && HeightAlpha < 0.87f);
	const bool bBlackDiagonalBand = Diagonal > 0.48f && Diagonal < 0.58f;
	const bool bPanelSeam = FMath::Frac(V * 6.0f) < 0.035f || U < 0.035f || U > 0.965f;
	if (bBlackHorizontalBand || bBlackDiagonalBand)
	{
		return FLinearColor(0.012f, 0.012f, 0.014f, 1.0f);
	}
	if (bPanelSeam)
	{
		return FLinearColor(0.18f, 0.18f, 0.18f, 1.0f);
	}

	const float SubtleTone = 0.78f + 0.12f * FMath::Frac(FaceIndex * 0.37f + HeightAlpha * 2.3f);
	return FLinearColor(SubtleTone, SubtleTone, SubtleTone * 0.96f, 1.0f);
}

FVector ATunaSweeperRollingBomberSpawner::BuildLaunchVelocity(float& OutYawDegrees) const
{
	OutYawDegrees = FMath::FRandRange(0.0f, 360.0f);
	const float PitchDegrees = FMath::FRandRange(LaunchPitchMinDegrees, LaunchPitchMaxDegrees);
	const float Speed = FMath::FRandRange(LaunchSpeedMin, LaunchSpeedMax);
	const FRotator LaunchRotation(PitchDegrees, OutYawDegrees, 0.0f);
	return LaunchRotation.Vector() * Speed;
}

void ATunaSweeperRollingBomberSpawner::PlayLaunchSound()
{
	if (USoundBase* LoadedLaunchSound = LaunchSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, LoadedLaunchSound, GetActorLocation());
		return;
	}

	PlayProceduralLaunchSound();
}

void ATunaSweeperRollingBomberSpawner::PlayProceduralLaunchSound()
{
	USoundWaveProcedural* ProceduralSound = CreateProceduralLaunchSound();
	if (!ProceduralSound)
	{
		return;
	}

	ActiveProceduralLaunchSounds.Add(ProceduralSound);
	UGameplayStatics::PlaySoundAtLocation(this, ProceduralSound, GetActorLocation());

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TWeakObjectPtr<ATunaSweeperRollingBomberSpawner> WeakThis(this);
	const TWeakObjectPtr<USoundWaveProcedural> WeakSound(ProceduralSound);
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateLambda([WeakThis, WeakSound]()
		{
			ATunaSweeperRollingBomberSpawner* Spawner = WeakThis.Get();
			USoundWaveProcedural* Sound = WeakSound.Get();
			if (!Spawner || !Sound)
			{
				return;
			}

			Spawner->ActiveProceduralLaunchSounds.RemoveAll(
				[Sound](const TObjectPtr<USoundWaveProcedural>& Candidate)
				{
					return Candidate == Sound;
				});
		}),
		TunaSweeperRollingBomberSpawner::ProceduralSoundDurationSeconds + 0.25f,
		false);
}

USoundWaveProcedural* ATunaSweeperRollingBomberSpawner::CreateProceduralLaunchSound()
{
	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(this);
	if (!SoundWave)
	{
		return nullptr;
	}

	const int32 SampleCount = FMath::CeilToInt(
		TunaSweeperRollingBomberSpawner::ProceduralSampleRate *
		TunaSweeperRollingBomberSpawner::ProceduralSoundDurationSeconds);
	TArray<uint8> PcmData;
	PcmData.SetNumUninitialized(SampleCount * sizeof(int16));
	int16* Samples = reinterpret_cast<int16*>(PcmData.GetData());

	const float CarrierFrequency = FMath::FRandRange(110.0f, 190.0f);
	const float ModulatorFrequency = FMath::FRandRange(390.0f, 720.0f);
	const float ModulationIndex = FMath::FRandRange(3.5f, 7.5f);
	const float ChirpFrequency = FMath::FRandRange(35.0f, 70.0f);
	const float DurationSeconds = TunaSweeperRollingBomberSpawner::ProceduralSoundDurationSeconds;
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float TimeSeconds = static_cast<float>(SampleIndex) / TunaSweeperRollingBomberSpawner::ProceduralSampleRate;
		const float NormalizedTime = FMath::Clamp(TimeSeconds / DurationSeconds, 0.0f, 1.0f);
		const float Envelope = TunaSweeperRollingBomberSpawner::CalculateEnvelope(TimeSeconds, DurationSeconds);
		const float CarrierSweep = CarrierFrequency + ChirpFrequency * NormalizedTime;
		const float Modulator = FMath::Sin(2.0f * UE_PI * ModulatorFrequency * TimeSeconds) * ModulationIndex * Envelope;
		const float Body = FMath::Sin(2.0f * UE_PI * CarrierSweep * TimeSeconds + Modulator);
		const float Click = FMath::Sin(2.0f * UE_PI * CarrierFrequency * 3.0f * TimeSeconds) * FMath::Pow(1.0f - NormalizedTime, 4.0f);
		const float Sample = FMath::Clamp((Body * 0.75f + Click * 0.25f) * Envelope * 0.45f, -1.0f, 1.0f);
		Samples[SampleIndex] = static_cast<int16>(Sample * 32767.0f);
	}

	SoundWave->SetSampleRate(TunaSweeperRollingBomberSpawner::ProceduralSampleRate);
	SoundWave->NumChannels = TunaSweeperRollingBomberSpawner::ProceduralChannelCount;
	SoundWave->Duration = DurationSeconds;
	SoundWave->bLooping = false;
	SoundWave->SoundGroup = SOUNDGROUP_Effects;
	SoundWave->QueueAudio(PcmData.GetData(), PcmData.Num());
	return SoundWave;
}
