#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperPiggyBankActor.generated.h"

class USoundWaveProcedural;
class UStaticMeshComponent;
class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPiggyBankActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperPiggyBankActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debug Currency")
	int32 GetGrantAmount() const { return GrantAmount; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug Currency")
	void SetGrantAmount(int32 InGrantAmount);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug Currency")
	bool GrantCurrency(APawn* InstigatorPawn);

	void ConfigurePiggyBankDefaults(
		int32 InGrantAmount,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debug Currency", meta = (ClampMin = "1", UIMin = "1"))
	int32 GrantAmount = 1000;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SnoutMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CoinSlotMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftEarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightEarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrontLeftLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FrontRightLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BackLeftLegMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BackRightLegMesh;

private:
	USoundWaveProcedural* CreateMoneySound();
	void PlayMoneySound();

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> ActiveMoneySounds;
};
