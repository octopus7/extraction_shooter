#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperSelfDestructInteractableActor.generated.h"

class APawn;
class USceneComponent;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperSpeechBubbleWidget;
class UWidgetComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSelfDestructInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSelfDestructInteractableActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Self Destruct")
	bool StartSelfDestruct(APawn* InstigatorPawn);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigureSelfDestructDefaults(
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> InSpeechBubbleWidgetClass);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> SpeechBubbleWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct", meta = (ClampMin = "1", UIMin = "1"))
	int32 CountdownStartNumber = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float CountdownStepSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BoomDisplaySeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExplosionDamage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Self Destruct|UI")
	TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> SpeechBubbleWidgetClass;

private:
	void AdvanceCountdown();
	void Explode();
	void ApplyExplosionDamage();
	void SetSpeechBubbleText(const FText& InText);
	void EnsureSpeechBubbleWidgetClass();

	FTimerHandle CountdownTimerHandle;
	FTimerHandle BoomTimerHandle;
	TWeakObjectPtr<APawn> CountdownInstigator;
	int32 CurrentCountdownValue = 0;
	bool bCountdownActive = false;
	bool bExploded = false;
};
