#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Game/TunaSweeperGameInstance.h"
#include "HAL/FileManager.h"
#include "IconEditor/IconEditorTerminal.h"
#include "IconEditor/IconStorageSubsystem.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Layout/ArrangedChildren.h"
#include "Layout/WidgetPath.h"
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
#include "UI/Astra/IconByteSpinBoxGPT6Astra.h"
#include "UI/Astra/IconCanvasGPT6Astra.h"
#include "UI/Astra/IconEditorGPT6Astra.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWindow.h"
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
    static const uint8 *HistoryData(const UIconEditorGPT6Astra *P)
    {
        return P->History.GetData();
    }
    static int32 HistoryCapacity(const UIconEditorGPT6Astra *P)
    {
        return P->History.Max();
    }
    static int32 HistoryCount(const UIconEditorGPT6Astra *P)
    {
        return P->HistoryCount;
    }
    static int32 HistoryCursor(const UIconEditorGPT6Astra *P)
    {
        return P->HistoryCursor;
    }
    static bool Drawing(const UIconCanvasGPT6Astra *C)
    {
        return C->bDrawing;
    }
    static void LoseCapture(UIconCanvasGPT6Astra *C)
    {
        C->NativeOnMouseCaptureLost(FCaptureLostEvent());
    }
    static FReply Shortcut(UIconEditorGPT6Astra *P, FKey Key, bool Shift = false)
    {
        return P->NativeOnPreviewKeyDown(
            FGeometry(),
            FKeyEvent(Key, FModifierKeysState(Shift, false, true, false, false, false, false, false, false), 0, false,
                      0, 0));
    }
    static int32 Preset(const UIconEditorGPT6Astra *Panel)
    {
        return Panel->ActivePreset;
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
    TestEqual(TEXT("Every required binding inspected"), BindingCount, 52);
    for (const TCHAR *Name :
         {TEXT("BrushText"), TEXT("EyedropperText"), TEXT("FillText"), TEXT("CancelText"), TEXT("ConfirmText")})
    {
        auto *Label = Cast<UTextBlock>(BPClass->GetWidgetTreeArchetype()->FindWidget(Name));
        if (TestNotNull(Name, Label))
            TestFalse(FString(Name) + TEXT(" keeps short translated button text on one line"),
                      Label->GetAutoWrapText());
    }
    for (const TCHAR *Prefix : {TEXT("Red"), TEXT("Green"), TEXT("Blue")})
    {
        TestNotNull(TEXT("Split-contrast native spin control"),
                    Cast<UIconByteSpinBoxGPT6Astra>(
                        BPClass->GetWidgetTreeArchetype()->FindWidget(FName(*(FString(Prefix) + TEXT("Input"))))));
        for (const TCHAR *Suffix : {TEXT("Decrease"), TEXT("Increase")})
        {
            auto *Button =
                Cast<UButton>(BPClass->GetWidgetTreeArchetype()->FindWidget(FName(*(FString(Prefix) + Suffix))));
            if (!TestNotNull(TEXT("Authored step button"), Button))
                continue;
            auto *GlyphSize = Cast<USizeBox>(Button->GetContent());
            if (TestNotNull(TEXT("Authored vector glyph bounds"), GlyphSize))
            {
                auto *Strokes = Cast<UOverlay>(GlyphSize->GetContent());
                if (TestNotNull(TEXT("Font-independent stroke icon"), Strokes))
                    TestEqual(TEXT("Minus has one stroke; plus has two"), Strokes->GetChildrenCount(),
                              FString(Suffix) == TEXT("Increase") ? 2 : 1);
            }
            auto *Slot = Cast<UCanvasPanelSlot>(Button->Slot);
            if (TestNotNull(TEXT("Step button canvas slot"), Slot))
                TestEqual(TEXT("Circular face uses square dimensions"), Slot->GetSize(), FVector2D(30, 30));
            TestEqual(TEXT("Circle has half-diameter corner radius"),
                      Button->GetStyle().Normal.OutlineSettings.CornerRadii.X, 15.0);
        }
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
    const uint8 *HistoryAddress = FIconEditorAutomationAccess::HistoryData(UI);
    const int32 HistoryCapacity = FIconEditorAutomationAccess::HistoryCapacity(UI);
    Test->TestEqual(TEXT("One baseline and twenty full snapshots"), HistoryCapacity,
                    21 * UIconStorageSubsystem::ByteCount);
    Test->TestEqual(TEXT("Initial history baseline"), FIconEditorAutomationAccess::HistoryCount(UI), 1);
    Test->TestFalse(TEXT("Undo initially disabled"),
                    Cast<UButton>(UI->GetWidgetFromName(TEXT("UndoButton")))->GetIsEnabled());
    Click(Test, UI, TEXT("Palette03"));
    Cast<UIconByteSpinBoxGPT6Astra>(UI->GetWidgetFromName(TEXT("RedInput")))
        ->SetEditingTextForAutomation(FText::AsNumber(244));
    Test->TestFalse(TEXT("RGB text editing retains native undo"),
                    FIconEditorAutomationAccess::Shortcut(UI, EKeys::Z).IsEventHandled());
    Stroke(Canvas, FIntPoint(0, 0), FIntPoint(31, 31));
    Test->TestEqual(TEXT("Continuous stroke adds exactly one snapshot"), FIconEditorAutomationAccess::HistoryCount(UI),
                    2);
    Test->TestTrue(TEXT("Ctrl Z handles pixel undo after RGB edit then drawing"),
                   FIconEditorAutomationAccess::Shortcut(UI, EKeys::Z).IsEventHandled());
    Test->TestTrue(TEXT("Stroke undo restores exact loaded bytes"), FIconEditorAutomationAccess::Draft(UI) == Baseline);
    FIconEditorAutomationAccess::Shortcut(UI, EKeys::Z, true);
    Test->TestEqual(TEXT("Ctrl Shift Z redo cursor"), FIconEditorAutomationAccess::HistoryCursor(UI), 1);
    Click(Test, UI, TEXT("UndoButton"));
    FIconEditorAutomationAccess::Shortcut(UI, EKeys::Y);
    Test->TestEqual(TEXT("Ctrl Y redo cursor"), FIconEditorAutomationAccess::HistoryCursor(UI), 1);
    // Complete an interrupted drag on capture loss, and make the next move inert.
    const FGeometry HistoryGeo = FGeometry::MakeRoot(FVector2D(576), FSlateLayoutTransform());
    FIconEditorAutomationAccess::Down(Canvas, HistoryGeo, Pointer(FVector2D(9, 27), EKeys::LeftMouseButton, true));
    Test->TestEqual(TEXT("Pending stroke has no intermediate snapshot"), FIconEditorAutomationAccess::HistoryCount(UI),
                    2);
    FIconEditorAutomationAccess::LoseCapture(Canvas);
    Test->TestEqual(TEXT("Capture loss commits one stroke"), FIconEditorAutomationAccess::HistoryCount(UI), 3);
    Test->TestFalse(TEXT("Capture loss ends drawing"), FIconEditorAutomationAccess::Drawing(Canvas));
    const auto AfterLoss = FIconEditorAutomationAccess::Draft(UI);
    FIconEditorAutomationAccess::Move(Canvas, HistoryGeo, Pointer(FVector2D(500, 9), EKeys::LeftMouseButton, true));
    Test->TestTrue(TEXT("No painting after capture loss"), FIconEditorAutomationAccess::Draft(UI) == AfterLoss);
    // More than twenty edits forces ring wrap without reallocating its storage.
    for (int32 I = 1; I <= 25; ++I)
    {
        Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")))->SetValue(I);
        Click(Test, UI, TEXT("FillButton"));
        Test->TestTrue(TEXT("History address stable while pushing"),
                       FIconEditorAutomationAccess::HistoryData(UI) == HistoryAddress);
        Test->TestEqual(TEXT("History capacity stable while pushing"), FIconEditorAutomationAccess::HistoryCapacity(UI),
                        HistoryCapacity);
    }
    for (int32 I = 0; I < 20; ++I)
        Click(Test, UI, TEXT("UndoButton"));
    Test->TestEqual(TEXT("Exactly twenty undos retain edit five"), int32(FIconEditorAutomationAccess::Draft(UI)[0]), 5);
    Click(Test, UI, TEXT("UndoButton"));
    Test->TestEqual(TEXT("Undo limit does not move"), int32(FIconEditorAutomationAccess::Draft(UI)[0]), 5);
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")))->SetValue(5);
    Click(Test, UI, TEXT("FillButton"));
    Stroke(Canvas, FIntPoint(0, 0), FIntPoint(31, 31));
    Test->TestEqual(TEXT("No-op fill and stroke preserve redo branch"), FIconEditorAutomationAccess::HistoryCount(UI),
                    21);
    Test->TestEqual(TEXT("No-op leaves cursor"), FIconEditorAutomationAccess::HistoryCursor(UI), 0);
    for (int32 I = 0; I < 10; ++I)
        Click(Test, UI, TEXT("RedoButton"));
    Test->TestEqual(TEXT("Redo restores exact ring state"), int32(FIconEditorAutomationAccess::Draft(UI)[0]), 15);
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")))->SetValue(99);
    Click(Test, UI, TEXT("FillButton"));
    Test->TestEqual(TEXT("New edit truncates redo branch"), FIconEditorAutomationAccess::HistoryCount(UI), 12);
    Test->TestFalse(TEXT("Redo disabled after branching"),
                    Cast<UButton>(UI->GetWidgetFromName(TEXT("RedoButton")))->GetIsEnabled());
    Test->TestTrue(TEXT("History pointer stable through undo redo branch"),
                   FIconEditorAutomationAccess::HistoryData(UI) == HistoryAddress);
    Test->TestEqual(TEXT("History capacity stable through undo redo branch"),
                    FIconEditorAutomationAccess::HistoryCapacity(UI), HistoryCapacity);
    // Undo must finalize a currently held brush stroke before restoring the previous snapshot.
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")))->SetValue(100);
    FIconEditorAutomationAccess::Down(Canvas, HistoryGeo, Pointer(FVector2D(9, 9), EKeys::LeftMouseButton, true));
    Click(Test, UI, TEXT("UndoButton"));
    Test->TestEqual(TEXT("Undo finalizes pending stroke"), int32(FIconEditorAutomationAccess::Draft(UI)[0]), 99);
    Test->TestFalse(TEXT("Undo ends captured drawing"), FIconEditorAutomationAccess::Drawing(Canvas));
    Click(Test, UI, TEXT("CancelButton"));
    Test->TestTrue(TEXT("Cancel history changes never writes"), Read(Storage) == Baseline);
    if (!Test->TestTrue(TEXT("Reopen after history exercise"), Terminal->OpenFor(Player)))
        return true;
    UI = Panel(World);
    Canvas = Cast<UIconCanvasGPT6Astra>(UI->GetWidgetFromName(TEXT("PixelCanvas")));
    Test->TestEqual(TEXT("Reopen resets history to loaded baseline"), FIconEditorAutomationAccess::HistoryCount(UI), 1);
    Test->TestTrue(TEXT("Reopen loads saved bytes"), FIconEditorAutomationAccess::Draft(UI) == Baseline);
    Test->TestEqual(TEXT("Opening defaults to preset one"), FIconEditorAutomationAccess::Preset(UI), 0);
    Test->TestNull(TEXT("Palette title removed"), UI->GetWidgetFromName(TEXT("PaletteText")));
    const TArray<uint8> BeforePresets = FIconEditorAutomationAccess::Draft(UI);
    TSet<FString> UniquePresets;
    for (int32 P = 0; P < 6; ++P)
    {
        const FColor BeforeColor = FIconEditorAutomationAccess::Color(UI);
        Click(Test, UI, *FString::Printf(TEXT("Preset%d"), P + 1));
        Test->TestEqual(TEXT("Tab selects its preset"), FIconEditorAutomationAccess::Preset(UI), P);
        Test->TestEqual(TEXT("Tab preserves current RGB"), FIconEditorAutomationAccess::Color(UI), BeforeColor);
        Test->TestTrue(TEXT("Tab preserves draft"), FIconEditorAutomationAccess::Draft(UI) == BeforePresets);
        FString Fingerprint;
        for (int32 I = 0; I < 6; ++I)
        {
            auto *Tab = Cast<UButton>(UI->GetWidgetFromName(FName(*FString::Printf(TEXT("Preset%d"), I + 1))));
            auto *Label = Cast<UTextBlock>(Tab->GetContent());
            const FLinearColor Bg = Tab->GetStyle().Normal.TintColor.GetSpecifiedColor();
            const FLinearColor Fg = Label->GetColorAndOpacity().GetSpecifiedColor();
            Test->TestTrue(TEXT("Selected bright face with dark number, others inverse"),
                           I == P ? Bg.R > .8f && Fg.R < .1f : Bg.R < .1f && Fg.R > .8f);
            auto *Slot = Cast<UCanvasPanelSlot>(Tab->Slot);
            if (Test->TestNotNull(TEXT("Authored tab canvas slot"), Slot))
                Test->TestEqual(TEXT("Tab dimensions"), Slot->GetSize(), FVector2D(26, 26));
            Test->TestEqual(TEXT("Tab rounded rectangle radius"), Tab->GetStyle().Normal.OutlineSettings.CornerRadii.X,
                            6.0);
        }
        for (int32 I = 0; I < 12; ++I)
        {
            const FString Name = FString::Printf(TEXT("Palette%02d"), I);
            auto *Swatch = Cast<UButton>(UI->GetWidgetFromName(FName(*Name)));
            Click(Test, UI, *Name);
            const FColor Chosen = FIconEditorAutomationAccess::Color(UI);
            Test->TestTrue(
                TEXT("Swatch color matches displayed preset RGB"),
                FLinearColor(Chosen).Equals(Swatch->GetStyle().Normal.TintColor.GetSpecifiedColor(), .0001f));
            Test->TestTrue(TEXT("Picking colors preserves draft"),
                           FIconEditorAutomationAccess::Draft(UI) == BeforePresets);
            Fingerprint += Chosen.ToHex();
        }
        UniquePresets.Add(Fingerprint);
    }
    Test->TestEqual(TEXT("Six distinct twelve-color presets"), UniquePresets.Num(), 6);
    Click(Test, UI, TEXT("Preset1"));
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
    for (const TCHAR *Prefix : {TEXT("Red"), TEXT("Green"), TEXT("Blue")})
    {
        auto *Input = Cast<UIconByteSpinBoxGPT6Astra>(UI->GetWidgetFromName(FName(*(FString(Prefix) + TEXT("Input")))));
        if (!Test->TestNotNull(TEXT("Byte input instance"), Input))
            return true;
        Input->SetValue(100);
        {
            // Route real SSpinBox pointer handlers with Slate capture; no direct value manipulation simulates drag.
            const auto Slate = Input->TakeWidget();
            const FGeometry SpinGeometry = FGeometry::MakeRoot(FVector2D(268, 34), FSlateLayoutTransform());
            const auto Window = SNew(SWindow);
            FArrangedChildren PathWidgets(EVisibility::All);
            PathWidgets.AddWidget(FArrangedWidget(Window, SpinGeometry));
            PathWidgets.AddWidget(FArrangedWidget(Slate, SpinGeometry));
            const FWidgetPath Path(Window, PathWidgets);
            TSet<FKey> Held;
            Held.Add(EKeys::LeftMouseButton);
            const uint32 PointerIndex = FSlateApplication::CursorPointerIndex;
            FPointerEvent Down(PointerIndex, FVector2D(100, 17), FVector2D(100, 17), Held, EKeys::LeftMouseButton, 0,
                               FModifierKeysState());
            Slate->OnMouseButtonDown(SpinGeometry, Down);
            Test->TestTrue(TEXT("Native spin obtains test pointer capture"),
                           FSlateApplication::Get().GetUser(0)->SetPointerCaptor(PointerIndex, Slate, Path));
            FPointerEvent Move1(PointerIndex, FVector2D(120, 17), FVector2D(100, 17), Held, EKeys::LeftMouseButton, 0,
                                FModifierKeysState());
            FPointerEvent Move2(PointerIndex, FVector2D(150, 17), FVector2D(120, 17), Held, EKeys::LeftMouseButton, 0,
                                FModifierKeysState());
            Slate->OnMouseMove(SpinGeometry, Move1);
            Slate->OnMouseMove(SpinGeometry, Move2);
            Slate->Tick(SpinGeometry, FPlatformTime::Seconds(), 0.016f);
            Test->TestTrue(TEXT("Native horizontal drag changes RGB value"), Input->GetValue() > 100);
            FPointerEvent Up(PointerIndex, FVector2D(150, 17), FVector2D(150, 17), TSet<FKey>(), EKeys::LeftMouseButton,
                             0, FModifierKeysState());
            Slate->OnMouseButtonUp(SpinGeometry, Up);
            FSlateApplication::Get().GetUser(0)->ReleaseCapture(PointerIndex);
        }
        Input->SetValue(127);
        Click(Test, UI, *(FString(Prefix) + TEXT("Increase")));
        Test->TestEqual(TEXT("Plus increments by one"), Input->GetValue(), 128.0f);
        Click(Test, UI, *(FString(Prefix) + TEXT("Decrease")));
        Test->TestEqual(TEXT("Minus decrements by one"), Input->GetValue(), 127.0f);
        Input->SetValue(0);
        Click(Test, UI, *(FString(Prefix) + TEXT("Decrease")));
        Test->TestEqual(TEXT("Minus clamps at zero"), Input->GetValue(), 0.0f);
        Input->SetValue(255);
        Click(Test, UI, *(FString(Prefix) + TEXT("Increase")));
        Test->TestEqual(TEXT("Plus clamps at 255"), Input->GetValue(), 255.0f);
        Input->SetEditingTextForAutomation(FText::AsNumber(160));
        Input->CommitEditingTextForAutomation(FText::AsNumber(160));
        Test->TestEqual(TEXT("Native typed editing still commits"), Input->GetValue(), 160.0f);
        Input->SetEditingTextForAutomation(FText::AsNumber(999));
        Input->CommitEditingTextForAutomation(FText::AsNumber(999));
        Test->TestEqual(TEXT("Typed editing clamps to byte maximum"), Input->GetValue(), 255.0f);
    }
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
    Click(Test, UI, TEXT("Preset6"));
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
        Test->TestEqual(TEXT("Reopening resets transient preset to one"), FIconEditorAutomationAccess::Preset(UI), 0);
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
    if (auto *Hud = Controller->GetGameHudWidget(); Hud && Hud->GetHudMode() != ETunaSweeperHudMode::None)
    {
        Hud->SetHudMode(ETunaSweeperHudMode::None);
        return false;
    }
    if (Controller->IsInventoryUiOpen())
    {
        if (FPlatformTime::Seconds() - StartTime < 45)
            return false;
        Test->AddError(TEXT("HUD did not finish closing before localized render"));
        return true;
    }
    Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(Language), false);
    if (!Test->TestTrue(TEXT("Open localized editor for render"), Terminal->OpenFor(Controller->GetPawn())))
        return true;
    auto *UI = Panel(World);
    if (!UI)
        return true;
    for (const auto &Pair :
         {TPair<FName, FString>(TEXT("RedText"), TEXT("R")), TPair<FName, FString>(TEXT("GreenText"), TEXT("G")),
          TPair<FName, FString>(TEXT("BlueText"), TEXT("B"))})
        Test->TestEqual(TEXT("RGB channel label is language-independent"),
                        Cast<UTextBlock>(UI->GetWidgetFromName(Pair.Key))->GetText().ToString(), Pair.Value);
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("RedInput")))->SetValue(160);
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("GreenInput")))->SetValue(105);
    Cast<USpinBox>(UI->GetWidgetFromName(TEXT("BlueInput")))->SetValue(183);
    for (int32 I = 1; I <= 6; ++I)
    {
        auto *Tab = Cast<UButton>(UI->GetWidgetFromName(FName(*FString::Printf(TEXT("Preset%d"), I))));
        Test->TestEqual(TEXT("Localized numeric tab has no alias"),
                        Cast<UTextBlock>(Tab->GetContent())->GetText().ToString(), FString::FromInt(I));
    }
    // Render linear values and explicitly convert once to display sRGB for PNG export.
    FWidgetRenderer Renderer(false);
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("IconEditor/Screenshots");
    IFileManager::Get().MakeDirectory(*Dir, true);
    for (int32 Preset = 0; Preset < (Language == 0 ? 6 : 1); ++Preset)
    {
        Click(Test, UI, *FString::Printf(TEXT("Preset%d"), Preset + 1));
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
                FImage DisplayImage;
                Image.CopyTo(DisplayImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
                if (Size.Y == 1080)
                {
                    const FColor DisplayColor =
                        reinterpret_cast<const FColor *>(DisplayImage.RawData.GetData())[420 * Size.X + 1330];
                    Test->TestTrue(FString::Printf(TEXT("Display RGB fidelity: %d,%d,%d"), DisplayColor.R,
                                                   DisplayColor.G, DisplayColor.B),
                                   FMath::Abs(int32(DisplayColor.R) - 160) <= 2 &&
                                       FMath::Abs(int32(DisplayColor.G) - 105) <= 2 &&
                                       FMath::Abs(int32(DisplayColor.B) - 183) <= 2);
                }
                Test->TestTrue(
                    TEXT("Save localized screen"),
                    FImageUtils::SaveImageByExtension(
                        *(Dir / FString::Printf(TEXT("icon_%d_preset%d_%d.png"), Language, Preset + 1, Size.Y)),
                        DisplayImage));
            }
        }
    }
    if (Language == 0)
    {
        auto *Input = Cast<UIconByteSpinBoxGPT6Astra>(UI->GetWidgetFromName(TEXT("RedInput")));
        Input->SetValue(18);
        for (int32 Editing = 0; Editing < 2; ++Editing)
        {
            if (Editing)
                Input->SetEditingTextForAutomation(FText::AsNumber(18));
            const TSharedRef<SWidget> Slate = Input->TakeWidget();
            TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(268, 34)));
            FlushRenderingCommands();
            Renderer.DrawWidget(Target.Get(), Slate, FVector2D(268, 34), 0);
            FlushRenderingCommands();
            FImage Image;
            if (Test->TestTrue(TEXT("Read split-glyph contrast image"),
                               FImageUtils::GetRenderTargetImage(Target.Get(), Image)))
            {
                Image.GammaSpace = EGammaSpace::Linear;
                FImage DisplayImage;
                Image.CopyTo(DisplayImage, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
                int32 LightGlyphPixels = 0, DarkGlyphPixels = 0;
                for (int32 Y = 3; Y < 31; ++Y)
                    for (int32 X = 8; X < 48; ++X)
                    {
                        const FLinearColor P = ERawImageFormat::GetOnePixelLinear(
                            Image.RawData.GetData() + Image.GetPixelOffsetBytes(X, Y), Image.Format, Image.GammaSpace);
                        if (X < 18 && P.R > 0.75f && P.G > 0.75f && P.B > 0.75f)
                            ++LightGlyphPixels;
                        if (X > 20 && P.R < 0.12f && P.G < 0.12f && P.B < 0.12f)
                            ++DarkGlyphPixels;
                    }
                Test->TestTrue(FString::Printf(TEXT("%s glyph pixels are light over dark fill"),
                                               Editing ? TEXT("Editing") : TEXT("Display")),
                               LightGlyphPixels > 3);
                Test->TestTrue(FString::Printf(TEXT("%s glyph pixels are dark over light remainder"),
                                               Editing ? TEXT("Editing") : TEXT("Display")),
                               DarkGlyphPixels > 3);
                Test->TestTrue(TEXT("Save split-glyph capture"),
                               FImageUtils::SaveImageByExtension(
                                   *(Dir / FString::Printf(TEXT("rgb_split_%d.png"), Editing)), DisplayImage));
            }
        }
        Input->CommitEditingTextForAutomation(FText::AsNumber(18));
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
