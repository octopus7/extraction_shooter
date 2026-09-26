#if WITH_DEV_AUTOMATION_TESTS
#include "UI/TunaSweeperWardrobePanelWidget.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

namespace TunaWardrobePanelTests
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Game->*Member = Context;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWardrobePanelTest,
	"TunaSweeper.Wardrobe.Panel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperWardrobePanelTest::RunTest(const FString&)
{
	using namespace TunaWardrobePanelTests;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
	Game->bOutfitUnlocksLoaded = true;
	Game->UnlockedOutfitIds.Add(TEXT("Maid"));
	Game->bUnlockAllOutfitsOverride = true;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game.Get(),nullptr);
		World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	auto* Controller = World->SpawnActor<APlayerController>();
	auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Controller->SetPlayer(LocalPlayer);
	auto* Strings = Game->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Real localization subsystem exists"), Strings) ||
		!TestTrue(TEXT("Localization data loads"), Strings->LoadTextData())) return false;
	TStrongObjectPtr<UTunaSweeperGameHudWidget> Hud(CreateWidget<UTunaSweeperGameHudWidget>(Controller));
	if (!Hud->WidgetTree) Hud->WidgetTree = NewObject<UWidgetTree>(Hud.Get());
	auto* HudCanvas = Hud->WidgetTree->ConstructWidget<UCanvasPanel>();
	Hud->WidgetTree->RootWidget = HudCanvas;
	Hud->EnsureWardrobePanelWidget();
	TStrongObjectPtr<UTunaSweeperWardrobePanelWidget> Panel(Hud->WardrobePanelWidget.Get());
	if (!TestNotNull(TEXT("HUD creates wardrobe screen"), Panel.Get())) return false;
	Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Panel->TakeWidget();
	TSharedRef<SWidget> Slate = HudCanvas->TakeWidget();
	if (!TestNotNull(TEXT("Wardrobe attaches directly to viewport canvas"), Cast<UCanvasPanelSlot>(Panel->Slot))) return false;
	Panel->OpenWardrobe();
	Panel->ForceLayoutPrepass();
	auto* Grid = Cast<UUniformGridPanel>(Panel->WidgetTree->FindWidget(TEXT("WardrobeOutfitGrid")));
	auto* Equip = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("WardrobeEquipButton")));
	auto* Preview = Cast<UImage>(Panel->WidgetTree->FindWidget(TEXT("WardrobePreviewImage")));
	auto* Status = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("WardrobeStatus")));
	if (!TestNotNull(TEXT("Grid exists"), Grid) || !TestNotNull(TEXT("Equip button exists"), Equip) ||
		!TestNotNull(TEXT("Preview exists"), Preview) || !TestNotNull(TEXT("Status exists"), Status)) return false;
	if (!TestEqual(TEXT("All eight cards are available"), Grid->GetChildrenCount(), 8)) return false;
	auto* ListScroll = Cast<UScrollBox>(Grid->GetParent());
	if (!TestNotNull(TEXT("The complete outfit grid is scrollable"), ListScroll)) return false;
	auto* RaincoatCard = Cast<UTunaSweeperOutfitCardWidget>(Grid->GetChildAt(6));
	if (!TestNotNull(TEXT("Seventh raincoat card exists"), RaincoatCard)) return false;
	auto* RaincoatSlot = Cast<UUniformGridSlot>(RaincoatCard->Slot);
	if (!TestNotNull(TEXT("Raincoat has a grid slot"), RaincoatSlot)) return false;
	TestEqual(TEXT("Raincoat starts the third row"), RaincoatSlot->GetRow(), 2);
	RaincoatCard->TakeWidget();
	auto* RaincoatButton = Cast<UButton>(RaincoatCard->WidgetTree->FindWidget(TEXT("OutfitCardButton")));
	if (!TestNotNull(TEXT("Raincoat is selectable"), RaincoatButton)) return false;
	auto* SciFiSuitCard = Cast<UTunaSweeperOutfitCardWidget>(Grid->GetChildAt(7));
	if (!TestNotNull(TEXT("Eighth sci-fi suit card exists"), SciFiSuitCard)) return false;
	auto* SciFiSuitSlot = Cast<UUniformGridSlot>(SciFiSuitCard->Slot);
	if (!TestNotNull(TEXT("Sci-fi suit has a grid slot"), SciFiSuitSlot)) return false;
	TestEqual(TEXT("Sci-fi suit shares the third row"), SciFiSuitSlot->GetRow(), 2);
	TestEqual(TEXT("Sci-fi suit uses the second column"), SciFiSuitSlot->GetColumn(), 1);
	SciFiSuitCard->TakeWidget();
	auto* SciFiSuitButton = Cast<UButton>(SciFiSuitCard->WidgetTree->FindWidget(TEXT("OutfitCardButton")));
	if (!TestNotNull(TEXT("Sci-fi suit is selectable"), SciFiSuitButton)) return false;
	TestFalse(TEXT("Current outfit cannot be redundantly equipped"), Equip->GetIsEnabled());
	auto* SchoolCard = Cast<UTunaSweeperOutfitCardWidget>(Grid->GetChildAt(1));
	if (!TestNotNull(TEXT("School card exists"), SchoolCard)) return false;
	SchoolCard->TakeWidget();
	auto* CardButton = Cast<UButton>(SchoolCard->WidgetTree->FindWidget(TEXT("OutfitCardButton")));
	if (!TestNotNull(TEXT("Card button exists"), CardButton)) return false;
	CardButton->OnClicked.Broadcast();
	TestTrue(TEXT("Different unlocked outfit enables equip"), Equip->GetIsEnabled());
	TestEqual(TEXT("Selection preserves card widgets and keyboard focus targets"), Grid->GetChildAt(1), static_cast<UWidget*>(SchoolCard));
	TestTrue(TEXT("Preview displays selected school texture"), Preview->GetBrush().GetResourceObject() &&
		Preview->GetBrush().GetResourceObject()->GetName() == TEXT("T_UIOutfit_SchoolUniform"));
	Game->bUnlockAllOutfitsOverride = false;
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestFalse(TEXT("Locked preview cannot be equipped"), Equip->GetIsEnabled());
	TestTrue(TEXT("Locked state has localized explanation"), !Status->GetText().IsEmpty());
	Game->UnlockedOutfitIds.Add(TEXT("SchoolUniform"));
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestTrue(TEXT("New unlock enables equip without reopening"), Equip->GetIsEnabled());
	// Generic controller has no wardrobe session: failed apply must be visible and reset on reopening.
	Equip->OnClicked.Broadcast();
	TestEqual(TEXT("Rejected apply reports localized failure"), Status->GetText().ToString(),
		Game->ResolveLocalizedText(TEXT("ui.wardrobe.apply_failed"), FText::GetEmpty()).ToString());
	Panel->OpenWardrobe();
	TestEqual(TEXT("Reopening clears prior failure"), Status->GetText().ToString(),
		Game->ResolveLocalizedText(TEXT("ui.wardrobe.equipped"), FText::GetEmpty()).ToString());
	Game->bUnlockAllOutfitsOverride = true;
	Panel->RefreshWardrobe();
	RaincoatButton->OnClicked.Broadcast();
	TestTrue(TEXT("Seventh outfit enables equip"), Equip->GetIsEnabled());
	TestTrue(TEXT("Raincoat selection displays its own thumbnail"), Preview->GetBrush().GetResourceObject() &&
		Preview->GetBrush().GetResourceObject()->GetName() == TEXT("T_UIOutfit_Raincoat"));
	Game->bUnlockAllOutfitsOverride = false;
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestFalse(TEXT("An unowned raincoat stays previewable but cannot be equipped"), Equip->GetIsEnabled());
	Game->UnlockedOutfitIds.Add(TEXT("Raincoat"));
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestTrue(TEXT("Raincoat unlock enables equip without reopening"), Equip->GetIsEnabled());
	Game->bUnlockAllOutfitsOverride = true;
	Panel->RefreshWardrobe();
	SciFiSuitButton->OnClicked.Broadcast();
	TestTrue(TEXT("Eighth outfit enables equip"), Equip->GetIsEnabled());
	TestTrue(TEXT("Sci-fi suit selection displays its own thumbnail"), Preview->GetBrush().GetResourceObject() &&
		Preview->GetBrush().GetResourceObject()->GetName() == TEXT("T_UIOutfit_SciFiSuit"));
	Game->bUnlockAllOutfitsOverride = false;
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestFalse(TEXT("An unowned sci-fi suit stays previewable but cannot be equipped"), Equip->GetIsEnabled());
	Game->UnlockedOutfitIds.Add(TEXT("SciFiSuit"));
	Game->OnOutfitUnlocksChanged.Broadcast();
	TestTrue(TEXT("Sci-fi suit unlock enables equip without reopening"), Equip->GetIsEnabled());
	Game->bUnlockAllOutfitsOverride = true;
	for (const ETunaSweeperItemTextLanguage Language : {
		ETunaSweeperItemTextLanguage::Korean, ETunaSweeperItemTextLanguage::English, ETunaSweeperItemTextLanguage::Japanese})
	{
		Game->SetCurrentTextLanguage(Language,false);
		auto* Title = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("WardrobeTitle")));
		TestEqual(TEXT("Title refreshes with language changes"), Title->GetText().ToString(),
			Game->ResolveLocalizedText(TEXT("ui.wardrobe.title"),FText::GetEmpty()).ToString());
		for (int32 Index=0; Index<Grid->GetChildrenCount(); ++Index)
		{
			auto* Card = Cast<UTunaSweeperOutfitCardWidget>(Grid->GetChildAt(Index));
			Card->TakeWidget();
			auto* Name = Cast<UTextBlock>(Card->WidgetTree->FindWidget(TEXT("OutfitCardName")));
			TestTrue(TEXT("Every outfit name is translated"), Name && !Name->GetText().IsEmpty());
		}
		auto* RaincoatName = Cast<UTextBlock>(RaincoatCard->WidgetTree->FindWidget(TEXT("OutfitCardName")));
		if (!TestNotNull(TEXT("Raincoat name label exists"), RaincoatName)) return false;
		const TCHAR* ExpectedRaincoatName = Language == ETunaSweeperItemTextLanguage::Korean ? TEXT("우의")
			: Language == ETunaSweeperItemTextLanguage::Japanese ? TEXT("レインコート") : TEXT("Raincoat");
		TestEqual(TEXT("Raincoat label resolves in each supported language"), RaincoatName->GetText().ToString(), FString(ExpectedRaincoatName));
		auto* SciFiSuitName = Cast<UTextBlock>(SciFiSuitCard->WidgetTree->FindWidget(TEXT("OutfitCardName")));
		if (!TestNotNull(TEXT("Sci-fi suit name label exists"), SciFiSuitName)) return false;
		const TCHAR* ExpectedSciFiSuitName = Language == ETunaSweeperItemTextLanguage::Korean ? TEXT("SF 슈트")
			: Language == ETunaSweeperItemTextLanguage::Japanese ? TEXT("SFスーツ") : TEXT("Sci-Fi Suit");
		TestEqual(TEXT("Sci-fi suit label resolves in each supported language"), SciFiSuitName->GetText().ToString(), FString(ExpectedSciFiSuitName));
		if (FParse::Param(FCommandLine::Get(), TEXT("WardrobeUIPreview")))
		{
#if WITH_EDITOR
			FAssetCompilingManager::Get().FinishAllCompilation();
#endif
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("WardrobePreview");
			IFileManager::Get().MakeDirectory(*Directory,true);
			FWidgetRenderer Renderer(false);
			for (FIntPoint Size : {FIntPoint(1280,720), FIntPoint(1920,1080), FIntPoint(2560,1080), FIntPoint(960,540)})
			{
				ListScroll->SetScrollOffset(0.0f);
				// ScaleBox normalizes its layout on the frame following a geometry change.
				UTextureRenderTarget2D* Target = nullptr;
				for (int32 Frame = 0; Frame < 3; ++Frame)
					Target = Renderer.DrawWidget(Slate,FVector2D(Size.X,Size.Y));
				if (!TestNotNull(TEXT("Panel render target exists"),Target)) return false;
				const FVector2D PanelSize = Panel->GetCachedGeometry().GetAbsoluteSize();
				TestTrue(TEXT("Wardrobe fills both dimensions of the viewport"),
					FMath::IsNearlyEqual(PanelSize.X, static_cast<double>(Size.X), 1.0) &&
					FMath::IsNearlyEqual(PanelSize.Y, static_cast<double>(Size.Y), 1.0));
				TestTrue(TEXT("Wardrobe begins at the viewport origin"), Panel->GetCachedGeometry().GetAbsolutePosition().IsNearlyZero(1.0));
				ListScroll->ScrollWidgetIntoView(SciFiSuitCard, false, EDescendantScrollDestination::IntoView, 5.0f);
				for (int32 Frame = 0; Frame < 3; ++Frame)
					Target = Renderer.DrawWidget(Slate,FVector2D(Size.X,Size.Y));
				const FGeometry& ScrollGeometry = ListScroll->GetCachedGeometry();
				const FGeometry& CardGeometry = SciFiSuitCard->GetCachedGeometry();
				const FVector2D CardTop = ScrollGeometry.AbsoluteToLocal(CardGeometry.GetAbsolutePosition());
				const FVector2D CardBottom = ScrollGeometry.AbsoluteToLocal(CardGeometry.LocalToAbsolute(CardGeometry.GetLocalSize()));
				TestTrue(TEXT("The whole eighth card fits inside the scroll viewport"),
					CardTop.Y >= -1.0f && CardBottom.Y <= ScrollGeometry.GetLocalSize().Y + 1.0f);
				const FVector2D PreviewSize = Preview->GetCachedGeometry().GetAbsoluteSize();
				TestTrue(TEXT("The selected sci-fi suit portrait keeps its 2:3 aspect"),
					PreviewSize.Y > 0.0 && FMath::IsNearlyEqual(PreviewSize.X / PreviewSize.Y, 2.0 / 3.0, 0.001));
				TestTrue(TEXT("The full-body preview expands on large screens"), Size.Y < 1080 || PreviewSize.Y > 650.0);
				for (int32 Index = 0; Index < Grid->GetChildrenCount(); ++Index)
				{
					auto* Card = Cast<UTunaSweeperOutfitCardWidget>(Grid->GetChildAt(Index));
					auto* Label = Cast<UTextBlock>(Card->WidgetTree->FindWidget(TEXT("OutfitCardName")));
					TestTrue(TEXT("Localized outfit names fit their own card width"), Label &&
						Label->GetDesiredSize().X <= Card->GetCachedGeometry().GetLocalSize().X + 1.0f);
				}
				for (UWidget* Control : {static_cast<UWidget*>(Preview), static_cast<UWidget*>(Equip),
					Panel->WidgetTree->FindWidget(TEXT("WardrobeCloseButton"))})
				{
					if (!TestNotNull(TEXT("Fullscreen control exists"), Control)) return false;
					const FGeometry& Geometry = Control->GetCachedGeometry();
					const FVector2D Top = Geometry.GetAbsolutePosition();
					const FVector2D Bottom = Geometry.LocalToAbsolute(Geometry.GetLocalSize());
					TestTrue(TEXT("Preview and actions remain inside the viewport"), Top.X >= -1 && Top.Y >= -1 &&
						Bottom.X <= Size.X + 1 && Bottom.Y <= Size.Y + 1);
				}
				SciFiSuitButton->OnClicked.Broadcast();
				TestTrue(TEXT("Scrolled sci-fi suit remains equipable"), Equip->GetIsEnabled());
				Target = Renderer.DrawWidget(Slate,FVector2D(Size.X,Size.Y));
				FlushRenderingCommands();
				TArray<FColor> Pixels; FReadSurfaceDataFlags ReadFlags; ReadFlags.SetLinearToGamma(false);
				Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,ReadFlags);
				TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
				TestTrue(TEXT("Panel preview writes"),FFileHelper::SaveArrayToFile(Png,
					*(Directory/FString::Printf(TEXT("Panel_%d_%dx%d_SciFiSuit.png"),static_cast<int32>(Language),Size.X,Size.Y))));
			}
		}
	}
	return true;
}
#endif
