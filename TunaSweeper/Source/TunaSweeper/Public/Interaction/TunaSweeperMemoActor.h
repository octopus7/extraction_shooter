#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMemoActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMemoActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMemoActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
	int32 GetMemoId() const { return MemoId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Interaction")
	UTunaSweeperInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	void ConfigureMemoDefaults(
		int32 InMemoId,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		TSoftObjectPtr<UStaticMesh> InVisualMesh,
		TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
		FVector InVisualScale,
		FVector InVisualRelativeLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memo")
	int32 MemoId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memo|Visual")
	TSoftObjectPtr<UStaticMesh> VisualMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memo|Visual")
	TSoftObjectPtr<UMaterialInterface> VisualMaterialAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memo|Visual")
	FVector VisualScale = FVector(0.85f, 0.55f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Memo|Visual")
	FVector VisualRelativeLocation = FVector::ZeroVector;

private:
	void ApplyVisualDefaults();
};
