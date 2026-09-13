#if WITH_DEV_AUTOMATION_TESTS
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDemoEndingAssetsTest,"TunaSweeper.DemoEnding.AssetsAndInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaDemoEndingAssetsTest::RunTest(const FString&)
{
    auto* WarehouseClass=LoadClass<ATunaSweeperFoodWarehouseActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_FoodWarehouse.BP_FoodWarehouse_C"));
    auto* SceneClass=LoadClass<ATunaSweeperDemoEndingActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_DemoDinnerEnding.BP_DemoDinnerEnding_C"));
    auto* Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/DemoEnding/T_DemoFarewell.T_DemoFarewell"));
    if (!TestNotNull(TEXT("Warehouse BP"),WarehouseClass)||!TestNotNull(TEXT("Dinner BP"),SceneClass)||!TestNotNull(TEXT("Farewell texture"),Texture)) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    TestEqual(TEXT("Source width"),Texture->GetSizeX(),1350);
    TestEqual(TEXT("Source height"),Texture->GetSizeY(),900);
    TestTrue(TEXT("UI texture group"),Texture->LODGroup==TEXTUREGROUP_UI);
    const auto* SceneCDO=SceneClass->GetDefaultObject<ATunaSweeperDemoEndingActor>();
    TestTrue(TEXT("Dinner dialogue authored"),SceneCDO->DinnerDialogue.Num()>=6);
    TestTrue(TEXT("Illustration serialized on BP"),SceneCDO->FarewellIllustration.LoadSynchronous()==Texture);
    TestTrue(TEXT("Farewell BGM fades out over time"),SceneCDO->FarewellBgmFadeOutSeconds>0.f);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    auto* GI=NewObject<UTunaSweeperGameInstance>(World); World->SetGameInstance(GI);
    // No item-data subsystem is initialized in this isolated fixture; lab weapon setup is irrelevant.
    AddExpectedError(TEXT("Boss lab could not initialize rifle and ammunition"),EAutomationExpectedErrorFlags::Contains,1);
    GI->BeginCombatTestSession(); // Disable disk saves in this isolated gameplay test.
    auto* Warehouse=World->SpawnActor<ATunaSweeperFoodWarehouseActor>(WarehouseClass);
    TestNotNull(TEXT("Replaceable static mesh"),Warehouse->WarehouseMesh->GetStaticMesh().Get());
    TestEqual(TEXT("Interaction connected"),Warehouse->Interactable->GetInteractionType(),ETunaSweeperInteractionType::WorldProgress);
    int32 Before=GI->CountInventoryItemById(3004);
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
