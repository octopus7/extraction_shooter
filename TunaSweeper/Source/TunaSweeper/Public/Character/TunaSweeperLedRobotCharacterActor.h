#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLedRobotCharacterActor.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperLedExpressionComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLedRobotCharacterActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLedRobotCharacterActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	void ConfigureRobotDefaults(
		FName InRobotId,
		const FString& InExpressionPresetFilePath,
		FName InInitialExpressionName,
		FLinearColor InLedColor,
		FLinearColor InOffColor,
		float InLedPitch,
		float InLedRadius,
		TSoftObjectPtr<UMaterialInterface> InBodyMaterial);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Robot")
	bool SetExpressionByName(FName ExpressionName);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Robot")
	FName GetRobotId() const { return RobotId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Robot")
	UTunaSweeperLedExpressionComponent* GetExpressionComponent() const { return ExpressionComponent; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void RefreshRobotVisuals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperLedExpressionComponent> ExpressionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	FName RobotId = TEXT("BunkerRobot");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot", meta = (AllowPrivateAccess = "true"))
	FName InitialExpressionName = TEXT("Smile");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> BodyMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodyRelativeLocation = FVector(0.0f, 0.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Body", meta = (AllowPrivateAccess = "true"))
	FVector BodyScale = FVector(0.82f, 1.70f, 1.80f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true"))
	FVector FaceRelativeLocation = FVector(52.0f, 0.0f, 122.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true"))
	FRotator FaceRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Robot|Face", meta = (AllowPrivateAccess = "true"))
	FString ExpressionPresetFilePath = TEXT("Data/LedExpressionPresets.txt");
};
