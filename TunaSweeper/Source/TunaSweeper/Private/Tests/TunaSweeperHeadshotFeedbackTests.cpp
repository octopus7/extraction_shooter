#if WITH_DEV_AUTOMATION_TESTS
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Components/Border.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperHeadshotFeedbackTest,
	"TunaSweeper.UI.Combat.HeadshotFeedbackLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperHeadshotFeedbackTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UTunaSweeperGameHudWidget> Hud(NewObject<UTunaSweeperGameHudWidget>());
	Hud->WidgetTree = NewObject<UWidgetTree>(Hud.Get());
	auto* Canvas = Hud->WidgetTree->ConstructWidget<UCanvasPanel>();
	Hud->WidgetTree->RootWidget = Canvas;
	Hud->ShowDamageNumber(12, FVector::ZeroVector, ETunaSweeperDamageNumberType::Normal);
	TestEqual(TEXT("Normal damage has only its number"), Canvas->GetChildrenCount(), 1);
	Hud->ShowDamageNumber(24, FVector::ZeroVector, ETunaSweeperDamageNumberType::Headshot);
	TestEqual(TEXT("Only headshot adds a separate impact background"), Canvas->GetChildrenCount(), 3);
	auto& Popup = Hud->DamageNumberPopups.Last();
	auto* Burst = Popup.BurstWidget.Get();
	auto* Number = Popup.TextWidget.Get();
	if (!TestNotNull(TEXT("Actual headshot burst widget"), Burst)) return false;
	TestTrue(TEXT("Burst renders behind its number"), Cast<UCanvasPanelSlot>(Burst->Slot)->GetZOrder() < Cast<UCanvasPanelSlot>(Number->Slot)->GetZOrder());
	Popup.ElapsedSeconds = .028f;
	Hud->UpdateDamageNumberPresentation(Popup, FVector2D(300, 200));
	const FVector2D EarlyBurstScale = Burst->GetRenderTransform().Scale;
	const FVector2D EarlyNumberScale = Number->GetRenderTransform().Scale;
	Popup.ElapsedSeconds = .07f;
	Hud->UpdateDamageNumberPresentation(Popup, FVector2D(300, 200));
	TestTrue(TEXT("Burst squashes while the delayed number grows"),
		Burst->GetRenderTransform().Scale.Y < EarlyBurstScale.Y && Number->GetRenderTransform().Scale.Y > EarlyNumberScale.Y);
	TestTrue(TEXT("Burst uses non-uniform impact squash"), Burst->GetRenderTransform().Scale.X > Burst->GetRenderTransform().Scale.Y);
	Popup.ElapsedSeconds = .42f;
	Hud->UpdateDamageNumberPresentation(Popup, FVector2D(300, 200));
	TestEqual(TEXT("Brief burst is gone before the readable number"), Burst->GetRenderOpacity(), 0.0f);
	TestEqual(TEXT("Number remains readable after burst"), Number->GetRenderOpacity(), 1.0f);
	Hud->RemoveDamageNumberPopupAt(1);
	TestEqual(TEXT("Removing a headshot removes its background too"), Canvas->GetChildrenCount(), 1);
	Hud->RemoveDamageNumberPopupAt(0);
	for (int32 Hit = 0; Hit < 70; ++Hit)
		Hud->ShowDamageNumber(24, FVector::ZeroVector, ETunaSweeperDamageNumberType::Headshot);
	TestEqual(TEXT("Rapid fire retains at most 64 complete popup pairs"), Canvas->GetChildrenCount(), 128);
	Hud->TickDamageNumberPopups(2.0f);
	TestEqual(TEXT("Expired feedback leaves no background even without a projection owner"), Canvas->GetChildrenCount(), 0);
	TestEqual(TEXT("Expired feedback leaves no tracking records"), Hud->DamageNumberPopups.Num(), 0);
	Hud->ShowDamageNumber(24, FVector::ZeroVector, ETunaSweeperDamageNumberType::Headshot);
	Hud->DamageNumberPopups.Last().TextWidget.Reset();
	Hud->TickDamageNumberPopups(0);
	TestEqual(TEXT("Lost text releases its paired background"), Hud->DamageNumberPopups.Num(), 0);
	TestEqual(TEXT("Lost text leaves no orphaned burst"), Canvas->GetChildrenCount(), 1);
	// Drop the deliberately orphaned text used to simulate a lost weak reference.
	Canvas->ClearChildren();
	Hud->ShowDamageNumber(24, FVector::ZeroVector, ETunaSweeperDamageNumberType::Headshot);
	Hud->NativeDestruct();
	TestEqual(TEXT("HUD teardown removes complete popup pairs"), Canvas->GetChildrenCount(), 0);

	if (FApp::CanEverRender() && FParse::Param(FCommandLine::Get(), TEXT("HeadshotFeedbackCapture")))
	{
		IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("HeadshotBurst/Frames")), true);
		auto* Backdrop = Hud->WidgetTree->ConstructWidget<UBorder>();
		Backdrop->SetBrushColor(FLinearColor(.025f,.032f,.043f));
		Cast<UCanvasPanelSlot>(Canvas->AddChild(Backdrop))->SetSize(FVector2D(640, 440));
		TSharedRef<SWidget> Slate = Canvas->TakeWidget();
		Hud->ShowDamageNumber(48, FVector::ZeroVector, ETunaSweeperDamageNumberType::Headshot);
		FWidgetRenderer Renderer(true);
		for (int32 Frame = 0; Frame < 72; ++Frame)
		{
			auto& CapturePopup = Hud->DamageNumberPopups.Last();
			CapturePopup.ElapsedSeconds = Frame / 60.0f;
			CapturePopup.ScreenDrift = FVector2D::ZeroVector;
			Hud->UpdateDamageNumberPresentation(CapturePopup, FVector2D(320, 265));
			TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(640, 440)));
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
			ReadFlags.SetLinearToGamma(false);
			Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(640, 440, Pixels, PNG);
			const FString Path = FPaths::ProjectSavedDir() / FString::Printf(TEXT("HeadshotBurst/Frames/Frame_%03d.png"), Frame);
			TestTrue(TEXT("Feedback preview frame saved"), FFileHelper::SaveArrayToFile(PNG, *Path));
		}
		Hud->RemoveDamageNumberPopupAt(0);
	}
	return true;
}
#endif
