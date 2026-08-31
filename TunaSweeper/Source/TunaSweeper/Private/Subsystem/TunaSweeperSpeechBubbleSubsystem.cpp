#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystemInternal.h"
#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"
#include "UI/TunaSweeperSpeechBubbleLayerWidget.h"

namespace
{
	constexpr int32 SpeechBubbleViewportZOrder = 40;

	bool IsTailDirectionValid(const ETunaSweeperSpeechBubbleTailDirection Direction)
	{
		const UEnum* Enum = StaticEnum<ETunaSweeperSpeechBubbleTailDirection>();
		return Enum && Enum->IsValidEnumValue(static_cast<int64>(Direction));
	}
}

void UTunaSweeperSpeechBubbleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
		this,
		&UTunaSweeperSpeechBubbleSubsystem::HandleWorldCleanup);
}

void UTunaSweeperSpeechBubbleSubsystem::Deinitialize()
{
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

	HideAllSpeechBubbles();
	if (LayerWidget)
	{
		LayerWidget->RemoveFromParent();
		LayerWidget = nullptr;
	}
	Super::Deinitialize();
}

FGuid UTunaSweeperSpeechBubbleSubsystem::ShowAtScreen(
	const FText& Text,
	const FVector2D ScreenPosition,
	const ETunaSweeperSpeechBubbleTailDirection TailDirection,
	const float DurationSeconds)
{
	if (Text.IsEmpty() || ScreenPosition.ContainsNaN())
	{
		return FGuid();
	}

	FActiveSpeechBubble Candidate;
	Candidate.AnchorType = EAnchorType::Screen;
	Candidate.ScreenPosition = ScreenPosition;
	return ShowBubble(Text, Candidate, TailDirection, DurationSeconds);
}

FGuid UTunaSweeperSpeechBubbleSubsystem::ShowAtWorld(
	const FText& Text,
	const FVector WorldLocation,
	const ETunaSweeperSpeechBubbleTailDirection TailDirection,
	const float DurationSeconds)
{
	if (Text.IsEmpty() || WorldLocation.ContainsNaN())
	{
		return FGuid();
	}

	FActiveSpeechBubble Candidate;
	Candidate.AnchorType = EAnchorType::World;
	Candidate.WorldLocation = WorldLocation;
	return ShowBubble(Text, Candidate, TailDirection, DurationSeconds);
}

FGuid UTunaSweeperSpeechBubbleSubsystem::ShowForActor(
	const FText& Text,
	AActor* Actor,
	const FVector WorldOffset,
	const ETunaSweeperSpeechBubbleTailDirection TailDirection,
	const float DurationSeconds)
{
	if (Text.IsEmpty() || !IsValid(Actor) || WorldOffset.ContainsNaN())
	{
		return FGuid();
	}

	FActiveSpeechBubble Candidate;
	Candidate.AnchorType = EAnchorType::Actor;
	Candidate.Actor = Actor;
	Candidate.WorldOffset = WorldOffset;
	return ShowBubble(Text, Candidate, TailDirection, DurationSeconds);
}

FGuid UTunaSweeperSpeechBubbleSubsystem::ShowBubble(
	const FText& Text,
	const FActiveSpeechBubble& Candidate,
	const ETunaSweeperSpeechBubbleTailDirection TailDirection,
	const float DurationSeconds)
{
	if (!TunaSweeperSpeechBubbleInternal::IsValidDuration(DurationSeconds)
		|| !IsTailDirectionValid(TailDirection)
		|| !EnsureLayer())
	{
		return FGuid();
	}

	UTunaSweeperScreenSpaceSpeechBubbleWidget* Widget = CreateWidget<UTunaSweeperScreenSpaceSpeechBubbleWidget>(
		GetGameInstance(),
		UTunaSweeperScreenSpaceSpeechBubbleWidget::StaticClass());
	if (!Widget)
	{
		return FGuid();
	}
	Widget->Configure(Text, TailDirection);
	if (!LayerWidget->AddBubble(Widget))
	{
		return FGuid();
	}

	for (int32 Index = ActiveBubbles.Num() - 1; Index >= 0; --Index)
	{
		if (IsSameTarget(ActiveBubbles[Index], Candidate))
		{
			RemoveBubbleAt(Index);
		}
	}

	FActiveSpeechBubble NewBubble = Candidate;
	NewBubble.Handle = FGuid::NewGuid();
	NewBubble.Widget = Widget;
	NewBubble.DurationSeconds = FMath::Max(0.0f, DurationSeconds);
	ActiveBubbles.Add(NewBubble);

	FVector2D LogicalAnchor;
	if (ResolveLogicalAnchor(ActiveBubbles.Last(), LogicalAnchor))
	{
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		LayerWidget->SetBubbleAnchor(Widget, LogicalAnchor);
	}
	else
	{
		Widget->SetVisibility(ESlateVisibility::Hidden);
	}

	return NewBubble.Handle;
}

bool UTunaSweeperSpeechBubbleSubsystem::HideSpeechBubble(const FGuid Handle)
{
	const int32 Index = TunaSweeperSpeechBubbleInternal::FindHandleIndex(ActiveBubbles, Handle);
	if (Index != INDEX_NONE)
	{
		RemoveBubbleAt(Index);
		return true;
	}
	return false;
}

void UTunaSweeperSpeechBubbleSubsystem::HideAllSpeechBubbles()
{
	if (LayerWidget)
	{
		LayerWidget->RemoveAllBubbles();
	}
	ActiveBubbles.Reset();
}

void UTunaSweeperSpeechBubbleSubsystem::Tick(const float DeltaTime)
{
	if (ActiveBubbles.IsEmpty())
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float LifetimeDelta = World && World->IsPaused() ? 0.0f : FMath::Max(0.0f, DeltaTime);
	for (int32 Index = ActiveBubbles.Num() - 1; Index >= 0; --Index)
	{
		FActiveSpeechBubble& Bubble = ActiveBubbles[Index];
		if (Bubble.AnchorType == EAnchorType::Actor && !Bubble.Actor.IsValid())
		{
			RemoveBubbleAt(Index);
			continue;
		}

		Bubble.ElapsedSeconds += LifetimeDelta;
		if (TunaSweeperSpeechBubbleInternal::HasExpired(Bubble.DurationSeconds, Bubble.ElapsedSeconds))
		{
			RemoveBubbleAt(Index);
			continue;
		}

		UTunaSweeperScreenSpaceSpeechBubbleWidget* Widget = Bubble.Widget.Get();
		if (!IsValid(Widget))
		{
			RemoveBubbleAt(Index);
			continue;
		}

		FVector2D LogicalAnchor;
		if (ResolveLogicalAnchor(Bubble, LogicalAnchor))
		{
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			LayerWidget->SetBubbleAnchor(Widget, LogicalAnchor);
		}
		else
		{
			Widget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

bool UTunaSweeperSpeechBubbleSubsystem::IsTickable() const
{
	return !IsTemplate() && !ActiveBubbles.IsEmpty();
}

UWorld* UTunaSweeperSpeechBubbleSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

TStatId UTunaSweeperSpeechBubbleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperSpeechBubbleSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperSpeechBubbleSubsystem::EnsureLayer()
{
	if (IsValid(LayerWidget) && LayerWidget->IsInViewport())
	{
		return true;
	}

	LayerWidget = CreateWidget<UTunaSweeperSpeechBubbleLayerWidget>(
		GetGameInstance(),
		UTunaSweeperSpeechBubbleLayerWidget::StaticClass());
	if (!LayerWidget)
	{
		return false;
	}
	LayerWidget->AddToViewport(SpeechBubbleViewportZOrder);
	return LayerWidget->IsInViewport();
}

bool UTunaSweeperSpeechBubbleSubsystem::ResolveLogicalAnchor(
	const FActiveSpeechBubble& Bubble,
	FVector2D& OutLogicalAnchor) const
{
	if (!LayerWidget)
	{
		return false;
	}

	if (Bubble.AnchorType == EAnchorType::Screen)
	{
		OutLogicalAnchor = Bubble.ScreenPosition;
	}
	else
	{
		UWorld* World = GetWorld();
		APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
		if (!PlayerController)
		{
			return false;
		}

		FVector AnchorWorldLocation = Bubble.WorldLocation;
		if (Bubble.AnchorType == EAnchorType::Actor)
		{
			AActor* Actor = Bubble.Actor.Get();
			if (!IsValid(Actor))
			{
				return false;
			}
			FVector BoundsOrigin;
			FVector BoundsExtent;
			Actor->GetActorBounds(false, BoundsOrigin, BoundsExtent, false);
			AnchorWorldLocation = BoundsOrigin + FVector(0.0, 0.0, BoundsExtent.Z) + Bubble.WorldOffset;
		}

		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			AnchorWorldLocation,
			OutLogicalAnchor,
			false))
		{
			return false;
		}
	}

	if (OutLogicalAnchor.ContainsNaN())
	{
		return false;
	}
	const FVector2D LayerSize = LayerWidget->GetLogicalLayerSize();
	return LayerSize.X > 0.0f
		&& LayerSize.Y > 0.0f
		&& OutLogicalAnchor.X >= 0.0f
		&& OutLogicalAnchor.Y >= 0.0f
		&& OutLogicalAnchor.X <= LayerSize.X
		&& OutLogicalAnchor.Y <= LayerSize.Y;
}

bool UTunaSweeperSpeechBubbleSubsystem::IsSameTarget(
	const FActiveSpeechBubble& A,
	const FActiveSpeechBubble& B) const
{
	using namespace TunaSweeperSpeechBubbleInternal;
	FTargetIdentity IdentityA;
	IdentityA.Type = static_cast<ETargetType>(A.AnchorType);
	IdentityA.ScreenPosition = A.ScreenPosition;
	IdentityA.WorldLocation = A.WorldLocation;
	IdentityA.Actor = A.Actor;

	FTargetIdentity IdentityB;
	IdentityB.Type = static_cast<ETargetType>(B.AnchorType);
	IdentityB.ScreenPosition = B.ScreenPosition;
	IdentityB.WorldLocation = B.WorldLocation;
	IdentityB.Actor = B.Actor;
	return AreSameTarget(IdentityA, IdentityB);
}

void UTunaSweeperSpeechBubbleSubsystem::RemoveBubbleAt(const int32 Index)
{
	if (!ActiveBubbles.IsValidIndex(Index))
	{
		return;
	}
	if (LayerWidget)
	{
		LayerWidget->RemoveBubble(ActiveBubbles[Index].Widget.Get());
	}
	ActiveBubbles.RemoveAtSwap(Index, 1, EAllowShrinking::No);
}

void UTunaSweeperSpeechBubbleSubsystem::HandleWorldCleanup(
	UWorld* World,
	const bool bSessionEnded,
	const bool bCleanupResources)
{
	(void)bSessionEnded;
	(void)bCleanupResources;
	if (World && World->GetGameInstance() == GetGameInstance())
	{
		HideAllSpeechBubbles();
		if (LayerWidget)
		{
			LayerWidget->RemoveFromParent();
			LayerWidget = nullptr;
		}
	}
}
