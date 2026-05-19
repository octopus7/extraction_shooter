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
#include "UI/TunaSweeperScenarioPresentationWidget.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "UI/TunaSweeperWorldProgressWidget.h"
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
}

namespace TunaSweeperCanBotIntro
{
	const FName DialogueCompletionFlag(TEXT("dialogue.canbot.bunker_intro"));
	const FText SpeakerName = FText::FromString(TEXT("캔봇"));
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
	else if (IsOpeningScenarioMap())
	{
		EnsureScenarioPresentationWidget();
	}
	else
	{
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

	auto AddCanBotLine = [&OutDialogueLines](const FText& DialogueText)
	{
		FTunaSweeperDialogueLine Line;
		Line.SpeakerName = TunaSweeperCanBotIntro::SpeakerName;
		Line.DialogueText = DialogueText;
		OutDialogueLines.Add(Line);
	};

	AddCanBotLine(FText::FromString(TEXT("생존자 확인. 신경 반응 정상. 캔봇이 응답합니다.")));
	AddCanBotLine(FText::FromString(TEXT("이곳은 B-07 벙커입니다. 방금 깨어난 당신의 임시 거점입니다.")));

	FTunaSweeperDialogueLine CameraLine;
	CameraLine.SpeakerName = TunaSweeperCanBotIntro::SpeakerName;
	CameraLine.DialogueText = FText::FromString(TEXT("저쪽 이동 사다리를 사용하면 외부 구역으로 출동할 수 있습니다. 준비 전에는 접근하지 않는 편이 안전합니다."));
	CameraLine.bUseCameraFocus = true;
	CameraLine.CameraFocusLocation = TunaSweeperCanBotIntro::DeployLadderFocusLocation;
	CameraLine.CameraBlendSeconds = 0.8f;
	OutDialogueLines.Add(CameraLine);

	AddCanBotLine(FText::FromString(TEXT("기본 생체 수치와 장비 상태를 확인했습니다. 조작 권한을 돌려드리겠습니다.")));
	AddCanBotLine(FText::FromString(TEXT("필요하면 다시 말을 걸어 안내를 요청하십시오.")));
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
	if (!TunaGameInstance || !TunaGameInstance->HasHoveredItemSlot())
	{
		return false;
	}

	const FTunaSweeperItemSlotReference HoveredSlot = TunaGameInstance->GetHoveredItemSlotReference();
	FTunaSweeperItemInstance HoveredItemInstance;
	if (!TunaGameInstance->TryGetSlotItemInstance(HoveredSlot, HoveredItemInstance))
	{
		TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		return true;
	}

	FTunaSweeperItemSlotReference TargetSlot;
	if (TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, true, TargetSlot) ||
		TunaSweeperHoveredItemInteraction::TryFindEquipmentTarget(TunaGameInstance, HoveredSlot, false, TargetSlot))
	{
		TunaGameInstance->MoveItemBetweenSlots(HoveredSlot, TargetSlot);
		TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
		return true;
	}

	if (HoveredSlot.Source == ETunaSweeperItemSlotSource::LootContainer &&
		TunaSweeperHoveredItemInteraction::TryFindFirstInventoryTarget(TunaGameInstance, HoveredSlot, TargetSlot))
	{
		TunaGameInstance->MoveItemBetweenSlots(HoveredSlot, TargetSlot);
		TunaGameInstance->ClearHoveredItemSlot(HoveredSlot);
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
		if (IsInventoryUiOpen())
		{
			CancelPawnGameplayActions();
		}
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

void ATunaSweeperPlayerController::OpenWorldProgressPanel(ATunaSweeperWorldProgressActor* ProgressActor)
{
	if (!IsLocalController() || !ProgressActor)
	{
		return;
	}

	if (!WorldProgressWidget)
	{
		WorldProgressWidget = CreateWidget<UTunaSweeperWorldProgressWidget>(
			this,
			UTunaSweeperWorldProgressWidget::StaticClass());
	}

	if (!WorldProgressWidget)
	{
		return;
	}

	WorldProgressWidget->SetProgressActor(ProgressActor);
	if (!WorldProgressWidget->IsInViewport())
	{
		WorldProgressWidget->AddToViewport(40);
	}

	CancelPawnGameplayActions();

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(WorldProgressWidget->TakeWidget());
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

bool ATunaSweeperPlayerController::IsInventoryUiOpen() const
{
	return GameHudWidget && GameHudWidget->IsInventoryUiOpen();
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
