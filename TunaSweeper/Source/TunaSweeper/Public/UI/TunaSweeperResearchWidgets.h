#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Research/TunaSweeperResearchTypes.h"
#include "TunaSweeperResearchWidgets.generated.h"

class UButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperResearchNodeWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") FName NodeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research|Icon", meta = (ClampMin = "0.0", ClampMax = "1.0")) float LockedIconOpacity = 0.35f;
	void RefreshFromSubsystem();
protected:
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UButton> NodeButton;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UImage> IconImage;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> RequirementText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> RemainingTimeText;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UProgressBar> ResearchProgressBar;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget)) TObjectPtr<UTextBlock> ActionText;
private:
	void ClearIcon();
	UFUNCTION() void HandleNodeClicked();
	UPROPERTY(Transient) TObjectPtr<UTexture2D> CachedIconTexture;
	FSoftObjectPath CachedIconPath;
	bool bIconLoadAttempted = false;
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
