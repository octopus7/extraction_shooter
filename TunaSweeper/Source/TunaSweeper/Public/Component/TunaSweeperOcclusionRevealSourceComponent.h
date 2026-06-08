#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "TunaSweeperOcclusionRevealSourceComponent.generated.h"

class APlayerController;
class UMaterialParameterCollection;

UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperOcclusionRevealSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperOcclusionRevealSourceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void ForceRefreshRevealParameters();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	TSoftObjectPtr<UMaterialParameterCollection> RevealParameterCollection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CharacterRevealRadiusCm = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CursorRevealRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float RevealFeatherCm = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RevealStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bUseCharacterWeaponAimPlaneForCursor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bUpdateOnlyForLocalPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bUpdateOnlyInGameWorld = true;

private:
	APlayerController* ResolveLocalPlayerController() const;
	UMaterialParameterCollection* ResolveRevealParameterCollection();
	float ResolveCursorPlaneZ() const;
	bool ResolveCursorWorldPoint(APlayerController* PlayerController, FVector& OutCursorWorldPoint) const;
	void PushRevealParameters(APlayerController* PlayerController);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> CachedRevealParameterCollection;

	bool bLoggedMissingCollection = false;
};
