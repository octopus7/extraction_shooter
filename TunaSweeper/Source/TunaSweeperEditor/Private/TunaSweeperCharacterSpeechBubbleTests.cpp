#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaCharacterSpeechBubbleDesignerTest,
    "TunaSweeper.UI.SpeechBubble.DesignerAsset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaCharacterSpeechBubbleDesignerTest::RunTest(const FString& Parameters)
{
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr,
        TEXT("/Game/UI/SpeechBubble/WBP_CharacterSpeechBubble.WBP_CharacterSpeechBubble"));
    if (!TestNotNull(TEXT("Editable character bubble WBP exists"), BP)) return false;
    TestTrue(TEXT("Saved WBP compiles"), BP->Status != BS_Error && BP->GeneratedClass);
    if (!BP->GeneratedClass || !TestNotNull(TEXT("Designer hierarchy is serialized"), BP->WidgetTree.Get())) return false;
    TStrongObjectPtr<UTunaSweeperScreenSpaceSpeechBubbleWidget> Widget(
        CreateWidget<UTunaSweeperScreenSpaceSpeechBubbleWidget>(GEditor->GetEditorWorldContext().World(), BP->GeneratedClass.Get()));
    if (!TestNotNull(TEXT("Saved class instantiates"), Widget.Get())) return false;
    auto Slate = Widget->TakeWidget();
    UWidget* Root = Widget->WidgetTree->RootWidget;
    auto* Text = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("BubbleText")));
    auto* Body = Cast<USizeBox>(Widget->WidgetTree->FindWidget(TEXT("BodyBox")));
    auto* Tail = Cast<USizeBox>(Widget->WidgetTree->FindWidget(TEXT("TailBox")));
    auto* Image = Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("BodyImage")));
    if (!TestNotNull(TEXT("Bound text"), Text) || !TestNotNull(TEXT("Body"), Body)
        || !TestNotNull(TEXT("Tail"), Tail) || !TestNotNull(TEXT("Designer brush"), Image)) return false;
    TArray<UWidget*> Authored, Instanced;
    BP->WidgetTree->GetAllWidgets(Authored);
    Widget->WidgetTree->GetAllWidgets(Instanced);
    TestEqual(TEXT("No extra runtime widget construction"), Instanced.Num(), Authored.Num());

    // Simulate designer edits on an instance: runtime configuration must preserve them.
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = 31;
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
    FSlateBrush Brush = Image->GetBrush();
    Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
    Brush.TintColor = FSlateColor(FLinearColor::Blue);
    Image->SetBrush(Brush);
    Body->ClearMinDesiredWidth();
    Body->ClearMinDesiredHeight();
    Body->SetWidthOverride(100.0f);
    Body->SetHeightOverride(40.0f);
    Tail->SetWidthOverride(50.0f);
    Tail->SetHeightOverride(50.0f);
    const FText Message = FText::AsNumber(123);
    for (int32 Direction = 0; Direction < 9; ++Direction)
    {
        const auto Value = static_cast<ETunaSweeperSpeechBubbleTailDirection>(Direction);
        Widget->Configure(Message, Value);
        Widget->ForceLayoutPrepass();
        TestTrue(TEXT("Runtime text still updates"), Text->GetText().EqualTo(Message));
        TestEqual(TEXT("Direction is retained"), Widget->GetTailDirection(), Value);
        TestEqual(TEXT("Designer root survives Configure"), Widget->WidgetTree->RootWidget.Get(), Root);
        TestEqual(TEXT("Designer font size survives"), Text->GetFont().Size, Font.Size);
        TestTrue(TEXT("Designer text color survives"), Text->GetColorAndOpacity() == FSlateColor(FLinearColor::Green));
        TestTrue(TEXT("Designer image brush survives"), Image->GetBrush() == Brush);
        TestEqual(TEXT("No-tail mode collapses tail"), Tail->GetVisibility(),
            Value == ETunaSweeperSpeechBubbleTailDirection::None ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
    Widget->Configure(Message, ETunaSweeperSpeechBubbleTailDirection::None);
    Widget->ForceLayoutPrepass();
    TestTrue(TEXT("Smaller designer body remains centered, without native minimum size"),
        Widget->GetLocalAnchorPoint().Equals(FVector2D(50.0, 20.0), 0.1));
    Widget->Configure(Message, ETunaSweeperSpeechBubbleTailDirection::Down);
    Widget->ForceLayoutPrepass();
    TestTrue(TEXT("Resized designer tail remains anchored at its tip"),
        Widget->GetLocalAnchorPoint().Equals(FVector2D(50.0, 63.0), 0.1));
    Widget->ReleaseSlateResources(true);
    Slate = Widget->TakeWidget();
    TestEqual(TEXT("Slate rebuild retains designer tree"), Widget->WidgetTree->RootWidget.Get(), Root);
    TestEqual(TEXT("Slate rebuild retains designer font"), Text->GetFont().Size, Font.Size);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaCharacterSpeechBubbleSilhouetteTest,
    "TunaSweeper.UI.SpeechBubble.ContinuousSilhouette",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaCharacterSpeechBubbleSilhouetteTest::RunTest(const FString& Parameters)
{
    auto* Class = LoadClass<UTunaSweeperScreenSpaceSpeechBubbleWidget>(nullptr,
        TEXT("/Game/UI/SpeechBubble/WBP_CharacterSpeechBubble.WBP_CharacterSpeechBubble_C"));
    if (!TestNotNull(TEXT("Saved WBP class"), Class)) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    FlushRenderingCommands();
    const FString Output = FPaths::ProjectSavedDir() / TEXT("Tests/CharacterSpeechBubble");
    IFileManager::Get().MakeDirectory(*Output, true);
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    FWidgetRenderer Renderer(true);
    for (int32 Direction = 0; Direction < 9; ++Direction)
    {
        TStrongObjectPtr<UTunaSweeperScreenSpaceSpeechBubbleWidget> Widget(
            CreateWidget<UTunaSweeperScreenSpaceSpeechBubbleWidget>(GEditor->GetEditorWorldContext().World(), Class));
        auto Slate = Widget->TakeWidget();
        auto* Body = Cast<USizeBox>(Widget->WidgetTree->FindWidget(TEXT("BodyBox")));
        Body->SetWidthOverride(200); Body->SetHeightOverride(70);
        Widget->Configure(FText::GetEmpty(), static_cast<ETunaSweeperSpeechBubbleTailDirection>(Direction));
        Widget->ForceLayoutPrepass();
        Widget->GetLocalAnchorPoint(); // Updates geometry parameters after layout.
        const FVector2D Size = Widget->GetDesiredSize();
        TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, Size));
        if (!TestNotNull(TEXT("Rendered bubble"), Target.Get())) return false;
        // Slate requests its shader variants on the render thread during the first draw.
        FlushRenderingCommands();
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        FlushRenderingCommands();
        Slate->Invalidate(EInvalidateWidgetReason::Paint);
        Renderer.DrawWidget(Target.Get(), Slate, Size, 0.0f);
        FlushRenderingCommands();
        TArray<FColor> Pixels;
        Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        const int32 W = Target->SizeX, H = Target->SizeY;
        if (!TestEqual(TEXT("Readback covers the render target"), Pixels.Num(), W*H)) return false;
        TestTrue(TEXT("Outside shape stays transparent; no fallback rectangle"), Pixels[0].A < 128);
        TestTrue(TEXT("Body center is opaque"), Pixels[(H/2)*W+W/2].A > 250);
        TArray64<uint8> PNG;
        FImageUtils::PNGCompressImageArray(W, H, Pixels, PNG);
        FFileHelper::SaveArrayToFile(PNG, *(Output / FString::Printf(TEXT("direction_%d.png"), Direction)));
        // A separated tail would produce two disconnected opaque components.
        TSet<int32> Remaining;
        for (int32 Index=0; Index<Pixels.Num(); ++Index) if (Pixels[Index].A > 128) Remaining.Add(Index);
        int32 Components = 0;
        while (!Remaining.IsEmpty())
        {
            ++Components;
            TArray<int32> Queue;
            Queue.Add(*Remaining.CreateConstIterator()); Remaining.Remove(Queue[0]);
            for (int32 Head=0; Head<Queue.Num(); ++Head)
            {
                const int32 Current=Queue[Head], X=Current%W, Y=Current/W;
                // Diagonal tip pixels touch at their corners after antialiasing.
                for (int32 DY = -1; DY <= 1; ++DY)
                for (int32 DX = -1; DX <= 1; ++DX)
                {
                    const int32 NX = X + DX, NY = Y + DY;
                    if (NX >= 0 && NX < W && NY >= 0 && NY < H)
                    {
                        const int32 Next = NY * W + NX;
                        if (Remaining.Remove(Next)) Queue.Add(Next);
                    }
                }
            }
        }
        TestEqual(*FString::Printf(TEXT("Direction %d has one continuous silhouette"), Direction), Components, 1);
        if (Direction == static_cast<int32>(ETunaSweeperSpeechBubbleTailDirection::Down))
        {
            const FColor Center = Pixels[(H/3)*W + W/2];
            const FColor Join = Pixels[69*W + W/2];
            TestTrue(TEXT("No internal outline across the body/tail join"),
                Join.A > 250 && FMath::Abs(int32(Join.R)-Center.R) < 5
                && FMath::Abs(int32(Join.G)-Center.G) < 5);
        }
    }
    return true;
}
#endif
