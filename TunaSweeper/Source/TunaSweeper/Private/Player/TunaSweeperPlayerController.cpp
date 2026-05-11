#include "Player/TunaSweeperPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/World.h"
#include "UI/TunaSweeperGameHudWidget.h"

ATunaSweeperPlayerController::ATunaSweeperPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	GameHudWidgetClass = TSoftClassPtr<UTunaSweeperGameHudWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C")));
}

void ATunaSweeperPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	EnsureGameHudWidget();
}

void ATunaSweeperPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!ControlledCharacter)
	{
		return;
	}

	FVector AimPoint;
	if (GetMouseAimPointOnPlane(ControlledCharacter->GetActorLocation().Z, AimPoint))
	{
		ControlledCharacter->SetAimWorldPoint(AimPoint);
	}
}

void ATunaSweeperPlayerController::EnsureGameHudWidget()
{
	if (GameHudWidget || !IsLocalController())
	{
		return;
	}

	TSubclassOf<UTunaSweeperGameHudWidget> LoadedHudWidgetClass = GameHudWidgetClass.LoadSynchronous();
	if (!LoadedHudWidgetClass)
	{
		return;
	}

	GameHudWidget = CreateWidget<UTunaSweeperGameHudWidget>(this, LoadedHudWidgetClass);
	if (GameHudWidget)
	{
		GameHudWidget->AddToViewport(0);
	}
}

void ATunaSweeperPlayerController::ToggleInventoryOnlyPanel()
{
	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ToggleInventoryOnlyPanel();
	}
}

void ATunaSweeperPlayerController::OpenLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowLootContainerPanel(ContainerInstance);
	}
}

bool ATunaSweeperPlayerController::GetMouseAimPointOnPlane(float PlaneZ, FVector& OutAimPoint) const
{
	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperMouseAim), true);
		if (const APawn* ControlledPawn = GetPawn())
		{
			QueryParams.AddIgnoredActor(ControlledPawn);
		}

		FHitResult Hit;
		const FVector TraceEnd = WorldLocation + WorldDirection * 100000.0f;
		if (World->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
		{
			OutAimPoint = Hit.ImpactPoint;
			return true;
		}
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float DistanceToPlane = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	if (DistanceToPlane < 0.0f)
	{
		return false;
	}

	OutAimPoint = WorldLocation + WorldDirection * DistanceToPlane;
	return true;
}
