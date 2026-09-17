#if WITH_DEV_AUTOMATION_TESTS
#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Slate/WidgetRenderer.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperHudDockTest,
	"TunaSweeper.UI.Hud.TopEdgeDock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperHudDockTest::RunTest(const FString& Parameters)
{
	UTunaSweeperBuildTargetSettings* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	const ETunaSweeperBuildTarget OriginalTarget = Settings->BuildTarget;
	ON_SCOPE_EXIT { Settings->BuildTarget = OriginalTarget; };
	UClass* Class = LoadClass<UTunaSweeperGameHudWidget>(nullptr, TEXT("/Game/UI/WBP_GameHud.WBP_GameHud_C"));
	if (!TestNotNull(TEXT("Authored game HUD loads"), Class)) return false;
	TStrongObjectPtr<UTunaSweeperGameHudWidget> Hud(CreateWidget<UTunaSweeperGameHudWidget>(
		GEditor->GetEditorWorldContext().World(), Class));
	if (!TestNotNull(TEXT("Authored game HUD instantiates"), Hud.Get())) return false;
	TSharedRef<SWidget> Slate = Hud->TakeWidget();
	UTunaSweeperHudTopReserveWidget* Dock = Cast<UTunaSweeperHudTopReserveWidget>(
		Hud->WidgetTree->FindWidget(TEXT("TopStatusReserveWidget")));
	if (!TestNotNull(TEXT("Actual HUD contains the menu dock"), Dock)) return false;
	UCanvasPanelSlot* DockSlot = Cast<UCanvasPanelSlot>(Dock->Slot);
	if (!TestNotNull(TEXT("Dock belongs to the screen canvas"), DockSlot)) return false;
	TestEqual(TEXT("No gap between dock and screen top"), DockSlot->GetPosition().Y, 0.0);
	TestTrue(TEXT("Dock fits only its visible tabs"), DockSlot->GetAutoSize());
	UBorder* Background = Cast<UBorder>(Dock->WidgetTree->FindWidget(TEXT("ReservedBackground")));
	if (!TestNotNull(TEXT("Dock backdrop exists"), Background)) return false;
	TestEqual(TEXT("Tabs have no top inset inside the dock"), Background->GetPadding().Top, 0.0f);
	const auto& Corners = Background->Background.OutlineSettings.CornerRadii;
	TestTrue(TEXT("Backdrop has square top and gently rounded bottom corners"),
		Corners.X == 0.0f && Corners.Y == 0.0f && Corners.Z == 6.0f && Corners.W == 6.0f);

	// Keep the authored screen canvas, but isolate the dock for deterministic geometry and captures.
	if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Hud->WidgetTree->RootWidget))
		for (UWidget* Child : Canvas->GetAllChildren()) Child->SetVisibility(ESlateVisibility::Collapsed);
	Dock->SetVisibility(ESlateVisibility::Visible);
	Dock->SetRenderTranslation(FVector2D::ZeroVector);
	Dock->SetRenderOpacity(1.0f);
	FAssetCompilingManager::Get().FinishAllCompilation();
	FWidgetRenderer Renderer(true);
	float DemoWidth = 0.0f;
	for (ETunaSweeperBuildTarget Target : {ETunaSweeperBuildTarget::SteamDemo, ETunaSweeperBuildTarget::SteamFull})
	{
		Settings->BuildTarget = Target;
		Dock->SetActiveMode(ETunaSweeperHudMode::Inventory);
		const FVector2D Viewport = Settings->IsDemoBuild() ? FVector2D(1280, 720) : FVector2D(1920, 1080);
		Hud->InvalidateLayoutAndVolatility();
		Slate->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
		Hud->ForceLayoutPrepass();
		TStrongObjectPtr<UTextureRenderTarget2D> Image(Renderer.DrawWidget(Slate, Viewport));
		FlushRenderingCommands();
		const FGeometry& Geometry = Dock->GetCachedGeometry();
		const FVector2D Origin = Geometry.GetAbsolutePosition();
		const FVector2D Size = Geometry.GetLocalSize();
		TestTrue(TEXT("Rendered dock starts exactly at top edge"), FMath::IsNearlyZero(Origin.Y));
		TestTrue(TEXT("Rendered dock remains horizontally centered"),
			FMath::IsNearlyEqual(Origin.X + Size.X * 0.5, Viewport.X * 0.5, 0.5));
		UWidget* InventoryButton = Dock->WidgetTree->FindWidget(TEXT("InventoryModeButton"));
		if (TestNotNull(TEXT("First tab exists"), InventoryButton))
			TestTrue(TEXT("Rendered tab touches the top edge too"),
				FMath::IsNearlyZero(InventoryButton->GetCachedGeometry().GetAbsolutePosition().Y));
		if (Settings->IsDemoBuild()) DemoWidth = Size.X;
		else TestTrue(TEXT("Full dock grows to include the research tab"), Size.X > DemoWidth);
		FImage Pixels;
		if (Image.IsValid() && FImageUtils::GetRenderTargetImage(Image.Get(), Pixels))
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() /
				(Settings->IsDemoBuild() ? TEXT("Screenshots/HudDockDemo.png") : TEXT("Screenshots/HudDockFull.png"))), Pixels);
	}
	return true;
}
#endif
