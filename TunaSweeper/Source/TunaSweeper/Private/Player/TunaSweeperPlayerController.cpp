#include "Player/TunaSweeperPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperQuestWidget.h"

ATunaSweeperPlayerController::ATunaSweeperPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	GameHudWidgetClass = TSoftClassPtr<UTunaSweeperGameHudWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C")));
	IntroMenuWidgetClass = TSoftClassPtr<UTunaSweeperIntroMenuWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C")));
	QuestWidgetClass = TSoftClassPtr<UTunaSweeperQuestWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_Quest.WBP_Quest_C")));

	QuickSlotActions.Reserve(8);
	for (int32 SlotNumber = 1; SlotNumber <= 8; ++SlotNumber)
	{
		QuickSlotActions.Add(TSoftObjectPtr<UInputAction>(FSoftObjectPath(FString::Printf(
			TEXT("/Game/Input/IA_QuickSlot%d.IA_QuickSlot%d"),
			SlotNumber,
			SlotNumber))));
	}
}

void ATunaSweeperPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	ApplyDefaultGameInputMode();

	if (IsIntroMap())
	{
		EnsureIntroMenuWidget();
	}
	else
	{
		EnsureGameHudWidget();
	}
}

void ATunaSweeperPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	auto BindQuickSlotInputAction = [this, EnhancedInputComponent](int32 SlotIndex, auto Handler)
	{
		if (!QuickSlotActions.IsValidIndex(SlotIndex))
		{
			return;
		}

		if (UInputAction* LoadedQuickSlotAction = QuickSlotActions[SlotIndex].LoadSynchronous())
		{
			EnhancedInputComponent->BindAction(LoadedQuickSlotAction, ETriggerEvent::Started, this, Handler);
		}
	};

	BindQuickSlotInputAction(0, &ATunaSweeperPlayerController::HandleQuickSlot1);
	BindQuickSlotInputAction(1, &ATunaSweeperPlayerController::HandleQuickSlot2);
	BindQuickSlotInputAction(2, &ATunaSweeperPlayerController::HandleQuickSlot3);
	BindQuickSlotInputAction(3, &ATunaSweeperPlayerController::HandleQuickSlot4);
	BindQuickSlotInputAction(4, &ATunaSweeperPlayerController::HandleQuickSlot5);
	BindQuickSlotInputAction(5, &ATunaSweeperPlayerController::HandleQuickSlot6);
	BindQuickSlotInputAction(6, &ATunaSweeperPlayerController::HandleQuickSlot7);
	BindQuickSlotInputAction(7, &ATunaSweeperPlayerController::HandleQuickSlot8);
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

void ATunaSweeperPlayerController::EnsureIntroMenuWidget()
{
	if (IntroMenuWidget || !IsLocalController())
	{
		return;
	}

	TSubclassOf<UTunaSweeperIntroMenuWidget> LoadedIntroMenuWidgetClass = IntroMenuWidgetClass.LoadSynchronous();
	if (!LoadedIntroMenuWidgetClass)
	{
		return;
	}

	IntroMenuWidget = CreateWidget<UTunaSweeperIntroMenuWidget>(this, LoadedIntroMenuWidgetClass);
	if (IntroMenuWidget)
	{
		IntroMenuWidget->AddToViewport(50);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(IntroMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

bool ATunaSweeperPlayerController::IsIntroMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("IntroMap"));
}

void ATunaSweeperPlayerController::HandleQuickSlot(int32 SlotNumber)
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	if (UTunaSweeperKeyboardInputSubsystem* KeyboardInputSubsystem = GameInstance->GetSubsystem<UTunaSweeperKeyboardInputSubsystem>())
	{
		KeyboardInputSubsystem->ReceiveQuickSlotKeyInput(SlotNumber, GetPawn());
	}
}

void ATunaSweeperPlayerController::HandleQuickSlot1(const FInputActionValue&)
{
	HandleQuickSlot(1);
}

void ATunaSweeperPlayerController::HandleQuickSlot2(const FInputActionValue&)
{
	HandleQuickSlot(2);
}

void ATunaSweeperPlayerController::HandleQuickSlot3(const FInputActionValue&)
{
	HandleQuickSlot(3);
}

void ATunaSweeperPlayerController::HandleQuickSlot4(const FInputActionValue&)
{
	HandleQuickSlot(4);
}

void ATunaSweeperPlayerController::HandleQuickSlot5(const FInputActionValue&)
{
	HandleQuickSlot(5);
}

void ATunaSweeperPlayerController::HandleQuickSlot6(const FInputActionValue&)
{
	HandleQuickSlot(6);
}

void ATunaSweeperPlayerController::HandleQuickSlot7(const FInputActionValue&)
{
	HandleQuickSlot(7);
}

void ATunaSweeperPlayerController::HandleQuickSlot8(const FInputActionValue&)
{
	HandleQuickSlot(8);
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

void ATunaSweeperPlayerController::OpenQuestPanel(FName QuestId)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!QuestWidget)
	{
		TSubclassOf<UTunaSweeperQuestWidget> LoadedQuestWidgetClass = QuestWidgetClass.LoadSynchronous();
		if (!LoadedQuestWidgetClass)
		{
			return;
		}

		QuestWidget = CreateWidget<UTunaSweeperQuestWidget>(this, LoadedQuestWidgetClass);
	}

	if (!QuestWidget)
	{
		return;
	}

	QuestWidget->InitializeQuest(QuestId);
	if (!QuestWidget->IsInViewport())
	{
		QuestWidget->AddToViewport(30);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(QuestWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ATunaSweeperPlayerController::ApplyDefaultGameInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
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
