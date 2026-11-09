#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Scenario/TunaSweeperEndingPresentation.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Layout/SBorder.h"

namespace FreePlayEndingPresentationTests
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* GI, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; GI->*Member = Context; }
    };
    struct FDialogueAccess : UTunaSweeperDialogueWidget
    {
        static auto Key() { return &FDialogueAccess::NativeOnKeyDown; }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFreePlayEndingRenderTest,
    "TunaSweeper.FreePlayEnding.RenderStories", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperFreePlayEndingRenderTest::RunTest(const FString&)
{
    if (!FApp::CanEverRender()) { AddWarning(TEXT("Run with rendering enabled to inspect ending story screenshots.")); return true; }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FreePlayEndingPresentationTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FreePlayEndingPresentationTests::FContextAccess::Attach(Game.Get(), nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    World->InitializeActorsForPlay(FURL());
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine); LocalPlayer->SetControllerId(0); Controller->SetPlayer(LocalPlayer);
    Game->GetSubsystem<UTunaSweeperTextSubsystem>()->LoadTextData();
    Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
    for (const TCHAR* Variant : {TEXT("Normal"), TEXT("True")})
    {
        auto* Story = LoadObject<UTunaSweeperEndingPresentation>(nullptr,
            *FString::Printf(TEXT("/Game/Interactables/Ending/DA_Ending_%s.DA_Ending_%s"), Variant, Variant));
        if (!TestNotNull(TEXT("Story asset loads"), Story)) return false;
        TStrongObjectPtr<UTunaSweeperDialogueWidget> Widget(CreateWidget<UTunaSweeperDialogueWidget>(Controller));
        auto Slate = Widget->TakeWidget();
        TArray<FTunaSweeperDialogueLine> Lines;
        TestTrue(TEXT("All presentation keys resolve"), Story->ResolveDialogue(Game.Get(), Lines));
        TestEqual(TEXT("Title card and four story cards"), Lines.Num(), 5);
        for (const auto& Line : Lines)
        {
            TestFalse(TEXT("No literal newline escape in displayed text"), Line.DialogueText.ToString().Contains(TEXT("\\n")));
            TestTrue(TEXT("No title overflowing NPC-name tab"), Line.SpeakerName.IsEmpty());
        }
        auto* Invalid = DuplicateObject<UTunaSweeperEndingPresentation>(Story, GetTransientPackage());
        Invalid->LineStringKeys[0] = TEXT("missing.ending.test");
        TArray<FTunaSweeperDialogueLine> InvalidLines;
        TestFalse(TEXT("Missing authored key fails without exposing identifiers"), Invalid->ResolveDialogue(Game.Get(), InvalidLines));
        TestTrue(TEXT("Missing key does not leave partial story"), InvalidLines.IsEmpty());
        int32 Completed = 0;
        Widget->SetFinishedDelegate(FSimpleDelegate::CreateLambda([&] { ++Completed; }));
        Widget->StartDialogue(Lines, 20.f);
        auto Key = [&] { (Widget.Get()->*FreePlayEndingPresentationTests::FDialogueAccess::Key())(
            FGeometry(), FKeyEvent(EKeys::SpaceBar, FModifierKeysState(), 0, false, 0, 0)); };
        Key(); Key(); Key(); // Fill title, advance, fill first story card.
        auto* Body = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("DialogueBodyText")));
        TestTrue(TEXT("Localized ending text visible"), Body && !Body->GetText().IsEmpty());
        FAssetCompilingManager::Get().FinishAllCompilation();
        FWidgetRenderer Renderer(true);
        auto Screen = SNew(SBorder).BorderBackgroundColor(FLinearColor::Black).Padding(0)[Slate];
        TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Screen, FVector2D(1920,1080)));
        TArray<FColor> Pixels; FReadSurfaceDataFlags Flags; Flags.SetLinearToGamma(false);
        if (TestNotNull(TEXT("Ending render target"), Target.Get()) && Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))
        {
            TArray64<uint8> PNG; FImageUtils::PNGCompressImageArray(1920,1080,Pixels,PNG);
            TestTrue(TEXT("Ending screenshot written"), FFileHelper::SaveArrayToFile(PNG,
                *(FPaths::ProjectSavedDir()/FString::Printf(TEXT("FreePlayEnding-%s.png"),Variant))));
        }
        for (int32 Index=0; Index<7; ++Index) Key();
        TestEqual(TEXT("Title and four story lines finish once"), Completed, 1);
    }
    return true;
}
#endif
