#if WITH_DEV_AUTOMATION_TESTS

#include "UI/TunaSweeperBossLabWidget.h"
#include "Blueprint/WidgetTree.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CheckBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Layout/SConstraintCanvas.h"

namespace TunaBossLabWidgetTests
{
	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossLabWidgetTest,
	"TunaSweeper.BossLab.Widget.EditingAndConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperBossLabWidgetTest::RunTest(const FString& Parameters)
{
	using namespace TunaBossLabWidgetTests;
	using EAction = ETunaSweeperBossLabAction;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient UI world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	// Initialize real subsystems without loading or writing the player's saved game/settings.
	Instance->UGameInstance::Init();
	const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Automation/BossLabWidget") / FGuid::NewGuid().ToString();
	ON_SCOPE_EXIT
	{
		IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
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
	if (!TestNotNull(TEXT("Generic test controller exists"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->SetControllerId(0);
	Controller->SetPlayer(LocalPlayer);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	UTunaSweeperBossLabSubsystem* Library = Instance->GetSubsystem<UTunaSweeperBossLabSubsystem>();
	if (!TestNotNull(TEXT("Real text subsystem exists"), Strings) || !TestNotNull(TEXT("Real boss library exists"), Library)) return false;
	if (!TestTrue(TEXT("Localization CSV loads"), Strings->LoadTextData())) return false;
	Library->SetLibraryDirectoryForTesting(TestDirectory);
	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
	TStrongObjectPtr<UTunaSweeperBossLabWidget> Widget(CreateWidget<UTunaSweeperBossLabWidget>(Controller));
	if (!TestNotNull(TEXT("Native workshop widget creates"), Widget.Get())) return false;
	TSharedPtr<SWidget> SlateWidget = Widget->TakeWidget();
	Widget->ForceLayoutPrepass();
	TestEqual(TEXT("Twelve actual slots are shown"), Widget->SlotCombo->GetOptionCount(), 12);
	TestEqual(TEXT("Empty library disables load"), Widget->LoadButton->GetIsEnabled(), false);
	TestFalse(TEXT("Localized title is nonempty"), Widget->TitleText->GetText().IsEmpty());
	TestTrue(TEXT("Unnamed definition receives localized display name"),
		Widget->SummaryText->GetText().ToString().Contains(Instance->ResolveLocalizedText(TEXT("ui.boss_lab.unnamed"), FText::GetEmpty()).ToString()));
	for (const auto& Label : Widget->StaticLabels)
	{
		TestTrue(TEXT("Every static UI key resolves"), Label.Key.IsValid() && !Label.Key->GetText().IsEmpty());
	}

	// Optional real Slate renders support visual review without opening a game window.
	if (FParse::Param(FCommandLine::Get(), TEXT("BossLabUIPreview")))
	{
		const FString PreviewDirectory = FPaths::ProjectSavedDir() / TEXT("BossLabUIPreview");
		IFileManager::Get().MakeDirectory(*PreviewDirectory, true);
		FWidgetRenderer Renderer(false);
		auto Canvas = SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot().Anchors(FAnchors(0, 0, 1, 1)).Offset(FMargin(0))
			[SlateWidget.ToSharedRef()];
		for (const ETunaSweeperItemTextLanguage Language : {
			ETunaSweeperItemTextLanguage::Korean, ETunaSweeperItemTextLanguage::English, ETunaSweeperItemTextLanguage::Japanese})
		{
			Instance->SetCurrentTextLanguage(Language, false);
			UTextureRenderTarget2D* Target = Renderer.DrawWidget(Canvas, FVector2D(1280, 720));
			if (!TestNotNull(TEXT("Widget render target exists"), Target)) return false;
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags Flags;
			Flags.SetLinearToGamma(false);
			Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags);
			if (!TestEqual(TEXT("Preview contains complete viewport"), Pixels.Num(), 1280 * 720)) return false;
			TestTrue(TEXT("Center preview remains unobstructed"), Pixels[360 * 1280 + 640].A < 20);
			TestTrue(TEXT("Library backing is rendered"), Pixels[90 * 1280 + 24].A > 200);
			TestTrue(TEXT("Editor backing is rendered"), Pixels[90 * 1280 + 960].A > 200);
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(1280, 720, Pixels, Png);
			TestTrue(TEXT("Rendered preview writes"), FFileHelper::SaveArrayToFile(Png,
				*(PreviewDirectory / FString::Printf(TEXT("Editor_%d_1280x720.png"), static_cast<int32>(Language)))));
		}
	}

	FTunaSweeperBossDefinition Draft = Library->GetDraft();
	Draft.Name = TEXT("Automation draft");
	Library->SetDraft(Draft);
	Library->SetDirty(true);
	Widget->RefreshFromSession();
	Widget->HandleAction(EAction::New);
	TestEqual(TEXT("New asks before losing dirty content"), Widget->PendingAction, EAction::New);
	TestEqual(TEXT("Confirmation is visible"), Widget->ConfirmationLayer->GetVisibility(), ESlateVisibility::Visible);
	TestFalse(TEXT("Underlying controls are disabled"), Widget->WorkingLayer->GetIsEnabled());
	Widget->HandleAction(EAction::Cancel);
	TestEqual(TEXT("Cancelling new keeps the draft"), Library->GetDraft().Name, Draft.Name);
	TestTrue(TEXT("Cancel restores controls"), Widget->WorkingLayer->GetIsEnabled());
	Widget->HandleAction(EAction::Save);
	TestTrue(TEXT("Saving an empty slot succeeds"), Library->SlotExists(0));
	TestFalse(TEXT("Saved draft is clean"), Library->IsDirty());
	Draft.Name = TEXT("Changed draft");
	Library->SetDraft(Draft);
	Library->SetDirty(true);
	Widget->HandleAction(EAction::Save);
	TestEqual(TEXT("Occupied save requires confirmation"), Widget->PendingAction, EAction::Save);
	Widget->HandleAction(EAction::Cancel);
	FTunaSweeperBossDefinition Saved;
	FName Error;
	TestTrue(TEXT("Original saved file loads"), Library->LoadSlot(0, Saved, Error));
	TestEqual(TEXT("Cancel preserves previous disk content"), Saved.Name, FString(TEXT("Automation draft")));
	Widget->HandleAction(EAction::Save);
	Widget->HandleAction(EAction::Confirm);
	Library->LoadSlot(0, Saved, Error);
	TestEqual(TEXT("Confirm replaces intended file"), Saved.Name, Draft.Name);

	// A copied invalid file appears in the list, but loading it cannot replace a valid draft.
	TestTrue(TEXT("External invalid file writes"), FFileHelper::SaveStringToFile(TEXT("{}"), *Library->GetSlotPath(2)));
	Widget->HandleAction(EAction::Refresh);
	Widget->SlotCombo->SetSelectedIndex(2);
	TestTrue(TEXT("External copied file is visible"), Widget->LoadButton->GetIsEnabled());
	Widget->HandleAction(EAction::Load);
	TestEqual(TEXT("Failed load preserves draft identity"), Library->GetDraft().BossId, Draft.BossId);
	TestEqual(TEXT("Failed load preserves draft name"), Library->GetDraft().Name, Draft.Name);
	TestTrue(TEXT("Corrupt file is preserved"), Library->SlotExists(2));
	Widget->HandleAction(EAction::Duplicate);
	TestEqual(TEXT("Duplicate selects first empty slot"), Library->GetSelectedSlot(), 1);
	TestNotEqual(TEXT("Duplicate has independent identity"), Library->GetDraft().BossId, Draft.BossId);
	TestTrue(TEXT("Duplicate is saved"), Library->SlotExists(1));

	const int32 OriginalCount = Library->GetDraft().Parts.Num();
	Widget->ParentCombo->SetSelectedIndex(0);
	Widget->ModuleCombo->SetSelectedIndex(0);
	Widget->SocketCombo->SetSelectedIndex(1);
	Widget->HandleAction(EAction::AddPart);
	TestEqual(TEXT("Valid connection updates actual draft"), Library->GetDraft().Parts.Num(), OriginalCount + 1);
	const int32 AddedId = Widget->SelectedPartId;
	Widget->ParentCombo->SetSelectedIndex(0);
	Widget->HandleAction(EAction::AddPart);
	TestEqual(TEXT("Occupied socket preserves actual draft"), Library->GetDraft().Parts.Num(), OriginalCount + 1);
	TestEqual(TEXT("Failed connection preserves selected part"), Widget->SelectedPartId, AddedId);
	Widget->HandleAction(EAction::RemoveBranch);
	TestEqual(TEXT("Branch removal needs confirmation"), Widget->PendingAction, EAction::RemoveBranch);
	Widget->HandleAction(EAction::Cancel);
	TestEqual(TEXT("Cancelled removal preserves assembly"), Library->GetDraft().Parts.Num(), OriginalCount + 1);
	Widget->HandleAction(EAction::RemoveBranch);
	Widget->HandleAction(EAction::Confirm);
	TestEqual(TEXT("Confirmed removal edits actual draft"), Library->GetDraft().Parts.Num(), OriginalCount);
	Widget->PartCombo->SetSelectedIndex(0);
	Widget->HandleAction(EAction::RemoveBranch);
	TestEqual(TEXT("Core cannot be removed"), Widget->PendingAction, EAction::None);
	Widget->IntervalInput->SetValue(4.f);
	Widget->IntervalInput->OnValueChanged.Broadcast(4.f);
	Widget->PhaseInput->SetValue(40.f);
	Widget->PhaseInput->OnValueChanged.Broadcast(40.f);
	const float PhaseAfterInput = Library->GetDraft().PhaseThreshold;
	Widget->TacticCombo->SetSelectedIndex(2);
	const float PhaseAfterTactic = Library->GetDraft().PhaseThreshold;
	Widget->AlternateInput->SetIsChecked(false);
	Widget->AlternateInput->OnCheckStateChanged.Broadcast(false);
	const float PhaseAfterPattern = Library->GetDraft().PhaseThreshold;
	AddInfo(FString::Printf(TEXT("Phase 40%% trace: input=%.9g tactic=%.9g pattern=%.9g expected=%.9g difference=%.9g"),
		PhaseAfterInput, PhaseAfterTactic, PhaseAfterPattern, 0.4f, FMath::Abs(PhaseAfterPattern - 0.4f)));
	TestEqual(TEXT("Interval edits the draft"), Library->GetDraft().AttackInterval, 4.f);
	// /fp:fast may implement /100 using a float reciprocal. Allow its one-ULP rounding,
	// while still detecting a change far smaller than the control's one-percent step.
	TestEqual(TEXT("Phase percent edits the draft"), PhaseAfterInput, 0.4f, 0.000001f);
	TestEqual(TEXT("Tactic selection preserves phase"), PhaseAfterTactic, PhaseAfterInput);
	TestEqual(TEXT("Weapon pattern selection preserves phase"), PhaseAfterPattern, PhaseAfterInput);
	TestEqual(TEXT("Tactic edits the draft"), Library->GetDraft().Tactic, ETunaSweeperBossTactic::Advance);
	TestFalse(TEXT("Weapon pattern edits the draft"), Library->GetDraft().bAlternateWeapons);
	Widget->PhaseInput->SetValue(10.f);
	Widget->PhaseInput->OnValueChanged.Broadcast(10.f);
	TestEqual(TEXT("Minimum phase percent is valid"), Library->GetDraft().PhaseThreshold, 0.1f);
	Widget->PhaseInput->SetValue(90.f);
	Widget->PhaseInput->OnValueChanged.Broadcast(90.f);
	TestEqual(TEXT("Maximum phase percent is valid"), Library->GetDraft().PhaseThreshold, 0.9f);
	Widget->HandleAction(EAction::Delete);
	Widget->HandleAction(EAction::Cancel);
	TestTrue(TEXT("Cancelled deletion keeps file"), Library->SlotExists(1));
	Widget->HandleAction(EAction::Delete);
	Widget->HandleAction(EAction::Confirm);
	TestFalse(TEXT("Confirmed deletion removes file"), Library->SlotExists(1));
	TestTrue(TEXT("Deletion preserves in-memory content"), Library->IsDirty());
	const FTunaSweeperBossDefinition EditorDraft = Library->GetDraft();
	Library->SetDevelopmentMode(false);
	Library->SetSelectedSlot(0);
	TStrongObjectPtr<UTunaSweeperBossLabWidget> Single(CreateWidget<UTunaSweeperBossLabWidget>(Controller));
	TSharedPtr<SWidget> SingleSlate = Single->TakeWidget();
	TestEqual(TEXT("Single mode hides the editor"), Single->EditorPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Single mode hides mutation actions"), Single->LibraryEditActions->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("Single mode cannot play an unloaded draft"), Single->PlayButton->GetIsEnabled());
	Single->HandleAction(EAction::Load);
	TestTrue(TEXT("Loading a saved slot enables single play"), Single->PlayButton->GetIsEnabled());
	Single->RefreshFromSession();
	TestTrue(TEXT("Battle return keeps loaded single boss ready"), Single->PlayButton->GetIsEnabled());
	Single->SlotCombo->SetSelectedIndex(2);
	Single->HandleAction(EAction::Load);
	TestFalse(TEXT("Corrupt selected slot cannot launch prior draft"), Single->PlayButton->GetIsEnabled());
	Single->SlotCombo->SetSelectedIndex(0);
	Single->HandleAction(EAction::Refresh);
	TestFalse(TEXT("External refresh requires explicit reload"), Single->PlayButton->GetIsEnabled());
	Library->SetDevelopmentMode(true);
	TestEqual(TEXT("Single loads preserve independent editing identity"), Library->GetDraft().BossId, EditorDraft.BossId);
	TestEqual(TEXT("Single loads preserve independent editing tactic"), Library->GetDraft().Tactic, EditorDraft.Tactic);
	TestTrue(TEXT("Single loads preserve dirty editing state"), Library->IsDirty());
	SingleSlate.Reset();
	SlateWidget.Reset();
	return true;
}

#endif
