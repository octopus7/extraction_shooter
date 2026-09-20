#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperTitleStudioActor.generated.h"

class ATunaSweeperTitlePresentationActor;
class USceneComponent;
class UStaticMeshComponent;
class USkyLightComponent;
class UPointLightComponent;
class UCameraComponent;

// Independently editable title set, lighting, and camera-aligned matte backdrop.
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTitleStudioActor : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperTitleStudioActor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	static FVector2D CalculateBackdropSize(const UCameraComponent* Camera, FIntPoint ViewportSize,
		EAspectRatioAxisConstraint AxisConstraint);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "TunaSweeper|Title|Backdrop")
	TObjectPtr<ATunaSweeperTitlePresentationActor> PresentationActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Studio")
	bool bShowStudioGeometry = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> MatteBackdrop;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> BackWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> LeftWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> RightWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USkyLightComponent> AmbientLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UPointLightComponent> CharacterKeyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UPointLightComponent> EmptyWallLight;


private:
	void UpdateBackdrop();
};
