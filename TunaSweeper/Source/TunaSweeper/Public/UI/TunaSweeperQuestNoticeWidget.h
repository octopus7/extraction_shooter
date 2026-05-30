#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperQuestNoticeWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperQuestNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildNoticeWidget();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> NoticeBubble;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoticeText;
};
