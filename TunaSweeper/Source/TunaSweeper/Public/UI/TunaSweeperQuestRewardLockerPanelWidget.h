#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperQuestRewardLockerPanelWidget.generated.h"

class UTextBlock;
class UUniformGridPanel;
class UTexture2D;

DECLARE_DELEGATE_ThreeParams(FTunaSweeperReceiveQuestReward, int32, int32, int32);

UCLASS()
class TUNASWEEPER_API UTunaSweeperQuestRewardCardWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Configure(int32 InIndex, int32 InItemId, int32 InQuantity, UTexture2D* InIcon,
		const FText& InName, FTunaSweeperReceiveQuestReward InReceive);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
private:
	UFUNCTION() void HandleReceive();
	int32 Index = INDEX_NONE;
	int32 ItemId = INDEX_NONE;
	int32 Quantity = 0;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> Icon;
	FText ItemName;
	FTunaSweeperReceiveQuestReward Receive;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperQuestRewardLockerPanelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void OpenQuestRewardLocker();
	void RefreshRewards();
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void BuildPanel();
	void ReceiveReward(int32 Index, int32 ItemId, int32 Quantity);
	UFUNCTION() void HandleClose();
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EmptyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CloseText;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> RewardGrid;
};
