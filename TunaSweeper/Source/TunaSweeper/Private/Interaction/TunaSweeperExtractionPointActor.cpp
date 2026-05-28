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
#include "UI/TunaSweeperExtractionProgressWidget.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* ExtractionVisualMaterialPath = TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive");
	const TCHAR* LevelTransitionWidgetClassPath = TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C");
	constexpr int32 FallbackParticleCount = 10;

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
	ExtractionEffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FallbackParticleBaseHeight));

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
}

void ATunaSweeperExtractionPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshExtractionComponents();
	UpdateProgressWidget();
	UpdateFallbackParticleEffect(0.0f);
}

void ATunaSweeperExtractionPointActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshExtractionComponents();
	ApplyFallbackParticleMaterials();
	UpdateProgressWidget();
}

void ATunaSweeperExtractionPointActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFallbackParticleEffect(DeltaSeconds);
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
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance))
		{
			TunaGameInstance->HandleLevelTravelPersistence(SourceLevelName, TargetLevelName);
		}

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyLevelTravelRequested(SourceLevelName, TargetLevelName);
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
	if (!ExtractionEffectComponent)
	{
		return;
	}

	ExtractionEffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FallbackParticleBaseHeight));
	if (UNiagaraSystem* LoadedSystem = ExtractionParticleSystem.LoadSynchronous())
	{
		ExtractionEffectComponent->SetAsset(LoadedSystem);
		ExtractionEffectComponent->SetVariableLinearColor(FName(TEXT("User.Color")), ParticleColor);
		ExtractionEffectComponent->Activate(true);
	}
	else
	{
		ExtractionEffectComponent->Deactivate();
	}
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
	const float OrbitRadius = FMath::Min(FallbackParticleOrbitRadius, FMath::Max(0.0f, ExtractionRadius * 0.42f));
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
		const float Angle = EffectElapsedSeconds * 1.35f + IndexAlpha * 2.0f * PI;
		const float VerticalOffset = FMath::Sin(EffectElapsedSeconds * 2.4f + IndexAlpha * 2.0f * PI) * FallbackParticleVerticalAmplitude;
		const float Pulse = 0.78f + 0.22f * FMath::Sin(EffectElapsedSeconds * 3.1f + IndexAlpha * 4.0f * PI);
		const FVector RelativeLocation(
			FMath::Cos(Angle) * OrbitRadius,
			FMath::Sin(Angle) * OrbitRadius,
			FallbackParticleBaseHeight + VerticalOffset);
		const float RelativeScale = 0.055f * Pulse;

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
