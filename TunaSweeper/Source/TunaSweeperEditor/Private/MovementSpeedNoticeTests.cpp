#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "Components/CanvasPanelSlot.h"
#include "Misc/AutomationTest.h"
#include "UI/MovementSpeedNotice.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMovementSpeedNoticeWidgetTest, "TunaSweeper.UI.MovementSpeedNotice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMovementSpeedNoticeWidgetTest::RunTest(const FString&)
{
    const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
    ON_SCOPE_EXIT { FInternationalization::Get().SetCurrentCulture(OriginalCulture); };
    AddExpectedError(TEXT("Duplicate text string key ui.common.confirm found while loading UI data"), EAutomationExpectedErrorFlags::Contains, 0);
    auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Debug/WBP_MovementSpeedNotice"));
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
    const uint32 OriginalToken = TunaSweeperMovementCheatControl.EnableToken;
    ON_SCOPE_EXIT { TunaSweeperMovementCheatControl.EnableToken = OriginalToken; };
    auto* Cheats = Game->GetSubsystem<UTunaSweeperMovementCheatSubsystem>();
    if (!TestNotNull(TEXT("Movement subsystem"), Cheats)) return false;
    TunaSweeperMovementCheatControl.EnableToken = 0; Cheats->Tick(0);
    TStrongObjectPtr<UMovementSpeedNotice> Notice(CreateWidget<UMovementSpeedNotice>(Controller, Blueprint->GeneratedClass.Get()));
    if (!TestNotNull(TEXT("Notice instance"), Notice.Get())) return false;
    auto Slate = Notice->TakeWidget();
    auto* Label = Cast<UTextBlock>(Notice->GetWidgetFromName(TEXT("SpeedText")));
    if (!TestNotNull(TEXT("Authored label"), Label)) return false;
    auto* Slot = Cast<UCanvasPanelSlot>(Label->Slot);
    if (!TestNotNull(TEXT("Editable center canvas slot"), Slot)) return false;
    TestTrue(TEXT("Exact center anchors"), Slot->GetAnchors().Minimum.Equals(FVector2D(0.5f,0.5f)));
    TestTrue(TEXT("Centered alignment"), Slot->GetAlignment().Equals(FVector2D(0.5f,0.5f)));
    const FLinearColor Color = Label->GetColorAndOpacity().GetSpecifiedColor();
    TestTrue(TEXT("Red notice"), Color.R > 0.9f && Color.G < 0.1f && Color.B < 0.1f);
    Notice->UpdateSpeed(Cheats->GetSpeedMultiplier());
    TestEqual(TEXT("Gate off hides"), Notice->GetVisibility(), ESlateVisibility::Collapsed);
    TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
    Notice->UpdateSpeed(Cheats->GetSpeedMultiplier());
    TestEqual(TEXT("Normal speed hides"), Notice->GetVisibility(), ESlateVisibility::Collapsed);
    for (const int32 Speed : {3, 8})
    {
        Cheats->CycleSpeed();
        TestEqual(TEXT("Real speed cycle"), Cheats->GetSpeedMultiplier(), static_cast<float>(Speed));
        for (int32 Language = 0; Language < 6; ++Language)
        {
            Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(Language), false);
            Notice->UpdateSpeed(Cheats->GetSpeedMultiplier());
            TestEqual(TEXT("Notice cannot intercept input"), Notice->GetVisibility(), ESlateVisibility::HitTestInvisible);
            FFormatNamedArguments Args; Args.Add(TEXT("Multiplier"), FText::AsNumber(Speed));
            const FText Pattern = Game->ResolveLocalizedText(TEXT("ui.movement_cheat.speed_notice"), FText::GetEmpty());
            TestFalse(TEXT("Translation exists"), Pattern.IsEmpty());
            TestTrue(TEXT("Localized multiplier notice"), Label->GetText().EqualTo(FText::Format(Pattern, Args)));
            if (FApp::CanEverRender() && Language == 0)
            {
                FAssetCompilingManager::Get().FinishAllCompilation(); FWidgetRenderer Renderer(true);
                TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(1280,720)));
                FAssetCompilingManager::Get().FinishAllCompilation(); FlushRenderingCommands();
                Notice->InvalidateLayoutAndVolatility(); Notice->ForceLayoutPrepass();
                Renderer.DrawWidget(Target.Get(), Slate, FVector2D(1280,720), 0);
                FlushRenderingCommands(); FImage Image;
                TestTrue(TEXT("Render readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Image));
                const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/MovementSpeedNotice"));
                IFileManager::Get().MakeDirectory(*Directory, true);
                TestTrue(TEXT("Preview saved"), FImageUtils::SaveImageByExtension(
                    *FPaths::Combine(Directory, FString::Printf(TEXT("ko_%dx.png"), Speed)), Image));
            }
        }
    }
    Cheats->CycleSpeed(); Notice->UpdateSpeed(Cheats->GetSpeedMultiplier());
    TestEqual(TEXT("Cycle back to normal hides"), Notice->GetVisibility(), ESlateVisibility::Collapsed);
    Cheats->CycleSpeed(); TunaSweeperMovementCheatControl.EnableToken = 0;
    Cheats->Tick(0); Notice->UpdateSpeed(Cheats->GetSpeedMultiplier());
    TestEqual(TEXT("Disabling accelerated cheat hides"), Notice->GetVisibility(), ESlateVisibility::Collapsed);
    TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
    TestEqual(TEXT("Reenable stays normal"), Cheats->GetSpeedMultiplier(), 1.0f);
    return true;
}