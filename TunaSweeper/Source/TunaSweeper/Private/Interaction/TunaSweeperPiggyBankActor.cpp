#include "Interaction/TunaSweeperPiggyBankActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Sound/SoundWaveProcedural.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperSpeechBubbleWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperPiggyBank
{
	constexpr int32 SampleRate = 24000;
	constexpr int32 ChannelCount = 1;
	constexpr float SoundDurationSeconds = 0.58f;
	constexpr float SpeechBubbleHeight = 176.0f;
	constexpr float SpeechBubbleWidth = 210.0f;
	constexpr float SpeechBubbleDrawHeight = 78.0f;

	float SmoothStart(float TimeSeconds, float Speed)
	{
		return 1.0f - FMath::Exp(-FMath::Max(0.0f, TimeSeconds) * Speed);
	}

	float Decay(float TimeSeconds, float Speed)
	{
		return FMath::Exp(-FMath::Max(0.0f, TimeSeconds) * Speed);
	}

	float HashNoise(int32 Index)
	{
		return FMath::Frac(FMath::Sin(static_cast<float>(Index) * 12.9898f) * 43758.5453f) * 2.0f - 1.0f;
	}

}

ATunaSweeperPiggyBankActor::ATunaSweeperPiggyBankActor()
{
	const TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));

	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::PiggyBank,
		FText::FromString(TEXT("\uB3C8\uB0B4\uB194")),
		MarkerWidgetClass,
		FName(TEXT("ui.interaction.piggy_bank")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* SphereMesh = SphereMeshFinder.Succeeded() ? SphereMeshFinder.Object : nullptr;
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
		InteractableComponent->SetInteractionOrder(0);
	}

	DepositInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("DepositInteractable"));
	if (DepositInteractableComponent)
	{
		DepositInteractableComponent->SetupAttachment(RootComponent);
		DepositInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
		DepositInteractableComponent->SetInteractionOrder(1);
		DepositInteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::PiggyBankDeposit,
			FText::FromString(TEXT("\uC800\uAE08")),
			MarkerWidgetClass,
			FName(TEXT("ui.interaction.piggy_bank_deposit")));
	}

	WithdrawInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("WithdrawInteractable"));
	if (WithdrawInteractableComponent)
	{
		WithdrawInteractableComponent->SetupAttachment(RootComponent);
		WithdrawInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
		WithdrawInteractableComponent->SetInteractionOrder(2);
		WithdrawInteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::PiggyBankWithdraw,
			FText::FromString(TEXT("\uBE7C\uAE30")),
			MarkerWidgetClass,
			FName(TEXT("ui.interaction.piggy_bank_withdraw")));
	}

	SpeechBubbleWidgetClass = TSoftClassPtr<UTunaSweeperSpeechBubbleWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_SpeechBubble.WBP_SpeechBubble_C")));
	SpeechBubbleWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SpeechBubble"));
	if (SpeechBubbleWidgetComponent)
	{
		SpeechBubbleWidgetComponent->SetupAttachment(RootComponent);
		SpeechBubbleWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, TunaSweeperPiggyBank::SpeechBubbleHeight));
		SpeechBubbleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		SpeechBubbleWidgetComponent->SetDrawSize(FVector2D(
			TunaSweeperPiggyBank::SpeechBubbleWidth,
			TunaSweeperPiggyBank::SpeechBubbleDrawHeight));
		SpeechBubbleWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
		SpeechBubbleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpeechBubbleWidgetComponent->SetVisibility(false);
		SpeechBubbleWidgetComponent->SetHiddenInGame(false);
	}

	if (VisualMesh && SphereMesh)
	{
		VisualMesh->SetStaticMesh(SphereMesh);
		VisualMesh->SetRelativeScale3D(FVector(0.86f, 0.52f, 0.48f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 52.0f));
	}

	SnoutMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnoutMesh"));
	CoinSlotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinSlotMesh"));
	LeftEarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEarMesh"));
	RightEarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEarMesh"));
	FrontLeftLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftLegMesh"));
	FrontRightLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightLegMesh"));
	BackLeftLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackLeftLegMesh"));
	BackRightLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRightLegMesh"));

	if (SnoutMesh)
	{
		SnoutMesh->SetupAttachment(RootComponent);
		SnoutMesh->SetStaticMesh(SphereMesh);
		SnoutMesh->SetRelativeLocation(FVector(52.0f, 0.0f, 54.0f));
		SnoutMesh->SetRelativeScale3D(FVector(0.22f, 0.28f, 0.20f));
		SnoutMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (CoinSlotMesh)
	{
		CoinSlotMesh->SetupAttachment(RootComponent);
		CoinSlotMesh->SetStaticMesh(CubeMesh);
		CoinSlotMesh->SetRelativeLocation(FVector(4.0f, 0.0f, 92.0f));
		CoinSlotMesh->SetRelativeScale3D(FVector(0.38f, 0.055f, 0.025f));
		CoinSlotMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	const FVector EarScale(0.14f, 0.08f, 0.22f);
	if (LeftEarMesh)
	{
		LeftEarMesh->SetupAttachment(RootComponent);
		LeftEarMesh->SetStaticMesh(SphereMesh);
		LeftEarMesh->SetRelativeLocation(FVector(-18.0f, -28.0f, 82.0f));
		LeftEarMesh->SetRelativeScale3D(EarScale);
		LeftEarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (RightEarMesh)
	{
		RightEarMesh->SetupAttachment(RootComponent);
		RightEarMesh->SetStaticMesh(SphereMesh);
		RightEarMesh->SetRelativeLocation(FVector(-18.0f, 28.0f, 82.0f));
		RightEarMesh->SetRelativeScale3D(EarScale);
		RightEarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	struct FLegPlacement
	{
		UStaticMeshComponent* Mesh = nullptr;
		FVector Location = FVector::ZeroVector;
	};

	const FLegPlacement Legs[] = {
		{ FrontLeftLegMesh, FVector(28.0f, -24.0f, 14.0f) },
		{ FrontRightLegMesh, FVector(28.0f, 24.0f, 14.0f) },
		{ BackLeftLegMesh, FVector(-30.0f, -24.0f, 14.0f) },
		{ BackRightLegMesh, FVector(-30.0f, 24.0f, 14.0f) }
	};

	for (const FLegPlacement& Leg : Legs)
	{
		if (!Leg.Mesh)
		{
			continue;
		}

		Leg.Mesh->SetupAttachment(RootComponent);
		Leg.Mesh->SetStaticMesh(CubeMesh);
		Leg.Mesh->SetRelativeLocation(Leg.Location);
		Leg.Mesh->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.22f));
		Leg.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ATunaSweeperPiggyBankActor::SetGrantAmount(int32 InGrantAmount)
{
	GrantAmount = FMath::Max(1, InGrantAmount);
}

void ATunaSweeperPiggyBankActor::SetPiggyBankId(FName InPiggyBankId)
{
	if (!InPiggyBankId.IsNone())
	{
		PiggyBankId = InPiggyBankId;
	}
}

void ATunaSweeperPiggyBankActor::ConfigurePiggyBankDefaults(
	int32 InGrantAmount,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	FName InPiggyBankId)
{
	SetGrantAmount(InGrantAmount);
	SetPiggyBankId(InPiggyBankId);
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::PiggyBank,
		InInteractionDisplayName,
		InMarkerWidgetClass,
		FName(TEXT("ui.interaction.piggy_bank")));
	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionOrder(0);
	}
	if (DepositInteractableComponent)
	{
		DepositInteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::PiggyBankDeposit,
			FText::FromString(TEXT("\uC800\uAE08")),
			InMarkerWidgetClass,
			FName(TEXT("ui.interaction.piggy_bank_deposit")));
		DepositInteractableComponent->SetInteractionOrder(1);
	}
	if (WithdrawInteractableComponent)
	{
		WithdrawInteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::PiggyBankWithdraw,
			FText::FromString(TEXT("\uBE7C\uAE30")),
			InMarkerWidgetClass,
			FName(TEXT("ui.interaction.piggy_bank_withdraw")));
		WithdrawInteractableComponent->SetInteractionOrder(2);
	}
}

bool ATunaSweeperPiggyBankActor::GrantCurrency(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || GrantAmount <= 0)
	{
		return false;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	QuestSubsystem->AddCoins(GrantAmount, true);
	PlayMoneySound();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.5f,
			FColor::Green,
			FString::Printf(TEXT("[Debug] +%d coins"), GrantAmount));
	}
	return true;
}

int32 ATunaSweeperPiggyBankActor::GetStoredAncientCoinValue() const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr;
	return TunaGameInstance ? TunaGameInstance->GetPiggyBankStoredAncientCoinValue(PiggyBankId) : 0;
}

bool ATunaSweeperPiggyBankActor::DepositAncientCurrencyItems(APawn* InstigatorPawn)
{
	if (!InstigatorPawn)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr;
	if (!TunaGameInstance)
	{
		return false;
	}

	const int32 AvailableCoins = TunaGameInstance->CountInventoryItemById(AncientCoinItemId);
	const int32 AvailableBanknotes = TunaGameInstance->CountInventoryItemById(AncientBanknoteItemId);
	if (AvailableCoins <= 0 && AvailableBanknotes <= 0)
	{
		ShowSpeechBubble(FText::FromString(TEXT("\uC800\uAE08\uD560 \uB3D9\uC804\uC774\uB098 \uC9C0\uD3D0\uAC00 \uC5C6\uB2E4")));
		return true;
	}

	const int32 ConsumedCoins = AvailableCoins > 0
		? TunaGameInstance->ConsumeInventoryItemById(AncientCoinItemId, AvailableCoins)
		: 0;
	const int32 ConsumedBanknotes = AvailableBanknotes > 0
		? TunaGameInstance->ConsumeInventoryItemById(AncientBanknoteItemId, AvailableBanknotes)
		: 0;
	const int32 DepositedCoinValue =
		FMath::Max(0, ConsumedCoins) +
		FMath::Max(0, ConsumedBanknotes) * FMath::Max(1, AncientBanknoteCoinValue);
	if (DepositedCoinValue <= 0)
	{
		ShowSpeechBubble(FText::FromString(TEXT("\uC800\uAE08 \uC2E4\uD328")));
		return true;
	}

	TunaGameInstance->AddPiggyBankStoredAncientCoinValue(PiggyBankId, DepositedCoinValue, true);
	PlayMoneySound();
	ShowSpeechBubble(FText::Format(
		FText::FromString(TEXT("\uC800\uAE08 {0}")),
		FText::AsNumber(DepositedCoinValue)));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			1.8f,
			FColor::Cyan,
			FString::Printf(
				TEXT("[PiggyBank] Stored ancient value +%d (coin %d, banknote %d) / total %d"),
				DepositedCoinValue,
				ConsumedCoins,
				ConsumedBanknotes,
				GetStoredAncientCoinValue()));
	}
	return true;
}

bool ATunaSweeperPiggyBankActor::ShowWithdrawNotImplemented(APawn* InstigatorPawn)
{
	ShowSpeechBubble(FText::FromString(TEXT("\uBBF8\uAD6C\uD604")));
	StartWithdrawDialogue(InstigatorPawn);
	return true;
}

USoundWaveProcedural* ATunaSweeperPiggyBankActor::CreateMoneySound()
{
	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(this);
	if (!SoundWave)
	{
		return nullptr;
	}

	SoundWave->SetSampleRate(TunaSweeperPiggyBank::SampleRate);
	SoundWave->NumChannels = TunaSweeperPiggyBank::ChannelCount;
	SoundWave->Duration = TunaSweeperPiggyBank::SoundDurationSeconds;
	SoundWave->SoundGroup = SOUNDGROUP_Effects;
	SoundWave->bLooping = false;

	const int32 SampleCount = FMath::CeilToInt(TunaSweeperPiggyBank::SoundDurationSeconds * TunaSweeperPiggyBank::SampleRate);
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount * TunaSweeperPiggyBank::ChannelCount);

	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float TimeSeconds = static_cast<float>(SampleIndex) / static_cast<float>(TunaSweeperPiggyBank::SampleRate);
		const float MainBell = FMath::Sin(2.0f * UE_PI * 1040.0f * TimeSeconds) *
			TunaSweeperPiggyBank::SmoothStart(TimeSeconds, 70.0f) *
			TunaSweeperPiggyBank::Decay(TimeSeconds, 6.8f);
		const float RingDelay = FMath::Max(0.0f, TimeSeconds - 0.085f);
		const float HighBell =
			(FMath::Sin(2.0f * UE_PI * 1560.0f * RingDelay) +
				FMath::Sin(2.0f * UE_PI * 2080.0f * RingDelay) * 0.55f) *
			TunaSweeperPiggyBank::SmoothStart(RingDelay, 85.0f) *
			TunaSweeperPiggyBank::Decay(RingDelay, 8.5f);

		const bool bCoinClickWindow =
			(TimeSeconds > 0.025f && TimeSeconds < 0.075f) ||
			(TimeSeconds > 0.145f && TimeSeconds < 0.205f) ||
			(TimeSeconds > 0.265f && TimeSeconds < 0.310f);
		const float CoinClick = bCoinClickWindow
			? TunaSweeperPiggyBank::HashNoise(SampleIndex) * TunaSweeperPiggyBank::Decay(TimeSeconds, 5.8f)
			: 0.0f;

		const float Mixed = FMath::Clamp(MainBell * 0.40f + HighBell * 0.34f + CoinClick * 0.12f, -0.92f, 0.92f);
		Samples[SampleIndex] = static_cast<int16>(Mixed * 32767.0f);
	}

	SoundWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	return SoundWave;
}

void ATunaSweeperPiggyBankActor::PlayMoneySound()
{
	USoundWaveProcedural* SoundWave = CreateMoneySound();
	if (!SoundWave)
	{
		return;
	}

	ActiveMoneySounds.Add(SoundWave);
	UGameplayStatics::PlaySoundAtLocation(this, SoundWave, GetActorLocation(), 1.0f, 1.0f);

	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<ATunaSweeperPiggyBankActor> WeakThis(this);
		TWeakObjectPtr<USoundWaveProcedural> WeakSound(SoundWave);
		FTimerHandle CleanupTimerHandle;
		World->GetTimerManager().SetTimer(
			CleanupTimerHandle,
			FTimerDelegate::CreateLambda([WeakThis, WeakSound]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->ActiveMoneySounds.Remove(WeakSound.Get());
				}
			}),
			TunaSweeperPiggyBank::SoundDurationSeconds + 0.2f,
			false);
	}
}

void ATunaSweeperPiggyBankActor::ShowSpeechBubble(const FText& InText, float DisplaySeconds)
{
	if (!SpeechBubbleWidgetComponent)
	{
		return;
	}

	EnsureSpeechBubbleWidgetClass();
	SpeechBubbleWidgetComponent->SetVisibility(true);
	if (UTunaSweeperSpeechBubbleWidget* SpeechBubbleWidget =
		Cast<UTunaSweeperSpeechBubbleWidget>(SpeechBubbleWidgetComponent->GetUserWidgetObject()))
	{
		SpeechBubbleWidget->SetBubbleText(InText);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpeechBubbleTimerHandle);
		World->GetTimerManager().SetTimer(
			SpeechBubbleTimerHandle,
			this,
			&ATunaSweeperPiggyBankActor::HideSpeechBubble,
			FMath::Max(0.1f, DisplaySeconds),
			false);
	}
}

void ATunaSweeperPiggyBankActor::HideSpeechBubble()
{
	if (SpeechBubbleWidgetComponent)
	{
		SpeechBubbleWidgetComponent->SetVisibility(false);
	}
}

void ATunaSweeperPiggyBankActor::EnsureSpeechBubbleWidgetClass()
{
	if (!SpeechBubbleWidgetComponent)
	{
		return;
	}

	if (TSubclassOf<UTunaSweeperSpeechBubbleWidget> LoadedClass = SpeechBubbleWidgetClass.LoadSynchronous())
	{
		if (SpeechBubbleWidgetComponent->GetWidgetClass() != LoadedClass)
		{
			SpeechBubbleWidgetComponent->SetWidgetClass(LoadedClass);
		}
		SpeechBubbleWidgetComponent->InitWidget();
	}
}

void ATunaSweeperPiggyBankActor::StartWithdrawDialogue(APawn* InstigatorPawn)
{
	ATunaSweeperPlayerController* TunaPlayerController = InstigatorPawn
		? Cast<ATunaSweeperPlayerController>(InstigatorPawn->GetController())
		: nullptr;
	if (!TunaPlayerController || TunaPlayerController->IsDialogueSequenceActive())
	{
		return;
	}

	FTunaSweeperDialogueLine PlayerLine;
	PlayerLine.SpeakerName = FText::FromString(TEXT("\uD50C\uB808\uC774\uC5B4"));
	PlayerLine.DialogueText = FText::FromString(TEXT("\uBA39\uC5C8\uB0D0?"));

	TArray<FTunaSweeperDialogueLine> DialogueLines;
	DialogueLines.Add(PlayerLine);
	TunaPlayerController->StartDialogueSequence(DialogueLines, NAME_None);
}
