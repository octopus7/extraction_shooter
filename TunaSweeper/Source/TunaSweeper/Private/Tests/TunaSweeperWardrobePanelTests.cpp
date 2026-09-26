#if WITH_DEV_AUTOMATION_TESTS
#include "UI/TunaSweeperWardrobePanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
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
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
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
	TStrongObjectPtr<UTunaSweeperWardrobePanelWidget> Panel(CreateWidget<UTunaSweeperWardrobePanelWidget>(Controller));
	TSharedRef<SWidget> Slate = Panel->TakeWidget();
	Panel->OpenWardrobe();
	Panel->ForceLayoutPrepass();
	auto* Grid = Cast<UUniformGridPanel>(Panel->WidgetTree->FindWidget(TEXT("WardrobeOutfitGrid")));
	auto* Equip = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("WardrobeEquipButton")));
	auto* Preview = Cast<UImage>(Panel->WidgetTree->FindWidget(TEXT("WardrobePreviewImage")));
	auto* Status = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("WardrobeStatus")));
	if (!TestNotNull(TEXT("Grid exists"), Grid) || !TestNotNull(TEXT("Equip button exists"), Equip) ||
		!TestNotNull(TEXT("Preview exists"), Preview) || !TestNotNull(TEXT("Status exists"), Status)) return false;
	if (!TestEqual(TEXT("All six cards are available"), Grid->GetChildrenCount(), 6)) return false;
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
		if (FParse::Param(FCommandLine::Get(), TEXT("WardrobeUIPreview")))
		{
#if WITH_EDITOR
			FAssetCompilingManager::Get().FinishAllCompilation();
#endif
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("WardrobePreview");
			IFileManager::Get().MakeDirectory(*Directory,true);
			FWidgetRenderer Renderer(false);
			auto Fitted = SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
				[SNew(SBox).WidthOverride(1280).HeightOverride(760)[Slate]];
			for (FIntPoint Size : {FIntPoint(1280,760), FIntPoint(960,570)})
			{
				// ScaleBox normalizes its layout on the frame following a geometry change.
				UTextureRenderTarget2D* Target = nullptr;
				for (int32 Frame = 0; Frame < 3; ++Frame)
					Target = Renderer.DrawWidget(Fitted,FVector2D(Size.X,Size.Y));
				if (!TestNotNull(TEXT("Panel render target exists"),Target)) return false;
				const FVector2D PanelSize = Panel->GetCachedGeometry().GetAbsoluteSize();
				TestTrue(TEXT("Scaled panel fits the complete viewport"), PanelSize.X <= Size.X + 1 && PanelSize.Y <= Size.Y + 1);
				FlushRenderingCommands();
				TArray<FColor> Pixels; FReadSurfaceDataFlags ReadFlags; ReadFlags.SetLinearToGamma(false);
				Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels,ReadFlags);
				TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Png);
				TestTrue(TEXT("Panel preview writes"),FFileHelper::SaveArrayToFile(Png,
					*(Directory/FString::Printf(TEXT("Panel_%d_%dx%d.png"),static_cast<int32>(Language),Size.X,Size.Y))));
			}
		}
	}
	return true;
}
#endif
