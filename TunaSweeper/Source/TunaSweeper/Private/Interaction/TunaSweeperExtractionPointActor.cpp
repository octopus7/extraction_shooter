#include "Interaction/TunaSweeperExtractionPointActor.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperRaidExperienceReturnSubsystem.h"
#include "UI/TunaSweeperExtractionProgressWidget.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* ExtractionVisualMaterialPath = TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive");
	const TCHAR* SmokeSignalMaterialPath = TEXT("/Game/Effects/M_LocalExplosionFlipbook.M_LocalExplosionFlipbook");
	const TCHAR* LevelTransitionWidgetClassPath = TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C");
	constexpr int32 FallbackParticleCount = 10;
	constexpr int32 SmokeSignalSpriteCount = 18;
	constexpr float ExtractionSmokePlaneMeshSizeCm = 100.0f;

	float SmoothExtractionSmokeAlpha(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float RangeExtractionSmokeAlpha(float Start, float End, float Value)
	{
		if (FMath::IsNearlyEqual(Start, End))
		{
			return Value >= End ? 1.0f : 0.0f;
		}

		return FMath::Clamp((Value - Start) / (End - Start), 0.0f, 1.0f);
	}

	void ApplyExtractionColorParameters(UMaterialInstanceDynamic* DynamicMaterial, const FLinearColor& Color)
	{
		if (!DynamicMaterial)
		{
			return;
		}

		const FLinearColor EmissiveColor = Color * 4.5f;
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("LedColor"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
		DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), 4.5f);
		DynamicMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), 4.5f);
		DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), 4.5f);
	}
}

ATunaSweeperExtractionPointActor::ATunaSweeperExtractionPointActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ExtractionArea = CreateDefaultSubobject<USphereComponent>(TEXT("ExtractionArea"));
	ExtractionArea->SetupAttachment(RootComponent);
	ExtractionArea->SetSphereRadius(ExtractionRadius);
	ExtractionArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExtractionArea->SetGenerateOverlapEvents(false);

	RadiusVisualMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RadiusVisualMesh"));
	RadiusVisualMesh->SetupAttachment(RootComponent);
	RadiusVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RadiusVisualMesh->SetGenerateOverlapEvents(false);
	RadiusVisualMesh->SetCastShadow(false);

	ExtractionEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ExtractionEffect"));
	ExtractionEffectComponent->SetupAttachment(RootComponent);
	ExtractionEffectComponent->SetAutoActivate(true);
	ExtractionEffectComponent->SetRelativeLocation(ExtractionNiagaraRelativeLocation);
	ExtractionEffectComponent->SetRelativeRotation(ExtractionNiagaraRelativeRotation);
	ExtractionEffectComponent->SetRelativeScale3D(ExtractionNiagaraRelativeScale);

	ProgressWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ExtractionProgressWidget"));
	ProgressWidgetComponent->SetupAttachment(RootComponent);
	ProgressWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ProgressWidgetComponent->SetWidgetClass(UTunaSweeperExtractionProgressWidget::StaticClass());
	ProgressWidgetComponent->SetDrawSize(ProgressWidgetDrawSize);
	ProgressWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	ProgressWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ProgressWidgetHeightOffset));
	ProgressWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProgressWidgetComponent->SetGenerateOverlapEvents(false);

	RadiusVisualMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(ExtractionVisualMaterialPath));
	SmokeSignalSpriteMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(SmokeSignalMaterialPath));
	TransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(FSoftObjectPath(LevelTransitionWidgetClassPath));
	TransitionMessage = FText::FromString(TEXT("Returning to Bunker"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* ParticleMesh = SphereMesh.Succeeded() ? SphereMesh.Object : nullptr;
	FallbackParticleMeshes.Reserve(FallbackParticleCount);
	for (int32 Index = 0; Index < FallbackParticleCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("FallbackGreenParticle%d"), Index));
		UStaticMeshComponent* ParticleComponent = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		ParticleComponent->SetupAttachment(RootComponent);
		ParticleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ParticleComponent->SetGenerateOverlapEvents(false);
		ParticleComponent->SetCastShadow(false);
		ParticleComponent->SetVisibility(true);
		ParticleComponent->SetHiddenInGame(false);
		if (ParticleMesh)
		{
			ParticleComponent->SetStaticMesh(ParticleMesh);
		}
		FallbackParticleMeshes.Add(ParticleComponent);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* SmokeSpriteMesh = PlaneMesh.Succeeded() ? PlaneMesh.Object : nullptr;
	SmokeSignalSprites.Reserve(SmokeSignalSpriteCount);
	for (int32 Index = 0; Index < SmokeSignalSpriteCount; ++Index)
	{
		const FName ComponentName(*FString::Printf(TEXT("SmokeSignalSprite%d"), Index));
		UStaticMeshComponent* SmokeSprite = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		SmokeSprite->SetupAttachment(RootComponent);
		SmokeSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SmokeSprite->SetGenerateOverlapEvents(false);
		SmokeSprite->SetCastShadow(false);
		SmokeSprite->SetCanEverAffectNavigation(false);
		SmokeSprite->SetTranslucentSortPriority(4 + Index);
		SmokeSprite->SetVisibility(true);
		SmokeSprite->SetHiddenInGame(false);
		if (SmokeSpriteMesh)
		{
			SmokeSprite->SetStaticMesh(SmokeSpriteMesh);
		}
		SmokeSignalSprites.Add(SmokeSprite);
	}
}

void ATunaSweeperExtractionPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshExtractionComponents();
	ApplySmokeSignalMaterials();
	UpdateProgressWidget();
	UpdateFallbackParticleEffect(0.0f);
	UpdateSmokeSignalEffect(0.0f);
}

void ATunaSweeperExtractionPointActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshExtractionComponents();
	ApplyFallbackParticleMaterials();
	ApplySmokeSignalMaterials();
	UpdateProgressWidget();
	UpdateSmokeSignalEffect(0.0f);
}

void ATunaSweeperExtractionPointActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFallbackParticleEffect(DeltaSeconds);
	UpdateSmokeSignalEffect(DeltaSeconds);
	UpdateExtractionProgress(DeltaSeconds);
}

void ATunaSweeperExtractionPointActor::ConfigureExtractionPointDefaults(
	FName InTargetLevelName,
	float InExtractionRadius,
	float InExtractionHoldSeconds,
	float InRadiusRingWidth,
	TSoftClassPtr<UTunaSweeperExtractionProgressWidget> InProgressWidgetClass,
	TSoftObjectPtr<UNiagaraSystem> InExtractionParticleSystem,
	TSoftObjectPtr<UMaterialInterface> InRadiusVisualMaterial,
	TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
	const FText& InTransitionMessage)
{
	Modify();
	TargetLevelName = InTargetLevelName;
	ExtractionRadius = FMath::Max(1.0f, InExtractionRadius);
	ExtractionHoldSeconds = FMath::Max(0.1f, InExtractionHoldSeconds);
	RadiusRingWidth = FMath::Max(1.0f, InRadiusRingWidth);
	ProgressWidgetClass = InProgressWidgetClass;
	ExtractionParticleSystem = InExtractionParticleSystem;
	if (!InRadiusVisualMaterial.IsNull())
	{
		RadiusVisualMaterial = InRadiusVisualMaterial;
	}
	TransitionMediaSource = InTransitionMediaSource;
	if (!InTransitionWidgetClass.IsNull())
	{
		TransitionWidgetClass = InTransitionWidgetClass;
	}
	TransitionMessage = InTransitionMessage;
	RefreshExtractionComponents();
	UpdateProgressWidget();
}

void ATunaSweeperExtractionPointActor::SetSmokeSignalWind(FVector2D InWindDirection, float InWindSpeedCmPerSecond)
{
	SmokeSignalWindDirection = InWindDirection.GetSafeNormal(0.0f);
	if (SmokeSignalWindDirection.IsNearlyZero())
	{
		SmokeSignalWindDirection = FVector2D(1.0f, 0.0f);
	}

	SmokeSignalWindSpeedCmPerSecond = FMath::Max(0.0f, InWindSpeedCmPerSecond);
	ApplyExtractionNiagaraParameters();
}

float ATunaSweeperExtractionPointActor::GetCurrentHoldProgress() const
{
	return ExtractionHoldSeconds > 0.0f
		? FMath::Clamp(CurrentHoldSeconds / ExtractionHoldSeconds, 0.0f, 1.0f)
		: 0.0f;
}

bool ATunaSweeperExtractionPointActor::ExtractPawn(APawn* InstigatorPawn)
{
	if (bExtractionTriggered || TargetLevelName.IsNone() || !CanExtractPawn(InstigatorPawn))
	{
		return false;
	}

	bExtractionTriggered = true;
	StopPawnForExtraction(InstigatorPawn);
	if (ProgressWidgetComponent)
	{
		ProgressWidgetComponent->SetVisibility(false);
	}

	const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
	UObject* WorldContextObject = InstigatorPawn ? Cast<UObject>(InstigatorPawn) : Cast<UObject>(this);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance);
		if (TunaGameInstance)
		{
			TunaGameInstance->HandleLevelTravelPersistence(SourceLevelName, TargetLevelName);
		}

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyLevelTravelRequested(SourceLevelName, TargetLevelName);
		}

		if (TunaGameInstance && TunaGameInstance->HasPendingRaidExperienceAnimationState())
		{
			if (UTunaSweeperRaidExperienceReturnSubsystem* ExperienceReturnSubsystem =
				GameInstance->GetSubsystem<UTunaSweeperRaidExperienceReturnSubsystem>())
			{
				if (ExperienceReturnSubsystem->StartReturnPresentation(
					WorldContextObject,
					TargetLevelName,
					TransitionMediaSource,
					TransitionWidgetClass,
					FadeToBlackDuration,
					FadeFromBlackDuration,
					TransitionMessage))
				{
					return true;
				}
			}
		}

		if (!TransitionMediaSource.IsNull() && !TransitionWidgetClass.IsNull())
		{
			if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
			{
				if (TransitionSubsystem->StartTransition(
					WorldContextObject,
					TargetLevelName,
					TransitionMediaSource,
					TransitionWidgetClass,
					FadeToBlackDuration,
					FadeFromBlackDuration,
					TransitionMessage))
				{
					return true;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}

void ATunaSweeperExtractionPointActor::RefreshExtractionComponents()
{
	ExtractionRadius = FMath::Max(1.0f, ExtractionRadius);
	ExtractionHoldSeconds = FMath::Max(0.1f, ExtractionHoldSeconds);

	if (ExtractionArea)
	{
		ExtractionArea->SetSphereRadius(ExtractionRadius);
	}

	RebuildRadiusVisualMesh();
	ApplyRadiusVisualMaterial();
	RefreshEffectComponent();
	RefreshProgressWidgetComponent();
}

void ATunaSweeperExtractionPointActor::RebuildRadiusVisualMesh()
{
	if (!RadiusVisualMesh)
	{
		return;
	}

	const int32 SegmentCount = FMath::Clamp(RadiusVisualSegments, 12, 256);
	const float OuterRadius = FMath::Max(1.0f, ExtractionRadius);
	const float InnerRadius = FMath::Max(0.0f, OuterRadius - FMath::Max(1.0f, RadiusRingWidth));

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve((SegmentCount + 1) * 2);
	Normals.Reserve((SegmentCount + 1) * 2);
	UV0.Reserve((SegmentCount + 1) * 2);
	VertexColors.Reserve((SegmentCount + 1) * 2);
	Tangents.Reserve((SegmentCount + 1) * 2);
	Triangles.Reserve(SegmentCount * 6);

	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float Angle = Alpha * 2.0f * PI;
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		Vertices.Add(Direction * InnerRadius + FVector(0.0f, 0.0f, RadiusVisualZOffset));
		Vertices.Add(Direction * OuterRadius + FVector(0.0f, 0.0f, RadiusVisualZOffset));
		Normals.Add(FVector::UpVector);
		Normals.Add(FVector::UpVector);
		UV0.Add(FVector2D(Alpha, 0.0f));
		UV0.Add(FVector2D(Alpha, 1.0f));
		VertexColors.Add(RadiusVisualColor);
		VertexColors.Add(RadiusVisualColor);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
	}

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const int32 InnerA = SegmentIndex * 2;
		const int32 OuterA = InnerA + 1;
		const int32 InnerB = InnerA + 2;
		const int32 OuterB = InnerA + 3;

		Triangles.Add(InnerA);
		Triangles.Add(OuterA);
		Triangles.Add(OuterB);
		Triangles.Add(InnerA);
		Triangles.Add(OuterB);
		Triangles.Add(InnerB);
	}

	RadiusVisualMesh->ClearAllMeshSections();
	RadiusVisualMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		false);
	RadiusVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RadiusVisualMesh->SetGenerateOverlapEvents(false);
	RadiusVisualMesh->SetCastShadow(false);
}

void ATunaSweeperExtractionPointActor::ApplyRadiusVisualMaterial()
{
	if (!RadiusVisualMesh)
	{
		return;
	}

	UMaterialInterface* Material = RadiusVisualMaterial.IsNull()
		? nullptr
		: RadiusVisualMaterial.LoadSynchronous();
	if (!Material)
	{
		Material = LoadObject<UMaterialInterface>(nullptr, ExtractionVisualMaterialPath);
	}
	if (!Material)
	{
		return;
	}

	RadiusVisualDynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
	ApplyExtractionColorParameters(RadiusVisualDynamicMaterial, RadiusVisualColor);
	RadiusVisualMesh->SetMaterial(0, RadiusVisualDynamicMaterial);
}

void ATunaSweeperExtractionPointActor::RefreshEffectComponent()
{
	bHasActiveNiagaraExtractionEffect = false;
	if (!ExtractionEffectComponent)
	{
		return;
	}

	const FVector SafeNiagaraScale(
		FMath::Max(0.01f, FMath::Abs(ExtractionNiagaraRelativeScale.X)),
		FMath::Max(0.01f, FMath::Abs(ExtractionNiagaraRelativeScale.Y)),
		FMath::Max(0.01f, FMath::Abs(ExtractionNiagaraRelativeScale.Z)));
	ExtractionEffectComponent->SetRelativeLocation(ExtractionNiagaraRelativeLocation);
	ExtractionEffectComponent->SetRelativeRotation(ExtractionNiagaraRelativeRotation);
	ExtractionEffectComponent->SetRelativeScale3D(SafeNiagaraScale);
	if (UNiagaraSystem* LoadedSystem = ExtractionParticleSystem.LoadSynchronous())
	{
		ExtractionEffectComponent->SetAsset(LoadedSystem);
		ApplyExtractionNiagaraParameters();
		bHasActiveNiagaraExtractionEffect = true;
		ExtractionEffectComponent->Activate(true);
	}
	else
	{
		ExtractionEffectComponent->Deactivate();
	}
}

void ATunaSweeperExtractionPointActor::ApplyExtractionNiagaraParameters()
{
	if (!ExtractionEffectComponent)
	{
		return;
	}

	const FVector WindVelocity = GetSmokeSignalWindVelocity();
	FVector2D WindDirection(WindVelocity.X, WindVelocity.Y);
	WindDirection = WindDirection.IsNearlyZero() ? FVector2D(1.0f, 0.0f) : WindDirection.GetSafeNormal();
	const FLinearColor GreenSmokeColor(0.10f, 0.58f, 0.16f, 1.0f);
	const float SourceRadius = FMath::Max(8.0f, SmokeSignalBaseDiameter * 0.5f);
	const float TopRadius = FMath::Max(SourceRadius, SmokeSignalTopDiameter * 0.5f);
	const FQuat NiagaraRotation = ExtractionNiagaraRelativeRotation.Quaternion();
	const FVector NiagaraUpAxis = NiagaraRotation.RotateVector(FVector::UpVector).GetSafeNormal();
	const FVector NiagaraForwardAxis = NiagaraRotation.RotateVector(FVector::ForwardVector).GetSafeNormal();

	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.Color")), SmokeSignalBaseColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.BaseColor")), SmokeSignalBaseColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.SourceColor")), SmokeSignalBaseColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.SmokeBaseColor")), SmokeSignalBaseColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.SmokeMidColor")), GreenSmokeColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.SmokeColor")), SmokeSignalTopColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.TopColor")), SmokeSignalTopColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.Albedo")), SmokeSignalTopColor);
	ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.TintColor")), SmokeSignalTopColor);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.Wind")), WindVelocity);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.WindVelocity")), WindVelocity);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.Wind Direction")), FVector(WindDirection.X, WindDirection.Y, 0.0f));
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.UpVector")), NiagaraUpAxis);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.PlumeAxis")), NiagaraUpAxis);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.SignalAxis")), NiagaraUpAxis);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.EmitDirection")), NiagaraUpAxis);
	ExtractionEffectComponent->SetVariableVec3(FName(TEXT("User.ForwardVector")), NiagaraForwardAxis);
	ExtractionEffectComponent->SetVariableVec2(FName(TEXT("User.WindDirection")), WindDirection);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.WindSpeed")), FMath::Max(0.0f, SmokeSignalWindSpeedCmPerSecond));
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.SourceRadius")), SourceRadius);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.SmokeSourceRadius")), SourceRadius);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.PlumeHeight")), FMath::Max(120.0f, SmokeSignalColumnHeight));
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.SmokeRadius")), TopRadius);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.Density")), 1.35f);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.DensityScale")), 1.35f);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.Temperature")), 0.18f);
	ExtractionEffectComponent->SetVariableFloat(FName(TEXT("User.Buoyancy")), 1.1f);
}

void ATunaSweeperExtractionPointActor::RefreshProgressWidgetComponent()
{
	if (!ProgressWidgetComponent)
	{
		return;
	}

	TSubclassOf<UTunaSweeperExtractionProgressWidget> LoadedWidgetClass = ProgressWidgetClass.IsNull()
		? UTunaSweeperExtractionProgressWidget::StaticClass()
		: ProgressWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		LoadedWidgetClass = UTunaSweeperExtractionProgressWidget::StaticClass();
	}

	ProgressWidgetComponent->SetWidgetClass(LoadedWidgetClass);
	ProgressWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ProgressWidgetComponent->SetDrawSize(ProgressWidgetDrawSize);
	ProgressWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	ProgressWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, ProgressWidgetHeightOffset));
	ProgressWidgetComponent->SetVisibility(bShowProgressWidget && !bExtractionTriggered && CurrentHoldSeconds > 0.0f);
	ProgressWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProgressWidgetComponent->SetGenerateOverlapEvents(false);
}

void ATunaSweeperExtractionPointActor::UpdateExtractionProgress(float DeltaSeconds)
{
	if (bExtractionTriggered)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!CanExtractPawn(PlayerPawn) || !IsPawnInsideExtractionArea(PlayerPawn))
	{
		ResetHoldProgress();
		UpdateProgressWidget();
		return;
	}

	CurrentHoldSeconds = FMath::Min(
		CurrentHoldSeconds + FMath::Max(0.0f, DeltaSeconds),
		ExtractionHoldSeconds);
	UpdateProgressWidget();

	if (CurrentHoldSeconds >= ExtractionHoldSeconds)
	{
		ExtractPawn(PlayerPawn);
	}
}

void ATunaSweeperExtractionPointActor::UpdateProgressWidget()
{
	if (!ProgressWidgetComponent)
	{
		return;
	}

	const bool bShouldShowProgress = bShowProgressWidget && !bExtractionTriggered && CurrentHoldSeconds > 0.0f;
	ProgressWidgetComponent->SetVisibility(bShouldShowProgress);
	if (UTunaSweeperExtractionProgressWidget* ProgressWidget =
		Cast<UTunaSweeperExtractionProgressWidget>(ProgressWidgetComponent->GetUserWidgetObject()))
	{
		ProgressWidget->SetExtractionProgress(
			CurrentHoldSeconds,
			ExtractionHoldSeconds,
			bShouldShowProgress);
	}
}

void ATunaSweeperExtractionPointActor::ResetHoldProgress()
{
	CurrentHoldSeconds = 0.0f;
}

bool ATunaSweeperExtractionPointActor::IsPawnInsideExtractionArea(const APawn* Pawn) const
{
	return Pawn &&
		FVector::DistSquared2D(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(FMath::Max(1.0f, ExtractionRadius));
}

bool ATunaSweeperExtractionPointActor::CanExtractPawn(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn);
	return !TunaCharacter || !TunaCharacter->IsDead();
}

void ATunaSweeperExtractionPointActor::StopPawnForExtraction(APawn* Pawn) const
{
	if (!Pawn)
	{
		return;
	}

	if (ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
	{
		TunaCharacter->CancelActiveGameplayActions();
	}

	if (UPawnMovementComponent* MovementComponent = Pawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (AController* Controller = Pawn->GetController())
	{
		Controller->StopMovement();
	}
}

void ATunaSweeperExtractionPointActor::UpdateFallbackParticleEffect(float DeltaSeconds)
{
	EffectElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);

	const int32 ParticleCount = FallbackParticleMeshes.Num();
	const bool bShowFallbackParticles = bEnableFallbackParticleEffect && ParticleCount > 0;
	const float CoreRadius = FMath::Min(FallbackParticleOrbitRadius, FMath::Max(0.0f, ExtractionRadius * 0.14f));
	const FVector WindVelocity = GetSmokeSignalWindVelocity();
	for (int32 Index = 0; Index < ParticleCount; ++Index)
	{
		UStaticMeshComponent* ParticleComponent = FallbackParticleMeshes[Index];
		if (!ParticleComponent)
		{
			continue;
		}

		ParticleComponent->SetVisibility(bShowFallbackParticles);
		ParticleComponent->SetHiddenInGame(!bShowFallbackParticles);
		if (!bShowFallbackParticles)
		{
			continue;
		}

		const float IndexAlpha = static_cast<float>(Index) / static_cast<float>(FMath::Max(1, ParticleCount));
		const float LifeAlpha = FMath::Frac(EffectElapsedSeconds * 0.72f + IndexAlpha);
		const float LiftAlpha = SmoothExtractionSmokeAlpha(LifeAlpha);
		const float Angle = EffectElapsedSeconds * 2.2f + IndexAlpha * 5.0f * PI;
		const float SparkRadius = FMath::Lerp(5.0f, CoreRadius, SmoothExtractionSmokeAlpha(FMath::Sin(LifeAlpha * PI)));
		const float VerticalOffset = LiftAlpha * (FallbackParticleVerticalAmplitude + 58.0f);
		const float Pulse = 0.76f + 0.24f * FMath::Sin(EffectElapsedSeconds * 7.1f + IndexAlpha * 6.0f * PI);
		const FVector RelativeLocation(
			FMath::Cos(Angle) * SparkRadius + WindVelocity.X * LifeAlpha * 0.22f,
			FMath::Sin(Angle) * SparkRadius + WindVelocity.Y * LifeAlpha * 0.22f,
			FallbackParticleBaseHeight + VerticalOffset);
		const float RelativeScale = FMath::Lerp(0.075f, 0.022f, LifeAlpha) * Pulse;

		ParticleComponent->SetRelativeLocation(RelativeLocation);
		ParticleComponent->SetRelativeScale3D(FVector(RelativeScale));
	}
}

void ATunaSweeperExtractionPointActor::ApplyFallbackParticleMaterials()
{
	FallbackParticleDynamicMaterials.Reset();
	UMaterialInterface* ParticleMaterial = LoadObject<UMaterialInterface>(nullptr, ExtractionVisualMaterialPath);
	if (!ParticleMaterial)
	{
		return;
	}

	for (UStaticMeshComponent* ParticleComponent : FallbackParticleMeshes)
	{
		if (!ParticleComponent)
		{
			continue;
		}

		ParticleComponent->SetMaterial(0, ParticleMaterial);
		UMaterialInstanceDynamic* DynamicMaterial = ParticleComponent->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			ApplyExtractionColorParameters(DynamicMaterial, ParticleColor);
			FallbackParticleDynamicMaterials.Add(DynamicMaterial);
		}
	}
}

void ATunaSweeperExtractionPointActor::UpdateSmokeSignalEffect(float DeltaSeconds)
{
	(void)DeltaSeconds;

	const int32 SpriteCount = SmokeSignalSprites.Num();
	const bool bShowSmokeSignal =
		bEnableSmokeSignalEffect &&
		!bHasActiveNiagaraExtractionEffect &&
		SpriteCount > 0 &&
		SmokeSignalDynamicMaterials.Num() > 0;
	const FVector WindVelocity = GetSmokeSignalWindVelocity();
	const float LoopDuration = FMath::Max(0.25f, SmokeSignalLoopDurationSeconds);
	const float ColumnHeight = FMath::Max(1.0f, SmokeSignalColumnHeight);
	const float BaseDiameter = FMath::Max(1.0f, SmokeSignalBaseDiameter);
	const float TopDiameter = FMath::Max(BaseDiameter, SmokeSignalTopDiameter);
	const float HorizontalSpread = FMath::Max(0.0f, SmokeSignalHorizontalSpread);

	for (int32 Index = 0; Index < SpriteCount; ++Index)
	{
		UStaticMeshComponent* SmokeSprite = SmokeSignalSprites[Index];
		if (!SmokeSprite)
		{
			continue;
		}

		SmokeSprite->SetVisibility(bShowSmokeSignal);
		SmokeSprite->SetHiddenInGame(!bShowSmokeSignal);
		if (!bShowSmokeSignal)
		{
			continue;
		}

		const float IndexAlpha = static_cast<float>(Index) / static_cast<float>(FMath::Max(1, SpriteCount));
		const float LifeAlpha = FMath::Frac(EffectElapsedSeconds / LoopDuration + IndexAlpha);
		const float RiseAlpha = SmoothExtractionSmokeAlpha(LifeAlpha);
		const float SpawnFade = SmoothExtractionSmokeAlpha(RangeExtractionSmokeAlpha(0.0f, 0.12f, LifeAlpha));
		const float DeathFade = 1.0f - SmoothExtractionSmokeAlpha(RangeExtractionSmokeAlpha(0.78f, 1.0f, LifeAlpha));
		const float SmokeOpacity = SpawnFade * DeathFade * FMath::Lerp(0.82f, 0.54f, RiseAlpha);

		const float SpiralAngle = IndexAlpha * 7.0f * PI + EffectElapsedSeconds * 0.24f;
		const float TurbulenceAngle = IndexAlpha * 13.0f * PI + EffectElapsedSeconds * 0.47f;
		const float SpreadRadius = HorizontalSpread * SmoothExtractionSmokeAlpha(RiseAlpha);
		const FVector TurbulenceOffset(
			FMath::Cos(SpiralAngle) * SpreadRadius * 0.62f + FMath::Cos(TurbulenceAngle) * SpreadRadius * 0.28f,
			FMath::Sin(SpiralAngle) * SpreadRadius * 0.62f + FMath::Sin(TurbulenceAngle) * SpreadRadius * 0.28f,
			0.0f);
		const FVector WindOffset = WindVelocity * (LifeAlpha * LoopDuration);
		const FVector RelativeLocation = FVector(0.0f, 0.0f, SmokeSignalBaseHeight + ColumnHeight * RiseAlpha)
			+ TurbulenceOffset
			+ WindOffset;

		const float DiameterPulse = 0.92f + 0.08f * FMath::Sin(EffectElapsedSeconds * 1.9f + IndexAlpha * 8.0f * PI);
		const float Diameter = FMath::Lerp(BaseDiameter, TopDiameter, RiseAlpha) * DiameterPulse;
		const float SmokeSpriteScale = Diameter / ExtractionSmokePlaneMeshSizeCm;
		SmokeSprite->SetRelativeLocation(RelativeLocation);
		SmokeSprite->SetRelativeScale3D(FVector(SmokeSpriteScale, SmokeSpriteScale, 1.0f));
		SmokeSprite->SetRelativeRotation(FRotator(0.0f, 0.0f, LifeAlpha * 180.0f + IndexAlpha * 360.0f));

		FLinearColor TintColor = SmokeSignalTopColor;
		if (LifeAlpha < 0.32f)
		{
			const FLinearColor GreenSmokeColor(0.10f, 0.58f, 0.16f, 1.0f);
			TintColor = FMath::Lerp(SmokeSignalBaseColor, GreenSmokeColor, SmoothExtractionSmokeAlpha(LifeAlpha / 0.32f));
		}
		else
		{
			const FLinearColor GreenSmokeColor(0.10f, 0.58f, 0.16f, 1.0f);
			TintColor = FMath::Lerp(GreenSmokeColor, SmokeSignalTopColor, SmoothExtractionSmokeAlpha((LifeAlpha - 0.32f) / 0.68f));
		}
		TintColor.A = 1.0f;

		const float EmissiveStrength = FMath::Lerp(
			6.2f,
			0.48f,
			SmoothExtractionSmokeAlpha(RangeExtractionSmokeAlpha(0.06f, 0.58f, LifeAlpha)));
		const int32 FrameIndex = FMath::Clamp(
			8 + FMath::FloorToInt(RiseAlpha * 7.0f) + (Index % 2),
			8,
			15);

		UMaterialInstanceDynamic* DynamicMaterial = SmokeSignalDynamicMaterials.IsValidIndex(Index)
			? SmokeSignalDynamicMaterials[Index]
			: nullptr;
		UpdateSmokeSignalSpriteMaterial(
			DynamicMaterial,
			FrameIndex,
			TintColor,
			EmissiveStrength,
			SmokeOpacity);
	}
}

void ATunaSweeperExtractionPointActor::ApplySmokeSignalMaterials()
{
	SmokeSignalDynamicMaterials.Reset();
	UMaterialInterface* SmokeMaterial = SmokeSignalSpriteMaterial.IsNull()
		? nullptr
		: SmokeSignalSpriteMaterial.LoadSynchronous();
	if (!SmokeMaterial)
	{
		SmokeMaterial = LoadObject<UMaterialInterface>(nullptr, SmokeSignalMaterialPath);
	}
	if (!SmokeMaterial)
	{
		return;
	}

	for (UStaticMeshComponent* SmokeSprite : SmokeSignalSprites)
	{
		if (!SmokeSprite)
		{
			continue;
		}

		SmokeSprite->SetMaterial(0, SmokeMaterial);
		UMaterialInstanceDynamic* DynamicMaterial = SmokeSprite->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			UpdateSmokeSignalSpriteMaterial(DynamicMaterial, 8, SmokeSignalBaseColor, 3.0f, 0.0f);
			SmokeSignalDynamicMaterials.Add(DynamicMaterial);
		}
	}
}

void ATunaSweeperExtractionPointActor::UpdateSmokeSignalSpriteMaterial(
	UMaterialInstanceDynamic* DynamicMaterial,
	int32 FrameIndex,
	const FLinearColor& TintColor,
	float EmissiveStrength,
	float Opacity) const
{
	if (!DynamicMaterial)
	{
		return;
	}

	const int32 SafeFrameIndex = FMath::Clamp(FrameIndex, 0, 15);
	const int32 Column = SafeFrameIndex % 4;
	const int32 Row = SafeFrameIndex / 4;
	const float FrameScale = 0.25f;
	const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	const float SafeEmissiveStrength = FMath::Max(0.0f, EmissiveStrength);

	DynamicMaterial->SetScalarParameterValue(TEXT("FrameScale"), FrameScale);
	DynamicMaterial->SetScalarParameterValue(TEXT("FrameU"), static_cast<float>(Column) * FrameScale);
	DynamicMaterial->SetScalarParameterValue(TEXT("FrameV"), static_cast<float>(Row) * FrameScale);
	DynamicMaterial->SetVectorParameterValue(TEXT("TintColor"), TintColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), SafeEmissiveStrength);
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), ClampedOpacity);

	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), TintColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TintColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), TintColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("LedColor"), TintColor);
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), TintColor * SafeEmissiveStrength);
	DynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), TintColor * SafeEmissiveStrength);
	DynamicMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), SafeEmissiveStrength);
	DynamicMaterial->SetScalarParameterValue(TEXT("Intensity"), SafeEmissiveStrength);
}

FVector ATunaSweeperExtractionPointActor::GetSmokeSignalWindVelocity() const
{
	FVector2D SafeDirection = SmokeSignalWindDirection.GetSafeNormal(0.0f);
	if (SafeDirection.IsNearlyZero())
	{
		SafeDirection = FVector2D(1.0f, 0.0f);
	}

	const float WindSpeed = FMath::Max(0.0f, SmokeSignalWindSpeedCmPerSecond);
	return FVector(SafeDirection.X * WindSpeed, SafeDirection.Y * WindSpeed, 0.0f);
}
