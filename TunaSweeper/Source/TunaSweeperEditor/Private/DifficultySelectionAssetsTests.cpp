#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/DifficultySelectionWidget.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Settings/TunaSweeperLanguage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDifficultySelectionAssetsTest,
    "TunaSweeper.UI.Difficulty.AuthoredAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDifficultySelectionAssetsTest::RunTest(const FString& Parameters)
{
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Difficulty/WBP_DifficultySelection.WBP_DifficultySelection"));
    if (!TestNotNull(TEXT("Authored difficulty blueprint"), BP)) return false;
    TestTrue(TEXT("Purpose-specific widget class"), BP->ParentClass == UDifficultySelectionWidget::StaticClass());
    auto* Defaults = BP->GeneratedClass ? Cast<UDifficultySelectionWidget>(BP->GeneratedClass->GetDefaultObject()) : nullptr;
    if (!TestNotNull(TEXT("Compiled defaults"), Defaults)) return false;
    TestEqual(TEXT("Six labels in each of six cultures"), Defaults->LabelTextures.Num(), 36);
    const TCHAR* Keys[] = {TEXT("ui.difficulty.select_title"), TEXT("difficulty.stage.1.title"), TEXT("difficulty.stage.2.title"), TEXT("difficulty.stage.3.title"), TEXT("ui.common.back"), TEXT("ui.difficulty.start")};
    for (const auto Language : TunaSweeperLanguage::SupportedLanguages)
        for (const auto* Key : Keys)
        {
            const FName Name(FString(TunaSweeperLanguage::ToLanguageCode(Language)) + TEXT(".") + Key);
            const auto* Texture = Defaults->LabelTextures.Find(Name);
            TestTrue(Name.ToString(), Texture && *Texture && (*Texture)->GetSizeX() > 0);
        }
    for (const TCHAR* Name : {TEXT("DifficultyFarmingButton"), TEXT("DifficultyNormalButton"), TEXT("DifficultyHardButton"), TEXT("DifficultyStartButton"), TEXT("DifficultyBackButton")})
        TestNotNull(Name, Cast<UButton>(BP->WidgetTree->FindWidget(Name)));
    for (int32 I=0; I<3; ++I)
    {
        TestNotNull(TEXT("Description exists"), BP->WidgetTree->FindWidget(*FString::Printf(TEXT("Description%d"),I)));
        TestNotNull(TEXT("Animated card exists"), BP->WidgetTree->FindWidget(*FString::Printf(TEXT("Card%d"),I)));
    }
    auto* Intro = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu"));
    TestTrue(TEXT("Intro embeds saved difficulty widget"), Intro && Cast<UDifficultySelectionWidget>(Intro->WidgetTree->FindWidget(TEXT("DifficultySelectPanel"))));
    if(Intro) if(auto* Child=Cast<UDifficultySelectionWidget>(Intro->WidgetTree->FindWidget(TEXT("DifficultySelectPanel"))))
    {
        TestEqual(TEXT("Nested template retains language assets"),Child->LabelTextures.Num(),36);
        TestNotNull(TEXT("Nested template retains mask material"),Child->LabelMaterial.Get());
    }
    return true;
}
#endif
