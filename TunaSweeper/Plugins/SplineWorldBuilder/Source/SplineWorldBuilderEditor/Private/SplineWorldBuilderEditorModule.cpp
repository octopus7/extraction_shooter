#include "SplineWorldBuilderActor.h"
#include "SplineWorldBuilderJunctionActor.h"
#include "SplineWorldBuilderProfile.h"

#include "Components/SplineComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "SplineWorldBuilderEditor"

namespace SplineWorldBuilderEditor
{
	FVector ChooseSpawnLocation()
	{
		if (GEditor)
		{
			if (USelection* Selection = GEditor->GetSelectedActors())
			{
				if (AActor* Actor = Selection->GetTop<AActor>())
				{
					return Actor->GetActorLocation() + FVector(0.0, 0.0, 20.0);
				}
			}
		}
		return FVector::ZeroVector;
	}

	ASplineWorldBuilderActor* SpawnChain(
		UWorld* World,
		ASplineWorldJunctionActor* Junction,
		USplineWorldBuilderProfile* Profile,
		const FVector& Direction,
		const FString& Label)
	{
		if (!World || !Junction || !Profile)
		{
			return nullptr;
		}

		ASplineWorldBuilderActor* Chain = World->SpawnActor<ASplineWorldBuilderActor>(
			ASplineWorldBuilderActor::StaticClass(),
			Junction->GetActorTransform());
		if (!Chain)
		{
			return nullptr;
		}
		Chain->SetFlags(RF_Transactional);
		Chain->SetActorLabel(Label);
		Chain->SetFolderPath(TEXT("SplineWorldBuilder/TestJunctions"));
		Chain->Profile = Profile;
		Chain->StartJunction = Junction;
		Chain->bAutoRebuild = false;
		USplineComponent* Spline = Chain->GetBuilderSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(Direction.GetSafeNormal() * 600.0, ESplineCoordinateSpace::Local, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		Chain->RebuildGenerated();

		FSplineWorldJunctionConnection& Connection = Junction->Connections.AddDefaulted_GetRef();
		Connection.Chain = Chain;
		Connection.Endpoint = ESplineWorldEndpoint::Start;
		return Chain;
	}

	ASplineWorldJunctionActor* SpawnJunctionExample(
		UWorld* World,
		USplineWorldBuilderProfile* Profile,
		const FVector& Location,
		const FString& Label,
		const TArray<FVector>& Directions)
	{
		ASplineWorldJunctionActor* Junction = World->SpawnActor<ASplineWorldJunctionActor>(
			ASplineWorldJunctionActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Location));
		if (!Junction)
		{
			return nullptr;
		}
		Junction->SetFlags(RF_Transactional);
		Junction->SetActorLabel(Label);
		Junction->SetFolderPath(TEXT("SplineWorldBuilder/TestJunctions"));
		Junction->Profile = Profile;
		Junction->bAutoRebuild = false;

		for (int32 Index = 0; Index < Directions.Num(); ++Index)
		{
			SpawnChain(World, Junction, Profile, Directions[Index], FString::Printf(TEXT("%s_Arm_%d"), *Label, Index + 1));
		}
		Junction->RebuildGenerated();
		return Junction;
	}
}

class FSplineWorldBuilderEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSplineWorldBuilderEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"), NAME_None, EMultiBoxType::Menu, false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
			TEXT("WorldBuilding"), LOCTEXT("WorldBuildingSection", "World Building"));
		Section.AddMenuEntry(
			TEXT("AddSplineWorldBuilderJunctionTests"),
			LOCTEXT("AddJunctionTests", "Spline World Builder: Add Junction Test Set"),
			LOCTEXT("AddJunctionTestsTooltip", "Add End, Straight, Corner, T, and Cross test networks using the saved stone profile."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FSplineWorldBuilderEditorModule::AddJunctionTestSet)));
	}

	void AddJunctionTestSet()
	{
		if (!GEditor)
		{
			return;
		}
		USplineWorldBuilderProfile* Profile = LoadObject<USplineWorldBuilderProfile>(
			nullptr, TEXT("/SplineWorldBuilder/Profiles/DA_SWB_TestStoneWall.DA_SWB_TestStoneWall"));
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!Profile || !World)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddJunctionTestSetTransaction", "Add Spline Junction Test Set"));
		World->Modify();
		const FVector Origin = SplineWorldBuilderEditor::ChooseSpawnLocation();
		TArray<ASplineWorldJunctionActor*> Junctions;
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Profile, Origin, TEXT("SWB_End"), { FVector::ForwardVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Profile, Origin + FVector(900.0, 0.0, 0.0), TEXT("SWB_Straight"),
			{ FVector::ForwardVector, -FVector::ForwardVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Profile, Origin + FVector(0.0, 900.0, 0.0), TEXT("SWB_Corner"),
			{ FVector::ForwardVector, FVector::RightVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Profile, Origin + FVector(900.0, 900.0, 0.0), TEXT("SWB_T"),
			{ FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Profile, Origin + FVector(1800.0, 900.0, 0.0), TEXT("SWB_Cross"),
			{ FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector }));

		GEditor->SelectNone(false, true, false);
		for (ASplineWorldJunctionActor* Junction : Junctions)
		{
			if (Junction)
			{
				GEditor->SelectActor(Junction, true, false, true);
			}
		}
		GEditor->NoteSelectionChange();
	}

};

IMPLEMENT_MODULE(FSplineWorldBuilderEditorModule, SplineWorldBuilderEditor)

#undef LOCTEXT_NAMESPACE
