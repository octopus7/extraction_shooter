#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "TunaSweeperBlockedIntakeScreenActor.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UTunaSweeperGameInstance;
class UTunaSweeperInteractableComponent;

UENUM(BlueprintType)
enum class ETunaSweeperWaterIntakeInteractionPhase : uint8
{
	None,
	Inspect,
	ClearDebris,
	RepairValve
};

/**
 * Level-placed water-intake facility whose permanent facility and screen meshes stay in
 * place while an optional debris mesh is toggled from persistent world progress. Quest progress selects
 * whether the shared interaction marker inspects, clears, repairs, or remains disabled.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperBlockedIntakeScreenActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperBlockedIntakeScreenActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Appearance")
	UStaticMeshComponent* GetFacilityMeshComponent() const { return VisualMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Appearance")
	UStaticMeshComponent* GetScreenMeshComponent() const { return ScreenMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Appearance")
	UStaticMeshComponent* GetDebrisMeshComponent() const { return DebrisMesh; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool IsScreenCleared() const { return bScreenCleared; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool IsValveRepaired() const { return bValveRepaired; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetClearDebrisRequiredItemId() const { return RequiredItemId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	int32 GetRepairValveRequiredItemId() const { return ValveRequiredItemId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	ETunaSweeperWaterIntakeInteractionPhase GetActiveInteractionPhase() const { return ActiveInteractionPhase; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Interaction")
	bool Interact(bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool CanClearScreen() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	bool ClearScreen(bool bSaveImmediately = true);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|World Progress")
	bool CanRepairValve() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	bool RepairValve(bool bSaveImmediately = true);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	/** Optional debris visual assigned later; clearing the intake toggles only this component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DebrisMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	/** Stable and unique save key. Do not change it after shipping a level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressObjectId = TEXT("demo.water_intake.blocked_screen");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Progress")
	FName ProgressInfoId = TEXT("world_progress.blocked_intake_screen");

	/** Editor-only placement preview. Runtime always follows the saved state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	bool bPreviewClearedStateInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FName InspectQuestId = TEXT("demo_q1_water_intake_check");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FName InspectObjectiveId = TEXT("inspect_water_intake");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FName InspectObjectiveEventId = TEXT("demo.water_intake.inspect");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FText InspectInteractionDisplayName = FText::FromString(TEXT("취수 시설 조사"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FName InspectInteractionDisplayNameStringKey = TEXT("ui.interaction.inspect");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Inspect")
	FVector InspectInteractionLocation = FVector(0.0f, 0.0f, 140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FName ClearDebrisQuestId = TEXT("demo_q2_clear_water_screen");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FName ClearDebrisObjectiveId = TEXT("clear_water_screen");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FName ClearDebrisObjectiveEventId = TEXT("demo.water_intake.screen_clear");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FText ClearDebrisInteractionDisplayName = FText::FromString(TEXT("이물질 제거"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FName ClearDebrisInteractionDisplayNameStringKey = TEXT("ui.interaction.clear_debris");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris")
	FVector ClearDebrisInteractionLocation = FVector(0.0f, 0.0f, 140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris|Requirement")
	bool bRequiresItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris|Requirement", meta = (EditCondition = "bRequiresItem"))
	int32 RequiredItemId = 6003;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris|Requirement", meta = (ClampMin = "1", UIMin = "1", EditCondition = "bRequiresItem"))
	int32 RequiredItemQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Clear Debris|Requirement", meta = (EditCondition = "bRequiresItem"))
	bool bConsumeRequiredItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName RepairValveQuestId = TEXT("demo_q3a_repair_valve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName RepairValveObjectiveId = TEXT("repair_valve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName RepairValveObjectiveEventId = TEXT("demo.water_intake.valve_repair");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FText RepairValveInteractionDisplayName = FText::FromString(TEXT("밸브 수리"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName RepairValveInteractionDisplayNameStringKey = TEXT("ui.interaction.repair");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FVector RepairValveInteractionLocation = FVector(0.0f, 0.0f, 140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName ValveProgressObjectId = TEXT("demo.water_intake.valve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve")
	FName ValveProgressInfoId = TEXT("world_progress.water_intake_valve");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve|Requirement")
	int32 ValveRequiredItemId = 6004;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve|Requirement", meta = (ClampMin = "1", UIMin = "1"))
	int32 ValveRequiredItemQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Repair Valve|Requirement")
	bool bConsumeValveRequiredItem = true;

private:
	void ApplySavedState();
	void ApplyVisualState();
	void RefreshPresentation();
	ETunaSweeperWaterIntakeInteractionPhase ResolveActiveInteractionPhase() const;
	bool IsQuestObjectiveActive(FName QuestId, FName ObjectiveId) const;
	FText ResolveLocalizedText(FName StringKey, const FText& FallbackText) const;
	UTexture2D* LoadItemIconTexture(int32 ItemId) const;
	FName GetEffectiveProgressObjectId() const;
	FName GetEffectiveValveProgressObjectId() const;
	FTunaSweeperWorldProgressSaveData GetOrCreateProgressState(FName ObjectId, FName InfoId) const;
	UTunaSweeperGameInstance* GetTunaGameInstance() const;

	UPROPERTY(Transient)
	bool bScreenCleared = false;

	UPROPERTY(Transient)
	bool bValveRepaired = false;

	UPROPERTY(Transient)
	ETunaSweeperWaterIntakeInteractionPhase ActiveInteractionPhase = ETunaSweeperWaterIntakeInteractionPhase::None;
};
