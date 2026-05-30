#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Debuff/TunaSweeperDebuffTypes.h"
#include "TunaSweeperHudDebuffBarWidget.generated.h"

class UHorizontalBox;
class UImage;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudDebuffBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD|Debuff")
	void SetActiveDebuffs(const TArray<FTunaSweeperActiveDebuffState>& InActiveDebuffs);

protected:
	virtual void NativeConstruct() override;

private:
	void EnsureDebuffRow();
	void RebuildEntries();
	void RefreshEntryTexts();
	bool DoesEntryLayoutMatch() const;
	FText ResolveDebuffNameText(const FTunaSweeperActiveDebuffState& DebuffState) const;
	FText ResolveDebuffTimeText(const FTunaSweeperActiveDebuffState& DebuffState) const;
	void ApplyDebuffIcon(UImage* IconImage, const FTunaSweeperActiveDebuffState& DebuffState) const;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> DebuffRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DebuffNameTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> DebuffTimeTexts;

	TArray<FName> EntryDebuffIds;
	TArray<FTunaSweeperActiveDebuffState> ActiveDebuffs;
};
