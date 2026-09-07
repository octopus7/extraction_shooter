// One-off authoring tool. Commit the saved WBP with this file, then remove this file and its UMGEditor dependency.
#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "UI/TunaSweeperResearchWidgets.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperAddBurnResearchNodes,
	"TunaSweeper.Tools.AddBurnResearchNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperAddBurnResearchNodes::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/WBP_ResearchTree.WBP_ResearchTree"));
	UClass* NodeClass = LoadClass<UTunaSweeperResearchNodeWidget>(nullptr, TEXT("/Game/UI/WBP_ResearchNode.WBP_ResearchNode_C"));
	if (!TestNotNull(TEXT("Research tree Blueprint"), Blueprint) ||
		!TestNotNull(TEXT("Research node class"), NodeClass)) return false;
	UWidgetTree* Tree = Blueprint->WidgetTree;
	if (!TestNotNull(TEXT("Research source widget tree"), Tree)) return false;
	UVerticalBox* TreeRows = Cast<UVerticalBox>(Tree->FindWidget(TEXT("TreeRows")));
	if (!TestNotNull(TEXT("Existing scrollable row container"), TreeRows)) return false;

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperResearchSubsystem* Research = NewObject<UTunaSweeperResearchSubsystem>(GameInstance);
	TArray<FTunaSweeperResearchNodeView> Views;
	if (!TestTrue(TEXT("Research definitions load"), Research->GetAllNodeViews(Views))) return false;
	TArray<UWidget*> NewWidgets;
	Blueprint->Modify();
	Tree->Modify();
	TreeRows->Modify();
	for (const int32 RowIndex : {7, 8})
	{
		const FName RowName(*FString::Printf(TEXT("ResearchRow_%d"), RowIndex));
		UHorizontalBox* Row = Cast<UHorizontalBox>(Tree->FindWidget(RowName));
		if (!Row)
		{
			Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), RowName);
			if (!TestNotNull(TEXT("New research row"), Row)) return false;
			NewWidgets.Add(Row);
			if (UVerticalBoxSlot* RowSlot = TreeRows->AddChildToVerticalBox(Row)) RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		for (int32 Column = 0; Column < 3; ++Column)
		{
			const FTunaSweeperResearchNodeView* View = Views.FindByPredicate([RowIndex, Column](const FTunaSweeperResearchNodeView& Candidate)
			{
				return Candidate.Row == RowIndex && Candidate.Column == Column;
			});
			if (Column < Row->GetChildrenCount())
			{
				const UTunaSweeperResearchNodeWidget* ExistingNode = Cast<UTunaSweeperResearchNodeWidget>(Row->GetChildAt(Column));
				if (View && !TestTrue(TEXT("Existing burn row agrees with authored data"), ExistingNode && ExistingNode->NodeId == View->NodeId)) return false;
				continue;
			}
			UWidget* Cell = nullptr;
			if (View)
			{
				const FName NodeName(*FString::Printf(TEXT("ResearchNode_%s"), *View->NodeId.ToString()));
				UTunaSweeperResearchNodeWidget* Node = Cast<UTunaSweeperResearchNodeWidget>(
					Tree->ConstructWidget<UUserWidget>(NodeClass, NodeName));
				if (!TestNotNull(TEXT("Serialized research node"), Node)) return false;
				Node->NodeId = View->NodeId;
				Cell = Node;
			}
			else
			{
				Cell = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Empty_%d_%d"), RowIndex, Column));
			}
			if (!TestNotNull(TEXT("Research row cell"), Cell)) return false;
			NewWidgets.Add(Cell);
			if (UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell))
			{
				CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				CellSlot->SetHorizontalAlignment(HAlign_Center);
				CellSlot->SetPadding(FMargin(10.0f, 12.0f));
			}
		}
	}
	for (UWidget* Widget : NewWidgets)
	{
		Widget->bIsVariable = true;
		if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())) Blueprint->OnVariableAdded(Widget->GetFName());
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (!TestTrue(TEXT("Research Blueprint compiles"), Blueprint->Status != BS_Error)) return false;
	Blueprint->MarkPackageDirty();
	UPackage* Package = Blueprint->GetOutermost();
	const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	return TestTrue(TEXT("Research Blueprint saved"), UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs));
}

#endif
