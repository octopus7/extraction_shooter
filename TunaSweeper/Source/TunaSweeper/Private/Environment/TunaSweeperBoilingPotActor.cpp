#include "Environment/TunaSweeperBoilingPotActor.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

ATunaSweeperBoilingPotActor::ATunaSweeperBoilingPotActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(SceneRoot);
	BlockingCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 27.0f));
	BlockingCollision->SetBoxExtent(FVector(43.0f, 43.0f, 27.0f));
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetCanEverAffectNavigation(true);

	PotBodyMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PotBodyMesh"));
	PotBodyMeshComponent->SetupAttachment(SceneRoot);
	PotBodyMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PotBodyMeshComponent->SetGenerateOverlapEvents(false);
	PotBodyMeshComponent->SetCanEverAffectNavigation(false);

	LidPivot = CreateDefaultSubobject<USceneComponent>(TEXT("LidPivot"));
	LidPivot->SetupAttachment(SceneRoot);
	LidPivot->SetRelativeLocation(FVector(0.0f, 0.0f, LidRestHeightCm));

	LidMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMeshComponent->SetupAttachment(LidPivot);
	LidMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LidMeshComponent->SetGenerateOverlapEvents(false);
	LidMeshComponent->SetCanEverAffectNavigation(false);

	SteamOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("SteamOrigin"));
	SteamOrigin->SetupAttachment(SceneRoot);
	SteamOrigin->SetRelativeLocation(FVector(0.0f, 0.0f, SteamHeightCm));

	SteamEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SteamEffect"));
	SteamEffectComponent->SetupAttachment(SteamOrigin);
	SteamEffectComponent->SetAutoActivate(false);
	SteamEffectComponent->SetRelativeScale3D(FVector(SteamVisualScale));

	LidClatterAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("LidClatterAudio"));
	LidClatterAudioComponent->SetupAttachment(LidPivot);
	LidClatterAudioComponent->bAutoActivate = false;
}

void ATunaSweeperBoilingPotActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPresentation();
	ResetLidTransform();
}

void ATunaSweeperBoilingPotActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyPresentation();

	const int32 LocationHash = HashCombineFast(
		GetTypeHash(FMath::RoundToInt(GetActorLocation().X)),
		HashCombineFast(
			GetTypeHash(FMath::RoundToInt(GetActorLocation().Y)),
			GetTypeHash(FMath::RoundToInt(GetActorLocation().Z))));
	RattleRandom.Initialize(HashCombineFast(RattleSeed, LocationHash));
	SetBoiling(bStartBoiling);
}

void ATunaSweeperBoilingPotActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bBoiling || !LidPivot)
	{
		return;
	}

	if (!bRattleBurstActive)
	{
		TimeUntilNextBurst -= DeltaSeconds;
		if (TimeUntilNextBurst <= 0.0f)
		{
			StartRattleBurst();
		}
		return;
	}

	BurstElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(BurstElapsedSeconds / FMath::Max(BurstDurationSeconds, UE_SMALL_NUMBER), 0.0f, 1.0f);
	const float Envelope = FMath::Sin(Alpha * UE_PI);
	const float HitPhase = Alpha * static_cast<float>(BurstHitCount);
	const float LiftWave = FMath::Abs(FMath::Sin(HitPhase * UE_PI));
	const float TiltWave = FMath::Sin(HitPhase * UE_TWO_PI);
	const float SettlingScale = FMath::Lerp(1.0f, 0.62f, Alpha);

	const FVector BaseLocation = LidRestTransform.GetLocation();
	const FVector AnimatedLocation = BaseLocation + FVector(0.0f, 0.0f, BurstLiftCm * LiftWave * SettlingScale);
	const FRotator AnimatedRotation(
		BurstTiltAxis.X * BurstTiltDegrees * TiltWave * Envelope,
		BurstTiltDegrees * 0.12f * TiltWave * Envelope,
		BurstTiltAxis.Y * BurstTiltDegrees * TiltWave * Envelope);
	LidPivot->SetRelativeLocationAndRotation(AnimatedLocation, AnimatedRotation);

	const int32 HitIndex = FMath::Min(FMath::FloorToInt(HitPhase), BurstHitCount - 1);
	if (HitIndex != LastPlayedHitIndex && LiftWave < 0.22f)
	{
		LastPlayedHitIndex = HitIndex;
		PlayLidClatter();
	}

	if (Alpha >= 1.0f)
	{
		FinishRattleBurst();
	}
}

void ATunaSweeperBoilingPotActor::SetBoiling(bool bEnabled)
{
	bBoiling = bEnabled;
	SetActorTickEnabled(bBoiling);

	if (SteamEffectComponent)
	{
		if (bBoiling)
		{
			SteamEffectComponent->Activate(true);
		}
		else
		{
			SteamEffectComponent->Deactivate();
		}
	}

	if (bBoiling)
	{
		TimeUntilNextBurst = GetRandomRange(0.05f, 0.2f);
	}
	else
	{
		bRattleBurstActive = false;
		ResetLidTransform();
	}
}

void ATunaSweeperBoilingPotActor::ConfigurePresentationDefaults(
	TSoftObjectPtr<UStaticMesh> InPotBodyMesh,
	TSoftObjectPtr<UStaticMesh> InLidMesh,
	TSoftObjectPtr<UNiagaraSystem> InSteamSystem,
	TSoftObjectPtr<USoundBase> InLidClatterSound)
{
	PotBodyMesh = MoveTemp(InPotBodyMesh);
	LidMesh = MoveTemp(InLidMesh);
	SteamSystem = MoveTemp(InSteamSystem);
	LidClatterSound = MoveTemp(InLidClatterSound);
	ApplyPresentation();
}

void ATunaSweeperBoilingPotActor::ApplyPresentation()
{
	MinPauseSeconds = FMath::Max(0.05f, MinPauseSeconds);
	MaxPauseSeconds = FMath::Max(MinPauseSeconds, MaxPauseSeconds);
	MinBurstDurationSeconds = FMath::Max(0.05f, MinBurstDurationSeconds);
	MaxBurstDurationSeconds = FMath::Max(MinBurstDurationSeconds, MaxBurstDurationSeconds);
	MinLidLiftCm = FMath::Max(0.0f, MinLidLiftCm);
	MaxLidLiftCm = FMath::Max(MinLidLiftCm, MaxLidLiftCm);
	MinTiltDegrees = FMath::Max(0.0f, MinTiltDegrees);
	MaxTiltDegrees = FMath::Max(MinTiltDegrees, MaxTiltDegrees);
	MinHitsPerBurst = FMath::Clamp(MinHitsPerBurst, 1, 8);
	MaxHitsPerBurst = FMath::Clamp(MaxHitsPerBurst, MinHitsPerBurst, 8);

	if (PotBodyMeshComponent)
	{
		PotBodyMeshComponent->SetStaticMesh(PotBodyMesh.LoadSynchronous());
	}
	if (LidMeshComponent)
	{
		LidMeshComponent->SetStaticMesh(LidMesh.LoadSynchronous());
	}
	if (SteamOrigin)
	{
		SteamOrigin->SetRelativeLocation(FVector(0.0f, 0.0f, SteamHeightCm));
	}
	if (SteamEffectComponent)
	{
		SteamEffectComponent->SetAsset(SteamSystem.LoadSynchronous());
		SteamEffectComponent->SetRelativeScale3D(FVector(FMath::Max(0.01f, SteamVisualScale)));
		SteamEffectComponent->SetVariableLinearColor(
			FName(TEXT("User.SmokeColor")),
			FLinearColor(0.86f, 0.93f, 1.0f, 1.0f));
	}
	if (LidClatterAudioComponent)
	{
		LidClatterAudioComponent->SetSound(LidClatterSound.LoadSynchronous());
	}
}

void ATunaSweeperBoilingPotActor::StartRattleBurst()
{
	bRattleBurstActive = true;
	BurstElapsedSeconds = 0.0f;
	BurstDurationSeconds = GetRandomRange(MinBurstDurationSeconds, MaxBurstDurationSeconds);
	BurstLiftCm = GetRandomRange(MinLidLiftCm, MaxLidLiftCm);
	BurstTiltDegrees = GetRandomRange(MinTiltDegrees, MaxTiltDegrees);
	BurstHitCount = RattleRandom.RandRange(MinHitsPerBurst, MaxHitsPerBurst);
	LastPlayedHitIndex = 0;

	const float AxisAngle = GetRandomRange(0.0f, UE_TWO_PI);
	BurstTiltAxis = FVector2D(FMath::Cos(AxisAngle), FMath::Sin(AxisAngle));
	PlayLidClatter();
}

void ATunaSweeperBoilingPotActor::FinishRattleBurst()
{
	bRattleBurstActive = false;
	TimeUntilNextBurst = GetRandomRange(MinPauseSeconds, MaxPauseSeconds);
	ResetLidTransform();
}

void ATunaSweeperBoilingPotActor::ResetLidTransform()
{
	if (!LidPivot)
	{
		return;
	}

	LidRestTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, LidRestHeightCm));
	LidPivot->SetRelativeTransform(LidRestTransform);
}

void ATunaSweeperBoilingPotActor::PlayLidClatter()
{
	if (LidClatterAudioComponent && LidClatterAudioComponent->Sound)
	{
		LidClatterAudioComponent->SetPitchMultiplier(GetRandomRange(0.92f, 1.08f));
		LidClatterAudioComponent->SetVolumeMultiplier(GetRandomRange(0.72f, 1.0f));
		LidClatterAudioComponent->Play(0.0f);
	}
}

float ATunaSweeperBoilingPotActor::GetRandomRange(float Minimum, float Maximum)
{
	return RattleRandom.FRandRange(FMath::Min(Minimum, Maximum), FMath::Max(Minimum, Maximum));
}
