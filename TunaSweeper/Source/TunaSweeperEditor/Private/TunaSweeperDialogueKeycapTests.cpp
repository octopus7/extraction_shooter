#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperDialogueKeycapTests
{
	constexpr float LoopSeconds = 1.3f;
	constexpr int32 FrameCount = 16;
	constexpr int32 GridSize = 4;
	constexpr float FrameUvSize = 0.25f;

	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};

	struct FDialogueWidgetAccess : UTunaSweeperDialogueWidget
	{
		using FNativeTickMember = decltype(&FDialogueWidgetAccess::NativeTick);
		using FNativeKeyMember = decltype(&FDialogueWidgetAccess::NativeOnKeyDown);

		static FNativeTickMember GetNativeTickMember()
		{
			return &FDialogueWidgetAccess::NativeTick;
		}

		static FNativeKeyMember GetNativeKeyMember()
		{
			return &FDialogueWidgetAccess::NativeOnKeyDown;
		}
	};

	void TickDialogue(UTunaSweeperDialogueWidget* Widget, float DeltaSeconds)
	{
		(Widget->*FDialogueWidgetAccess::GetNativeTickMember())(FGeometry(), DeltaSeconds);
	}

	FReply SendKey(UTunaSweeperDialogueWidget* Widget, const FKey& Key, bool bRepeat = false)
	{
		const FKeyEvent KeyEvent(Key, FModifierKeysState(), 0, bRepeat, 0, 0);
		return (Widget->*FDialogueWidgetAccess::GetNativeKeyMember())(FGeometry(), KeyEvent);
	}

	bool TestFrameUv(FAutomationTestBase& Test, UImage* Image, int32 FrameIndex)
	{
		if (!Image)
		{
			return false;
		}
		const FBox2f UvRegion = static_cast<FBox2f>(Image->GetBrush().GetUVRegion());
		const FVector2f ExpectedMin(
			static_cast<float>(FrameIndex % GridSize) * FrameUvSize,
			static_cast<float>(FrameIndex / GridSize) * FrameUvSize);
		const FString Prefix = FString::Printf(TEXT("Frame %d"), FrameIndex);
		Test.TestTrue(*FString::Printf(TEXT("%s U"), *Prefix), FMath::IsNearlyEqual(UvRegion.Min.X, ExpectedMin.X));
		Test.TestTrue(*FString::Printf(TEXT("%s V"), *Prefix), FMath::IsNearlyEqual(UvRegion.Min.Y, ExpectedMin.Y));
		Test.TestTrue(*FString::Printf(TEXT("%s width"), *Prefix), FMath::IsNearlyEqual(UvRegion.Max.X - UvRegion.Min.X, FrameUvSize));
		Test.TestTrue(*FString::Printf(TEXT("%s height"), *Prefix), FMath::IsNearlyEqual(UvRegion.Max.Y - UvRegion.Min.Y, FrameUvSize));
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperDialogueKeycapTest,
	"TunaSweeper.UI.Dialogue.KeycapAnimationAndAnyKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperDialogueKeycapTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperDialogueKeycapTests;
	(void)Parameters;

	UTexture2D* KeycapTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Dialogue/T_KeycapPress_4x4.T_KeycapPress_4x4"));
	if (!TestNotNull(TEXT("Keycap atlas loads"), KeycapTexture))
	{
		return false;
	}
	// Wait for editor texture compilation before querying the platform dimensions.
	FAssetCompilingManager::Get().FinishAllCompilation();
	TestEqual(TEXT("Keycap atlas width"), KeycapTexture->GetSizeX(), 1024);
	TestEqual(TEXT("Keycap atlas height"), KeycapTexture->GetSizeY(), 1024);
	TestEqual(TEXT("Keycap atlas uses UI texture group"), KeycapTexture->LODGroup, TEXTUREGROUP_UI);
	TestEqual(TEXT("Keycap atlas keeps UI alpha compression"), KeycapTexture->CompressionSettings, TC_EditorIcon);
	TestTrue(TEXT("Keycap atlas has an alpha channel"), KeycapTexture->HasAlphaChannel());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient dialogue world exists"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FWorldContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Dialogue controller exists"), Controller))
	{
		return false;
	}
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	Player->SetControllerId(0);
	Controller->SetPlayer(Player);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Dialogue text subsystem exists"), Strings) || !Strings->LoadTextData())
	{
		return false;
	}

	TStrongObjectPtr<UTunaSweeperDialogueWidget> Widget(CreateWidget<UTunaSweeperDialogueWidget>(
		Controller,
		UTunaSweeperDialogueWidget::StaticClass()));
	if (!TestNotNull(TEXT("Dialogue widget is created"), Widget.Get()))
	{
		return false;
	}
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	Widget->ForceLayoutPrepass();
	UImage* KeycapImage = Cast<UImage>(Widget->WidgetTree->FindWidget(TEXT("ContinueKeycapImage")));
	UTextBlock* DialogueBody = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("DialogueBodyText")));
	UTextBlock* ContinuePrompt = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("ContinuePromptText")));
	if (!TestNotNull(TEXT("Keycap image exists"), KeycapImage) ||
		!TestNotNull(TEXT("Dialogue body exists"), DialogueBody) ||
		!TestNotNull(TEXT("Localized continue prompt exists"), ContinuePrompt))
	{
		return false;
	}
	TestNull(TEXT("Legacy textual keycap is absent"), Widget->WidgetTree->FindWidget(TEXT("ContinueKeyText")));
	TestFalse(TEXT("Continue prompt resolves localized text"), ContinuePrompt->GetText().IsEmpty());
	TestEqual(TEXT("Image uses imported keycap atlas"), KeycapImage->GetBrush().GetResourceObject(), static_cast<UObject*>(KeycapTexture));

	TArray<FTunaSweeperDialogueLine> Lines;
	for (const TCHAR* Text : { TEXT("First line"), TEXT("Second line"), TEXT("Third line") })
	{
		FTunaSweeperDialogueLine& Line = Lines.AddDefaulted_GetRef();
		Line.SpeakerName = FText::FromString(TEXT("Tester"));
		Line.DialogueText = FText::FromString(Text);
	}
	int32 FinishedCount = 0;
	Widget->SetFinishedDelegate(FTunaSweeperDialogueFinishedDelegate::CreateLambda([&FinishedCount]() { ++FinishedCount; }));
	Widget->StartDialogue(Lines, 1.0f);
	TestTrue(TEXT("Dialogue starts running"), Widget->IsDialogueRunning());
	TestTrue(TEXT("First line begins empty"), DialogueBody->GetText().IsEmpty());
	TestEqual(TEXT("Keycap stays hidden during typewriter"), KeycapImage->GetVisibility(), ESlateVisibility::Collapsed);

	TestTrue(TEXT("A key is handled"), SendKey(Widget.Get(), EKeys::A).IsEventHandled());
	TestEqual(TEXT("A fills the current line"), DialogueBody->GetText().ToString(), FString(TEXT("First line")));
	TestEqual(TEXT("Keycap appears after line fills"), KeycapImage->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestFrameUv(*this, KeycapImage, 0);

	FAssetCompilingManager::Get().FinishAllCompilation();
	FlushRenderingCommands();
	FWidgetRenderer Renderer(true);
	TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(SlateWidget, FVector2D(1920.0f, 1080.0f)));
	auto Capture = [&](const TCHAR* Name)
	{
		if (!Target)
		{
			AddError(TEXT("Dialogue render target was not created"));
			return;
		}
		Widget->InvalidateLayoutAndVolatility();
		SlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
		Widget->ForceLayoutPrepass();
		Renderer.DrawWidget(Target.Get(), SlateWidget, FVector2D(1920.0f, 1080.0f), 0.0f);
		FlushRenderingCommands();
		FImage Pixels;
		if (!TestTrue(TEXT("Dialogue render pixels are readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Pixels)))
		{
			return;
		}
		const FString ScreenshotDirectory = FPaths::ProjectSavedDir() / TEXT("Screenshots");
		IFileManager::Get().MakeDirectory(*ScreenshotDirectory, true);
		TestTrue(TEXT("Dialogue screenshot is saved"), FImageUtils::SaveImageByExtension(
			*(ScreenshotDirectory / FString::Printf(TEXT("DialogueKeycap%s.png"), Name)),
			Pixels));
	};
	Capture(TEXT("Rest"));

	const float FrameStep = LoopSeconds / static_cast<float>(FrameCount) + KINDA_SMALL_NUMBER;
	float AdvancedSeconds = 0.0f;
	for (int32 FrameIndex = 1; FrameIndex < FrameCount; ++FrameIndex)
	{
		TickDialogue(Widget.Get(), FrameStep);
		AdvancedSeconds += FrameStep;
		TestFrameUv(*this, KeycapImage, FrameIndex);
		if (FrameIndex == 8)
		{
			Capture(TEXT("Pressed"));
		}
	}
	TickDialogue(Widget.Get(), LoopSeconds - AdvancedSeconds + KINDA_SMALL_NUMBER);
	TestFrameUv(*this, KeycapImage, 0);

	TestTrue(TEXT("Space key is handled"), SendKey(Widget.Get(), EKeys::SpaceBar).IsEventHandled());
	TestTrue(TEXT("Second line resets to typewriter state"), DialogueBody->GetText().IsEmpty());
	TestEqual(TEXT("Keycap hides on the next line"), KeycapImage->GetVisibility(), ESlateVisibility::Collapsed);
	TestFrameUv(*this, KeycapImage, 0);

	TestTrue(TEXT("Repeated Escape is handled"), SendKey(Widget.Get(), EKeys::Escape, true).IsEventHandled());
	TestTrue(TEXT("Repeated key does not fill or advance"), DialogueBody->GetText().IsEmpty());
	TestTrue(TEXT("Dialogue remains active after repeated key"), Widget->IsDialogueRunning());
	TestTrue(TEXT("Escape is handled"), SendKey(Widget.Get(), EKeys::Escape).IsEventHandled());
	TestEqual(TEXT("Escape fills the second line"), DialogueBody->GetText().ToString(), FString(TEXT("Second line")));

	TestTrue(TEXT("Gamepad advance is handled"), SendKey(Widget.Get(), EKeys::Gamepad_FaceButton_Bottom).IsEventHandled());
	TestTrue(TEXT("Gamepad advance starts third line empty"), DialogueBody->GetText().IsEmpty());
	TestTrue(TEXT("Gamepad fill is handled"), SendKey(Widget.Get(), EKeys::Gamepad_DPad_Down).IsEventHandled());
	TestEqual(TEXT("Gamepad key fills the third line"), DialogueBody->GetText().ToString(), FString(TEXT("Third line")));
	TestTrue(TEXT("Gamepad finish is handled"), SendKey(Widget.Get(), EKeys::Gamepad_FaceButton_Right).IsEventHandled());
	TestFalse(TEXT("Non-repeat key finishes the final line"), Widget->IsDialogueRunning());
	TestEqual(TEXT("Finish delegate fires once"), FinishedCount, 1);
	SendKey(Widget.Get(), EKeys::A);
	TestEqual(TEXT("Keys after finish do not finish twice"), FinishedCount, 1);
	return true;
}

#endif
