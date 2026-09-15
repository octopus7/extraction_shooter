#if WITH_DEV_AUTOMATION_TESTS
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperFoodWarehouseActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Scenario/TunaSweeperDemoEndingActor.h"
#include "UI/TunaSweeperDemoFarewellWidget.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/Csv/CsvParser.h"

namespace TunaSweeperDemoEndingTests
{
    bool LoadCsvKeys(const FString& Path, TSet<FString>& OutKeys)
    {
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *Path)) return false;
        const FCsvParser Parser(Content);
        const FCsvParser::FRows& Rows = Parser.GetRows();
        if (Rows.Num() < 2) return false;
        for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
        {
            if (Rows[RowIndex].Num() > 0) OutKeys.Add(FString(Rows[RowIndex][0]).TrimStartAndEnd());
        }
        return !OutKeys.IsEmpty();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDemoEndingAssetsTest,"TunaSweeper.DemoEnding.AssetsAndInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaDemoEndingAssetsTest::RunTest(const FString&)
{
    const FString ScenarioDefinitionsPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/ScenarioDefinitions.json"));
    FString ScenarioDefinitionsJson;
    TestTrue(TEXT("Demo scenario definitions load"), FFileHelper::LoadFileToString(ScenarioDefinitionsJson, *ScenarioDefinitionsPath));
    TSharedPtr<FJsonObject> ScenarioRoot;
    TestTrue(TEXT("Demo scenario definitions parse"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ScenarioDefinitionsJson), ScenarioRoot));
    TSharedPtr<FJsonObject> EndingScenario;
    const TArray<TSharedPtr<FJsonValue>>* ScenarioValues = nullptr;
    if (ScenarioRoot.IsValid() && ScenarioRoot->TryGetArrayField(TEXT("scenarios"), ScenarioValues) && ScenarioValues)
    {
        for (const TSharedPtr<FJsonValue>& Value : *ScenarioValues)
        {
            const TSharedPtr<FJsonObject> Scenario = Value.IsValid() ? Value->AsObject() : nullptr;
            FString ScenarioId;
            if (Scenario.IsValid() && Scenario->TryGetStringField(TEXT("scenario_id"), ScenarioId) &&
                ScenarioId == TEXT("scenario.demo.ending.dinner"))
            {
                EndingScenario = Scenario;
                break;
            }
        }
    }
    TestNotNull(TEXT("Dinner ending scenario is authored in ScenarioDefinitions"), EndingScenario.Get());
    if (EndingScenario.IsValid())
    {
        const TArray<TSharedPtr<FJsonValue>>* Triggers = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* QuestConditions = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
        FString CompletionFlag;
        bool bOneShot = false;
        TestTrue(TEXT("Dinner ending has the direct trigger"), EndingScenario->TryGetArrayField(TEXT("triggers"), Triggers) &&
            Triggers && Triggers->ContainsByPredicate([](const TSharedPtr<FJsonValue>& Value)
            {
                return Value.IsValid() && Value->AsString() == TEXT("demo.ending.dinner");
            }));
        TestTrue(TEXT("Dinner ending requires the final quest reward"), EndingScenario->TryGetArrayField(TEXT("required_quest_states"), QuestConditions));
        TestTrue(TEXT("Dinner ending has eight scenario lines"), EndingScenario->TryGetArrayField(TEXT("lines"), Lines) && Lines && Lines->Num() == 8);
        TestTrue(TEXT("Dinner ending has a completion flag"), EndingScenario->TryGetStringField(TEXT("completion_flag"), CompletionFlag));
        TestEqual(TEXT("Dinner ending completion flag"), CompletionFlag, FString(TEXT("dialogue.demo.ending.dinner")));
        TestTrue(TEXT("Dinner ending is one-shot"), EndingScenario->TryGetBoolField(TEXT("one_shot"), bOneShot) && bOneShot);
        if (QuestConditions)
        {
            bool bFoundFinalQuestCondition = false;
            for (const TSharedPtr<FJsonValue>& ConditionValue : *QuestConditions)
            {
                const TSharedPtr<FJsonObject> Condition = ConditionValue.IsValid() ? ConditionValue->AsObject() : nullptr;
                FString QuestId;
                FString State;
                if (Condition.IsValid() && Condition->TryGetStringField(TEXT("quest_id"), QuestId) &&
                    Condition->TryGetStringField(TEXT("state"), State) &&
                    QuestId == TEXT("demo_q4_todays_reward") && State == TEXT("reward_completed"))
                {
                    bFoundFinalQuestCondition = true;
                    break;
                }
            }
            TestTrue(TEXT("Dinner ending quest condition is reward_completed"), bFoundFinalQuestCondition);
        }
    }

    TSet<FString> ScenarioTextKeys;
    TSet<FString> UiTextKeys;
    TestTrue(TEXT("Scenario text CSV loads"), TunaSweeperDemoEndingTests::LoadCsvKeys(
        FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/ScenarioTextStrings.csv")), ScenarioTextKeys));
    TestTrue(TEXT("UI text CSV loads"), TunaSweeperDemoEndingTests::LoadCsvKeys(
        FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/UITextStrings.csv")), UiTextKeys));
    for (int32 LineIndex = 1; LineIndex <= 8; ++LineIndex)
    {
        TestTrue(
            FString::Printf(TEXT("Dinner ending line %d has a ScenarioTextStrings key"), LineIndex),
            ScenarioTextKeys.Contains(FString::Printf(TEXT("scenario.demo.ending.dinner.line%d"), LineIndex)));
    }
    TestTrue(TEXT("Farewell title uses a UI text key"), UiTextKeys.Contains(TEXT("ui.demo_ending.farewell_title")));
    TestTrue(TEXT("Return-to-title instruction uses a UI text key"), UiTextKeys.Contains(TEXT("ui.demo_ending.return_to_title")));
    FString FarewellSource;
    const FString FarewellSourcePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/TunaSweeper/Private/UI/TunaSweeperDemoFarewellWidget.cpp"));
    TestTrue(TEXT("Farewell widget source loads"), FFileHelper::LoadFileToString(FarewellSource, *FarewellSourcePath));
    TestFalse(TEXT("Farewell title is not hardcoded in C++"), FarewellSource.Contains(TEXT("본편에서 만나요")));
    TestFalse(TEXT("Return-to-title instruction is not hardcoded in C++"), FarewellSource.Contains(TEXT("아무 키나 누르면 타이틀로 돌아갑니다")));

    auto* WarehouseClass=LoadClass<ATunaSweeperFoodWarehouseActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_FoodWarehouse.BP_FoodWarehouse_C"));
    auto* SceneClass=LoadClass<ATunaSweeperDemoEndingActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_DemoDinnerEnding.BP_DemoDinnerEnding_C"));
    auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/DemoEnding/T_DemoFarewell.T_DemoFarewell"));
    if (!TestNotNull(TEXT("Warehouse BP"),WarehouseClass)||!TestNotNull(TEXT("Dinner BP"),SceneClass)||!TestNotNull(TEXT("Farewell texture"),Texture)) return false;
    auto* SceneCDO=SceneClass->GetDefaultObject<ATunaSweeperDemoEndingActor>();
    if (!TestNotNull(TEXT("Dinner BP defaults"),SceneCDO)) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    TestEqual(TEXT("Source width"),Texture->GetSizeX(),1350);
    TestEqual(TEXT("Source height"),Texture->GetSizeY(),900);
    TestTrue(TEXT("UI texture group"),Texture->LODGroup==TEXTUREGROUP_UI);
    TestNull(TEXT("Ending BP has no serialized DinnerDialogue property"),SceneClass->FindPropertyByName(TEXT("DinnerDialogue")));
    TestTrue(TEXT("Illustration serialized on BP"),SceneCDO->FarewellIllustration.LoadSynchronous()==Texture);
    TestTrue(TEXT("Farewell BGM fades out over time"),SceneCDO->FarewellBgmFadeOutSeconds>0.f);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* GI=NewObject<UTunaSweeperGameInstance>(World); World->SetGameInstance(GI);
    // No item-data subsystem is initialized in this isolated fixture; lab weapon setup is irrelevant.
    AddExpectedError(TEXT("Boss lab could not initialize rifle and ammunition"),EAutomationExpectedErrorFlags::Contains,1);
    GI->BeginCombatTestSession(); // Disable disk saves in this isolated gameplay test.
    auto* Warehouse=World->SpawnActor<ATunaSweeperFoodWarehouseActor>(WarehouseClass);
    TestNotNull(TEXT("Replaceable static mesh"),Warehouse->WarehouseMesh->GetStaticMesh().Get());
    TestEqual(TEXT("Final quest gates warehouse"),Warehouse->RequiredQuestId,FName(TEXT("demo_q4_todays_reward")));
    TestEqual(TEXT("Interaction hidden before final quest"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::None);
    int32 Before=GI->CountInventoryItemById(3004);
    TestFalse(TEXT("Cannot take a can before final quest"),Warehouse->TakeFood());
    TestEqual(TEXT("No can granted before final quest"),GI->CountInventoryItemById(3004),Before);

    // This isolated world has no game-instance subsystems. Clearing the configurable requirement
    // exercises the active presentation and collection path without touching the user's save data.
    Warehouse->RequiredQuestId=NAME_None;
    Warehouse->DispatchBeginPlay();
    TestEqual(TEXT("Interaction connected"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::WorldProgress);
    TestEqual(TEXT("Interaction says take tuna can"),Warehouse->Interactable->GetInteractionDisplayName().ToString(),FString(TEXT("참치캔 획득")));
    TestTrue(TEXT("Take a can"),Warehouse->TakeFood());
    TestEqual(TEXT("Exactly one can"),GI->CountInventoryItemById(3004),Before+1);
    TestFalse(TEXT("Repeated interaction cannot duplicate"),Warehouse->TakeFood());
    TestEqual(TEXT("Collected disables interaction"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::None);
    Warehouse->WarehouseMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Warehouse->RerunConstructionScripts();
    TestEqual(TEXT("Replacement mesh survives construction"),Warehouse->WarehouseMesh->GetStaticMesh()->GetName(),FString(TEXT("Cube")));
    World->DestroyWorld(false);

    auto* Widget=NewObject<UTunaSweeperDemoFarewellWidget>();
    Widget->Illustration=Texture;
    Widget->Initialize();
    auto SlateWidget=Widget->TakeWidget();
    TestTrue(TEXT("Farewell widget accepts keyboard focus"),Widget->IsFocusable());
    auto* Frame=Cast<UScaleBox>(Widget->WidgetTree->FindWidget(TEXT("IllustrationFrame")));
    if (!TestNotNull(TEXT("Illustration frame"),Frame)) return false;
    auto* Slot=CastChecked<UCanvasPanelSlot>(Frame->Slot);
    TestTrue(TEXT("Image frame uses 2/3 viewport height"),FMath::IsNearlyEqual(Slot->GetAnchors().Maximum.Y-Slot->GetAnchors().Minimum.Y,2.f/3.f,1.e-5f));
    int32 Returns=0; Widget->OnContinue=FSimpleDelegate::CreateLambda([&](){++Returns;});
    Widget->AcceptInputAfter=FPlatformTime::Seconds()+1;
    Widget->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::A,FModifierKeysState(),0,false,0,0));
    TestEqual(TEXT("Transition input does not skip card"),Returns,0);
    Widget->AcceptInputAfter=0;
    Widget->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::A,FModifierKeysState(),0,true,0,0));
    TestEqual(TEXT("Held keys do not skip card"),Returns,0);
    Widget->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::A,FModifierKeysState(),0,false,0,0));
    Widget->NativeOnKeyDown(FGeometry(),FKeyEvent(EKeys::B,FModifierKeysState(),0,false,0,0));
    TestEqual(TEXT("Any key returns once"),Returns,1);
    Widget->bLeaving=false;
    Widget->NativeOnMouseButtonDown(FGeometry(),FPointerEvent());
    TestEqual(TEXT("Mouse returns too"),Returns,2);
    FWidgetRenderer Renderer(true);
    auto* Target=Renderer.DrawWidget(SlateWidget,FVector2D(1280,720));
    TArray<FColor> Pixels;
    FReadSurfaceDataFlags ReadFlags; ReadFlags.SetLinearToGamma(false);
    if (TestNotNull(TEXT("Farewell render target"),Target) && Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,ReadFlags))
    {
        TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(1280,720,Pixels,PNG);
        TestTrue(TEXT("Farewell preview written"),FFileHelper::SaveArrayToFile(PNG,*(FPaths::ProjectSavedDir()/TEXT("DemoEndingFarewellPreview.png"))));
    }
    return true;
}
#endif
