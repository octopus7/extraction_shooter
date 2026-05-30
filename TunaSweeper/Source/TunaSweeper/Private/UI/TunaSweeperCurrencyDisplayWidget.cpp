#include "UI/TunaSweeperCurrencyDisplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperUIFont.h"
#include "Widgets/SWidget.h"

namespace TunaSweeperCurrencyDisplay
{
	const TCHAR* CurrencyCoinIconPath =
		TEXT("/Game/UI/Icons/T_UIIcon_CurrencyCoin_White.T_UIIcon_CurrencyCoin_White");
	constexpr float CoinIconSize = 18.0f;
}

UTexture2D* UTunaSweeperCurrencyDisplayWidget::LoadCurrencyCoinIconTexture()
{
	static TWeakObjectPtr<UTexture2D> CachedTexture;
	if (CachedTexture.IsValid())
	{
		return CachedTexture.Get();
	}

	UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperCurrencyDisplay::CurrencyCoinIconPath);
	CachedTexture = LoadedTexture;
	return LoadedTexture;
}

void UTunaSweeperCurrencyDisplayWidget::RefreshCurrencyBalance()
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	SetCurrencyAmount(QuestSubsystem ? QuestSubsystem->GetCoinBalance() : 0);
}

void UTunaSweeperCurrencyDisplayWidget::SetCurrencyAmount(int32 InCurrencyAmount)
{
	CurrencyAmount = FMath::Max(0, InCurrencyAmount);
	ApplyCurrencyPresentation();
}

TSharedRef<SWidget> UTunaSweeperCurrencyDisplayWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildNativeWidgetTree();
	}

	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	CacheNamedWidgets();
	ApplyCurrencyPresentation();
	return RebuiltWidget;
}

void UTunaSweeperCurrencyDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	CacheNamedWidgets();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
			QuestSubsystem->OnQuestProgressChanged.AddUObject(
				this,
				&UTunaSweeperCurrencyDisplayWidget::HandleQuestProgressChanged);
		}
	}

	RefreshCurrencyBalance();
}

void UTunaSweeperCurrencyDisplayWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UTunaSweeperCurrencyDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CacheNamedWidgets();
	ApplyCurrencyPresentation();
}

void UTunaSweeperCurrencyDisplayWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootBox"));
	USizeBox* CoinSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CoinSizeBox"));
	CoinImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CoinImage"));
	BalanceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BalanceText"));
	if (!RootBox || !CoinSizeBox || !CoinImage || !BalanceText)
	{
		return;
	}

	WidgetTree->RootWidget = RootBox;
	RootBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	CoinSizeBox->SetWidthOverride(TunaSweeperCurrencyDisplay::CoinIconSize);
	CoinSizeBox->SetHeightOverride(TunaSweeperCurrencyDisplay::CoinIconSize);
	CoinSizeBox->SetContent(CoinImage);
	UHorizontalBoxSlot* CoinSlot = RootBox->AddChildToHorizontalBox(CoinSizeBox);
	if (CoinSlot)
	{
		CoinSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CoinSlot->SetVerticalAlignment(VAlign_Center);
	}

	TunaSweeperUIFont::ApplyFont(BalanceText, 16, ETunaSweeperUIFontWeight::Bold);
	BalanceText->SetAutoWrapText(false);
	BalanceText->SetJustification(ETextJustify::Left);
	UHorizontalBoxSlot* BalanceSlot = RootBox->AddChildToHorizontalBox(BalanceText);
	if (BalanceSlot)
	{
		BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		BalanceSlot->SetVerticalAlignment(VAlign_Center);
		BalanceSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
	}
}

void UTunaSweeperCurrencyDisplayWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBox)
	{
		RootBox = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("RootBox"))));
	}
	if (!CoinImage)
	{
		CoinImage = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("CoinImage"))));
	}
	if (!BalanceText)
	{
		BalanceText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("BalanceText"))));
	}
}

void UTunaSweeperCurrencyDisplayWidget::ApplyCurrencyPresentation()
{
	if (CoinImage)
	{
		CoinImage->SetBrushFromTexture(LoadCurrencyCoinIconTexture(), true);
		CoinImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		CoinImage->SetOpacity(1.0f);
	}

	if (BalanceText)
	{
		BalanceText->SetText(FText::AsNumber(CurrencyAmount));
		BalanceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TunaSweeperUIFont::ApplyFont(BalanceText, 16, ETunaSweeperUIFontWeight::Bold);
	}
}

void UTunaSweeperCurrencyDisplayWidget::HandleQuestProgressChanged()
{
	RefreshCurrencyBalance();
}
