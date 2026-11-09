#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Game/TunaSweeperGameInstance.h"
#include "HAL/FileManager.h"
#include "IconEditor/IconEditorTerminal.h"
#include "IconEditor/IconStorageSubsystem.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/TunaSweeperPlayerController.h"
#include "RenderingThread.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Slate/WidgetRenderer.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/Astra/IconCanvasGPT6Astra.h"
#include "UI/Astra/IconEditorGPT6Astra.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

// Test access never changes the shipping widget API or builds a runtime hierarchy.
struct FIconEditorAutomationAccess
{
    static const TArray<uint8> &Draft(const UIconEditorGPT6Astra *Panel)
    {
        return Panel->Draft;
    }
    static FColor Color(const UIconEditorGPT6Astra *Panel)
    {
        return Panel->Color;
    }
    static FReply Down(UIconCanvasGPT6Astra *Canvas, const FGeometry &Geo, const FPointerEvent &Event)
    {
        return Canvas->NativeOnMouseButtonDown(Geo, Event);
    }
    static FReply Move(UIconCanvasGPT6Astra *Canvas, const FGeometry &Geo, const FPointerEvent &Event)
    {
        return Canvas->NativeOnMouseMove(Geo, Event);
    }
    static FReply Up(UIconCanvasGPT6Astra *Canvas, const FGeometry &Geo, const FPointerEvent &Event)
    {
        return Canvas->NativeOnMouseButtonUp(Geo, Event);
    }
    static FReply Escape(UIconEditorGPT6Astra *Panel)
    {
        return Panel->NativeOnPreviewKeyDown(FGeometry(),
                                             FKeyEvent(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0));
    }
};

namespace IconEditorTests
{
FString Root;
TWeakObjectPtr<AIconEditorTerminal> Terminal;
ETunaSweeperItemTextLanguage OriginalLanguage;
bool bHaveOriginalLanguage = false;

UIconEditorGPT6Astra *Panel(UWorld *World)
{
    TArray<UUserWidget *> Widgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UIconEditorGPT6Astra::StaticClass(), false);
    for (UUserWidget *Widget : Widgets)
        if (Widget && Widget->IsInViewport())
            return Cast<UIconEditorGPT6Astra>(Widget);
    return nullptr;
}
bool Click(FAutomationTestBase *Test, UIconEditorGPT6Astra *Panel, const TCHAR *Name)
{
    auto *Button = Panel ? Cast<UButton>(Panel->GetWidgetFromName(Name)) : nullptr;
    if (!Test->TestNotNull(Name, Button))
        return false;
    Button->OnClicked.Broadcast();
    return true;
}
TArray<uint8> Read(UIconStorageSubsystem *Storage)
{
    TArray<uint8> Bytes;
    FFileHelper::LoadFileToArray(Bytes, *Storage->GetIconFilePath());
    return Bytes;
}
FPointerEvent Pointer(FVector2D Position, FKey Button, bool Pressed)
{
    TSet<FKey> Buttons;
    if (Pressed)
        Buttons.Add(Button);
    return FPointerEvent(0, Position, Position, Buttons, Button, 0, FModifierKeysState());
}
void Stroke(UIconCanvasGPT6Astra *Canvas, FIntPoint From, FIntPoint To, float Size = 576)
{
    const FGeometry Geo = FGeometry::MakeRoot(FVector2D(Size), FSlateLayoutTransform());
    const double Cell = Size / UIconStorageSubsystem::Width;
    const FVector2D A((From.X + 0.5) * Cell, (From.Y + 0.5) * Cell), B((To.X + 0.5) * Cell, (To.Y + 0.5) * Cell);
    FIconEditorAutomationAccess::Down(Canvas, Geo, Pointer(A, EKeys::LeftMouseButton, true));
    FIconEditorAutomationAccess::Move(Canvas, Geo, Pointer(B, EKeys::LeftMouseButton, true));
    FIconEditorAutomationAccess::Up(Canvas, Geo, Pointer(B, EKeys::LeftMouseButton, false));
}
} // namespace IconEditorTests

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIconEditorAssetTest, "TunaSweeper.IconEditor.Assets",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIconEditorAssetTest::RunTest(const FString &)
{
    UClass *Class = LoadClass<UIconEditorGPT6Astra>(
        nullptr, TEXT("/Game/UI/IconEditor/WBP_IconEditor_GPT6Astra.WBP_IconEditor_GPT6Astra_C"));
    if (!TestNotNull(TEXT("Saved editor WBP loads"), Class))
        return false;
    auto *BPClass = Cast<UWidgetBlueprintGeneratedClass>(Class);
    if (!TestNotNull(TEXT("Saved Widget Blueprint generated class"), BPClass) ||
        !TestNotNull(TEXT("Serialized widget tree"), BPClass->GetWidgetTreeArchetype()))
        return false;
    int32 BindingCount = 0;
    for (TFieldIterator<FObjectProperty> It(UIconEditorGPT6Astra::StaticClass()); It; ++It)
    {
        if (!It->HasMetaData(TEXT("BindWidget")))
            continue;
        ++BindingCount;
        UWidget *Widget = BPClass->GetWidgetTreeArchetype()->FindWidget(It->GetFName());
        if (TestNotNull(It->GetName(), Widget))
            TestTrue(It->GetName() + TEXT(" correct authored type"), Widget->IsA(It->PropertyClass));
    }
    TestEqual(TEXT("Every required binding inspected"), BindingCount, 39);
    for (const TCHAR* Name : {TEXT("BrushText"), TEXT("EyedropperText"), TEXT("FillText"), TEXT("CancelText"), TEXT("ConfirmText")})
    {
        auto* Label = Cast<UTextBlock>(BPClass->GetWidgetTreeArchetype()->FindWidget(Name));
        if (TestNotNull(Name, Label))
            TestFalse(FString(Name) + TEXT(" keeps short translated button text on one line"), Label->GetAutoWrapText());
    }
    int32 ScaleBoxes = 0;
    BPClass->GetWidgetTreeArchetype()->ForEachWidget([&](UWidget *Widget) {
        if (auto *Scale = Cast<UScaleBox>(Widget))
        {
            ++ScaleBoxes;
            TestEqual(TEXT("Uniform fit"), Scale->GetStretch(), EStretch::ScaleToFit);
        }
    });
    TestEqual(TEXT("One responsive scale box"), ScaleBoxes, 1);
    TestNotNull(TEXT("Placeable terminal BP loads"),
                LoadClass<AIconEditorTerminal>(
                    nullptr, TEXT("/Game/Interaction/BP_IconEditorTerminal.BP_IconEditorTerminal_C")));
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FIconEditorExercise, FAutomationTestBase *, Test);
bool FIconEditorExercise::Update()
{
    using namespace IconEditorTests;
    UWorld *World = GEditor->PlayWorld;
    auto *Controller = World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
    auto *Player = Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    auto *Game = World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
    if (!Player || !Game)
    {
        if (FPlatformTime::Seconds() - StartTime < 45)
            return false;
        Test->AddError(TEXT("Icon PIE fixture did not become ready"));
        return true;
    }
    // HUD panel hides are animated. Wait for their real visibility to settle before opening another modal.
    if (auto *Hud = Controller->GetGameHudWidget(); Hud && Hud->GetHudMode() != ETunaSweeperHudMode::None)
    {
        Hud->SetHudMode(ETunaSweeperHudMode::None);
        return false;
    }
    if (Controller->IsInventoryUiOpen())
    {
        if (FPlatformTime::Seconds() - StartTime < 45)
            return false;
        Test->AddError(TEXT("Startup HUD did not finish closing"));
        return true;
    }
    OriginalLanguage = Game->GetCurrentTextLanguage();
    bHaveOriginalLanguage = true;
    auto *Storage = Game->GetSubsystem<UIconStorageSubsystem>();
    if (!Test->TestNotNull(TEXT("Storage subsystem"), Storage))
        return true;
    Test->TestTrue(TEXT("All writes under isolated automation root"),
                   Storage->GetIconFilePath().StartsWith(FPaths::ConvertRelativePathToFull(Root)));
    TArray<uint8> Bytes;
    Test->TestEqual(TEXT("Missing is a normal canvas"), Storage->LoadRgbIcon(Bytes), EIconLoadResult::Missing);
    Test->TestEqual(TEXT("Black draft exact RGB length"), Bytes.Num(), UIconStorageSubsystem::ByteCount);
    Test->TestTrue(TEXT("Black default"), !Bytes.ContainsByPredicate([](uint8 V) { return V != 0; }));
    TArray<uint8> Baseline;
    Baseline.SetNum(UIconStorageSubsystem::ByteCount);
    for (int32 I = 0; I < Baseline.Num(); ++I)
        Baseline[I] = static_cast<uint8>(I % 251);
    if (!Test->TestTrue(TEXT("Save baseline raw bytes"), Storage->TrySaveIcon(Baseline)))
        return true;
    Test->TestFalse(TEXT("Reject invalid write size"), Storage->TrySaveIcon(TArray<uint8>{1, 2, 3}));
    Test->TestTrue(TEXT("Invalid write preserves baseline"), Read(Storage) == Baseline);
    UClass *TerminalClass = LoadClass<AIconEditorTerminal>(
        nullptr, TEXT("/Game/Interaction/BP_IconEditorTerminal.BP_IconEditorTerminal_C"));
    Terminal = World->SpawnActor<AIconEditorTerminal>(TerminalClass, Player->GetActorLocation() + FVector(30, 0, 0),
                                                      FRotator::ZeroRotator);
    if (!Test->TestNotNull(TEXT("Spawn actual terminal BP"), Terminal.Get()))
        return true;
    const bool bCursorBefore = Controller->bShowMouseCursor;
    if (!Test->TestTrue(TEXT("Interaction opens editor"), Terminal->OpenFor(Player)))
        return true;
    UIconEditorGPT6Astra *UI = Panel(World);
    if (!Test->TestNotNull(TEXT("Authored modal in viewport"), UI))
        return true;
    Test->TestTrue(TEXT("Modal blocks move"), Controller->IsMoveInputIgnored());
    Test->TestTrue(TEXT("Modal blocks look"), Controller->IsLookInputIgnored());
    Test->TestTrue(TEXT("Standalone paused"), UGameplayStatics::IsGamePaused(World));
    Test->TestFalse(TEXT("Duplicate open rejected"), Terminal->OpenFor(Player));
    Test->TestTrue(TEXT("Opening loads exact disk pixels"), FIconEditorAutomationAccess::Draft(UI) == Baseline);
    auto *Canvas = Cast<UIconCanvasGPT6Astra>(UI->GetWidgetFromName(TEXT("PixelCanvas")));
    if (!Test->TestNotNull(TEXT("Bound pixel canvas"), Canvas))
        return true;
    Click(Test, UI, TEXT("Palette03"));
    Stroke(Canvas, FIntPoint(0, 0), FIntPoint(31, 31));
    const TArray<uint8> &Drawn = FIconEditorAutomationAccess::Draft(UI);
    for (int32 I = 0; I < 32; ++I)
    {
        const int32 Offset = (I * 32 + I) * 3;
        Test->TestTrue(FString::Printf(TEXT("Continuous diagonal cell %d"), I),
                       Drawn[Offset] == 244 && Drawn[Offset + 1] == 67 && Drawn[Offset + 2] == 54);
    }
    Test->TestTrue(TEXT("Painting never writes disk"), Read(Storage) == Baseline);
    const FGeometry Geo = FGeometry::MakeRoot(FVector2D(576), FSlateLayoutTransform());
    const FVector2D SamplePosition(31.5 * 18, 0.5 * 18);
    FIconEditorAutomationAccess::Down(Canvas, Geo, Pointer(SamplePosition, EKeys::RightMouseButton, true));
    FIconEditorAutomationAccess::Up(Canvas, Geo, Pointer(SamplePosition, EKeys::RightMouseButton, false));
    Test->TestEqual(TEXT("Right click samples existing RGB"), FIconEditorAutomationAccess::Color(UI),
                    FColor(Baseline[93], Baseline[94], Baseline[95]));
    Click(Test, UI, TEXT("EyedropperButton"));
    Stroke(Canvas, FIntPoint(0, 0), FIntPoint(0, 0), 288);
    Test->TestEqual(TEXT("Eyedropper at scaled geometry"), FIconEditorAutomationAccess::Color(UI), FColor(244, 67, 54));
    auto *Red = Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")));
    auto *Green = Cast<USpinBox>(UI->GetWidgetFromName(TEXT("GreenInput")));
    auto *Blue = Cast<USpinBox>(UI->GetWidgetFromName(TEXT("BlueInput")));
    Red->SetValue(12);
    Green->SetValue(34);
    Blue->SetValue(56);
    Click(Test, UI, TEXT("FillButton"));
    TArray<uint8> Filled;
    Filled.SetNum(UIconStorageSubsystem::ByteCount);
    for (int32 I = 0; I < Filled.Num(); I += 3)
    {
        Filled[I] = 12;
        Filled[I + 1] = 34;
        Filled[I + 2] = 56;
    }
    Test->TestTrue(TEXT("Fill affects complete draft"), FIconEditorAutomationAccess::Draft(UI) == Filled);
    Test->TestTrue(TEXT("Fill remains unsaved"), Read(Storage) == Baseline);
    Click(Test, UI, TEXT("ConfirmButton"));
    Test->TestNull(TEXT("Successful confirm closes modal"), Panel(World));
    Test->TestTrue(TEXT("Confirm writes exact 3072 RGB bytes"), Read(Storage) == Filled);
    Test->TestFalse(TEXT("Move input restored"), Controller->IsMoveInputIgnored());
    Test->TestFalse(TEXT("Look input restored"), Controller->IsLookInputIgnored());
    Test->TestFalse(TEXT("Standalone unpaused"), UGameplayStatics::IsGamePaused(World));
    Test->TestEqual(TEXT("Cursor mode restored"), Controller->bShowMouseCursor, bCursorBefore);
    if (Terminal->OpenFor(Player))
    {
        UI = Panel(World);
        Click(Test, UI, TEXT("Palette04"));
        Click(Test, UI, TEXT("FillButton"));
        Click(Test, UI, TEXT("CancelButton"));
        Test->TestTrue(TEXT("Cancel preserves saved pixels"), Read(Storage) == Filled);
    }
    else
        Test->AddError(TEXT("Reopen for Cancel failed"));
    if (Terminal->OpenFor(Player))
    {
        UI = Panel(World);
        Click(Test, UI, TEXT("Palette05"));
        Click(Test, UI, TEXT("FillButton"));
        Test->TestTrue(TEXT("Escape consumed by preview key handler"),
                       FIconEditorAutomationAccess::Escape(UI).IsEventHandled());
        Test->TestNull(TEXT("Escape closes modal"), Panel(World));
        Test->TestTrue(TEXT("Escape preserves saved pixels"), Read(Storage) == Filled);
    }
#if PLATFORM_WINDOWS
    if (Terminal->OpenFor(Player))
    {
        UI = Panel(World);
        Click(Test, UI, TEXT("Palette07"));
        Click(Test, UI, TEXT("FillButton"));
        const TArray<uint8> Pending = FIconEditorAutomationAccess::Draft(UI);
        const FString Physical =
            IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Storage->GetIconFilePath());
        HANDLE Lock = ::CreateFileW(*Physical, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (Test->TestTrue(TEXT("Lock existing icon for real replacement failure"), Lock != INVALID_HANDLE_VALUE))
        {
            TArray<uint8> FailedRead;
            Test->TestEqual(TEXT("Locked read returns ReadFailed"), Storage->LoadRgbIcon(FailedRead),
                            EIconLoadResult::ReadFailed);
            Click(Test, UI, TEXT("ConfirmButton"));
            Test->TestTrue(TEXT("Failed save leaves modal open"), Panel(World) == UI);
            Test->TestTrue(TEXT("Failed save retains complete draft"),
                           FIconEditorAutomationAccess::Draft(UI) == Pending);
            ::CloseHandle(Lock);
            Test->TestTrue(TEXT("Failed replace preserves original bytes"), Read(Storage) == Filled);
            Click(Test, UI, TEXT("ConfirmButton"));
            Test->TestTrue(TEXT("Retry confirms same draft"), Read(Storage) == Pending);
            Filled = Pending;
        }
        Controller->CloseIconEditorPanel();
    }
#endif
    TArray<FString> Candidates;
    IFileManager::Get().FindFiles(Candidates, *(FPaths::GetPath(Storage->GetIconFilePath()) / TEXT("*.candidate")),
                                  true, false);
    Test->TestEqual(TEXT("No partial candidate files remain"), Candidates.Num(), 0);
    const FString IconPath = Storage->GetIconFilePath();
    Test->TestTrue(TEXT("New game in slot 2"), Game->ActivateSaveSlot(2, true));
    Test->TestEqual(TEXT("Icon path independent of slot"), Storage->GetIconFilePath(), IconPath);
    Test->TestTrue(TEXT("New game leaves global icon unchanged"), Read(Storage) == Filled);
    Test->TestTrue(TEXT("Delete slot 2"), Game->DeleteSaveSlot(2));
    Test->TestTrue(TEXT("Deleting slot leaves global icon unchanged"), Read(Storage) == Filled);
    Game->SetActiveSaveSlotIndex(1);
    const TArray<uint8> Invalid{1, 2, 3};
    Test->TestTrue(TEXT("Stage malformed icon"), FFileHelper::SaveArrayToFile(Invalid, *IconPath));
    if (Terminal->OpenFor(Player))
    {
        UI = Panel(World);
        Test->TestFalse(TEXT("Invalid file shows localized warning"),
                        Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("StatusText")))->GetText().IsEmpty());
        Click(Test, UI, TEXT("CancelButton"));
        Test->TestTrue(TEXT("Invalid original untouched by Cancel"), Read(Storage) == Invalid);
    }
    Test->TestTrue(TEXT("Restore fixture icon"), Storage->TrySaveIcon(Filled));
    Controller->SetIgnoreMoveInput(true);
    Test->TestFalse(TEXT("Existing modal input block prevents editor opening"), Terminal->OpenFor(Player));
    Controller->SetIgnoreMoveInput(false);
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FIconEditorRender, FAutomationTestBase *, Test, int32, Language);
bool FIconEditorRender::Update()
{
    using namespace IconEditorTests;
    UWorld *World = GEditor->PlayWorld;
    auto *Controller = World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
    auto *Game = World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
    if (!Controller || !Game || !Terminal.IsValid())
        return true;
    Controller->CloseIconEditorPanel();
    if(auto* Hud=Controller->GetGameHudWidget();Hud&&Hud->GetHudMode()!=ETunaSweeperHudMode::None)
    {
        Hud->SetHudMode(ETunaSweeperHudMode::None);return false;
    }
    if(Controller->IsInventoryUiOpen())
    {
        if(FPlatformTime::Seconds()-StartTime<45)return false;
        Test->AddError(TEXT("HUD did not finish closing before localized render"));return true;
    }
    Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(Language), false);
    if (!Test->TestTrue(TEXT("Open localized editor for render"), Terminal->OpenFor(Controller->GetPawn())))
        return true;
    auto *UI = Panel(World);
    if (!UI)
        return true;
    FWidgetRenderer Renderer(false);
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("IconEditor/Screenshots");
    IFileManager::Get().MakeDirectory(*Dir, true);
    for (const FIntPoint Size : {FIntPoint(1920, 1080), FIntPoint(1280, 720)})
    {
        const TSharedRef<SWidget> Slate = UI->TakeWidget();
        TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(Size)));
        if (!Test->TestNotNull(TEXT("Offscreen render target"), Target.Get()))
            continue;
        FlushRenderingCommands();
        Renderer.DrawWidget(Target.Get(), Slate, FVector2D(Size), 0);
        FlushRenderingCommands();
        FImage Image;
        if (Test->TestTrue(TEXT("Read icon editor render"), FImageUtils::GetRenderTargetImage(Target.Get(), Image)))
        {
            Image.GammaSpace = EGammaSpace::Linear;
            Test->TestTrue(TEXT("Save localized screen"),
                           FImageUtils::SaveImageByExtension(
                               *(Dir / FString::Printf(TEXT("icon_%d_%d.png"), Language, Size.Y)), Image));
        }
    }
    return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND(FIconEditorCleanup);
bool FIconEditorCleanup::Update()
{
    using namespace IconEditorTests;
    if (UWorld *World = GEditor->PlayWorld)
    {
        if (auto *Controller = Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()))
            Controller->CloseIconEditorPanel();
        if (auto *Game = World->GetGameInstance<UTunaSweeperGameInstance>(); Game && bHaveOriginalLanguage)
            Game->SetCurrentTextLanguage(OriginalLanguage, false);
    }
    Terminal.Reset();
    bHaveOriginalLanguage = false;
    return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND(FIconEditorRestoreSaveRoot);
bool FIconEditorRestoreSaveRoot::Update()
{
    TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIconEditorPIETest, "TunaSweeper.IconEditor.IntegrationAndVisual",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FIconEditorPIETest::RunTest(const FString &)
{
    IconEditorTests::Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") /
                            (TEXT("IconEditor-") + FGuid::NewGuid().ToString());
    if (!TestTrue(TEXT("Isolated test save root enabled"),
                  TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(IconEditorTests::Root)))
        return false;
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("MainRaid/RaidPlains.umap"), false, true))
    {
        TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString());
        return false;
    }
    FRequestPlaySessionParams Play;
    Play.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
    Play.EditorPlaySettings->NewWindowWidth = 1280;
    Play.EditorPlaySettings->NewWindowHeight = 720;
    GEditor->RequestPlaySession(Play);
    ADD_LATENT_AUTOMATION_COMMAND(FIconEditorExercise(this));
    for (int32 Language = 0; Language < 6; ++Language)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FIconEditorRender(this, Language));
    }
    ADD_LATENT_AUTOMATION_COMMAND(FIconEditorCleanup());
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FIconEditorRestoreSaveRoot());
    return true;
}
#endif
