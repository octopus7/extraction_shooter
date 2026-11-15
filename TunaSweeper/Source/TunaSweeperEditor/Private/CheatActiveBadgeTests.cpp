#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/OverlaySlot.h"
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/UserInterfaceSettings.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "AssetCompilingManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheatActiveBadgeTest, "TunaSweeper.UI.CheatActiveBadge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCheatActiveBadgeTest::RunTest(const FString&)
{
    AddExpectedError(TEXT("Duplicate text string key ui.common.confirm found while loading UI data"), EAutomationExpectedErrorFlags::Contains, 0);
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
    if (!TestNotNull(TEXT("Controller"), Controller)) return false;
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
    Controller->SetPlayer(LocalPlayer); World->AddController(Controller);
    const uint32 OriginalToken = TunaSweeperMovementCheatControl.EnableToken;
    ON_SCOPE_EXIT { TunaSweeperMovementCheatControl.EnableToken = OriginalToken; };
    TunaSweeperMovementCheatControl.EnableToken = 0;
    auto* Cheats = Game->GetSubsystem<UTunaSweeperMovementCheatSubsystem>();
    if (!TestNotNull(TEXT("Cheats"), Cheats)) return false;
    auto* Class = LoadClass<UTunaSweeperGameHudWidget>(nullptr, TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C"));
    if (!TestNotNull(TEXT("Actual game HUD"), Class)) return false;
    TStrongObjectPtr<UTunaSweeperGameHudWidget> Hud(CreateWidget<UTunaSweeperGameHudWidget>(Controller, Class));
    if (!TestNotNull(TEXT("HUD instance"), Hud.Get())) return false;
    auto Slate = Hud->TakeWidget();
    auto* Bottom = Cast<UTunaSweeperHudBottomStatusWidget>(Hud->GetWidgetFromName(TEXT("BottomStatusWidget")));
    if (!TestNotNull(TEXT("Actual bottom status"), Bottom)) return false;
    Bottom->SetVisibility(ESlateVisibility::HitTestInvisible);
    Bottom->SetRenderTranslation(FVector2D::ZeroVector);
    Bottom->SetRenderOpacity(1.0f);
    auto* Badge = Cast<UImage>(Bottom->GetWidgetFromName(TEXT("CheatActiveBadge")));
    auto* Gauge = Cast<UProgressBar>(Bottom->GetWidgetFromName(TEXT("HealthGauge")));
    if (!TestNotNull(TEXT("Saved image badge"), Badge) || !TestNotNull(TEXT("Health gauge"), Gauge)) return false;
    TestTrue(TEXT("Serialized badge size survives reload"), FVector2D(Badge->GetBrush().ImageSize).Equals(FVector2D(150, 40), 0.01));
    TestEqual(TEXT("Anchored to health overlay"), Badge->GetParent(), Gauge->GetParent());
    auto* Texture = Cast<UTexture2D>(Badge->GetBrush().GetResourceObject());
    if (!TestNotNull(TEXT("Badge uses baked raster"), Texture)) return false;
    TestEqual(TEXT("Expected badge image"), Texture->GetPathName(), FString(TEXT("/Game/UI/Debug/T_CheatActiveBadge.T_CheatActiveBadge")));
    Bottom->SetHudState(FTunaSweeperPlayerHudState());
    TestEqual(TEXT("Off is hidden"), Badge->GetVisibility(), ESlateVisibility::Collapsed);
    TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
    for (int32 Speed : {1, 3, 8})
    {
        TestEqual(TEXT("Expected real cheat speed"), Cheats->GetSpeedMultiplier(), static_cast<float>(Speed));
        Bottom->SetHudState(FTunaSweeperPlayerHudState());
        TestEqual(TEXT("Enabled including 1x is visible and input transparent"), Badge->GetVisibility(), ESlateVisibility::HitTestInvisible);
        if (FApp::CanEverRender() && Speed == 1)
        {
            FAssetCompilingManager::Get().FinishAllCompilation();
            FWidgetRenderer Renderer(true);
            for (const FVector2D Viewport : {FVector2D(1280,720), FVector2D(1920,1080)})
            {
                const float DPIScale = GetDefault<UUserInterfaceSettings>()->GetDPIScaleBasedOnSize(FIntPoint(int32(Viewport.X), int32(Viewport.Y)));
                const TSharedRef<SDPIScaler> ScaledHud = SNew(SDPIScaler).DPIScale(DPIScale)[Slate];
                Hud->InvalidateLayoutAndVolatility(); Hud->ForceLayoutPrepass();
                TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(ScaledHud, Viewport));
                FlushRenderingCommands();
                const FGeometry& BG = Badge->GetCachedGeometry();
                const FGeometry& HG = Gauge->GetCachedGeometry();
                const FVector2D BadgeCenter = BG.LocalToAbsolute(BG.GetLocalSize() * 0.5);
                const FVector2D HealthCenter = HG.LocalToAbsolute(HG.GetLocalSize() * 0.5);
                TestTrue(TEXT("Badge centered on actual health gauge"), FMath::IsNearlyEqual(BadgeCenter.X, HealthCenter.X, 0.5));
                const double Gap = HG.LocalToAbsolute(FVector2D::ZeroVector).Y - BG.LocalToAbsolute(FVector2D(0, BG.GetLocalSize().Y)).Y;
                const FVector2D PixelSize = BG.LocalToAbsolute(BG.GetLocalSize()) - BG.LocalToAbsolute(FVector2D::ZeroVector);
                AddInfo(FString::Printf(TEXT("Viewport %.0fx%.0f DPI %.3f: badge origin %s local %s pixel %s; health origin %s size %s; gap %.3f"),
                    Viewport.X, Viewport.Y, DPIScale, *BG.GetAbsolutePosition().ToString(), *BG.GetLocalSize().ToString(), *PixelSize.ToString(),
                    *HG.GetAbsolutePosition().ToString(), *HG.GetLocalSize().ToString(), Gap));
                TestTrue(TEXT("Badge retains its readable 150x40 layout"), BG.GetLocalSize().Equals(FVector2D(150, 40), 0.1));
                TestTrue(TEXT("Badge has viewport-scaled 150x40 pixel dimensions"), PixelSize.Equals(FVector2D(150, 40) * DPIScale, 0.5));
                TestTrue(TEXT("Badge immediately above health gauge"), FMath::IsNearlyEqual(Gap, 4.0 * DPIScale, 0.5));
                const FVector2D TopLeft = BG.LocalToAbsolute(FVector2D::ZeroVector);
                const FVector2D BottomRight = BG.LocalToAbsolute(BG.GetLocalSize());
                TestTrue(TEXT("Entire badge lies within viewport"), TopLeft.X >= 0 && TopLeft.Y >= 0
                    && BottomRight.X <= Viewport.X && BottomRight.Y <= Viewport.Y);
                FImage Pixels;
                if (TestTrue(TEXT("Capture readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Pixels)))
                {
                    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/CheatActiveBadge");
                    IFileManager::Get().MakeDirectory(*Directory, true);
                    TestTrue(TEXT("Capture saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("enabled_1x_%dx%d.png"), int32(Viewport.X), int32(Viewport.Y))), Pixels));
                }
            }
        }
        Cheats->CycleSpeed();
    }
    TunaSweeperMovementCheatControl.EnableToken = 0;
    Bottom->SetHudState(FTunaSweeperPlayerHudState());
    TestEqual(TEXT("Disabling hides immediately on HUD refresh"), Badge->GetVisibility(), ESlateVisibility::Collapsed);
    return true;
}
#endif
