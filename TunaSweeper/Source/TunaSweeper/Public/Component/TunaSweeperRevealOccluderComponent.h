#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TunaSweeperRevealOccluderComponent.generated.h"

class UPrimitiveComponent;
struct FPropertyChangedEvent;

UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperRevealOccluderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperRevealOccluderComponent();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void ApplyRevealOccluderSettings();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void ConfigureRevealOccluderSettings(
		float InRevealIntensity,
		float InCharacterRadiusScale,
		float InCursorRadiusScale,
		float InPatternScale);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void RegisterRevealMesh(UPrimitiveComponent* PrimitiveComponent);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void UnregisterRevealMesh(UPrimitiveComponent* PrimitiveComponent);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Occlusion Reveal")
	void ClearRevealMeshes();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	TArray<TObjectPtr<UPrimitiveComponent>> RevealMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal")
	bool bAutoCollectOwnerPrimitiveComponents = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float RevealIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CharacterRadiusScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CursorRadiusScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Occlusion Reveal", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float PatternScale = 1.0f;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void CollectRevealMeshes(TArray<UPrimitiveComponent*>& OutMeshes) const;
	void ApplyRevealDataToPrimitive(UPrimitiveComponent* PrimitiveComponent) const;
};
