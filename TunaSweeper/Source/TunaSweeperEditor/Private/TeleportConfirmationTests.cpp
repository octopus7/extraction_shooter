#include "Misc/AutomationTest.h"
#include "UI/Astra/TeleportConfirmationGPT6Astra.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "AssetCompilingManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/StrongObjectPtr.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTeleportConfirmationWidgetTest, "TunaSweeper.UI.TeleportConfirmation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTeleportConfirmationWidgetTest::RunTest(const FString&)
{
    const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
    ON_SCOPE_EXIT { FInternationalization::Get().SetCurrentCulture(OriginalCulture); };
    AddExpectedError(TEXT("Duplicate text string key ui.common.confirm found while loading UI data"), EAutomationExpectedErrorFlags::Contains, 0);
    auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Map/WBP_TeleportConfirmation_GPT6Astra"));
    if (!TestNotNull(TEXT("Saved editable Blueprint"), Blueprint)) return false;
    if (!TestNotNull(TEXT("Saved tree"), Blueprint->WidgetTree.Get()) ||
        !TestNotNull(TEXT("Saved root"), Blueprint->WidgetTree->RootWidget.Get()) ||
        !TestNotNull(TEXT("Compiled class"), Blueprint->GeneratedClass.Get())) return false;
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        World->DestroyWorld(false); Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Controller = World->SpawnActor<ATunaSweeperPlayerController>();
    if (!TestNotNull(TEXT("Controller fixture"), Controller)) return false;
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
    Controller->SetPlayer(LocalPlayer);
    // This isolated world has not run actor initialization. Player-context lookup
    // uses the world's controller iterator, not ULocalPlayer::PlayerController.
    World->AddController(Controller);
    TestEqual(TEXT("Local player resolves controller in fixture world"), LocalPlayer->GetPlayerController(World), static_cast<APlayerController*>(Controller));
    for (int32 Language = 0; Language < 6; ++Language)
    {
        Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(Language), false);
        TStrongObjectPtr<UTeleportConfirmationGPT6Astra> Dialog(CreateWidget<UTeleportConfirmationGPT6Astra>(Controller, Blueprint->GeneratedClass.Get()));
        TestEqual(TEXT("Popup owns a valid local controller"), Dialog->GetOwningPlayer(), static_cast<APlayerController*>(Controller));
        if (!TestTrue(TEXT("All authored BindWidgets present"), Dialog->HasAuthoredControls())) return false;
        Dialog->IsStillValid.BindLambda([] { return true; });
        auto Slate = Dialog->TakeWidget();
        const TCHAR* Names[] = { TEXT("TitleText"), TEXT("MessageText"), TEXT("YesText"), TEXT("NoText") };
        const TCHAR* Keys[] = { TEXT("ui.movement_cheat.teleport_title"), TEXT("ui.movement_cheat.teleport_confirm"), TEXT("ui.movement_cheat.yes"), TEXT("ui.movement_cheat.no") };
        for (int32 I = 0; I < 4; ++I)
        {
            auto* Label = Cast<UTextBlock>(Dialog->GetWidgetFromName(Names[I]));
            const FText Expected = Game->ResolveLocalizedText(FName(Keys[I]), FText::GetEmpty());
            TestFalse(TEXT("Translation exists in each language"), Expected.IsEmpty());
            TestTrue(TEXT("Control resolves project localization key"), Label->GetText().EqualTo(Expected));
        }
        if (FApp::CanEverRender() && Language == 0)
        {
            FAssetCompilingManager::Get().FinishAllCompilation(); FWidgetRenderer Renderer(true);
            TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(1280,720)));
            FAssetCompilingManager::Get().FinishAllCompilation(); FlushRenderingCommands();
            Dialog->InvalidateLayoutAndVolatility(); Dialog->ForceLayoutPrepass();
            Renderer.DrawWidget(Target.Get(), Slate, FVector2D(1280,720), 0);
            FlushRenderingCommands(); FImage Image;
            TestTrue(TEXT("Render target readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Image));
            IFileManager::Get().MakeDirectory(*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TeleportConfirmation")), true);
            TestTrue(TEXT("Popup preview saved"), FImageUtils::SaveImageByExtension(
                *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/TeleportConfirmation/ko.png")), Image));
        }
        int32 Calls = 0; bool Confirmed = false;
        Dialog->OnResolved.BindLambda([&](bool Value) { ++Calls; Confirmed = Value; });
        if (Language == 0) Dialog->YesButton->OnClicked.Broadcast();
        else if (Language == 1) Dialog->NoButton->OnClicked.Broadcast();
        else if (Language == 2) { Dialog->IsStillValid.BindLambda([] { return false; }); Dialog->NativeTick(FGeometry(), 0); }
        else if (Language == 3) Dialog->NativeDestruct();
        else Dialog->Cancel();
        TestEqual(TEXT("Exactly one resolution"), Calls, 1);
        TestEqual(TEXT("Only Yes confirms"), Confirmed, Language == 0);
        Dialog->YesButton->OnClicked.Broadcast(); Dialog->Cancel();
        TestEqual(TEXT("Repeated input cannot teleport twice"), Calls, 1);
    }
    if (TestNotNull(TEXT("Controller fixture"), Controller))
    {
        Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true);
        Controller->RestoreInputModeAfterTransientModal();
        TestTrue(TEXT("Closing modal preserves movement lock"), Controller->IsMoveInputIgnored());
        TestTrue(TEXT("Closing modal preserves look lock"), Controller->IsLookInputIgnored());
    }
    return true;
}
