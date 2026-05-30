#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperPiggyBankActor.generated.h"

class USoundWaveProcedural;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperSpeechBubbleWidget;
class UWidgetComponent;

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

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Piggy Bank")
	FName GetPiggyBankId() const { return PiggyBankId; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Piggy Bank")
	void SetPiggyBankId(FName InPiggyBankId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Piggy Bank")
	int32 GetStoredAncientCoinValue() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Piggy Bank")
	bool DepositAncientCurrencyItems(APawn* InstigatorPawn);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Piggy Bank")
	bool ShowWithdrawNotImplemented(APawn* InstigatorPawn);

	void ConfigurePiggyBankDefaults(
		int32 InGrantAmount,
		const FText& InInteractionDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		FName InPiggyBankId = NAME_None);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Debug Currency", meta = (ClampMin = "1", UIMin = "1"))
	int32 GrantAmount = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank")
	FName PiggyBankId = FName(TEXT("debug.piggy_bank.default"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank", meta = (ClampMin = "1", UIMin = "1"))
	int32 AncientCoinItemId = 7002;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank", meta = (ClampMin = "1", UIMin = "1"))
	int32 AncientBanknoteItemId = 7003;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank", meta = (ClampMin = "1", UIMin = "1"))
	int32 AncientBanknoteCoinValue = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> DepositInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> WithdrawInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> SpeechBubbleWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Piggy Bank|UI")
	TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> SpeechBubbleWidgetClass;

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
	void ShowSpeechBubble(const FText& InText, float DisplaySeconds = 1.6f);
	void HideSpeechBubble();
	void EnsureSpeechBubbleWidgetClass();
	void StartWithdrawDialogue(APawn* InstigatorPawn);

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> ActiveMoneySounds;

	FTimerHandle SpeechBubbleTimerHandle;
};
