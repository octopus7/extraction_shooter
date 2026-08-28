#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Research/TunaSweeperResearchTypes.h"
#include "TunaSweeperResearchWidgets.generated.h"

class UButton;
class UProgressBar;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperResearchNodeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") FName NodeId;
	void RefreshFromSubsystem();
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> NodeButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> RequirementText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> RemainingTimeText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UProgressBar> ResearchProgressBar;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> ActionText;
private:
	UFUNCTION() void HandleNodeClicked();
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperResearchTreeWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> ResearchStatusText;
private:
	void RefreshAllNodes();
	TArray<TWeakObjectPtr<UTunaSweeperResearchNodeWidget>> NodeWidgets;
	float RefreshAccumulator = 0.0f;
};
