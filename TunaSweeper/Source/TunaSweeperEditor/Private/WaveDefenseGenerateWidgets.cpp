#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprintFactory.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "UI/Terra/WaveDefensePanelGPT61Terra.h"
#include "UI/Terra/WaveDefenseHUDGPT61Terra.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseGenerateWidgets, "TunaSweeper.WaveDefense.GenerateWidgets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseGenerateWidgets::RunTest(const FString& Parameters)
{
    auto SaveWidget = [this](const TCHAR* Name, UClass* Parent, bool bPanel)
    {
        const FString Path = FString(TEXT("/Game/MainRaid/WaveDefense/UI/")) + Name;
        UPackage* Package = CreatePackage(*Path);
        UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass = Parent;
        UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(), Package, FName(Name), RF_Public | RF_Standalone, nullptr, GWarn));
        if (!BP || !BP->WidgetTree) return false;
        UWidgetTree* Tree = BP->WidgetTree;
        UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas")); Tree->RootWidget = Root;
        UVerticalBox* Layout = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Layout"));
        UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Layout);
        CanvasSlot->SetPosition(bPanel ? FVector2D(300, 110) : FVector2D(360, 20));
        CanvasSlot->SetSize(bPanel ? FVector2D(1320, 850) : FVector2D(1200, 125));
        auto AddText = [&](const TCHAR* WidgetName, int32 FontSize)
        {
            auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(WidgetName));
            Label->bIsVariable = true; FSlateFontInfo Font = Label->GetFont(); Font.Size = FontSize; Label->SetFont(Font); Label->SetAutoWrapText(true); Layout->AddChildToVerticalBox(Label);
        };
        if (bPanel) { AddText(TEXT("TitleText"), 34); AddText(TEXT("IntroText"), 21); }
        AddText(TEXT("StatusText"), 24); AddText(TEXT("DetailText"), 20);
        if (bPanel)
        {
            AddText(TEXT("MessageText"), 20);
            auto AddRow = [&](std::initializer_list<const TCHAR*> Names)
            {
                auto* Row = Tree->ConstructWidget<UHorizontalBox>(); Layout->AddChildToVerticalBox(Row);
                for (const TCHAR* ButtonName : Names)
                {
                    UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), FName(ButtonName));
                    Button->bIsVariable = true; UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(); FSlateFontInfo Font = Label->GetFont(); Font.Size = 18; Label->SetFont(Font);
                    Button->AddChild(Label); Row->AddChildToHorizontalBox(Button);
                }
            };
            AddRow({TEXT("StartButton"),TEXT("ReadyButton"),TEXT("AbortButton"),TEXT("ClaimButton"),TEXT("CloseButton")});
            AddRow({TEXT("NorthBarrierButton"),TEXT("WestBarrierButton"),TEXT("EastBarrierButton")});
            AddRow({TEXT("NorthTurretButton"),TEXT("WestTurretButton"),TEXT("EastTurretButton")});
            AddRow({TEXT("BarrierButton"),TEXT("TurretButton"),TEXT("RepairCoreButton"),TEXT("RepairFacilityButton"),TEXT("ReloadButton")});
        }
        FAssetRegistryModule::AssetCreated(BP); BP->MarkPackageDirty(); FKismetEditorUtilities::CompileBlueprint(BP);
        if (BP->Status == BS_Error || !BP->GeneratedClass) return false;
        const FString Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, BP, *Filename, Args);
    };
    TestTrue(TEXT("Panel Blueprint saved"), SaveWidget(TEXT("WBP_WaveDefensePanel_GPT61Terra"), UWaveDefensePanelGPT61Terra::StaticClass(), true));
    TestTrue(TEXT("HUD Blueprint saved"), SaveWidget(TEXT("WBP_WaveDefenseHUD_GPT61Terra"), UWaveDefenseHUDGPT61Terra::StaticClass(), false));
    return true;
}
#endif
