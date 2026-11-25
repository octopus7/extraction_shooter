#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Font.h"
#include "Fonts/FontCache.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UI/TunaSweeperUIFont.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperUIFontTest, "TunaSweeper.UI.Fonts.LocalizedFaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperUIFontTest::RunTest(const FString& Parameters)
{
	const UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/F_TunaSweeperUI.F_TunaSweeperUI"));
	if (!TestNotNull(TEXT("Serialized UI font"), Font))
	{
		return false;
	}

	FInternationalization& Internationalization = FInternationalization::Get();
	const FString PreviousCulture = Internationalization.GetCurrentCulture()->GetName();
	ON_SCOPE_EXIT { Internationalization.SetCurrentCulture(PreviousCulture); };

	const struct
	{
		const TCHAR* Culture;
		TCHAR Character;
		const TCHAR* Face;
	} Cases[] = {
		{TEXT("en"), TEXT('A'), TEXT("FF_Nunito_Bold")},
		{TEXT("ru"), TEXT('\x0416'), TEXT("FF_Nunito_Bold")},
		{TEXT("ko"), TEXT('\xAC00'), TEXT("FF_Jua_Regular")},
		{TEXT("ja"), TEXT('\x3042'), TEXT("FF_ZenMaruGothic_Bold")},
		{TEXT("zh-Hans"), TEXT('\x9AA8'), TEXT("FF_ZCOOLKuaiLe_Regular")},
		{TEXT("zh-Hant"), TEXT('\x9AA8'), TEXT("FF_LXGWMarkerGothic_Regular")}
	};

	const auto Cache = FSlateApplication::Get().GetRenderer()->GetFontCache();
	for (const auto& Case : Cases)
	{
		TestTrue(Case.Culture, Internationalization.SetCurrentCulture(Case.Culture));
		// Culture changes request a flush that normally runs on the next Slate tick.
		Cache->ConditionalFlushCache();
		for (const bool bBold : {false, true})
		{
			const FSlateFontInfo FontInfo = TunaSweeperUIFont::MakeFont(nullptr, 16,
				bBold ? ETunaSweeperUIFontWeight::Bold : ETunaSweeperUIFontWeight::Regular);
			TestTrue(TEXT("Runtime uses the serialized font"), FontInfo.FontObject == Font);
			const auto& Glyph = Cache->GetCharacterList(FontInfo, 1.0f).GetCharacter(Case.Character, EFontFallback::FF_NoFallback);
			const UObject* Face = Glyph.FontData ? Glyph.FontData->GetFontFaceAsset() : nullptr;
			TestTrue(FString::Printf(TEXT("%s U+%04X selects %s (got %s, glyph %u)"),
				Case.Culture, static_cast<uint32>(Case.Character), Case.Face, *GetNameSafe(Face), Glyph.GlyphIndex),
				Glyph.GlyphIndex != 0 && Face && Face->GetName() == Case.Face);
		}
	}
	return true;
}

#endif
