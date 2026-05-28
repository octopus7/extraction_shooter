#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperLootContainerActor.generated.h"

class UTunaSweeperInteractableComponent;
class UTunaSweeperGameInstance;
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class ETunaSweeperLootContainerLidEasing : uint8
{
	Linear UMETA(DisplayName = "Linear"),
	EaseIn UMETA(DisplayName = "Ease In"),
	EaseOut UMETA(DisplayName = "Ease Out"),
	EaseInOut UMETA(DisplayName = "Ease In Out")
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLootContainerActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLootContainerActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetContainerDataIds(int32 InContainerDefinitionId, int32 InContentsId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContainerDefinitionId() const { return ContainerDefinitionId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	int32 GetContentsId() const { return ContentsId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	bool BuildContainerInstance(FTunaSweeperLootContainerInstance& OutInstance) const;

	bool OpenRuntimeContainer(UTunaSweeperGameInstance* TunaGameInstance, FTunaSweeperLootContainerInstance& OutInstance);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	UStaticMeshComponent* GetBodyMeshComponent() const { return VisualMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	USceneComponent* GetLidPivotComponent() const { return LidPivot; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	UStaticMeshComponent* GetLidMeshComponent() const { return LidMesh; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void PlayOpenAnimation();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void PlayCloseAnimation();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Loot Container")
	void SetLidOpen(bool bInOpen, bool bInstant = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Loot Container")
	bool IsLidOpen() const { return bLidOpen; }

	void ConfigureLootContainerDefaults(int32 InContainerDefinitionId, int32 InContentsId);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LidPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LidMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	int32 ContainerDefinitionId = 7001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	int32 ContentsId = 8001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container")
	ETunaSweeperItemTextLanguage DisplayLanguage = ETunaSweeperItemTextLanguage::English;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Meshes", meta = (DisplayName = "Body Mesh"))
	TObjectPtr<UStaticMesh> BodyMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Meshes", meta = (DisplayName = "Lid Mesh"))
	TObjectPtr<UStaticMesh> LidMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation")
	FRotator ClosedLidRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation")
	FRotator OpenLidRelativeRotation = FRotator(0.0f, 0.0f, -105.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OpenAnimationDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation")
	ETunaSweeperLootContainerLidEasing OpenEasing = ETunaSweeperLootContainerLidEasing::EaseOut;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation", meta = (DisplayName = "Use Separate Close Timing"))
	bool bUseSeparateCloseTiming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation", meta = (EditCondition = "bUseSeparateCloseTiming", ClampMin = "0.0", UIMin = "0.0"))
	float CloseAnimationDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Container|Lid Animation", meta = (EditCondition = "bUseSeparateCloseTiming"))
	ETunaSweeperLootContainerLidEasing CloseEasing = ETunaSweeperLootContainerLidEasing::EaseIn;

private:
	void RefreshContainerPresentation();
	void ResetRuntimeContainerState();
	void ApplyOpenedMarkerState();
	void CaptureRuntimeContentsFromActiveContainer();
	void HandleActiveLootContainerUiClosed();
	bool IsRuntimeContainerStateValid(const UTunaSweeperGameInstance* TunaGameInstance) const;
	FTunaSweeperLootContainerInstance BuildRuntimeContainerInstance() const;
	UTunaSweeperItemDataSubsystem* GetItemDataSubsystem() const;
	void StartLidAnimation(bool bOpenTarget);
	void ApplyLidRotation(const FRotator& Rotation);
	float EvaluateLidAnimationAlpha(float Alpha, ETunaSweeperLootContainerLidEasing Easing) const;

	UPROPERTY(Transient)
	bool bLidOpen = false;

	UPROPERTY(Transient)
	bool bAnimatingLid = false;

	UPROPERTY(Transient)
	bool bLidAnimationTargetOpen = false;

	UPROPERTY(Transient)
	float LidAnimationElapsed = 0.0f;

	UPROPERTY(Transient)
	float LidAnimationDuration = 0.0f;

	UPROPERTY(Transient)
	FRotator LidAnimationStartRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	FRotator LidAnimationTargetRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	ETunaSweeperLootContainerLidEasing ActiveLidEasing = ETunaSweeperLootContainerLidEasing::Linear;

	UPROPERTY(Transient)
	TWeakObjectPtr<UTunaSweeperGameInstance> RuntimeGameInstance;

	UPROPERTY(Transient)
	TArray<FTunaSweeperInventorySlot> RuntimeSlots;

	UPROPERTY(Transient)
	FText RuntimeDisplayName;

	UPROPERTY(Transient)
	int32 RuntimeCapacity = 0;

	UPROPERTY(Transient)
	int32 RuntimeContainerDefinitionId = INDEX_NONE;

	UPROPERTY(Transient)
	int32 RuntimeContentsId = INDEX_NONE;

	UPROPERTY(Transient)
	bool bHasRuntimeContainerState = false;
};
