#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseStyleWidgetsOnce, "TunaSweeper.WaveDefense.StyleWidgetsOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseStyleWidgetsOnce::RunTest(const FString& Parameters)
{
    auto Style=[this](const TCHAR* Name,bool bPanel)
    {
        const FString Path=FString(TEXT("/Game/MainRaid/WaveDefense/UI/"))+Name;
        auto* BP=LoadObject<UWidgetBlueprint>(nullptr,*(Path+TEXT(".")+Name));
        if (!BP || !BP->WidgetTree) return false;
        auto* Root=Cast<UCanvasPanel>(BP->WidgetTree->RootWidget);
        auto* Layout=Cast<UVerticalBox>(BP->WidgetTree->FindWidget(TEXT("Layout")));
        if (!Root || !Layout) return false;
        if (UWidget* Existing = BP->WidgetTree->FindWidget(TEXT("ReadableBackground")))
        {
            if (bPanel && !BP->WidgetTree->FindWidget(TEXT("FacilityActionRow")))
            {
                BP->Modify(); Layout->Modify();
                auto* Row = BP->WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FacilityActionRow"));
                Layout->AddChildToVerticalBox(Row);
                for (const TCHAR* ButtonName : {TEXT("RepairFacilityButton"), TEXT("ReloadButton")})
                {
                    UWidget* Button = BP->WidgetTree->FindWidget(ButtonName);
                    if (!Button) return false;
                    Button->RemoveFromParent(); Row->AddChildToHorizontalBox(Button);
                }
                BP->MarkPackageDirty();
            }
            if (!BP->WidgetVariableNameToGuidMap.Contains(Existing->GetFName())) BP->WidgetVariableNameToGuidMap.Add(Existing->GetFName(), FGuid::NewGuid());
            FKismetEditorUtilities::CompileBlueprint(BP);
            FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
            const FString Filename=FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension());
            return BP->Status!=BS_Error && UPackage::SavePackage(BP->GetOutermost(),BP,*Filename,Args);
        }
        BP->Modify(); Root->Modify(); Layout->Modify();
        Root->RemoveChild(Layout);
        auto* Background=BP->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("ReadableBackground"));
        Background->bIsVariable=true;
        BP->WidgetVariableNameToGuidMap.Add(Background->GetFName(), FGuid::NewGuid());
        Background->SetBrushColor(FLinearColor(0.015f,0.025f,0.035f,0.94f)); Background->SetPadding(FMargin(20.0f));
        Background->SetContent(Layout);
        auto* Canvas=Root->AddChildToCanvas(Background);
        Canvas->SetPosition(bPanel?FVector2D(280,90):FVector2D(340,10));
        Canvas->SetSize(bPanel?FVector2D(1360,850):FVector2D(1240,145));
        BP->MarkPackageDirty(); FKismetEditorUtilities::CompileBlueprint(BP);
        if (BP->Status==BS_Error || !BP->GeneratedClass) return false;
        const FString Filename=FPackageName::LongPackageNameToFilename(Path,FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
        return UPackage::SavePackage(BP->GetOutermost(),BP,*Filename,Args);
    };
    TestTrue(TEXT("Panel background saved"),Style(TEXT("WBP_WaveDefensePanel_GPT61Terra"),true));
    TestTrue(TEXT("HUD background saved"),Style(TEXT("WBP_WaveDefenseHUD_GPT61Terra"),false));
    return true;
}
#endif
