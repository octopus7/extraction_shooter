#include "Component/TunaSweeperHeadphoneListenerComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "UI/TunaSweeperHeadphoneRippleWidget.h"

namespace
{
	const FName EarCategoryTag(TEXT("item.category.ear"));
	const FName EarEquipmentSlotTag(TEXT("equipment.slot.ear"));
}

UTunaSweeperHeadphoneListenerComponent::UTunaSweeperHeadphoneListenerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	RippleWidgetClass = UTunaSweeperHeadphoneRippleWidget::StaticClass();
}

void UTunaSweeperHeadphoneListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	BindDelegates();
	RefreshEquippedHeadphone();
}

void UTunaSweeperHeadphoneListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveRippleWidget();
	UnbindDelegates();
	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperHeadphoneListenerComponent::BindDelegates()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.RemoveAll(this);
			NoiseSubsystem->OnNoiseReported.AddUObject(this, &UTunaSweeperHeadphoneListenerComponent::HandleNoiseReported);
		}
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr)
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHeadphoneListenerComponent::RefreshEquippedHeadphone);
	}
}

void UTunaSweeperHeadphoneListenerComponent::UnbindDelegates()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.RemoveAll(this);
		}
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr)
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}
}

void UTunaSweeperHeadphoneListenerComponent::RefreshEquippedHeadphone()
{
	bHeadphoneEquipped = false;
	CachedHearingRange = 0.0f;
	CachedSensitivity = 0.0f;
	CachedMinStrength = 0.0f;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = World->GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!TunaGameInstance || !ItemDataSubsystem)
	{
		return;
	}

	const TArray<FTunaSweeperInventorySlot>& EquipmentSlots = TunaGameInstance->GetEquipmentSlots();
	for (const FTunaSweeperInventorySlot& EquipmentSlot : EquipmentSlots)
	{
		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance->TryGetItemInstance(EquipmentSlot.ItemUid, ItemInstance))
		{
			continue;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		if (!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			continue;
		}

		if (ItemDefinition.CategoryTag != EarCategoryTag && ItemDefinition.EquipmentSlotTag != EarEquipmentSlotTag)
		{
			continue;
		}

		if (ItemDefinition.HeadphoneHearingRange <= 0.0f && ItemDefinition.HeadphoneSensitivity <= 0.0f)
		{
			continue;
		}

		bHeadphoneEquipped = true;
		CachedHearingRange = ItemDefinition.HeadphoneHearingRange > 0.0f
			? ItemDefinition.HeadphoneHearingRange
			: DefaultHearingRange;
		CachedSensitivity = ItemDefinition.HeadphoneSensitivity > 0.0f
			? ItemDefinition.HeadphoneSensitivity
			: DefaultSensitivity;
		CachedMinStrength = ItemDefinition.HeadphoneMinStrength > 0.0f
			? ItemDefinition.HeadphoneMinStrength
			: DefaultMinStrength;
		return;
	}

	RemoveRippleWidget();
}

bool UTunaSweeperHeadphoneListenerComponent::IsListenerPawnReady() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsPlayerControlled() && !OwnerPawn->IsHidden();
}

bool UTunaSweeperHeadphoneListenerComponent::ShouldIgnoreNoiseSource(const FTunaSweeperNoiseEvent& NoiseEvent) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return true;
	}

	if (NoiseEvent.SourceActor == OwnerActor || NoiseEvent.InstigatorActor == OwnerActor)
	{
		return true;
	}

	if (NoiseEvent.SourceActor && NoiseEvent.SourceActor->GetOwner() == OwnerActor)
	{
		return true;
	}

	return false;
}

void UTunaSweeperHeadphoneListenerComponent::HandleNoiseReported(const FTunaSweeperNoiseEvent& NoiseEvent)
{
	if (!bHeadphoneEquipped || !IsListenerPawnReady() || ShouldIgnoreNoiseSource(NoiseEvent))
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return;
	}

	const FVector ListenerLocation = OwnerActor->GetActorLocation();
	const float SourceDistance = FVector::Dist2D(ListenerLocation, NoiseEvent.SourceLocation);
	if (SourceDistance <= SelfNoiseIgnoreDistance || SourceDistance < MinVisualNoiseDistance)
	{
		return;
	}

	UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	if (!NoiseSubsystem)
	{
		return;
	}

	FTunaSweeperHeardNoiseEvent HeardNoise;
	if (!NoiseSubsystem->CalculateHeardNoiseAtLocation(
		NoiseEvent,
		ListenerLocation,
		CachedHearingRange,
		CachedSensitivity,
		CachedMinStrength,
		HeardNoise))
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	if (CurrentTimeSeconds - LastRippleSpawnTimeSeconds < RippleCooldownSeconds)
	{
		return;
	}

	UTunaSweeperHeadphoneRippleWidget* EffectiveRippleWidget = EnsureRippleWidget();
	if (!EffectiveRippleWidget)
	{
		return;
	}

	LastRippleSpawnTimeSeconds = CurrentTimeSeconds;
	EffectiveRippleWidget->AddNoiseRipple(OwnerActor, HeardNoise.DirectionFromListener, HeardNoise.Strength);
}

UTunaSweeperHeadphoneRippleWidget* UTunaSweeperHeadphoneListenerComponent::EnsureRippleWidget()
{
	if (RippleWidget && RippleWidget->IsInViewport())
	{
		RippleWidget->SetListenerActor(GetOwner());
		return RippleWidget;
	}

	AActor* OwnerActor = GetOwner();
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	APlayerController* PlayerController = OwnerPawn
		? Cast<APlayerController>(OwnerPawn->GetController())
		: nullptr;
	if (!PlayerController)
	{
		UWorld* World = GetWorld();
		PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	}
	if (!PlayerController)
	{
		return nullptr;
	}

	TSubclassOf<UTunaSweeperHeadphoneRippleWidget> EffectiveWidgetClass = RippleWidgetClass;
	if (!EffectiveWidgetClass)
	{
		EffectiveWidgetClass = UTunaSweeperHeadphoneRippleWidget::StaticClass();
	}
	if (!EffectiveWidgetClass)
	{
		return nullptr;
	}

	RippleWidget = CreateWidget<UTunaSweeperHeadphoneRippleWidget>(PlayerController, EffectiveWidgetClass);
	if (!RippleWidget)
	{
		return nullptr;
	}

	RippleWidget->SetListenerActor(OwnerActor);
	RippleWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	RippleWidget->SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	RippleWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	RippleWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
	RippleWidget->AddToViewport(35);
	return RippleWidget;
}

void UTunaSweeperHeadphoneListenerComponent::RemoveRippleWidget()
{
	if (!RippleWidget)
	{
		return;
	}

	RippleWidget->RemoveFromParent();
	RippleWidget = nullptr;
}
