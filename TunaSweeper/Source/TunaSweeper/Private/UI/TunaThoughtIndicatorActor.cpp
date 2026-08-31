#include "UI/TunaThoughtIndicatorActor.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Texture2D.h"
#include "UI/TunaThoughtIndicatorWidget.h"
#include "UObject/ConstructorHelpers.h"

ATunaThoughtIndicatorActor::ATunaThoughtIndicatorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	IndicatorWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TunaThoughtIndicator"));
	IndicatorWidgetComponent->SetupAttachment(SceneRoot);
	IndicatorWidgetComponent->SetWidgetClass(UTunaThoughtIndicatorWidget::StaticClass());
	IndicatorWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	IndicatorWidgetComponent->SetDrawAtDesiredSize(false);
	IndicatorWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
	IndicatorWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IndicatorWidgetComponent->SetGenerateOverlapEvents(false);
	IndicatorWidgetComponent->SetHiddenInGame(false);
	IndicatorWidgetComponent->SetTranslucentSortPriority(100);

	static ConstructorHelpers::FObjectFinder<UTexture2D> IndicatorTextureFinder(
		TEXT("/Game/UI/Indicators/T_TunaThoughtIndicator.T_TunaThoughtIndicator"));
	if (IndicatorTextureFinder.Succeeded())
	{
		IndicatorTexture = IndicatorTextureFinder.Object;
	}

	ApplyComponentSettings();
}

void ATunaThoughtIndicatorActor::BeginPlay()
{
	Super::BeginPlay();
	AnimationTimeSeconds = 0.0f;
	RefreshIndicator();
}

void ATunaThoughtIndicatorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimationTimeSeconds += FMath::Max(0.0f, DeltaSeconds);
	UpdateWidgetPresentation();
}

void ATunaThoughtIndicatorActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshIndicator();
}

#if WITH_EDITOR
bool ATunaThoughtIndicatorActor::ShouldTickIfViewportsOnly() const
{
#if WITH_EDITORONLY_DATA
	return bPreviewAnimationInEditor;
#else
	return false;
#endif
}
#endif

float ATunaThoughtIndicatorActor::CalculateBobOffsetPixels(float TimeSeconds) const
{
	const float PhaseRadians = FMath::DegreesToRadians(InitialPhaseDegrees);
	const float AngularSpeed = 2.0f * UE_PI * FMath::Max(0.0f, BobCyclesPerSecond);
	return FMath::Max(0.0f, BobAmplitudePixels) * FMath::Sin(AngularSpeed * TimeSeconds + PhaseRadians);
}

void ATunaThoughtIndicatorActor::RefreshIndicator()
{
	ApplyComponentSettings();
	UpdateWidgetPresentation();
}

FVector2D ATunaThoughtIndicatorActor::CalculateCanvasSizePixels() const
{
	const FVector2D ClampedImageSize(
		FMath::Max(16.0f, IndicatorImageSizePixels.X),
		FMath::Max(16.0f, IndicatorImageSizePixels.Y));
	const float VerticalMargin = FMath::Max(8.0f, FMath::Max(0.0f, BobAmplitudePixels) + 6.0f);
	return FVector2D(ClampedImageSize.X + 16.0f, ClampedImageSize.Y + VerticalMargin * 2.0f);
}

void ATunaThoughtIndicatorActor::ApplyComponentSettings()
{
	if (!IndicatorWidgetComponent)
	{
		return;
	}

	const FVector2D CanvasSize = CalculateCanvasSizePixels();
	IndicatorWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Max(0.0f, WorldHeightOffset)));
	IndicatorWidgetComponent->SetDrawSize(FVector2D(
		FMath::CeilToFloat(CanvasSize.X),
		FMath::CeilToFloat(CanvasSize.Y)));
	IndicatorWidgetComponent->SetVisibility(bIndicatorVisible, true);
}

void ATunaThoughtIndicatorActor::UpdateWidgetPresentation()
{
	if (!IndicatorWidgetComponent)
	{
		return;
	}

	IndicatorWidgetComponent->InitWidget();
	UTunaThoughtIndicatorWidget* IndicatorWidget =
		Cast<UTunaThoughtIndicatorWidget>(IndicatorWidgetComponent->GetUserWidgetObject());
	if (!IndicatorWidget)
	{
		return;
	}

	const FVector2D CanvasSize = CalculateCanvasSizePixels();
	const FVector2D ClampedImageSize(
		FMath::Max(16.0f, IndicatorImageSizePixels.X),
		FMath::Max(16.0f, IndicatorImageSizePixels.Y));
	IndicatorWidget->Configure(IndicatorTexture, ClampedImageSize, CanvasSize);
	IndicatorWidget->SetBobOffsetPixels(CalculateBobOffsetPixels(AnimationTimeSeconds));
}
