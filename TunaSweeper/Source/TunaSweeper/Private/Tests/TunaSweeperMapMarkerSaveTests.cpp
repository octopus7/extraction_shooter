#if WITH_DEV_AUTOMATION_TESTS
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperSafeSave.h"
#include "HAL/FileManager.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Settings/TunaSweeperBuildFlavor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMapMarkerSaveTest, "TunaSweeper.Save.MapMarkers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMapMarkerSaveTest::RunTest(const FString& Parameters)
{
	const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense/MapMarkers") / FGuid::NewGuid().ToString();
	if (!TestTrue(TEXT("Isolated save root"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root)))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString());
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	};

	const FString ProgressPath = Root / TEXT("TunaSweeperSave_Slot01.sav");
	auto* Legacy = NewObject<UTunaSweeperSaveGame>();
	Legacy->BuildFlavor = TunaSweeperBuildFlavor::GetName();
	Legacy->SaveSlotIndex = 1;
	auto& Marker = Legacy->MapMarkers.AddDefaulted_GetRef();
	Marker.MarkerId = 7;
	Marker.MapPosition = FVector2D(0.25, 0.75);
	Marker.MarkerIconIndex = 2;
	Marker.MarkerColorIndex = 4;
	const auto Validator = [](const USaveGame& Save) { return Save.IsA<UTunaSweeperSaveGame>(); };
	if (!TestTrue(TEXT("Legacy fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Legacy, ProgressPath, Validator)))
	{
		return false;
	}
	TArray<uint8> Original, Current;
	TestTrue(TEXT("Read legacy progress bytes"), FFileHelper::LoadFileToArray(Original, *ProgressPath));

	auto* Game = NewObject<UTunaSweeperGameInstance>();
	if (!TestTrue(TEXT("Load legacy slot"), Game->ActivateSaveSlot(1, false)))
	{
		return false;
	}
	TArray<FTunaSweeperMapMarkerSaveData> Markers;
	Game->GetMapMarkers(Markers);
	if (TestEqual(TEXT("Legacy marker count"), Markers.Num(), 1))
	{
		TestEqual(TEXT("ID migrated"), Markers[0].MarkerId, 7);
		TestEqual(TEXT("Position migrated"), Markers[0].MapPosition, FVector2D(0.25, 0.75));
		TestEqual(TEXT("Icon migrated"), Markers[0].MarkerIconIndex, 2);
		TestEqual(TEXT("Color migrated"), Markers[0].MarkerColorIndex, 4);
	}
	const FString MarkerPath = Root / TEXT("MapMarkers_Slot01.sav");
	TestTrue(TEXT("Load creates the separate marker file"), FPaths::FileExists(MarkerPath));
	TestTrue(TEXT("Delete the last marker"), Game->RemoveMapMarker(7, true));
	TestTrue(TEXT("Marker deletion leaves progress bytes intact"),
		FFileHelper::LoadFileToArray(Current, *ProgressPath) && Current == Original);
	TestTrue(TEXT("Reload the still-legacy progress file"), Game->ActivateSaveSlot(1, false));
	Game->GetMapMarkers(Markers);
	TestTrue(TEXT("An empty marker file prevents importing deleted markers"), Markers.IsEmpty());
	const int32 AddedId = Game->AddMapMarker(FVector2D(0.5, 0.5), 0, 0, true);
	TestTrue(TEXT("Immediately persist a new marker"), AddedId > 0);
	TestTrue(TEXT("Reload marker from disk"), Game->ActivateSaveSlot(1, false));
	Game->GetMapMarkers(Markers);
	if (TestEqual(TEXT("Only the new marker remains"), Markers.Num(), 1))
	{
		TestEqual(TEXT("New marker ID survives reload"), Markers[0].MarkerId, AddedId);
	}
	TestTrue(TEXT("Save progress after migrating markers"), Game->SetActiveSaveSlotDifficultyStage(2));
	const auto* Progress = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(ProgressPath, Validator));
	TestTrue(TEXT("New gameplay writes leave the legacy marker field empty"), Progress && Progress->MapMarkers.IsEmpty());
	TestTrue(TEXT("Simulate a corrupt active progress file"), FFileHelper::SaveArrayToFile(TArray<uint8>{1, 2, 3}, *ProgressPath));
	TestTrue(TEXT("Load the older progress generation"), Game->ActivateSaveSlot(1, false));
	Game->GetMapMarkers(Markers);
	if (TestEqual(TEXT("Automatic recovery leaves one current marker"), Markers.Num(), 1))
	{
		TestEqual(TEXT("Automatic recovery keeps the current marker"), Markers[0].MarkerId, AddedId);
	}
	if (!TunaSweeperBuildFlavor::IsDemo())
	{
		TestTrue(TEXT("Load a separate slot"), Game->SetActiveSaveSlotIndex(2));
		Game->GetMapMarkers(Markers);
		TestTrue(TEXT("The other slot starts without markers"), Markers.IsEmpty());
		TestTrue(TEXT("Write the other slot's marker independently"), Game->AddMapMarker(FVector2D(0.1, 0.1), 0, 0, true) > 0);
		TestTrue(TEXT("Slot 2 has its own marker file"), FPaths::FileExists(Root / TEXT("MapMarkers_Slot02.sav")));
	}
	TestTrue(TEXT("Delete the entire slot"), Game->DeleteSaveSlot(1));
	TestFalse(TEXT("Explicit slot deletion removes the marker file"), FPaths::FileExists(MarkerPath));
	TestFalse(TEXT("Explicit slot deletion removes the previous marker file"),
		FPaths::FileExists(TunaSweeperSafeSave::GetPreviousFilePath(MarkerPath)));
	if (!TunaSweeperBuildFlavor::IsDemo())
	{
		TestTrue(TEXT("Slot 1 deletion preserves the slot 2 marker file"), FPaths::FileExists(Root / TEXT("MapMarkers_Slot02.sav")));
	}
	return true;
}
#endif
