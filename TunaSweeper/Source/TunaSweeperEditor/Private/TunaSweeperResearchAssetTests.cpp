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
