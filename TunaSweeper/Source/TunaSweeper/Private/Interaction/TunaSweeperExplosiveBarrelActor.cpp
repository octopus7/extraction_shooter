#include "Interaction/TunaSweeperExplosiveBarrelActor.h"

#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperLocalExplosionEffectActor.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperCookableChickenActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName BurningFadeParameter(TEXT("User.Fade"));
	const FName ExplosionNoiseTag(TEXT("noise.explosion"));
	const FName StageSmokeIntensityParameter(TEXT("User.StageSmokeIntensity"));

	FVector SafeExtent(const FVector& Value)
	{
		return FVector(FMath::Max(1.0f, Value.X), FMath::Max(1.0f, Value.Y), FMath::Max(1.0f, Value.Z));
	}

	FTunaSweeperExplosiveBarrelVisualState MakeDefaultVisualState(const FVector& Extent, const FVector& SmokeLocation, const FVector& SmokeScale, float SmokeStrength)
	{
		FTunaSweeperExplosiveBarrelVisualState State;
		State.CollisionExtent = Extent;
		State.SmokeRelativeLocation = SmokeLocation;
		State.SmokeScale = SmokeScale;
		State.SmokeEmitterStrength = SmokeStrength;
		return State;
	}

	float GetConfiguredSmokeStrength(const FTunaSweeperExplosiveBarrelVisualState& State)
	{
		if (!FMath::IsNearlyEqual(State.SmokeEmitterStrength, 1.0f))
		{
			return FMath::Max(0.01f, State.SmokeEmitterStrength);
		}

		const FString AssetName = State.SmokeEffect.ToSoftObjectPath().GetAssetName();
		if (AssetName.Contains(TEXT("SmokeLight")))
		{
			return 0.65f;
		}
		if (AssetName.Contains(TEXT("SmokeHeavy")))
		{
			return 1.05f;
		}
		if (AssetName.Contains(TEXT("Burning")))
		{
			return 1.35f;
		}
		return FMath::Max(0.01f, State.SmokeEmitterStrength);
	}
}

ATunaSweeperExplosiveBarrelActor::ATunaSweeperExplosiveBarrelActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BarrelMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMeshComponent->SetupAttachment(RootComponent);
	BarrelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DamageSmokeEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DamageSmokeEffect"));
	DamageSmokeEffectComponent->SetupAttachment(RootComponent);
	DamageSmokeEffectComponent->SetAutoActivate(false);
	AttachedSmokeSprite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AttachedSmokeSprite"));
	AttachedSmokeSprite->SetupAttachment(RootComponent);
	AttachedSmokeSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachedSmokeSprite->SetGenerateOverlapEvents(false);
	AttachedSmokeSprite->SetCastShadow(false);
	AttachedSmokeSprite->SetTranslucentSortPriority(4);
	AttachedSmokeSprite->SetHiddenInGame(false);
	AttachedSmokeSprite->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SmokePlane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (SmokePlane.Succeeded())
	{
		AttachedSmokeSprite->SetStaticMesh(SmokePlane.Object);
	}
	DestroyedLoopEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DestroyedBurningEffect"));
	DestroyedLoopEffectComponent->SetupAttachment(RootComponent);
	DestroyedLoopEffectComponent->SetAutoActivate(false);
	DamageRadiusDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("DamageRadiusPreview"));
	DamageRadiusDecal->SetupAttachment(RootComponent);
	DamageRadiusDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	DamageRadiusDecal->SetHiddenInGame(true);
	DamageRadiusDecal->SetVisibility(false);
	DamageRadiusDecal->bUseAttachParentBound = true;
	// Visualization component metadata is editor-only in UE 5.7, so do not
	// reference the editor-only API from a Shipping build.
#if WITH_EDITORONLY_DATA
	DamageRadiusDecal->SetIsVisualizationComponent(true);
#endif
	VisualStates = {
		MakeDefaultVisualState(FVector(42.0f, 42.0f, 62.0f), FVector(0.0f, 0.0f, 118.0f), FVector::OneVector, 1.0f),
		MakeDefaultVisualState(FVector(44.0f, 42.0f, 61.0f), FVector(0.0f, 0.0f, 122.0f), FVector(0.62f), 0.65f),
		MakeDefaultVisualState(FVector(48.0f, 44.0f, 56.0f), FVector(0.0f, 0.0f, 116.0f), FVector(0.9f), 1.05f),
		MakeDefaultVisualState(FVector(42.0f, 42.0f, 18.0f), FVector(0.0f, 0.0f, 14.0f), FVector(0.8f), 1.35f)};
	DamageRadiusDecalMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Interaction/M_ExplosiveBarrel_DamageRadius.M_ExplosiveBarrel_DamageRadius")));
	AttachedSmokeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/M_LocalExplosionSmoke.M_LocalExplosionSmoke")));
	ExplosionEffectActorClass = TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>(FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperLocalExplosionEffectActor")));
}

void ATunaSweeperExplosiveBarrelActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	ExplosionVisualRadiusCm = FMath::Max(1.0f, ExplosionVisualRadiusCm);
	ExplosionDamageStrong = FMath::Max(0.0f, ExplosionDamageStrong);
	ExplosionDamageWeak = FMath::Clamp(ExplosionDamageWeak, 0.0f, ExplosionDamageStrong);
	ChainDetonationDelaySeconds = FMath::Max(0.01f, ChainDetonationDelaySeconds);
	ValidateDamageStageAdvanceDelay();
	DamageStageCount = GetSupportedDamageStageCount();
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDamageSmokeEffect();
	RefreshDestroyedLoopEffect();
	RefreshAttachedSmokeVisual();
	UpdateEditorDamageRadiusPreview();
}

void ATunaSweeperExplosiveBarrelActor::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	CurrentDamageStage = 0;
	bBarrelDestroyed = false;
	bChainDetonationPending = false;
	BurningElapsedSeconds = 0.0f;
	ChainDetonationDelaySeconds = FMath::Max(0.01f, ChainDetonationDelaySeconds);
	ValidateDamageStageAdvanceDelay();
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDamageSmokeEffect();
	RefreshDestroyedLoopEffect();
	RefreshAttachedSmokeVisual();
}

void ATunaSweeperExplosiveBarrelActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if WITH_EDITOR
	if (UWorld* World = GetWorld(); World && !World->IsGameWorld())
	{
		UpdateEditorDamageRadiusPreview();
		return;
	}
#endif
	UpdateStageSmokeCrossFade(DeltaSeconds);
	const bool bHasStageSmokeTransition = bIncomingStageSmokeFading || !FadingStageSmokeEffectComponents.IsEmpty();
	if (!bBarrelDestroyed)
	{
		if (!bHasStageSmokeTransition)
		{
			SetActorTickEnabled(false);
		}
		return;
	}
	if (BurningDurationSeconds <= 0.0f)
	{
		if (!bHasStageSmokeTransition)
		{
			SetActorTickEnabled(false);
		}
		return;
	}
	BurningElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float Fade = FMath::Clamp(1.0f - BurningElapsedSeconds / BurningDurationSeconds, 0.0f, 1.0f);
	DestroyedLoopEffectComponent->SetFloatParameter(BurningFadeParameter, Fade);
	DestroyedLoopEffectComponent->SetRelativeScale3D(FVector(Fade));
	SetAttachedSmokeOpacity(Fade);
	if (Fade <= 0.0f)
	{
		DestroyedLoopEffectComponent->Deactivate();
		if (AttachedSmokeSprite) AttachedSmokeSprite->SetVisibility(false);
		if (!bHasStageSmokeTransition)
		{
			SetActorTickEnabled(false);
		}
	}
}

#if WITH_EDITOR
bool ATunaSweeperExplosiveBarrelActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ATunaSweeperExplosiveBarrelActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!bBarrelDestroyed)
	{
		CurrentDamageStage = 0;
	}
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDamageSmokeEffect();
	RefreshDestroyedLoopEffect();
	RefreshAttachedSmokeVisual();
}
#endif

float ATunaSweeperExplosiveBarrelActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bBarrelDestroyed || bChainDetonationPending || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	if (const ATunaSweeperExplosiveBarrelActor* ExplodingBarrel = Cast<ATunaSweeperExplosiveBarrelActor>(DamageCauser);
		ExplodingBarrel && ExplodingBarrel != this)
	{
		const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
		ScheduleChainDetonation();
		return AppliedDamage;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		DestroyBarrel();
	}
	else
	{
		UpdateDamageStageFromHealth();
	}
	return AppliedDamage;
}

void ATunaSweeperExplosiveBarrelActor::SetDamageStageCount(int32 InDamageStageCount)
{
	DamageStageCount = FMath::Clamp(InDamageStageCount, 0, FMath::Max(0, VisualStates.Num() - 2));
	UpdateDamageStageFromHealth();
	ScheduleAutomaticDamageStageAdvance();
}

void ATunaSweeperExplosiveBarrelActor::ConfigureExplosiveBarrelDefaults(FName InBarrelId, float InMaxHealth, const TSoftObjectPtr<UStaticMesh>& InIntactMesh, const TSoftObjectPtr<UStaticMesh>& InDestroyedMesh, const TSoftObjectPtr<UNiagaraSystem>& InDestroyedLoopEffect, const TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>& InExplosionEffectActorClass, float InExplosionVisualRadiusCm, float InExplosionDurationSeconds)
{
	BarrelId = InBarrelId;
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	if (VisualStates.Num() < 2) VisualStates.SetNum(2);
	if (VisualStates.Num() >= 4)
	{
		VisualStates[0].SmokeRelativeLocation = FVector(0.0f, 0.0f, 118.0f);
		VisualStates[1].SmokeRelativeLocation = FVector(0.0f, 0.0f, 122.0f);
		VisualStates[2].SmokeRelativeLocation = FVector(0.0f, 0.0f, 116.0f);
		VisualStates[3].SmokeRelativeLocation = FVector(0.0f, 0.0f, 14.0f);
	}
	DestroyedLoopEffectRelativeLocation = FVector(0.0f, 0.0f, 14.0f);
	if (DamageRadiusDecal)
	{
		DamageRadiusDecal->bUseAttachParentBound = true;
	}
	// VisualStates is BP-authored source of truth. Spawn/default inputs fill only missing entries,
	// and must never replace an authored Step01/Step02/etc. state mesh.
	if (VisualStates[0].Mesh.IsNull() && BarrelMeshComponent && BarrelMeshComponent->GetStaticMesh())
	{
		VisualStates[0].Mesh = BarrelMeshComponent->GetStaticMesh();
	}
	else if (VisualStates[0].Mesh.IsNull() && !InIntactMesh.IsNull())
	{
		VisualStates[0].Mesh = InIntactMesh;
	}
	if (VisualStates.Last().Mesh.IsNull() && !InDestroyedMesh.IsNull()) VisualStates.Last().Mesh = InDestroyedMesh;
	if (VisualStates.Last().SmokeEffect.IsNull() && !InDestroyedLoopEffect.IsNull()) VisualStates.Last().SmokeEffect = InDestroyedLoopEffect;
	if (!InExplosionEffectActorClass.IsNull()) ExplosionEffectActorClass = InExplosionEffectActorClass;
	ExplosionVisualRadiusCm = FMath::Max(1.0f, InExplosionVisualRadiusCm);
	ExplosionDurationSeconds = FMath::Max(0.05f, InExplosionDurationSeconds);
	bUseNiagaraStageSmoke = true;
	ValidateDamageStageAdvanceDelay();
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDamageSmokeEffect();
	RefreshDestroyedLoopEffect();
	RefreshAttachedSmokeVisual();
}

int32 ATunaSweeperExplosiveBarrelActor::GetSupportedDamageStageCount() const
{
	return FMath::Clamp(DamageStageCount, 0, FMath::Max(0, VisualStates.Num() - 2));
}

int32 ATunaSweeperExplosiveBarrelActor::GetActiveVisualStateIndex() const
{
	if (VisualStates.IsEmpty()) return INDEX_NONE;
	return bBarrelDestroyed ? VisualStates.Num() - 1 : FMath::Clamp(CurrentDamageStage, 0, VisualStates.Num() - 2);
}

void ATunaSweeperExplosiveBarrelActor::UpdateDamageStageFromHealth()
{
	if (bBarrelDestroyed) return;
	const int32 StageCount = GetSupportedDamageStageCount();
	const int32 NewStage = StageCount > 0 ? FMath::Clamp(FMath::CeilToInt((1.0f - CurrentHealth / MaxHealth) * StageCount), 0, StageCount) : 0;
	if (CurrentDamageStage != NewStage)
	{
		BeginStageSmokeCrossFade();
		CurrentDamageStage = NewStage;
		ApplyCollisionDefaults();
		ApplyVisualState();
		RefreshDamageSmokeEffect();
		RefreshAttachedSmokeVisual();
		ScheduleAutomaticDamageStageAdvance();
	}
}

void ATunaSweeperExplosiveBarrelActor::ValidateDamageStageAdvanceDelay()
{
	if (DamageStageAdvanceDelaySeconds <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Explosive barrel '%s' received an invalid automatic stage delay (%f). Using 1.0 seconds."), *GetName(), DamageStageAdvanceDelaySeconds);
		DamageStageAdvanceDelaySeconds = 1.0f;
	}
}

void ATunaSweeperExplosiveBarrelActor::ScheduleAutomaticDamageStageAdvance()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	World->GetTimerManager().ClearTimer(DamageStageAdvanceTimerHandle);
	if (bBarrelDestroyed || CurrentDamageStage <= 0 || GetSupportedDamageStageCount() <= 0)
	{
		return;
	}
	ValidateDamageStageAdvanceDelay();
	World->GetTimerManager().SetTimer(
		DamageStageAdvanceTimerHandle,
		this,
		&ATunaSweeperExplosiveBarrelActor::AdvanceDamageStageAutomatically,
		DamageStageAdvanceDelaySeconds,
		false);
}

void ATunaSweeperExplosiveBarrelActor::AdvanceDamageStageAutomatically()
{
	if (bBarrelDestroyed)
	{
		return;
	}
	const int32 StageCount = GetSupportedDamageStageCount();
	if (CurrentDamageStage < StageCount)
	{
		BeginStageSmokeCrossFade();
		++CurrentDamageStage;
		ApplyCollisionDefaults();
		ApplyVisualState();
		RefreshDamageSmokeEffect();
		RefreshAttachedSmokeVisual();
		ScheduleAutomaticDamageStageAdvance();
		return;
	}
	DestroyBarrel();
}

void ATunaSweeperExplosiveBarrelActor::ScheduleChainDetonation()
{
	UWorld* World = GetWorld();
	if (!World || bBarrelDestroyed || bChainDetonationPending)
	{
		return;
	}

	bChainDetonationPending = true;
	CurrentHealth = 0.0f;
	World->GetTimerManager().ClearTimer(DamageStageAdvanceTimerHandle);
	ChainDetonationDelaySeconds = FMath::Max(0.01f, ChainDetonationDelaySeconds);
	World->GetTimerManager().SetTimer(
		ChainDetonationTimerHandle,
		this,
		&ATunaSweeperExplosiveBarrelActor::DetonateFromChainReaction,
		ChainDetonationDelaySeconds,
		false);
}

void ATunaSweeperExplosiveBarrelActor::DetonateFromChainReaction()
{
	bChainDetonationPending = false;
	DestroyBarrel();
}

void ATunaSweeperExplosiveBarrelActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision) return;
	const int32 StateIndex = GetActiveVisualStateIndex();
	const FVector Extent = StateIndex == INDEX_NONE ? FVector(42.0f, 42.0f, 62.0f) : SafeExtent(VisualStates[StateIndex].CollisionExtent);
	BlockingCollision->SetRelativeLocation(FVector(0.0f, 0.0f, Extent.Z));
	BlockingCollision->SetBoxExtent(Extent);
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
}

void ATunaSweeperExplosiveBarrelActor::ApplyVisualState()
{
	if (!BarrelMeshComponent) return;
	const int32 StateIndex = GetActiveVisualStateIndex();
	if (StateIndex == INDEX_NONE) return;
	const FTunaSweeperExplosiveBarrelVisualState& State = VisualStates[StateIndex];
	BarrelMeshComponent->SetStaticMesh(State.Mesh.LoadSynchronous());
	BarrelMeshComponent->EmptyOverrideMaterials();
	if (State.bOverrideMaterial)
	{
		if (UMaterialInterface* Material = State.Material.LoadSynchronous())
		{
			BarrelMeshComponent->SetMaterial(0, Material);
		}
	}
}

void ATunaSweeperExplosiveBarrelActor::RefreshDamageSmokeEffect()
{
	if (!DamageSmokeEffectComponent) return;
	const int32 StateIndex = GetActiveVisualStateIndex();
	if (!bUseNiagaraStageSmoke || bBarrelDestroyed || StateIndex == INDEX_NONE)
	{
		DamageSmokeEffectComponent->Deactivate();
		return;
	}
	const FTunaSweeperExplosiveBarrelVisualState& State = VisualStates[StateIndex];
	DamageSmokeEffectComponent->SetRelativeLocation(State.SmokeRelativeLocation);
	DamageSmokeEffectComponent->SetRelativeScale3D(State.SmokeScale);
	if (UNiagaraSystem* Effect = State.SmokeEffect.LoadSynchronous())
	{
		DamageSmokeEffectComponent->SetAsset(Effect);
		SetStageSmokeIntensity(
			DamageSmokeEffectComponent,
			GetConfiguredSmokeStrength(State),
			bIncomingStageSmokeFading ? 0.0f : 1.0f);
		IncomingStageSmokeStrength = GetConfiguredSmokeStrength(State);
		DamageSmokeEffectComponent->Activate(true);
	}
	else DamageSmokeEffectComponent->Deactivate();
}

void ATunaSweeperExplosiveBarrelActor::BeginStageSmokeCrossFade()
{
	if (!bUseNiagaraStageSmoke || StageSmokeCrossFadeSeconds <= 0.0f || !DamageSmokeEffectComponent)
	{
		return;
	}

	const int32 StateIndex = GetActiveVisualStateIndex();
	if (!VisualStates.IsValidIndex(StateIndex) || VisualStates[StateIndex].SmokeEffect.IsNull())
	{
		return;
	}

	FadingStageSmokeEffectComponents.Add(DamageSmokeEffectComponent);
	FadingStageSmokeElapsedSeconds.Add(0.0f);
	FadingStageSmokeStrengths.Add(GetConfiguredSmokeStrength(VisualStates[StateIndex]));
	UE_LOG(LogTemp, Log, TEXT("Explosive barrel '%s': preserving stage %d smoke for %.2fs cross-fade."), *GetName(), StateIndex, StageSmokeCrossFadeSeconds);
	DamageSmokeEffectComponent = CreateIncomingStageSmokeComponent();
	IncomingStageSmokeElapsedSeconds = 0.0f;
	IncomingStageSmokeStrength = 1.0f;
	bIncomingStageSmokeFading = DamageSmokeEffectComponent != nullptr;
	SetActorTickEnabled(bIncomingStageSmokeFading);
}

UNiagaraComponent* ATunaSweeperExplosiveBarrelActor::CreateIncomingStageSmokeComponent()
{
	if (!RootComponent)
	{
		return nullptr;
	}

	UNiagaraComponent* IncomingComponent = NewObject<UNiagaraComponent>(this, MakeUniqueObjectName(this, UNiagaraComponent::StaticClass(), TEXT("DamageSmokeEffectIncoming")));
	if (!IncomingComponent)
	{
		return nullptr;
	}

	IncomingComponent->SetupAttachment(RootComponent);
	IncomingComponent->SetAutoActivate(false);
	AddInstanceComponent(IncomingComponent);
	IncomingComponent->RegisterComponent();
	return IncomingComponent;
}

void ATunaSweeperExplosiveBarrelActor::SetStageSmokeIntensity(
	UNiagaraComponent* EffectComponent,
	float SmokeEmitterStrength,
	float Intensity) const
{
	if (!EffectComponent)
	{
		return;
	}

	const float SafeStrength = FMath::Max(0.01f, SmokeEmitterStrength);
	const float SafeIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	EffectComponent->SetFloatParameter(StageSmokeIntensityParameter, SafeIntensity);
	EffectComponent->SetFloatParameter(BurningFadeParameter, SafeIntensity);
	// Sprite smoke has no fluid source to inject. These optional user parameters let a
	// compatible Niagara setup honour the existing stage-transition fade without Grid3D.
	EffectComponent->SetFloatParameter(BurningFadeParameter, SafeIntensity * SafeStrength);
}

void ATunaSweeperExplosiveBarrelActor::UpdateStageSmokeCrossFade(float DeltaSeconds)
{
	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float Duration = FMath::Max(KINDA_SMALL_NUMBER, StageSmokeCrossFadeSeconds);
	for (int32 Index = FadingStageSmokeEffectComponents.Num() - 1; Index >= 0; --Index)
	{
		UNiagaraComponent* FadingComponent = FadingStageSmokeEffectComponents[Index];
		FadingStageSmokeElapsedSeconds[Index] += SafeDeltaSeconds;
		const float Intensity = 1.0f - FMath::Clamp(FadingStageSmokeElapsedSeconds[Index] / Duration, 0.0f, 1.0f);
		SetStageSmokeIntensity(FadingComponent, FadingStageSmokeStrengths[Index], Intensity);
		if (Intensity <= 0.0f)
		{
			if (FadingComponent)
			{
				FadingComponent->Deactivate();
				if (!FadingComponent->IsDefaultSubobject())
				{
					FadingComponent->DestroyComponent();
				}
			}
			FadingStageSmokeEffectComponents.RemoveAtSwap(Index);
			FadingStageSmokeElapsedSeconds.RemoveAtSwap(Index);
			FadingStageSmokeStrengths.RemoveAtSwap(Index);
		}
	}

	if (bIncomingStageSmokeFading)
	{
		IncomingStageSmokeElapsedSeconds += SafeDeltaSeconds;
		const float Intensity = FMath::Clamp(IncomingStageSmokeElapsedSeconds / Duration, 0.0f, 1.0f);
		SetStageSmokeIntensity(DamageSmokeEffectComponent, IncomingStageSmokeStrength, Intensity);
		if (Intensity >= 1.0f)
		{
			bIncomingStageSmokeFading = false;
		}
	}
}

void ATunaSweeperExplosiveBarrelActor::RefreshDestroyedLoopEffect()
{
	if (!DestroyedLoopEffectComponent || !bUseNiagaraStageSmoke || !bBarrelDestroyed || VisualStates.IsEmpty())
	{
		if (DestroyedLoopEffectComponent) DestroyedLoopEffectComponent->Deactivate();
		return;
	}
	const FTunaSweeperExplosiveBarrelVisualState& State = VisualStates.Last();
	DestroyedLoopEffectComponent->SetRelativeLocation(DestroyedLoopEffectRelativeLocation);
	DestroyedLoopEffectComponent->SetRelativeScale3D(State.SmokeScale);
	if (UNiagaraSystem* Effect = State.SmokeEffect.LoadSynchronous())
	{
		DestroyedLoopEffectComponent->SetAsset(Effect);
		DestroyedLoopEffectComponent->SetFloatParameter(BurningFadeParameter, 1.0f);
		DestroyedLoopEffectComponent->Activate(true);
		SetActorTickEnabled(BurningDurationSeconds > 0.0f);
	}
}

void ATunaSweeperExplosiveBarrelActor::RefreshAttachedSmokeVisual()
{
	if (!AttachedSmokeSprite)
	{
		return;
	}
	const int32 StateIndex = GetActiveVisualStateIndex();
	if (StateIndex == INDEX_NONE || (!bBarrelDestroyed && StateIndex == 0))
	{
		AttachedSmokeSprite->SetVisibility(false);
		return;
	}

	const FTunaSweeperExplosiveBarrelVisualState& State = VisualStates[StateIndex];
	if (bUseNiagaraStageSmoke && !State.SmokeEffect.IsNull())
	{
		AttachedSmokeSprite->SetVisibility(false);
		return;
	}
	const bool bDestroyed = bBarrelDestroyed;
	AttachedSmokeSprite->SetRelativeLocation(bDestroyed ? DestroyedLoopEffectRelativeLocation : State.SmokeRelativeLocation);
	const float BaseScale = bDestroyed ? 1.65f : StateIndex == 1 ? 1.45f : 1.95f;
	AttachedSmokeSprite->SetRelativeScale3D(State.SmokeScale * BaseScale);
	if (!AttachedSmokeDynamicMaterial)
	{
		UMaterialInterface* Material = AttachedSmokeMaterial.LoadSynchronous();
		if (!Material)
		{
			Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Effects/M_LocalExplosionSmoke.M_LocalExplosionSmoke"));
		}
		if (Material)
		{
			AttachedSmokeDynamicMaterial = AttachedSmokeSprite->CreateDynamicMaterialInstance(0, Material);
			if (!AttachedSmokeDynamicMaterial)
			{
				AttachedSmokeSprite->SetMaterial(0, Material);
			}
		}
	}
	if (AttachedSmokeDynamicMaterial)
	{
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("FrameScale"), 0.25f);
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("FrameU"), 0.0f);
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("FrameV"), 0.5f);
		AttachedSmokeDynamicMaterial->SetVectorParameterValue(
			TEXT("TintColor"),
			bDestroyed ? FLinearColor(1.0f, 0.075f, 0.008f, 1.0f) : FLinearColor(0.045f, 0.012f, 0.004f, 1.0f));
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), bDestroyed ? 2.3f : 0.0f);
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("AlphaBoost"), bDestroyed ? 2.1f : 1.7f);
	}
	SetAttachedSmokeOpacity(bDestroyed ? 0.95f : StateIndex == 1 ? 0.54f : 0.78f);
	AttachedSmokeSprite->SetVisibility(true);
}

void ATunaSweeperExplosiveBarrelActor::SetAttachedSmokeOpacity(float Opacity)
{
	if (AttachedSmokeDynamicMaterial)
	{
		AttachedSmokeDynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

void ATunaSweeperExplosiveBarrelActor::ApplyExplosionDamage()
{
	UWorld* World = GetWorld();
	if (!World || ExplosionVisualRadiusCm <= 0.0f || ExplosionDamageStrong <= 0.0f) return;
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQuery.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TunaSweeperExplosiveBarrelOverlap), false, this);
	if (!World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQuery, FCollisionShape::MakeSphere(ExplosionVisualRadiusCm), Query)) return;
	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Target = Result.GetActor();
		if (!IsValid(Target) || Target == this || DamagedActors.Contains(Target)) continue;
		if (!Target->IsA<APawn>() && !Target->IsA<ATunaSweeperCookableChickenActor>() && !Target->IsA<ATunaSweeperExplosiveBarrelActor>()) continue;
		DamagedActors.Add(Target);
		const float Distance = FVector::Distance(GetActorLocation(), Target->GetActorLocation());
		const float Alpha = FMath::Clamp((Distance - ExplosionDamageInnerRadiusCm) / FMath::Max(1.0f, ExplosionVisualRadiusCm - ExplosionDamageInnerRadiusCm), 0.0f, 1.0f);
		UGameplayStatics::ApplyDamage(Target, FMath::Lerp(ExplosionDamageStrong, ExplosionDamageWeak, Alpha), GetInstigatorController(), this, UDamageType::StaticClass());
	}
}

void ATunaSweeperExplosiveBarrelActor::UpdateEditorDamageRadiusPreview()
{
	if (!DamageRadiusDecal) return;
	DamageRadiusDecal->SetDecalMaterial(DamageRadiusDecalMaterial.LoadSynchronous());
	DamageRadiusDecal->DecalSize = FVector(20.0f, ExplosionVisualRadiusCm, ExplosionVisualRadiusCm);
	DamageRadiusDecal->MarkRenderStateDirty();
#if WITH_EDITOR
	DamageRadiusDecal->SetVisibility(IsSelected());
#else
	DamageRadiusDecal->SetVisibility(false);
#endif
}

void ATunaSweeperExplosiveBarrelActor::DestroyBarrel()
{
	if (bBarrelDestroyed) return;
	BeginStageSmokeCrossFade();
	bBarrelDestroyed = true;
	bChainDetonationPending = false;
	GetWorldTimerManager().ClearTimer(DamageStageAdvanceTimerHandle);
	GetWorldTimerManager().ClearTimer(ChainDetonationTimerHandle);
	CurrentDamageStage = GetSupportedDamageStageCount();
	BurningElapsedSeconds = 0.0f;
	SpawnExplosionEffect();
	ApplyExplosionDamage();
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDamageSmokeEffect();
	RefreshDestroyedLoopEffect();
}

void ATunaSweeperExplosiveBarrelActor::SpawnExplosionEffect()
{
	UWorld* World = GetWorld();
	if (!World) return;
	TSubclassOf<ATunaSweeperLocalExplosionEffectActor> Class = ExplosionEffectActorClass.LoadSynchronous();
	if (!Class) Class = ATunaSweeperLocalExplosionEffectActor::StaticClass();
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location = GetActorLocation() + GetActorTransform().TransformVectorNoScale(ExplosionEffectOffset);
	if (ATunaSweeperLocalExplosionEffectActor* Explosion = World->SpawnActor<ATunaSweeperLocalExplosionEffectActor>(Class, Location, GetActorRotation(), Params)) Explosion->ConfigureExplosion(ExplosionVisualRadiusCm, ExplosionDurationSeconds);
	if (UTunaSweeperNoiseSubsystem* Noise = World->GetSubsystem<UTunaSweeperNoiseSubsystem>()) Noise->ReportNoiseAtLocation(Location, 1.25f, FMath::Clamp(ExplosionVisualRadiusCm * 10.0f, 2000.0f, 4200.0f), ExplosionNoiseTag, this, GetInstigator());
}
