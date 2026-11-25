#include "UI/TunaSweeperUIFont.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Font.h"

namespace
{
	const FName RegularTypefaceName(TEXT("Regular"));
	const FName BoldTypefaceName(TEXT("Bold"));

	UFont* GetDefaultUIFont()
	{
		static TWeakObjectPtr<UFont> DefaultFont;
		if (!DefaultFont.IsValid())
		{
			DefaultFont = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/F_TunaSweeperUI.F_TunaSweeperUI"));
		}
		return DefaultFont.Get();
	}

	FName ResolveTypefaceName(const FSlateFontInfo& FontInfo, ETunaSweeperUIFontWeight Weight)
	{
		if (Weight == ETunaSweeperUIFontWeight::Bold)
		{
			return BoldTypefaceName;
		}

		if (Weight == ETunaSweeperUIFontWeight::Preserve)
		{
			const FString ExistingTypefaceName = FontInfo.TypefaceFontName.ToString();
			if (FontInfo.TypefaceFontName == BoldTypefaceName || ExistingTypefaceName.Contains(TEXT("Bold")))
			{
				return BoldTypefaceName;
			}
		}

		return RegularTypefaceName;
	}
}

FSlateFontInfo TunaSweeperUIFont::MakeFont(
	const UTextBlock* TextBlock,
	float Size,
	ETunaSweeperUIFontWeight Weight)
{
	FSlateFontInfo FontInfo = TextBlock ? TextBlock->GetFont() : FSlateFontInfo();

	if (UFont* DefaultFont = GetDefaultUIFont())
	{
		FontInfo.FontObject = DefaultFont;
		FontInfo.CompositeFont.Reset();
		FontInfo.TypefaceFontName = ResolveTypefaceName(FontInfo, Weight);
	}

	FontInfo.Size = Size;
	return FontInfo;
}

void TunaSweeperUIFont::ApplyFont(UTextBlock* TextBlock, float Size, ETunaSweeperUIFontWeight Weight)
{
	if (!TextBlock)
	{
		return;
	}

	TextBlock->SetFont(MakeFont(TextBlock, Size, Weight));
}

void TunaSweeperUIFont::ApplyFontToWidgetTree(UUserWidget* UserWidget)
{
	if (!UserWidget || !UserWidget->WidgetTree)
	{
		return;
	}

	TArray<UWidget*> Widgets;
	UserWidget->WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			ApplyFont(TextBlock, TextBlock->GetFont().Size);
		}
	}
}
