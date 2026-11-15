#include "Interaction/TunaSweeperChainLinkGate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

namespace
{
	constexpr double GateWidth = 240.0;
	constexpr double GateHeight = 200.0;
}

ATunaSweeperChainLinkGate::ATunaSweeperChainLinkGate()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(SceneRoot);

	HingePost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HingePost"));
	HingePost->SetupAttachment(SceneRoot);
	LatchPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LatchPost"));
	LatchPost->SetupAttachment(SceneRoot);
	for (UStaticMeshComponent* Post : {HingePost.Get(), LatchPost.Get()})
	{
		Post->SetStaticMesh(VisualMesh->GetStaticMesh());
		Post->SetRelativeScale3D(FVector(0.12, 0.12, 2.2));
		Post->SetCollisionProfileName(TEXT("BlockAll"));
	}
	HingePost->SetRelativeLocation(FVector(-8, 0, 100));
	LatchPost->SetRelativeLocation(FVector(GateWidth + 8, 0, 100));

	VisualMesh->SetupAttachment(Hinge);
	VisualMesh->SetRelativeScale3D(FVector::OneVector);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GateCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateCollision"));
	GateCollision->SetupAttachment(Hinge);
	GateCollision->SetBoxExtent(FVector(GateWidth / 2, 4, GateHeight / 2));
	GateCollision->SetRelativeLocation(FVector(GateWidth / 2, 0, GateHeight / 2));
	GateCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GateCollision->SetGenerateOverlapEvents(false);
	GateCollision->CanCharacterStepUpOn = ECB_No;

	InteractableComponent->SetRelativeLocation(FVector(GateWidth / 2, 0, 145));
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::ChainLinkGate, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(
			TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		TEXT("ui.gate.chain_link"));
}

void ATunaSweeperChainLinkGate::BeginPlay()
{
	Super::BeginPlay();

	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->GetInventorySlots(); // Initialize the selected slot before reading its flags.
		bUnlocked = Game->IsScenarioProgressFlagSet(GetUnlockFlag());
	}
	if (bUnlocked)
	{
		OpenAlpha = 1.0f;
		ApplyPose();
		DisableInteraction();
	}
}

bool ATunaSweeperChainLinkGate::InspectLock(APawn* InstigatorPawn)
{
	if (bUnlocked || !IsValid(InstigatorPawn))
	{
		return false;
	}
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>())
		{
			Speech->ShowForActor(Game->ResolveLocalizedText(TEXT("ui.gate.locked"), FText::GetEmpty()),
				InstigatorPawn, FVector(0, 0, 20), ETunaSweeperSpeechBubbleTailDirection::Down, 2.0f, -28.0f);
		}
	}
	return true;
}

bool ATunaSweeperChainLinkGate::Unlock()
{
	auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	const FName UnlockFlag = GetUnlockFlag();
	if (bUnlocked || !Game || UnlockFlag.IsNone() || !Game->TrySaveGateUnlock(UnlockFlag))
	{
		return false;
	}

	bUnlocked = true;
	DisableInteraction();
	SetActorTickEnabled(true);
	return true;
}

bool ATunaSweeperChainLinkGate::TryResetPersistently()
{
	auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	if (!Game || !Game->TrySaveGateReset(GetUnlockFlag())) return false;
	bUnlocked = false;
	OpenAlpha = 0.0f;
	SetActorTickEnabled(false);
	ApplyPose();
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType::ChainLinkGate, FText::GetEmpty(), TEXT("ui.gate.chain_link"));
	return true;
}

void ATunaSweeperChainLinkGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	OpenAlpha = FMath::Min(1.0f, OpenAlpha + DeltaSeconds / FMath::Max(0.05f, OpeningDuration));
	ApplyPose();
	if (OpenAlpha >= 1.0f)
	{
		SetActorTickEnabled(false);
	}
}

FName ATunaSweeperChainLinkGate::GetUnlockFlag() const
{
	if (GateId.IsNone() || !GetLevel())
	{
		return NAME_None;
	}
	return FName(FString::Printf(TEXT("gate.unlocked:%s:%s"),
		*UWorld::RemovePIEPrefix(GetLevel()->GetOutermost()->GetName()), *GateId.ToString()));
}

void ATunaSweeperChainLinkGate::ApplyPose()
{
	const float Angle = OpenAngle * FMath::SmoothStep(0.0f, 1.0f, OpenAlpha);
	Hinge->SetRelativeRotation(FRotator(0.0f, Angle, 0.0f));
}

void ATunaSweeperChainLinkGate::DisableInteraction()
{
	InteractableComponent->SetInteractionTypeDisplayNameAndStringKey(
		ETunaSweeperInteractionType::None, FText::GetEmpty(), NAME_None);
}
