#include "UI/TunaSweeperQuestNoticeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	FSlateBrush MakeQuestNoticeBubbleBrush()
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FLinearColor(1.0f, 0.82f, 0.08f, 0.98f));
		Brush.SetImageSize(FVector2D(52.0f, 52.0f));
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Brush.OutlineSettings.Color = FSlateColor(FLinearColor(0.18f, 0.13f, 0.02f, 0.95f));
		Brush.OutlineSettings.Width = 2.0f;
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}

TSharedRef<SWidget> UTunaSweeperQuestNoticeWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildNoticeWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperQuestNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildNoticeWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
}

void UTunaSweeperQuestNoticeWidget::BuildNoticeWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* RootBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestNoticeRoot"));
	NoticeBubble = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestNoticeBubble"));
	NoticeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestNoticeText"));
	if (!RootBox || !NoticeBubble || !NoticeText)
	{
		return;
	}

	WidgetTree->RootWidget = RootBox;
	RootBox->SetWidthOverride(56.0f);
	RootBox->SetHeightOverride(56.0f);
	RootBox->SetContent(NoticeBubble);

	NoticeBubble->SetPadding(FMargin(0.0f));
	NoticeBubble->SetBrush(MakeQuestNoticeBubbleBrush());
	NoticeBubble->SetContent(NoticeText);

	NoticeText->SetText(FText::FromString(TEXT("!")));
	NoticeText->SetJustification(ETextJustify::Center);
	NoticeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.07f, 0.055f, 0.015f, 1.0f)));
	NoticeText->SetShadowOffset(FVector2D(0.0f, 1.0f));
	NoticeText->SetShadowColorAndOpacity(FLinearColor(1.0f, 0.96f, 0.62f, 0.75f));
	TunaSweeperUIFont::ApplyFont(NoticeText, 38, ETunaSweeperUIFontWeight::Bold);
}
