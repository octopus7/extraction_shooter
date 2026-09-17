#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "UI/TunaSweeperResearchWidgets.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"
#include "Editor.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperResearchTabDistributionTest,
	"TunaSweeper.Research.HudTabDistribution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchTabDistributionTest::RunTest(const FString& Parameters)
{
	UTunaSweeperBuildTargetSettings* Settings = GetMutableDefault<UTunaSweeperBuildTargetSettings>();
	const ETunaSweeperBuildTarget OriginalTarget = Settings->BuildTarget;
	ON_SCOPE_EXIT { Settings->BuildTarget = OriginalTarget; };
	UClass* Class = LoadClass<UTunaSweeperHudTopReserveWidget>(nullptr, TEXT("/Game/UI/WBP_HudTopReserve.WBP_HudTopReserve_C"));
	if (!TestNotNull(TEXT("Authored HUD tabs load"), Class)) return false;
	TStrongObjectPtr<UTunaSweeperHudTopReserveWidget> Tabs(CreateWidget<UTunaSweeperHudTopReserveWidget>(
		GEditor->GetEditorWorldContext().World(), Class));
	if (!TestNotNull(TEXT("HUD tabs instantiate"), Tabs.Get())) return false;
	TSharedRef<SWidget> Slate = Tabs->TakeWidget();
	for (ETunaSweeperBuildTarget Target : {ETunaSweeperBuildTarget::SteamDemo, ETunaSweeperBuildTarget::SteamFull})
	{
		Settings->BuildTarget = Target;
		Tabs->SetActiveMode(ETunaSweeperHudMode::Inventory);
		for (const TCHAR* Name : {TEXT("ResearchModeButton"), TEXT("ResearchModeButtonFrame")})
		{
			UWidget* Widget = Tabs->WidgetTree->FindWidget(Name);
			if (!TestNotNull(TEXT("Research tab and layout frame exist"), Widget)) return false;
			TestEqual(TEXT("Demo removes research and its spacing; full game retains it"), Widget->GetVisibility(),
				Settings->IsDemoBuild() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
		for (const TCHAR* Name : {TEXT("InventoryModeButton"), TEXT("QuestModeButton"), TEXT("MapModeButton"), TEXT("MemoModeButton")})
		{
			UWidget* Widget = Tabs->WidgetTree->FindWidget(Name);
			if (!TestNotNull(TEXT("Other HUD tab exists"), Widget)) return false;
			TestTrue(TEXT("Other HUD tabs remain visible in both builds"), Widget->IsVisible());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperResearchAuthoredNodesTest,
	"TunaSweeper.Research.AuthoredWidgetNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchAuthoredNodesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UClass* TreeClass = LoadClass<UTunaSweeperResearchTreeWidget>(nullptr, TEXT("/Game/UI/WBP_ResearchTree.WBP_ResearchTree_C"));
	UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(TreeClass);
	if (!TestNotNull(TEXT("Research widget Blueprint class loads"), GeneratedClass)) return false;
	UWidgetTree* Tree = GeneratedClass->GetWidgetTreeArchetype();
	if (!TestNotNull(TEXT("Research widget hierarchy is serialized"), Tree)) return false;
	const UVerticalBox* Rows = Cast<UVerticalBox>(Tree->FindWidget(TEXT("TreeRows")));
	const UScrollBox* Scroll = Cast<UScrollBox>(Tree->FindWidget(TEXT("ResearchScrollBox")));
	if (!TestNotNull(TEXT("Research rows exist"), Rows) || !TestNotNull(TEXT("Research scroll container exists"), Scroll)) return false;
	TestTrue(TEXT("All research rows stay in the scroll container"), Rows->GetParent() == Scroll);

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	TArray<FTunaSweeperResearchNodeView> Views;
	if (!TestTrue(TEXT("Research definitions load"), Research->GetAllNodeViews(Views))) return false;
	TArray<UWidget*> Widgets;
	Tree->GetAllWidgets(Widgets);
	TMap<FName, UTunaSweeperResearchNodeWidget*> NodesById;
	for (UWidget* Widget : Widgets)
	{
		if (UTunaSweeperResearchNodeWidget* Node = Cast<UTunaSweeperResearchNodeWidget>(Widget))
		{
			TestFalse(FString::Printf(TEXT("Serialized research node is unique: %s"), *Node->NodeId.ToString()), NodesById.Contains(Node->NodeId));
			NodesById.Add(Node->NodeId, Node);
		}
	}
	TestEqual(TEXT("Every authored research definition has one actual WBP node"), NodesById.Num(), Views.Num());
	for (const FTunaSweeperResearchNodeView& View : Views)
	{
		UTunaSweeperResearchNodeWidget* const* Node = NodesById.Find(View.NodeId);
		if (!TestNotNull(FString::Printf(TEXT("Authored research node is reachable: %s"), *View.NodeId.ToString()), Node)) continue;
		const UHorizontalBox* Row = Cast<UHorizontalBox>(Tree->FindWidget(*FString::Printf(TEXT("ResearchRow_%d"), View.Row)));
		if (!TestNotNull(TEXT("Authored research row exists"), Row)) continue;
		TestTrue(TEXT("Research row belongs to the scrollable row container"), Row->GetParent() == Rows);
		TestEqual(FString::Printf(TEXT("Research node is in its authored column: %s"), *View.NodeId.ToString()), Row->GetChildIndex(*Node), View.Column);
		TestFalse(TEXT("Research effect description is localized"), View.Description.IsEmpty());
	}
	return true;
}

#endif
