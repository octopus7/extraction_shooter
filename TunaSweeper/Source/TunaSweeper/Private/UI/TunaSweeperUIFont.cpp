#include "UI/TunaSweeperUIFont.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"

namespace
{
	const FName RegularTypefaceName(TEXT("Regular"));
	const FName BoldTypefaceName(TEXT("Bold"));
	const TCHAR* RegularFontRelativePath = TEXT("Slate/Fonts/NanumSquareRoundR.ttf");
	const TCHAR* BoldFontRelativePath = TEXT("Slate/Fonts/NanumSquareRoundB.ttf");

	TSharedPtr<const FCompositeFont> GetDefaultCompositeFont()
	{
		static TSharedPtr<FStandaloneCompositeFont> DefaultCompositeFont;

		if (!DefaultCompositeFont.IsValid())
		{
			const FString RegularFontPath = FPaths::ProjectContentDir() / RegularFontRelativePath;
			const FString BoldFontPath = FPaths::ProjectContentDir() / BoldFontRelativePath;

			if (FPaths::FileExists(RegularFontPath) && FPaths::FileExists(BoldFontPath))
			{
				TSharedRef<FStandaloneCompositeFont> CompositeFont = MakeShared<FStandaloneCompositeFont>();
				CompositeFont->DefaultTypeface.Fonts.Emplace(
					RegularTypefaceName,
					RegularFontPath,
					EFontHinting::Default,
					EFontLoadingPolicy::LazyLoad);
				CompositeFont->DefaultTypeface.Fonts.Emplace(
					BoldTypefaceName,
					BoldFontPath,
					EFontHinting::Default,
					EFontLoadingPolicy::LazyLoad);
				DefaultCompositeFont = CompositeFont;
			}
		}

		return DefaultCompositeFont;
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

	if (const TSharedPtr<const FCompositeFont> CompositeFont = GetDefaultCompositeFont())
	{
		FontInfo.FontObject = nullptr;
		FontInfo.CompositeFont = CompositeFont;
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
