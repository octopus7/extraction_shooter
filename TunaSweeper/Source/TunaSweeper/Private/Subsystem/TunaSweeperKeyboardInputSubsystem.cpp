#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UnrealClient.h"

namespace
{
	class FTunaSweeperScreenshotInputProcessor final : public IInputProcessor
	{
	public:
		explicit FTunaSweeperScreenshotInputProcessor(UTunaSweeperKeyboardInputSubsystem* InOwner)
			: Owner(InOwner)
		{
		}

		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApplication, TSharedRef<ICursor> Cursor) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApplication, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.IsRepeat() || InKeyEvent.GetKey() != EKeys::F9)
			{
				return false;
			}

			UTunaSweeperKeyboardInputSubsystem* KeyboardInputSubsystem = Owner.Get();
			return KeyboardInputSubsystem && KeyboardInputSubsystem->CaptureScreenshotFromHotkey();
		}

	private:
		TWeakObjectPtr<UTunaSweeperKeyboardInputSubsystem> Owner;
	};
}

void UTunaSweeperKeyboardInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FSlateApplication::IsInitialized())
	{
		ScreenshotInputProcessor = MakeShared<FTunaSweeperScreenshotInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(ScreenshotInputProcessor, 0);
	}
}

void UTunaSweeperKeyboardInputSubsystem::Deinitialize()
{
	if (ScreenshotInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ScreenshotInputProcessor);
	}
	ScreenshotInputProcessor.Reset();

	Super::Deinitialize();
}

bool UTunaSweeperKeyboardInputSubsystem::CaptureScreenshotFromHotkey()
{
	UGameInstance* GameInstance = GetGameInstance();
	UGameViewportClient* GameViewport = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;
	if (!GameViewport || !GameViewport->GetWorld() || GameViewport->GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const TSharedPtr<SWindow> GameWindow = GameViewport->GetWindow();
	if (GameWindow.IsValid() && FSlateApplication::IsInitialized())
	{
		const TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
		if (ActiveWindow.IsValid() && ActiveWindow != GameWindow)
		{
			return false;
		}
	}

	if (!FScreenshotRequest::IsScreenshotRequested())
	{
		FScreenshotRequest::RequestScreenshot(true);
	}

	// Consume F9 so UE's development-only "shot showui" binding cannot request a duplicate capture.
	return true;
}

void UTunaSweeperKeyboardInputSubsystem::ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn* InstigatorPawn)
{
	const int32 ClampedSlotNumber = FMath::Clamp(SlotNumber, 1, 8);
	const int32 UsableQuickSlotIndex = ClampedSlotNumber - 3;

	FString Message = FString::Printf(TEXT("[QuickSlot] Slot %d is empty."), ClampedSlotNumber);
	if (ClampedSlotNumber >= 3)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			FTunaSweeperItemSlotReference QuickSlotReference;
			QuickSlotReference.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
			QuickSlotReference.SlotIndex = UsableQuickSlotIndex;
			FTunaSweeperItemInstance ItemInstance;
			if (TunaGameInstance->TryGetSlotItemInstance(QuickSlotReference, ItemInstance))
			{
				FText ItemName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
				if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
				{
					ItemDataSubsystem->TryGetItemNameText(ItemInstance.ItemId, TunaGameInstance->GetCurrentTextLanguage(), ItemName);
				}
				ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
				if (TunaCharacter && TunaCharacter->StartItemUseFromSlot(QuickSlotReference))
				{
					Message = FString::Printf(
						TEXT("[QuickSlot] Slot %d using: %s"),
						ClampedSlotNumber,
						*ItemName.ToString());
				}
				else
				{
					Message = FString::Printf(
						TEXT("[QuickSlot] Slot %d cannot use: %s"),
						ClampedSlotNumber,
						*ItemName.ToString());
				}
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			Message);
	}
}
