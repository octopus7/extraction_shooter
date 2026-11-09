#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Game/TunaSweeperGameInstance.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/Terra/WaveDefensePanelGPT61Terra.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderingThread.h"
#include "UObject/StrongObjectPtr.h"
namespace WaveDefenseVisual { ETunaSweeperItemTextLanguage Original; bool bHaveOriginal=false; }
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FWaveDefenseOpenLanguage, FAutomationTestBase*, Test, int32, LanguageIndex);
bool FWaveDefenseOpenLanguage::Update()
{
    UWorld* World=GEditor->PlayWorld;
    auto* Controller=World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
    auto* Player=Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    auto* Game=World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
    AWaveDefenseDirector* Director=nullptr;
    if (World) for (TActorIterator<AWaveDefenseDirector> It(World);It;++It) { Director=*It; break; }
    if (!Player || !Game || !Director)
    {
        if (FPlatformTime::Seconds()-StartTime < 45.0) return false;
        Test->AddError(TEXT("PIE visual fixture not ready")); return true;
    }
    if (!WaveDefenseVisual::bHaveOriginal) { WaveDefenseVisual::Original=Game->GetCurrentTextLanguage(); WaveDefenseVisual::bHaveOriginal=true; }
    Player->SetActorLocation(FVector(4100,0,190),false,nullptr,ETeleportType::TeleportPhysics);
    Controller->CloseWaveDefensePanel();
    Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(LanguageIndex),false);
    Test->TestTrue(TEXT("Open authored panel"),Controller->OpenWaveDefensePanel(Director));
    return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FWaveDefenseCaptureLanguage, FAutomationTestBase*, Test, int32, LanguageIndex);
bool FWaveDefenseCaptureLanguage::Update()
{
    FViewport* Viewport=GEditor->GetPIEViewport();
    if (!Test->TestNotNull(TEXT("PIE viewport"),Viewport)) return true;
    TArray<FColor> Pixels;
    if (!Test->TestTrue(TEXT("Viewport pixels available"),Viewport->ReadPixels(Pixels))) return true;
    const FIntPoint Size=Viewport->GetSizeXY();
    TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,TArrayView64<const FColor>(Pixels.GetData(),Pixels.Num()),Png);
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("WaveDefense/Screenshots"); IFileManager::Get().MakeDirectory(*Dir,true);
    Test->TestTrue(TEXT("World viewport capture saved"),FFileHelper::SaveArrayToFile(Png,*(Dir/FString::Printf(TEXT("world_%d.png"),LanguageIndex))));
    FScreenshotRequest::RequestScreenshot(Dir/FString::Printf(TEXT("world_ui_%d.png"),LanguageIndex), true, false);
    TArray<UUserWidget*> Panels;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GEditor->PlayWorld, Panels, UWaveDefensePanelGPT61Terra::StaticClass(), false);
    Panels.RemoveAll([](const UUserWidget* Widget) { return !Widget || !Widget->IsInViewport(); });
    if (!Test->TestEqual(TEXT("One open panel"), Panels.Num(), 1)) return true;
    FWidgetRenderer Renderer(false);
    const TSharedRef<SWidget> Slate = Panels[0]->TakeWidget();
    TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate,FVector2D(1920,1080)));
    if (!Test->TestNotNull(TEXT("1080p UI render target"),Target.Get())) return true;
    FlushRenderingCommands(); Renderer.DrawWidget(Target.Get(),Slate,FVector2D(1920,1080),0); FlushRenderingCommands();
    FImage Image;
    if (Test->TestTrue(TEXT("1080p UI pixels"),FImageUtils::GetRenderTargetImage(Target.Get(),Image)))
    {
        Image.GammaSpace=EGammaSpace::Linear;
        Test->TestTrue(TEXT("1080p panel capture saved"),FImageUtils::SaveImageByExtension(*(Dir/FString::Printf(TEXT("panel_%d.png"),LanguageIndex)),Image));
    }
    return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND(FWaveDefenseVisualCleanup);
bool FWaveDefenseVisualCleanup::Update()
{
    if (UWorld* World=GEditor->PlayWorld) if (auto* Game=World->GetGameInstance<UTunaSweeperGameInstance>(); Game && WaveDefenseVisual::bHaveOriginal) Game->SetCurrentTextLanguage(WaveDefenseVisual::Original,false);
    WaveDefenseVisual::bHaveOriginal=false; TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseVisualLanguagesTest, "TunaSweeper.WaveDefense.Visual.Languages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseVisualLanguagesTest::RunTest(const FString& Parameters)
{
    if (!FApp::CanEverRender()) { AddInfo(TEXT("Visual capture requires a rendering RHI; run this test separately with rendering enabled.")); return true; }
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/WaveDefense")/FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated save root"),TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("MainRaid/RaidPlains.umap"),false,true)) { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return false; }
    FRequestPlaySessionParams Play; Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage()); Play.EditorPlaySettings->NewWindowWidth=1920; Play.EditorPlaySettings->NewWindowHeight=1080;
    GEditor->RequestPlaySession(Play);
    for (int32 Language=0;Language<6;++Language) { ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseOpenLanguage(this,Language)); ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f)); ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseCaptureLanguage(this,Language)); ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f)); }
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseVisualCleanup()); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
