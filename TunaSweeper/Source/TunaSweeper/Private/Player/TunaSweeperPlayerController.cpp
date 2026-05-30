#include "Player/TunaSweeperPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "Housing/TunaSweeperHousingAreaActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"
#include "Subsystem/TunaSweeperBgmSubsystem.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "UI/TunaSweeperScenarioPresentationWidget.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "TimerManager.h"

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

namespace TunaSweeperHoveredItemInteraction
{
	bool TryFindEquipmentTarget(
		UTunaSweeperGameInstance* TunaGameInstance,
		const FTunaSweeperItemSlotReference& SourceSlot,
		bool bRequireEmptyTarget,
		FTunaSweeperItemSlotReference& OutTargetSlot)
	{
		if (!TunaGameInstance || SourceSlot.Source == ETunaSweeperItemSlotSource::Equipment)
		{
			return false;
		}

		const TArray<FTunaSweeperInventorySlot>& EquipmentSlots = TunaGameInstance->GetEquipmentSlots();
		for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
		{
			if (bRequireEmptyTarget != EquipmentSlots[SlotIndex].IsEmpty())
			{
				continue;
			}

			FTunaSweeperItemSlotReference TargetSlot;
			TargetSlot.Source = ETunaSweeperItemSlotSource::Equipment;
			TargetSlot.SlotIndex = SlotIndex;
			if (TunaGameInstance->CanMoveItemBetweenSlots(SourceSlot, TargetSlot))
			{
				OutTargetSlot = TargetSlot;
				return true;
			}
		}

		return false;
	}

	bool TryFindFirstInventoryTarget(
		UTunaSweeperGameInstance* TunaGameInstance,
		const FTunaSweeperItemSlotReference& SourceSlot,
		FTunaSweeperItemSlotReference& OutTargetSlot)
	{
		if (!TunaGameInstance)
		{
			return false;
		}

		const TArray<FTunaSweeperInventorySlot>& InventorySlots = TunaGameInstance->GetInventorySlots();
		for (int32 SlotIndex = 0; SlotIndex < InventorySlots.Num(); ++SlotIndex)
		{
			if (!InventorySlots[SlotIndex].IsEmpty())
			{
				continue;
			}

			FTunaSweeperItemSlotReference TargetSlot;
			TargetSlot.Source = ETunaSweeperItemSlotSource::Inventory;
			TargetSlot.SlotIndex = SlotIndex;
			if (TunaGameInstance->CanMoveItemBetweenSlots(SourceSlot, TargetSlot))
			{
				OutTargetSlot = TargetSlot;
				return true;
			}
		}

		return false;
	}

	void ShowInsufficientSpaceMessage()
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Red,
				TEXT("\uACF5\uAC04\uC774 \uBD80\uC871\uD569\uB2C8\uB2E4"));
		}
	}
}

namespace TunaSweeperIntroMap
{
	constexpr float EntryFadeFromBlackSeconds = 0.5f;
}

namespace TunaSweeperCanBotIntro
{
	const FName DialogueCompletionFlag(TEXT("dialogue.canbot.bunker_intro"));
	const FName SpeakerNameStringKey(TEXT("ui.dialogue.canbot.speaker"));
	constexpr float StartDelayAfterBunkerFadeSeconds = 1.15f;
	constexpr float CameraReturnBlendSeconds = 0.9f;
	constexpr float DialogueCameraDistance = 1200.0f;
	const FRotator DialogueCameraRotation(-60.0f, 0.0f, 0.0f);
	const FVector DeployLadderFocusLocation(220.0f, -220.0f, 4.0f);

	FVector CalculateCameraLocationForFocus(const FVector& FocusLocation)
	{
		return FocusLocation - DialogueCameraRotation.Vector() * DialogueCameraDistance;
	}
}

namespace TunaSweeperHousingCamera
{
	constexpr float BlendSeconds = 0.35f;
	constexpr float Distance = 1900.0f;
	constexpr float PitchDegrees = -68.0f;
	constexpr float FOV = 70.0f;
	constexpr float MoveSpeed = 900.0f;
}

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperHousingInput, Log, All);

ATunaSweeperPlayerController::ATunaSweeperPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
	GameHudWidgetClass = TSoftClassPtr<UTunaSweeperGameHudWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C")));
	IntroMenuWidgetClass = TSoftClassPtr<UTunaSweeperIntroMenuWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C")));
	QuestWidgetClass = TSoftClassPtr<UTunaSweeperQuestWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_Quest.WBP_Quest_C")));
	MeleeQuickSlotAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_MeleeQuickSlot.IA_MeleeQuickSlot")));
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
	BindHousingStateChanged();

	if (IsIntroMap())
	{
		ApplyInitialTitleDisplaySettings();
		if (GetWorld())
		{
			GetWorldTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &ATunaSweeperPlayerController::EnsureIntroMenuWidget));
		}
		else
		{
			EnsureIntroMenuWidget();
		}
	}
	else if (IsOpeningScenarioMap())
	{
		EnsureScenarioPresentationWidget();
	}
	else
	{
		ApplyLevelBgmState();
		EnsureGameHudWidget();
		const bool bShowingBunkerEntryFade = ShowBunkerEntryFadeIfNeeded();
		if (bShowingBunkerEntryFade && GetWorld())
		{
			GetWorldTimerManager().SetTimer(
				CanBotIntroDialogueTimerHandle,
				this,
				&ATunaSweeperPlayerController::MaybeStartCanBotIntroDialogue,
				TunaSweeperCanBotIntro::StartDelayAfterBunkerFadeSeconds,
				false);
		}
		else
		{
			MaybeStartCanBotIntroDialogue();
		}
	}
}

void ATunaSweeperPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>())
		{
			HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
		}
	}

	EndHousingCameraMode(0.0f);
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperPlayerController::BindHousingStateChanged()
{
	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (!HousingSubsystem)
	{
		return;
	}

	HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
	HousingSubsystem->OnHousingStateChanged.AddUObject(this, &ATunaSweeperPlayerController::HandleHousingStateChanged);
}

void ATunaSweeperPlayerController::HandleHousingStateChanged()
{
	if (!IsHousingModeOpen())
	{
		RestoreGameplayState(TunaSweeperHousingCamera::BlendSeconds);
	}
}

void ATunaSweeperPlayerController::ApplyInitialTitleDisplaySettings()
{
	if (!IsLocalController() || !GEngine)
	{
		return;
	}

	UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings();
	if (!GameUserSettings)
	{
		return;
	}

	GameUserSettings->LoadSettings(false);
	GameUserSettings->ApplySettings(false);
}

void ATunaSweeperPlayerController::ApplyLevelBgmState()
{
	if (!IsLocalController())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperBgmSubsystem* BgmSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperBgmSubsystem>()
		: nullptr;
	if (!BgmSubsystem)
	{
		return;
	}

	if (IsBunkerMap())
	{
		BgmSubsystem->PlayBunkerBgm();
	}
	else if (IsRaidMap())
	{
		BgmSubsystem->StopBgm();
	}
}

void ATunaSweeperPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::U, IE_Pressed, this, &ATunaSweeperPlayerController::HandleUseHoveredItem);
		InputComponent->BindKey(EKeys::L, IE_Pressed, this, &ATunaSweeperPlayerController::HandleToggleHoveredInventorySortLock);
		InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ATunaSweeperPlayerController::HandleMeleeQuickSlotPressed);
		InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingRotateLeft);
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingRotateRight);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingCancel);
		InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingMoveForwardPressed);
		InputComponent->BindKey(EKeys::W, IE_Released, this, &ATunaSweeperPlayerController::HandleHousingMoveForwardReleased);
		InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingMoveBackwardPressed);
		InputComponent->BindKey(EKeys::S, IE_Released, this, &ATunaSweeperPlayerController::HandleHousingMoveBackwardReleased);
		InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingMoveRightPressed);
		InputComponent->BindKey(EKeys::D, IE_Released, this, &ATunaSweeperPlayerController::HandleHousingMoveRightReleased);
		InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ATunaSweeperPlayerController::HandleHousingMoveLeftPressed);
		InputComponent->BindKey(EKeys::A, IE_Released, this, &ATunaSweeperPlayerController::HandleHousingMoveLeftReleased);
	}

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

	if (UInputAction* LoadedMeleeQuickSlotAction = MeleeQuickSlotAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedMeleeQuickSlotAction, ETriggerEvent::Started, this, &ATunaSweeperPlayerController::HandleMeleeQuickSlot);
	}

	if (UInputAction* LoadedDropAction = DropAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedDropAction, ETriggerEvent::Started, this, &ATunaSweeperPlayerController::HandleDrop);
	}
}

void ATunaSweeperPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bHousingCameraActive && !IsHousingModeOpen())
	{
		RestoreGameplayState(TunaSweeperHousingCamera::BlendSeconds);
	}

	ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!ControlledCharacter || ControlledCharacter->IsDead())
	{
		return;
	}

	if (IsOpeningScenarioMap())
	{
		return;
	}

	if (IsInventoryUiOpen())
	{
		return;
	}

	if (bDialogueSequenceActive)
	{
		return;
	}

	if (IsHousingModeOpen())
	{
		UpdateHousingCamera(DeltaTime);
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr)
		{
			HousingSubsystem->UpdateHousingMode(this);
		}
		return;
	}

	FVector AimPoint;
	FHitResult AimHit;
	const FVector2D AimScreenOffset = ControlledCharacter->GetWeaponRecoilCrosshairScreenOffset();
	if (GetMouseAimPointOnPlane(ControlledCharacter->GetWeaponAimPlaneZ(), AimScreenOffset, AimPoint, &AimHit))
	{
		if (AimHit.bBlockingHit &&
			AimHit.GetActor() &&
			AimHit.GetComponent() &&
			AimHit.GetComponent()->GetCollisionObjectType() != ECC_WorldStatic)
		{
			ControlledCharacter->SetAimWorldHit(AimPoint, AimHit);
		}
		else
		{
			ControlledCharacter->SetAimWorldPoint(AimPoint);
		}
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
		GameHudWidget->AddToViewport(10);
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
		IntroMenuWidget->PrepareForInitialViewport();
		IntroMenuWidget->AddToViewport(50);
		IntroMenuWidget->ForceLayoutPrepass();
		ScreenFadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(
			this,
			UTunaSweeperScreenFadeWidget::StaticClass());
		if (ScreenFadeWidget)
		{
			ScreenFadeWidget->AddToViewport(1000);
			ScreenFadeWidget->StartFadeFromBlack(TunaSweeperIntroMap::EntryFadeFromBlackSeconds);
		}

		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperBgmSubsystem* BgmSubsystem = GameInstance->GetSubsystem<UTunaSweeperBgmSubsystem>())
			{
				BgmSubsystem->PlayTitleBgm();
			}
		}

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(IntroMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		IntroMenuWidget->SetKeyboardFocus();
	}
}

void ATunaSweeperPlayerController::EnsureScenarioPresentationWidget()
{
	if (ScenarioPresentationWidget || !IsLocalController())
	{
		return;
	}

	ScenarioPresentationWidget = CreateWidget<UTunaSweeperScenarioPresentationWidget>(
		this,
		UTunaSweeperScenarioPresentationWidget::StaticClass());
	if (ScenarioPresentationWidget)
	{
		ScenarioPresentationWidget->AddToViewport(60);

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ScenarioPresentationWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
}

bool ATunaSweeperPlayerController::ShowBunkerEntryFadeIfNeeded()
{
	if (!IsBunkerMap() || !IsLocalController())
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->CompletePendingScenarioBunkerEntryIfNeeded())
	{
		return false;
	}

	ScreenFadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(
		this,
		UTunaSweeperScreenFadeWidget::StaticClass());
	if (ScreenFadeWidget)
	{
		ScreenFadeWidget->AddToViewport(100);
		ScreenFadeWidget->StartFadeFromBlack(1.1f);
	}

	return true;
}

void ATunaSweeperPlayerController::MaybeStartCanBotIntroDialogue()
{
	if (!IsBunkerMap() || !IsLocalController() || bDialogueSequenceActive)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance ||
		TunaGameInstance->IsScenarioProgressFlagSet(TunaSweeperCanBotIntro::DialogueCompletionFlag))
	{
		return;
	}

	TArray<FTunaSweeperDialogueLine> DialogueLines;
	BuildCanBotIntroDialogueLines(DialogueLines);
	StartDialogueSequence(DialogueLines, TunaSweeperCanBotIntro::DialogueCompletionFlag);
}

void ATunaSweeperPlayerController::BuildCanBotIntroDialogueLines(TArray<FTunaSweeperDialogueLine>& OutDialogueLines) const
{
	OutDialogueLines.Reset();

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	auto ResolveDialogueText = [TunaGameInstance](FName StringKey, const FText& FallbackText)
	{
		return TunaGameInstance
			? TunaGameInstance->ResolveLocalizedText(StringKey, FallbackText)
			: FallbackText;
	};

	const FText SpeakerName = ResolveDialogueText(
		TunaSweeperCanBotIntro::SpeakerNameStringKey,
		FText::FromString(TEXT("\uCE94\uBD07")));

	auto AddCanBotLine = [&OutDialogueLines, &SpeakerName](const FText& DialogueText)
	{
		FTunaSweeperDialogueLine Line;
		Line.SpeakerName = SpeakerName;
		Line.DialogueText = DialogueText;
		OutDialogueLines.Add(Line);
	};

	AddCanBotLine(ResolveDialogueText(
		FName(TEXT("ui.dialogue.canbot.intro1")),
		FText::FromString(TEXT("Survivor confirmed. Neural response normal. Canbot responding."))));
	AddCanBotLine(ResolveDialogueText(
		FName(TEXT("ui.dialogue.canbot.intro2")),
		FText::FromString(TEXT("This is B-07 bunker. It is your temporary base after waking."))));

	FTunaSweeperDialogueLine CameraLine;
	CameraLine.SpeakerName = SpeakerName;
	CameraLine.DialogueText = ResolveDialogueText(
		FName(TEXT("ui.dialogue.canbot.intro3")),
		FText::FromString(TEXT("Use that travel ladder to deploy to the outside area. It is safer not to approach before you are ready.")));
	CameraLine.bUseCameraFocus = true;
	CameraLine.CameraFocusLocation = TunaSweeperCanBotIntro::DeployLadderFocusLocation;
	CameraLine.CameraBlendSeconds = 0.8f;
	OutDialogueLines.Add(CameraLine);

	AddCanBotLine(ResolveDialogueText(
		FName(TEXT("ui.dialogue.canbot.intro4")),
		FText::FromString(TEXT("Basic vitals and equipment state confirmed. Returning control authority."))));
	AddCanBotLine(ResolveDialogueText(
		FName(TEXT("ui.dialogue.canbot.intro5")),
		FText::FromString(TEXT("Speak to me again if you need guidance."))));
}

bool ATunaSweeperPlayerController::StartDialogueSequence(
	const TArray<FTunaSweeperDialogueLine>& DialogueLines,
	FName CompletionFlag)
{
	if (!IsLocalController() || bDialogueSequenceActive || DialogueLines.IsEmpty())
	{
		return false;
	}

	if (!DialogueWidget)
	{
		DialogueWidget = CreateWidget<UTunaSweeperDialogueWidget>(
			this,
			UTunaSweeperDialogueWidget::StaticClass());
	}

	if (!DialogueWidget)
	{
		return false;
	}

	CancelPawnGameplayActions();
	bDialogueSequenceActive = true;
	bDialogueCameraHasFocus = false;
	ActiveDialogueCompletionFlag = CompletionFlag;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;

	DialogueWidget->SetLineActivatedDelegate(FTunaSweeperDialogueLineActivatedDelegate::CreateUObject(
		this,
		&ATunaSweeperPlayerController::HandleDialogueLineActivated));
	DialogueWidget->SetFinishedDelegate(FTunaSweeperDialogueFinishedDelegate::CreateUObject(
		this,
		&ATunaSweeperPlayerController::HandleDialogueFinished));

	if (!DialogueWidget->IsInViewport())
	{
		DialogueWidget->AddToViewport(90);
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const float CharactersPerSecond = TunaGameInstance
		? TunaGameInstance->GetDialogueCharactersPerSecond()
		: 5.0f;
	DialogueWidget->StartDialogue(DialogueLines, CharactersPerSecond);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(DialogueWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	DialogueWidget->SetKeyboardFocus();
	return true;
}

bool ATunaSweeperPlayerController::PlayQuestPresentation(
	FName QuestId,
	ETunaSweeperQuestPresentationTrigger Trigger)
{
	if (!IsLocalController() || QuestId.IsNone() || bDialogueSequenceActive)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	TArray<FTunaSweeperQuestPresentationLineView> LineViews;
	if (!QuestSubsystem->GetQuestPresentationLines(QuestId, Trigger, LineViews))
	{
		return false;
	}

	TArray<FTunaSweeperDialogueLine> DialogueLines;
	DialogueLines.Reserve(LineViews.Num());
	for (const FTunaSweeperQuestPresentationLineView& LineView : LineViews)
	{
		FTunaSweeperDialogueLine DialogueLine;
		DialogueLine.SpeakerName = LineView.SpeakerName;
		DialogueLine.DialogueText = LineView.DialogueText;
		DialogueLine.bUseCameraFocus = LineView.bUseCameraFocus;
		DialogueLine.CameraFocusLocation = LineView.CameraFocusLocation;
		DialogueLine.CameraBlendSeconds = LineView.CameraBlendSeconds;
		DialogueLines.Add(DialogueLine);
	}

	if (GameHudWidget)
	{
		GameHudWidget->SetHudMode(ETunaSweeperHudMode::None);
	}

	return StartDialogueSequence(DialogueLines, NAME_None);
}

void ATunaSweeperPlayerController::HandleDialogueLineActivated(const FTunaSweeperDialogueLine& DialogueLine)
{
	if (DialogueLine.bUseCameraFocus)
	{
		MoveDialogueCameraToFocusLocation(DialogueLine.CameraFocusLocation, DialogueLine.CameraBlendSeconds);
	}
}

void ATunaSweeperPlayerController::HandleDialogueFinished()
{
	if (DialogueWidget)
	{
		DialogueWidget->RemoveFromParent();
		DialogueWidget = nullptr;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->MarkScenarioProgressFlag(ActiveDialogueCompletionFlag, true);
	}

	const float ReturnBlendSeconds = bDialogueCameraHasFocus
		? TunaSweeperCanBotIntro::CameraReturnBlendSeconds
		: 0.0f;
	ReturnDialogueCameraToPlayer(ReturnBlendSeconds);

	if (ReturnBlendSeconds > 0.0f && GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			DialogueCameraReturnTimerHandle,
			this,
			&ATunaSweeperPlayerController::FinishDialogueCameraReturn,
			ReturnBlendSeconds,
			false);
	}
	else
	{
		FinishDialogueCameraReturn();
	}
}

void ATunaSweeperPlayerController::MoveDialogueCameraToFocusLocation(FVector FocusLocation, float BlendSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !IsLocalController())
	{
		return;
	}

	if (!DialogueCameraActor)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DialogueCameraActor = World->SpawnActor<ACameraActor>(
			ACameraActor::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
	}

	if (!DialogueCameraActor)
	{
		return;
	}

	DialogueCameraActor->SetActorLocationAndRotation(
		TunaSweeperCanBotIntro::CalculateCameraLocationForFocus(FocusLocation),
		TunaSweeperCanBotIntro::DialogueCameraRotation);
	if (UCameraComponent* CameraComponent = DialogueCameraActor->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(70.0f);
	}

	SetViewTargetWithBlend(DialogueCameraActor, FMath::Max(0.0f, BlendSeconds), VTBlend_Cubic);
	bDialogueCameraHasFocus = true;
}

void ATunaSweeperPlayerController::ReturnDialogueCameraToPlayer(float BlendSeconds)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTargetWithBlend(ControlledPawn, FMath::Max(0.0f, BlendSeconds), VTBlend_Cubic);
	}
}

void ATunaSweeperPlayerController::FinishDialogueCameraReturn()
{
	if (DialogueCameraActor)
	{
		DialogueCameraActor->Destroy();
		DialogueCameraActor = nullptr;
	}

	ActiveDialogueCompletionFlag = NAME_None;
	bDialogueCameraHasFocus = false;
	bDialogueSequenceActive = false;
	ApplyDefaultGameInputMode();
}

void ATunaSweeperPlayerController::CancelPawnGameplayActions() const
{
	if (ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		ControlledCharacter->CancelActiveGameplayActions();
	}
}

bool ATunaSweeperPlayerController::IsIntroMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("IntroMap"));
}

bool ATunaSweeperPlayerController::IsOpeningScenarioMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("OpeningScenarioMap"));
}

bool ATunaSweeperPlayerController::IsBunkerMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
}

bool ATunaSweeperPlayerController::IsRaidMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("RaidMap"));
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

bool ATunaSweeperPlayerController::TryHandleHoveredItemInteract()
{
	if (IsIntroMap() || IsOpeningScenarioMap())
	{
		return false;
	}

	if (bDialogueSequenceActive)
	{
		return true;
	}

	if (const ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		if (ControlledCharacter->IsDead())
		{
			return true;
		}
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	if (!TunaGameInstance->HasHoveredItemSlot())
	{
		if (!GameHudWidget || !GameHudWidget->IsWorkbenchPanelOpen())
		{
			return false;
		}

		const ETunaSweeperWorkbenchMode WorkbenchMode = GameHudWidget->GetWorkbenchPanelMode();
		if (TunaGameInstance->HasSelectedInventoryItem())
		{
			const FTunaSweeperItemSlotReference SelectedSlot = TunaGameInstance->GetSelectedItemSlotReference();
			if (WorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle &&
				GameHudWidget->TryAssignWorkbenchDismantleCandidateToTarget(SelectedSlot))
			{
				return true;
			}
			if (WorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister &&
				GameHudWidget->TryAssignWorkbenchBlueprintItemToTarget(SelectedSlot))
			{
				return true;
			}
		}

		if (WorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle)
		{
			return GameHudWidget->TryAssignFocusedWorkbenchDismantleCandidateToTarget();
		}
		if (WorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister)
		{
			return GameHudWidget->TryAssignFocusedWorkbenchBlueprintItemToTarget();
		}
		return false;
	}

	const FTunaSweeperItemSlotReference HoveredSlot = TunaGameInstance->GetHoveredItemSlotReference();
	if (GameHudWidget && GameHudWidget->IsShopPanelOpen())
	{
		if (HoveredSlot.Source == ETunaSweeperItemSlotSource::Shop)
		{
			TunaGameInstance->TryBuyActiveShopSlot(HoveredSlot.SlotIndex);
			return true;
		}

		GameHudWidget->TrySellSelectedShopItem();
		return true;
	}

	if (GameHudWidget && GameHudWidget->IsWorkbenchPanelOpen())
	{
		const ETunaSweeperWorkbenchMode WorkbenchMode = GameHudWidget->GetWorkbenchPanelMode();
		if (WorkbenchMode == ETunaSweeperWorkbenchMode::Craft &&
			HoveredSlot.Source == ETunaSweeperItemSlotSource::WorkbenchRecipe)
		{
			TunaGameInstance->TryCraftActiveWorkbenchRecipe(HoveredSlot.SlotIndex);
		}
		else if (WorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle)
		{
			GameHudWidget->TryAssignWorkbenchDismantleCandidateToTarget(HoveredSlot) ||
				GameHudWidget->TryAssignFocusedWorkbenchDismantleCandidateToTarget();
		}
		else if (WorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister)
		{
			GameHudWidget->TryAssignWorkbenchBlueprintItemToTarget(HoveredSlot) ||
				GameHudWidget->TryAssignFocusedWorkbenchBlueprintItemToTarget();
		}
		return true;
	}

	FTunaSweeperItemInstance HoveredItemInstance;
	if (!TunaGameInstance->TryGetSlotItemInstance(HoveredSlot, HoveredItemInstance))
	{
		TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		return true;
	}

	FTunaSweeperItemSlotReference TargetSlot;
	if (HoveredSlot.Source == ETunaSweeperItemSlotSource::Inventory)
	{
		if (TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, true, TargetSlot) ||
			TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, false, TargetSlot))
		{
			TunaGameInstance->MoveItemBetweenSlots(HoveredSlot, TargetSlot);
			TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		}
		return true;
	}

	if (HoveredSlot.Source == ETunaSweeperItemSlotSource::LootContainer)
	{
		if (TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, true, TargetSlot) ||
			TunaSweeperHoveredItemInteraction::TryFindFirstInventoryTarget(TunaGameInstance, HoveredSlot, TargetSlot))
		{
			TunaGameInstance->MoveItemBetweenSlots(HoveredSlot, TargetSlot);
			TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		}
		else
		{
			TunaSweeperHoveredItemInteraction::ShowInsufficientSpaceMessage();
		}
		return true;
	}

	if (HoveredSlot.Source == ETunaSweeperItemSlotSource::Storage)
	{
		if (TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, true, TargetSlot) ||
			TunaSweeperHoveredItemInteraction::TryFindFirstInventoryTarget(TunaGameInstance, HoveredSlot, TargetSlot))
		{
			TunaGameInstance->MoveItemBetweenSlots(HoveredSlot, TargetSlot);
			TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		}
		else
		{
			TunaSweeperHoveredItemInteraction::ShowInsufficientSpaceMessage();
		}
		return true;
	}

	return true;
}

void ATunaSweeperPlayerController::HandleQuickSlot(int32 SlotNumber)
{
	if (IsOpeningScenarioMap())
	{
		return;
	}

	if (bDialogueSequenceActive)
	{
		return;
	}

	if (IsInventoryUiOpen())
	{
		return;
	}

	if (ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		if (ControlledCharacter->IsDead())
		{
			return;
		}

		if (SlotNumber == 1 || SlotNumber == 2)
		{
			ControlledCharacter->SelectWeaponSlot(SlotNumber);
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

void ATunaSweeperPlayerController::HandleMeleeQuickSlotPressed()
{
	if (IsOpeningScenarioMap() || bDialogueSequenceActive || IsInventoryUiOpen())
	{
		return;
	}

	ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!ControlledCharacter || ControlledCharacter->IsDead())
	{
		return;
	}

	ControlledCharacter->SelectMeleeWeapon();
}

void ATunaSweeperPlayerController::HandleUseHoveredItem()
{
	if (IsIntroMap() || IsOpeningScenarioMap())
	{
		return;
	}

	if (bDialogueSequenceActive)
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

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->TryUseHoveredItem(GetPawn());
	}
}

void ATunaSweeperPlayerController::HandleToggleHoveredInventorySortLock()
{
	if (IsIntroMap() || IsOpeningScenarioMap())
	{
		return;
	}

	if (bDialogueSequenceActive)
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

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->ToggleHoveredInventorySlotSortLock();
	}
}

void ATunaSweeperPlayerController::HandleDrop(const FInputActionValue&)
{
	if (IsIntroMap() || IsOpeningScenarioMap())
	{
		return;
	}

	if (bDialogueSequenceActive)
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

void ATunaSweeperPlayerController::HandleMeleeQuickSlot(const FInputActionValue&)
{
	HandleMeleeQuickSlotPressed();
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

void ATunaSweeperPlayerController::HandleHousingRotateLeft()
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->RotateActivePlacement(-1);
	}
}

void ATunaSweeperPlayerController::HandleHousingRotateRight()
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->RotateActivePlacement(1);
	}
}

void ATunaSweeperPlayerController::HandleHousingCancel()
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		if (HousingSubsystem->HasActivePlacement())
		{
			HousingSubsystem->CancelPlacement();
		}
		else
		{
			HousingSubsystem->CloseHousingMode();
			RestoreGameplayState(TunaSweeperHousingCamera::BlendSeconds);
		}
	}
}

void ATunaSweeperPlayerController::RestoreGameplayState(float HousingCameraBlendSeconds)
{
	EndHousingCameraMode(HousingCameraBlendSeconds);
	ApplyDefaultGameInputMode();
	bShowMouseCursor = true;
}

void ATunaSweeperPlayerController::BeginHousingCameraMode()
{
	UWorld* World = GetWorld();
	if (!World || !IsLocalController())
	{
		return;
	}

	if (!HousingCameraActor)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HousingCameraActor = World->SpawnActor<ACameraActor>(
			ACameraActor::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
	}

	if (!HousingCameraActor)
	{
		return;
	}

	HousingCameraFocusLocation = ResolveHousingCameraFocusLocation();
	ClampHousingCameraFocusLocation(HousingCameraFocusLocation);
	const FRotator CameraRotation = ResolveHousingCameraRotation();
	HousingCameraActor->SetActorLocationAndRotation(
		CalculateHousingCameraLocation(HousingCameraFocusLocation, CameraRotation),
		CameraRotation);
	if (UCameraComponent* CameraComponent = HousingCameraActor->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(TunaSweeperHousingCamera::FOV);
	}

	SetHousingCharacterVisualHidden(true);
	SetViewTargetWithBlend(HousingCameraActor, TunaSweeperHousingCamera::BlendSeconds, VTBlend_Cubic);
	bHousingCameraActive = true;
}

void ATunaSweeperPlayerController::EndHousingCameraMode(float BlendSeconds)
{
	SetHousingCharacterVisualHidden(false);
	bHousingMoveForwardHeld = false;
	bHousingMoveBackwardHeld = false;
	bHousingMoveRightHeld = false;
	bHousingMoveLeftHeld = false;

	if (!bHousingCameraActive)
	{
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTargetWithBlend(ControlledPawn, FMath::Max(0.0f, BlendSeconds), VTBlend_Cubic);
	}

	bHousingCameraActive = false;
}

void ATunaSweeperPlayerController::UpdateHousingCamera(float DeltaTime)
{
	if (!bHousingCameraActive)
	{
		BeginHousingCameraMode();
	}

	if (!HousingCameraActor)
	{
		return;
	}

	const FVector2D MoveInput = GetHousingCameraMoveInput();
	if (!MoveInput.IsNearlyZero())
	{
		const FRotator AreaYawRotation(0.0f, ResolveHousingCameraRotation().Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(AreaYawRotation).GetScaledAxis(EAxis::X).GetSafeNormal2D();
		const FVector Right = FRotationMatrix(AreaYawRotation).GetScaledAxis(EAxis::Y).GetSafeNormal2D();
		FVector MoveDirection = Forward * MoveInput.Y + Right * MoveInput.X;
		if (!MoveDirection.Normalize())
		{
			MoveDirection = FVector::ZeroVector;
		}
		HousingCameraFocusLocation += MoveDirection * TunaSweeperHousingCamera::MoveSpeed * FMath::Max(0.0f, DeltaTime);
		ClampHousingCameraFocusLocation(HousingCameraFocusLocation);
	}

	const FRotator CameraRotation = ResolveHousingCameraRotation();
	HousingCameraActor->SetActorLocationAndRotation(
		CalculateHousingCameraLocation(HousingCameraFocusLocation, CameraRotation),
		CameraRotation);
}

void ATunaSweeperPlayerController::SetHousingCharacterVisualHidden(bool bShouldHide) const
{
	if (ATunaSweeperTopDownCharacter* ControlledCharacter = Cast<ATunaSweeperTopDownCharacter>(GetPawn()))
	{
		ControlledCharacter->SetHousingModeVisualHidden(bShouldHide);
	}
}

FVector ATunaSweeperPlayerController::ResolveHousingCameraFocusLocation() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (const ATunaSweeperHousingAreaActor* HousingArea = HousingSubsystem
		? HousingSubsystem->GetActiveHousingArea()
		: nullptr)
	{
		return HousingArea->GetWorldLocationForFootprintCenter(
			FIntPoint::ZeroValue,
			FIntPoint(
				FMath::Max(1, HousingArea->GetGridSizeX()),
				FMath::Max(1, HousingArea->GetGridSizeY())));
	}

	if (const APawn* ControlledPawn = GetPawn())
	{
		return ControlledPawn->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FRotator ATunaSweeperPlayerController::ResolveHousingCameraRotation() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (const ATunaSweeperHousingAreaActor* HousingArea = HousingSubsystem
		? HousingSubsystem->GetActiveHousingArea()
		: nullptr)
	{
		return FRotator(TunaSweeperHousingCamera::PitchDegrees, HousingArea->GetAreaYawRotation().Yaw, 0.0f);
	}

	const APawn* ControlledPawn = GetPawn();
	return FRotator(
		TunaSweeperHousingCamera::PitchDegrees,
		ControlledPawn ? ControlledPawn->GetActorRotation().Yaw : 0.0f,
		0.0f);
}

FVector ATunaSweeperPlayerController::CalculateHousingCameraLocation(
	const FVector& FocusLocation,
	const FRotator& CameraRotation) const
{
	return FocusLocation - CameraRotation.Vector() * TunaSweeperHousingCamera::Distance;
}

void ATunaSweeperPlayerController::ClampHousingCameraFocusLocation(FVector& InOutFocusLocation) const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	const ATunaSweeperHousingAreaActor* HousingArea = HousingSubsystem
		? HousingSubsystem->GetActiveHousingArea()
		: nullptr;
	if (!HousingArea)
	{
		return;
	}

	const float CellSize = FMath::Max(1.0f, HousingArea->GetCellSize());
	const float HalfWidth = static_cast<float>(FMath::Max(1, HousingArea->GetGridSizeX())) * CellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, HousingArea->GetGridSizeY())) * CellSize * 0.5f;
	FVector LocalLocation = HousingArea->GetActorTransform().InverseTransformPosition(InOutFocusLocation);
	LocalLocation.X = FMath::Clamp(LocalLocation.X, -HalfWidth, HalfWidth);
	LocalLocation.Y = FMath::Clamp(LocalLocation.Y, -HalfHeight, HalfHeight);
	LocalLocation.Z = 0.0f;
	InOutFocusLocation = HousingArea->GetActorTransform().TransformPosition(LocalLocation);
}

FVector2D ATunaSweeperPlayerController::GetHousingCameraMoveInput() const
{
	if (!IsHousingModeOpen())
	{
		return FVector2D::ZeroVector;
	}

	const float RightValue = (bHousingMoveRightHeld ? 1.0f : 0.0f) - (bHousingMoveLeftHeld ? 1.0f : 0.0f);
	const float ForwardValue = (bHousingMoveForwardHeld ? 1.0f : 0.0f) - (bHousingMoveBackwardHeld ? 1.0f : 0.0f);
	return FVector2D(RightValue, ForwardValue).GetClampedToMaxSize(1.0f);
}

void ATunaSweeperPlayerController::HandleHousingMoveForwardPressed()
{
	bHousingMoveForwardHeld = IsHousingModeOpen();
}

void ATunaSweeperPlayerController::HandleHousingMoveForwardReleased()
{
	bHousingMoveForwardHeld = false;
}

void ATunaSweeperPlayerController::HandleHousingMoveBackwardPressed()
{
	bHousingMoveBackwardHeld = IsHousingModeOpen();
}

void ATunaSweeperPlayerController::HandleHousingMoveBackwardReleased()
{
	bHousingMoveBackwardHeld = false;
}

void ATunaSweeperPlayerController::HandleHousingMoveRightPressed()
{
	bHousingMoveRightHeld = IsHousingModeOpen();
}

void ATunaSweeperPlayerController::HandleHousingMoveRightReleased()
{
	bHousingMoveRightHeld = false;
}

void ATunaSweeperPlayerController::HandleHousingMoveLeftPressed()
{
	bHousingMoveLeftHeld = IsHousingModeOpen();
}

void ATunaSweeperPlayerController::HandleHousingMoveLeftReleased()
{
	bHousingMoveLeftHeld = false;
}

void ATunaSweeperPlayerController::ToggleInventoryOnlyPanel()
{
	if (IsHousingModeOpen())
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
			: nullptr)
		{
			HousingSubsystem->CloseHousingMode();
		}
		RestoreGameplayState(TunaSweeperHousingCamera::BlendSeconds);
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		const bool bWasGameplayMode = GameHudWidget->GetHudMode() == ETunaSweeperHudMode::None;
		GameHudWidget->ToggleInventoryOnlyPanel();
		if (GameHudWidget->GetHudMode() == ETunaSweeperHudMode::Inventory)
		{
			CancelPawnGameplayActions();
		}
		else if (!bWasGameplayMode)
		{
			RestoreGameplayState(0.0f);
		}
	}
}

void ATunaSweeperPlayerController::ToggleMapPanel()
{
	if (IsIntroMap() || IsOpeningScenarioMap() || bDialogueSequenceActive || IsHousingModeOpen())
	{
		return;
	}

	EnsureGameHudWidget();
	if (!GameHudWidget)
	{
		return;
	}

	const bool bMapAlreadyOpen = GameHudWidget->GetHudMode() == ETunaSweeperHudMode::Map;
	GameHudWidget->SetHudMode(bMapAlreadyOpen ? ETunaSweeperHudMode::None : ETunaSweeperHudMode::Map);
	if (!bMapAlreadyOpen)
	{
		CancelPawnGameplayActions();
	}
	else
	{
		RestoreGameplayState(0.0f);
	}
}

void ATunaSweeperPlayerController::OpenLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowLootContainerPanel(ContainerInstance);
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::OpenStoragePanel()
{
	if (!IsBunkerMap())
	{
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowStoragePanel();
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::OpenShopPanel(int32 ShopId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowShopPanel(ShopId);
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::OpenWorkbenchPanel(int32 WorkbenchId)
{
	OpenWorkbenchCraftPanel(WorkbenchId);
}

void ATunaSweeperPlayerController::OpenWorkbenchCraftPanel(int32 WorkbenchId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowWorkbenchPanel(WorkbenchId, ETunaSweeperWorkbenchMode::Craft);
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::OpenWorkbenchDismantlePanel(int32 WorkbenchId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowWorkbenchPanel(WorkbenchId, ETunaSweeperWorkbenchMode::Dismantle);
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::OpenWorkbenchBlueprintRegisterPanel(int32 WorkbenchId)
{
	if (!IsBunkerMap())
	{
		return;
	}

	EnsureGameHudWidget();

	if (GameHudWidget)
	{
		GameHudWidget->ShowWorkbenchPanel(WorkbenchId, ETunaSweeperWorkbenchMode::BlueprintRegister);
		CancelPawnGameplayActions();
	}
}

void ATunaSweeperPlayerController::DropWorkbenchOverflowItems(const TArray<FTunaSweeperItemStack>& OverflowItems)
{
	for (const FTunaSweeperItemStack& OverflowItem : OverflowItems)
	{
		SpawnDroppedPickupItem(OverflowItem.ItemId, OverflowItem.Quantity);
	}
}

void ATunaSweeperPlayerController::OpenQuestPanel(FName QuestId)
{
	if (!IsLocalController())
	{
		return;
	}

	EnsureGameHudWidget();
	if (!GameHudWidget)
	{
		return;
	}

	GameHudWidget->ShowQuestPanel(QuestId);
	CancelPawnGameplayActions();
	ApplyDefaultGameInputMode();
	bShowMouseCursor = true;
}

void ATunaSweeperPlayerController::OpenMemoPanel(int32 MemoId)
{
	if (!IsLocalController())
	{
		return;
	}

	EnsureGameHudWidget();
	if (!GameHudWidget)
	{
		return;
	}

	GameHudWidget->ShowMemoPanel(MemoId);
	CancelPawnGameplayActions();
	ApplyDefaultGameInputMode();
}

bool ATunaSweeperPlayerController::OpenHousingMode()
{
	if (!IsLocalController() || IsIntroMap() || IsOpeningScenarioMap() || bDialogueSequenceActive)
	{
		return false;
	}

	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (!HousingSubsystem)
	{
		return false;
	}

	EnsureGameHudWidget();
	if (GameHudWidget)
	{
		GameHudWidget->SetHudMode(ETunaSweeperHudMode::None);
	}

	if (!HousingSubsystem->OpenHousingMode(this))
	{
		return false;
	}

	CancelPawnGameplayActions();
	BeginHousingCameraMode();
	ApplyDefaultGameInputMode();
	bShowMouseCursor = true;
	return true;
}

bool ATunaSweeperPlayerController::StartHousingFacilityPlacement(FName FacilityId, FGuid ExistingInstanceId)
{
	if (FacilityId.IsNone())
	{
		UE_LOG(LogTunaSweeperHousingInput, Warning, TEXT("StartHousingFacilityPlacement failed: facility id is none."));
		return false;
	}

	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (!HousingSubsystem)
	{
		UE_LOG(LogTunaSweeperHousingInput, Warning, TEXT("StartHousingFacilityPlacement failed for %s: housing subsystem is missing."), *FacilityId.ToString());
		return false;
	}

	if (!HousingSubsystem->IsHousingModeOpen())
	{
		if (!OpenHousingMode())
		{
			UE_LOG(LogTunaSweeperHousingInput, Warning, TEXT("StartHousingFacilityPlacement failed for %s: could not open housing mode."), *FacilityId.ToString());
			return false;
		}
	}
	else if (!bHousingCameraActive)
	{
		BeginHousingCameraMode();
	}

	const bool bStartedPlacement = HousingSubsystem->StartPlacement(FacilityId, ExistingInstanceId);
	if (bStartedPlacement)
	{
		UpdateHousingCamera(0.0f);
		ApplyDefaultGameInputMode();
		bShowMouseCursor = true;
	}
	else
	{
		UE_LOG(
			LogTunaSweeperHousingInput,
			Warning,
			TEXT("StartHousingFacilityPlacement failed for %s: subsystem rejected placement start. ExistingInstanceId=%s"),
			*FacilityId.ToString(),
			*ExistingInstanceId.ToString());
	}

	return bStartedPlacement;
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

bool ATunaSweeperPlayerController::IsInventoryUiOpen() const
{
	return GameHudWidget && GameHudWidget->IsInventoryUiOpen();
}

bool ATunaSweeperPlayerController::IsHousingPlacementActive() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	return HousingSubsystem && HousingSubsystem->HasActivePlacement();
}

bool ATunaSweeperPlayerController::IsHousingModeOpen() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	return HousingSubsystem && HousingSubsystem->IsHousingModeOpen();
}

bool ATunaSweeperPlayerController::TryCommitHousingPlacement()
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		return HousingSubsystem->TryCommitPlacement(this);
	}

	return false;
}

bool ATunaSweeperPlayerController::GetMouseAimPointOnPlane(
	float PlaneZ,
	const FVector2D& ScreenOffset,
	FVector& OutAimPoint,
	FHitResult* OutAimHit) const
{
	if (OutAimHit)
	{
		*OutAimHit = FHitResult();
	}

	FVector WorldLocation;
	FVector WorldDirection;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!GetMousePosition(MouseX, MouseY) ||
		!DeprojectScreenPositionToWorld(
			MouseX + ScreenOffset.X,
			MouseY + ScreenOffset.Y,
			WorldLocation,
			WorldDirection))
	{
		return false;
	}

	bool bHasPlaneAimPoint = false;
	if (!FMath::IsNearlyZero(WorldDirection.Z))
	{
		const float DistanceToPlane = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
		if (DistanceToPlane >= 0.0f)
		{
			OutAimPoint = WorldLocation + WorldDirection * DistanceToPlane;
			bHasPlaneAimPoint = true;
		}
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
			if (!bHasPlaneAimPoint)
			{
				OutAimPoint = Hit.ImpactPoint;
			}
			if (OutAimHit)
			{
				*OutAimHit = Hit;
			}
			return true;
		}
	}

	return bHasPlaneAimPoint;
}
