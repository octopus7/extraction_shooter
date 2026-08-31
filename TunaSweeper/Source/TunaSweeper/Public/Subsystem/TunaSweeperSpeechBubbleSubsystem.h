#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "UI/TunaSweeperSpeechBubbleTypes.h"

#include "TunaSweeperSpeechBubbleSubsystem.generated.h"

class AActor;
class UTunaSweeperScreenSpaceSpeechBubbleWidget;
class UTunaSweeperSpeechBubbleLayerWidget;

/** Local-only manager for transient screen-space speech bubbles. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperSpeechBubbleSubsystem final : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	FGuid ShowAtScreen(
		const FText& Text,
		FVector2D ScreenPosition,
		ETunaSweeperSpeechBubbleTailDirection TailDirection,
		float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	FGuid ShowAtWorld(
		const FText& Text,
		FVector WorldLocation,
		ETunaSweeperSpeechBubbleTailDirection TailDirection,
		float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	FGuid ShowForActor(
		const FText& Text,
		AActor* Actor,
		FVector WorldOffset,
		ETunaSweeperSpeechBubbleTailDirection TailDirection,
		float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	bool HideSpeechBubble(FGuid Handle);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Speech Bubble")
	void HideAllSpeechBubbles();

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;

private:
	enum class EAnchorType : uint8
	{
		Screen,
		World,
		Actor,
	};

	struct FActiveSpeechBubble
	{
		FGuid Handle;
		EAnchorType AnchorType = EAnchorType::Screen;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		FVector WorldLocation = FVector::ZeroVector;
		TWeakObjectPtr<AActor> Actor;
		FVector WorldOffset = FVector::ZeroVector;
		TWeakObjectPtr<UTunaSweeperScreenSpaceSpeechBubbleWidget> Widget;
		float DurationSeconds = 0.0f;
		float ElapsedSeconds = 0.0f;
	};

	FGuid ShowBubble(
		const FText& Text,
		const FActiveSpeechBubble& Candidate,
		ETunaSweeperSpeechBubbleTailDirection TailDirection,
		float DurationSeconds);
	bool EnsureLayer();
	bool ResolveLogicalAnchor(const FActiveSpeechBubble& Bubble, FVector2D& OutLogicalAnchor) const;
	bool IsSameTarget(const FActiveSpeechBubble& A, const FActiveSpeechBubble& B) const;
	void RemoveBubbleAt(int32 Index);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperSpeechBubbleLayerWidget> LayerWidget;

	TArray<FActiveSpeechBubble> ActiveBubbles;
	FDelegateHandle WorldCleanupHandle;
};
