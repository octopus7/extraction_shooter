#include "Player/TunaSweeperPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperQuestWidget.h"

namespace TunaSweeperDropPlacement
{
	constexpr float DropRootHeight = 8.0f;
	constexpr float GroundTraceUp = 500.0f;
	constexpr float GroundTraceDown = 900.0f;
	constexpr float MinGroundNormalZ = 0.72f;
	constexpr float ClearanceRadius = 38.0f;
	constexpr float ClearanceHalfHeight = 48.0f;
	constexpr float ClearanceBottomLift = 4.0f;
	constexpr float ExistingPickupSpacing = 74.0f;
	constexpr float CandidateDistances[] = { 118.0f, 156.0f, 204.0f, 264.0f };
	constexpr float CandidateAngles[] = { 0.0f, 32.0f, -32.0f, 64.0f, -64.0f, 104.0f, -104.0f, 180.0f };

	FVector GetPlanarForwardVector(const APawn* Pawn)
	{
		FVector Forward = Pawn ? Pawn->GetActorForwardVector() : FVector::ForwardVector;
		Forward.Z = 0.0f;
		if (!Forward.Normalize())
		{
			return FVector::ForwardVector;
		}
		return Forward;
	}

	bool IsGroundHitUsable(const FHitResult& Hit)
	{
		if (!Hit.bBlockingHit || Hit.ImpactNormal.Z < MinGroundNormalZ)
		{
			return false;
		}

		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		return HitComponent && HitComponent->GetCollisionObjectType() == ECC_WorldStatic;
	}

	bool HasExistingPickupTooClose(UWorld* World, const FVector& Location)
	{
		if (!World)
		{
			return true;
		}

		for (TActorIterator<ATunaSweeperPickupItemActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			const ATunaSweeperPickupItemActor* PickupItemActor = *ActorIt;
			if (IsValid(PickupItemActor) &&
				FVector::DistSquared2D(PickupItemActor->GetActorLocation(), Location) < FMath::Square(ExistingPickupSpacing))
			{
				return true;
			}
		}

		return false;
	}

	bool HasBlockingOverlap(UWorld* World, const FVector& FloorLocation)
	{
		if (!World)
		{
			return true;
		}

		FCollisionObjectQueryParams ObjectQueryParams;
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperDropPlacementOverlap), false);
		const FVector ClearanceCenter =
			FloorLocation + FVector(0.0f, 0.0f, ClearanceHalfHeight + ClearanceBottomLift);
		TArray<FOverlapResult> Overlaps;
		const bool bHasOverlap = World->OverlapMultiByObjectType(
			Overlaps,
			ClearanceCenter,
			FQuat::Identity,
			ObjectQueryParams,
			FCollisionShape::MakeCapsule(ClearanceRadius, ClearanceHalfHeight),
			QueryParams);
		return bHasOverlap;
	}
}

ATunaSweeperPlayerController::ATunaSweeperPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	GameHudWidgetClass = TSoftClassPtr<UTunaSweeperGameHudWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C")));
	IntroMenuWidgetClass = TSoftClassPtr<UTunaSweeperIntroMenuWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C")));
	QuestWidgetClass = TSoftClassPtr<UTunaSweeperQuestWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_Quest.WBP_Quest_C")));
	DropAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Drop.IA_Drop")));
	PickupItemActorClass = TSoftClassPtr<ATunaSweeperPickupItemActor>(
		FSoftObjectPath(TEXT("/Game/Interaction/BP_PickupItem.BP_PickupItem_C")));

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

	if (UInputAction* LoadedDropAction = DropAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedDropAction, ETriggerEvent::Started, this, &ATunaSweeperPlayerController::HandleDrop);
	}
}

void ATunaSweeperPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!ControlledCharacter || ControlledCharacter->IsDead())
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

bool ATunaSweeperPlayerController::FindDropLocationNearPlayer(FVector& OutDropLocation) const
{
	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return false;
	}

	const FVector PlayerLocation = ControlledPawn->GetActorLocation();
	const FVector Forward = TunaSweeperDropPlacement::GetPlanarForwardVector(ControlledPawn);
	FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(TunaSweeperDropPlacementGroundTrace), false);
	GroundQueryParams.AddIgnoredActor(ControlledPawn);

	for (const float Distance : TunaSweeperDropPlacement::CandidateDistances)
	{
		for (const float AngleDegrees : TunaSweeperDropPlacement::CandidateAngles)
		{
			const FVector CandidateDirection = Forward.RotateAngleAxis(AngleDegrees, FVector::UpVector);
			const FVector CandidateLocation = PlayerLocation + CandidateDirection * Distance;
			const FVector TraceStart = CandidateLocation + FVector(0.0f, 0.0f, TunaSweeperDropPlacement::GroundTraceUp);
			const FVector TraceEnd = CandidateLocation - FVector(0.0f, 0.0f, TunaSweeperDropPlacement::GroundTraceDown);

			FHitResult GroundHit;
			if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundQueryParams) ||
				!TunaSweeperDropPlacement::IsGroundHitUsable(GroundHit))
			{
				continue;
			}

			const FVector FloorLocation = GroundHit.ImpactPoint;
			if (TunaSweeperDropPlacement::HasBlockingOverlap(World, FloorLocation) ||
				TunaSweeperDropPlacement::HasExistingPickupTooClose(World, FloorLocation))
			{
				continue;
			}

			OutDropLocation = FloorLocation + FVector(0.0f, 0.0f, TunaSweeperDropPlacement::DropRootHeight);
			return true;
		}
	}

	return false;
}

ATunaSweeperPickupItemActor* ATunaSweeperPlayerController::SpawnDroppedPickupItem(int32 ItemId, int32 Quantity)
{
	UWorld* World = GetWorld();
	if (!World || ItemId == INDEX_NONE || Quantity <= 0)
	{
		return nullptr;
	}

	FVector DropLocation;
	if (!FindDropLocationNearPlayer(DropLocation))
	{
		return nullptr;
	}

	TSubclassOf<ATunaSweeperPickupItemActor> LoadedPickupClass = ATunaSweeperPickupItemActor::StaticClass();
	if (UClass* SoftPickupClass = PickupItemActorClass.LoadSynchronous())
	{
		if (SoftPickupClass->IsChildOf(ATunaSweeperPickupItemActor::StaticClass()))
		{
			LoadedPickupClass = SoftPickupClass;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Instigator = GetPawn();

	ATunaSweeperPickupItemActor* SpawnedPickupItem = World->SpawnActor<ATunaSweeperPickupItemActor>(
		LoadedPickupClass,
		DropLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (SpawnedPickupItem)
	{
		SpawnedPickupItem->SetItemStack(ItemId, Quantity);
	}

	return SpawnedPickupItem;
}

void ATunaSweeperPlayerController::HandleQuickSlot(int32 SlotNumber)
{
	if (const ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		if (ControlledCharacter->IsDead())
		{
			return;
		}
	}

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

void ATunaSweeperPlayerController::HandleDrop(const FInputActionValue&)
{
	if (IsIntroMap())
	{
		return;
	}

	if (const ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		if (ControlledCharacter->IsDead())
		{
			return;
		}
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->HasHoveredItemSlot())
	{
		return;
	}

	const FTunaSweeperItemSlotReference HoveredSlot = TunaGameInstance->GetHoveredItemSlotReference();
	FTunaSweeperItemInstance ItemInstance;
	if (!TunaGameInstance->TryGetSlotItemInstance(HoveredSlot, ItemInstance))
	{
		TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		return;
	}

	ATunaSweeperPickupItemActor* SpawnedPickupItem = SpawnDroppedPickupItem(ItemInstance.ItemId, ItemInstance.Quantity);
	if (!SpawnedPickupItem)
	{
		return;
	}

	FTunaSweeperItemInstance RemovedItemInstance;
	if (!TunaGameInstance->RemoveItemFromSlot(HoveredSlot, RemovedItemInstance))
	{
		SpawnedPickupItem->Destroy();
		return;
	}

	SpawnedPickupItem->SetItemStack(RemovedItemInstance.ItemId, RemovedItemInstance.Quantity);
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
