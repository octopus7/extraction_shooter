#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperGameUserSettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"

namespace HopperDailyIntroTest
{
	const TCHAR* Village = TEXT("/Game/MainRaid/RaidVillage");
	const TCHAR* TestMap = TEXT("/Game/Maps/HopperCombatTestMap");
	const FVector MechPosition(0., 0., 15000.);
	enum class EEncounter { Complete, Interrupt, Skip };

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCheckMap, FAutomationTestBase*, Test, FString, Package);
	bool FCheckMap::Update()
	{
		UWorld* World = GEditor->PlayWorld;
		auto* PC = World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
		auto* Pawn = PC ? Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn()) : nullptr;
		int32 Width = 0, Height = 0;
		if (PC) PC->GetViewportSize(Width, Height);
		if (!Pawn || !PC->PlayerCameraManager || !World->HasBegunPlay() || World->GetTimeSeconds() < .25f ||
			!PC->IsLocalPlayerController() || Width <= 0 || Height <= 0)
		{
			if (FPlatformTime::Seconds() - StartTime < 60.) return false;
			Test->AddError(TEXT("Daily intro PIE did not acquire a player and game viewport"));
			return true;
		}
		if (!Test->TestEqual(TEXT("PIE is playing the requested map"), UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()), Package)) return true;
		UClass* Mk2 = LoadClass<ATunaSweeperHopperEnemyCharacter>(nullptr, TEXT("/Game/Characters/Hopper/BP_HopperEnemyMk2.BP_HopperEnemyMk2_C"));
		if (!Test->TestNotNull(TEXT("Mk2 encounter class"), Mk2)) return true;

		// The production setters write immediately. Keep the engine object and its ini isolated within this update.
		const FString Ini = FConfigCacheIni::NormalizeConfigIniPath(FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("HopperIntro"), TEXT(".ini")));
		GConfig->Add(Ini, FConfigFile());
		ON_SCOPE_EXIT { GConfig->UnloadFile(Ini); IFileManager::Get().Delete(*Ini); };
		TGuardValue<FString> IniGuard(GGameUserSettingsIni, Ini);
		TStrongObjectPtr<UGameUserSettings> OriginalSettings(GEngine->GameUserSettings.Get());
		auto* Settings = NewObject<UTunaSweeperGameUserSettings>();
		TGuardValue<TObjectPtr<UGameUserSettings>> SettingsGuard(GEngine->GameUserSettings, Settings);
		const FString Section = Settings->GetClass()->GetPathName();
		const TCHAR* DateKey = TEXT("LastVillageCombatIntroDate");
		const FString Today = FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
		const FString OldDate = TEXT("2000-12-31");
		auto SeedDate = [&](const FString& Date)
		{
			GConfig->SetString(*Section, DateKey, *Date, Ini);
			GConfig->Flush(false, Ini);
			Settings->LoadConfig(Settings->GetClass(), *Ini);
		};
		auto ReadDate = [&]()
		{
			FConfigFile Saved;
			Saved.Read(Ini);
			FString Date;
			Saved.GetString(*Section, DateKey, Date);
			return Date;
		};
		Settings->SetToDefaults();
		Test->TestTrue(TEXT("The daily limit defaults to enabled"), Settings->IsDailyCombatIntroEnabled());
		SeedDate(OldDate);
		Test->TestFalse(TEXT("The stored local date is already used"), Settings->ShouldShowVillageCombatIntro(OldDate));
		Test->TestTrue(TEXT("A new calendar year permits a fresh showing"), Settings->ShouldShowVillageCombatIntro(TEXT("2001-01-01")));
		Settings->SetDailyCombatIntroEnabled(true);

		const FVector CameraLocation = MechPosition + FVector(-550., -420., 320.);
		auto* Camera = World->SpawnActor<ACameraActor>(CameraLocation, (MechPosition - CameraLocation).Rotation());
		if (!Test->TestNotNull(TEXT("A camera can frame the cockpit"), Camera)) return true;
		const FTransform OriginalPlayerTransform = Pawn->GetActorTransform();
		TWeakObjectPtr<AActor> PreviousView = PC->GetViewTarget();
		ON_SCOPE_EXIT
		{
			PC->SetViewTarget(PreviousView.IsValid() ? PreviousView.Get() : Pawn);
			if (PC->IsHopperShotActive()) PC->PlayerTick(.016f);
			Pawn->SetActorTransform(OriginalPlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Camera->Destroy();
		};
		auto Encounter = [&](EEncounter Route, const FString& ExpectedDate)
		{
			const FString Before = ReadDate();
			PC->SetViewTarget(Camera);
			if (PC->IsHopperShotActive()) PC->PlayerTick(0.f);
			PC->PlayerCameraManager->UpdateCamera(.016f);
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			auto* Hopper = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Mk2, MechPosition, FRotator::ZeroRotator, Spawn);
			if (!Test->TestNotNull(TEXT("A new encounter can spawn"), Hopper)) return;
			ON_SCOPE_EXIT { Hopper->Destroy(); };
			auto* Stage = Hopper->GetPresentation();
			Pawn->SetActorLocation(MechPosition - FVector(10000., 0., 0.), false, nullptr, ETeleportType::TeleportPhysics);
			AActor* EncounterActor = Hopper;
			EncounterActor->Tick(.016f);
			Test->TestEqual(TEXT("Even a repeat encounter waits for actual sight"), Hopper->GetCombatPhase(), EHopperCombatPhase::WaitingForSight);
			Test->TestEqual(TEXT("Waiting spends no date"), ReadDate(), Before);

			Pawn->SetActorLocation(MechPosition - FVector(300., 0., 0.), false, nullptr, ETeleportType::TeleportPhysics);
			Pawn->SetActorRotation(FRotator::ZeroRotator);
			Pawn->SetAimWorldPoint(MechPosition);
			const auto* Subject = Hopper->FindComponentByClass<UTunaSweeperVisionSubjectComponent>();
			const bool bSight = Subject && Pawn->GetPlayerVisionComponent()->HasSightOf(*Subject);
			FVector2D Screen = FVector2D::ZeroVector;
			const FVector Cockpit = Stage->GetBodyRoot()->GetComponentTransform().TransformPosition(Hopper->VisualData.Get()->SeatLocation);
			const bool bProjected = PC->ProjectWorldLocationToScreen(Cockpit, Screen);
			const bool bInViewport = bProjected && Screen.X >= 0 && Screen.Y >= 0 && Screen.X < Width && Screen.Y < Height;
			EncounterActor->Tick(.016f);
			if (!bSight || !bInViewport || Hopper->GetCombatPhase() == EHopperCombatPhase::WaitingForSight)
				Test->AddError(FString::Printf(TEXT("Daily intro handoff map=%s route=%d gameTime=%.3f pawn=%s pos=%s rot=%s dead=%d mech=%s sight=%d camera=%s eye=%s projection=%d pixel=%s viewport=%dx%d phase=%d owned=%d local=%d dialogue=%d inventory=%d pause=%d housing=%d"),
					*Package, static_cast<int32>(Route), World->GetTimeSeconds(), *GetNameSafe(Pawn), *Pawn->GetActorLocation().ToCompactString(),
					*Pawn->GetActorRotation().ToCompactString(), Pawn->IsDead(), *Hopper->GetActorLocation().ToCompactString(), bSight, *GetNameSafe(PC->GetViewTarget()),
					*PC->PlayerCameraManager->GetCameraLocation().ToCompactString(), bProjected, *Screen.ToString(), Width, Height, static_cast<int32>(Hopper->GetCombatPhase()),
					PC->IsHopperShotActive(), PC->IsLocalPlayerController(), PC->IsDialogueSequenceActive(), PC->IsInventoryUiOpen(), PC->IsPauseMenuOpen(), PC->IsHousingModeOpen()));
			if (Route == EEncounter::Skip)
			{
				const auto* Seat = Stage->GetPilotMesh()->GetAttachParent();
				Test->TestTrue(TEXT("A same-day repeat enters mounted walking combat without taking the player's camera or input"),
					Hopper->GetCombatPhase() == EHopperCombatPhase::Mech && Seat && Seat->GetAttachParent() == Stage->GetBodyRoot() &&
					Hopper->GetCharacterMovement()->MovementMode == MOVE_Walking && Hopper->GetCharacterMovement()->MaxWalkSpeed > 0.f &&
					!PC->IsHopperShotActive() && !PC->IsMoveInputIgnored() && PC->GetViewTarget() == Camera);
			}
			else
			{
				Test->TestTrue(TEXT("The visible first encounter starts the climb and takes the camera and input"),
					Hopper->GetCombatPhase() == EHopperCombatPhase::Boarding && Stage->IsBoardingInProgress() &&
					PC->IsViewingHopper(Hopper) && PC->IsMoveInputIgnored());
			}
			Test->TestEqual(TEXT("Starting the encounter does not mark boarding complete"), ReadDate(), Before);
			if (Route == EEncounter::Complete)
			{
				const float Docked = Stage->GetBoardingClimbEndTime(Hopper->BoardingSeconds) + Stage->GetSeatDockDuration() + .1f;
				Stage->TickComponent(Docked, LEVELTICK_All, nullptr);
				EncounterActor->Tick(Docked);
				Test->TestEqual(TEXT("The captain finishes boarding"), Hopper->GetCombatPhase(), EHopperCombatPhase::Mech);
			}
			else
			{
				Hopper->TakeDamage(10000.f, FDamageEvent(), nullptr, nullptr);
				Test->TestTrue(TEXT("A defeat still starts the ejection shot"), Hopper->IsDead() && PC->IsViewingHopper(Hopper) && PC->IsMoveInputIgnored());
				Stage->TickComponent(1.1f, LEVELTICK_All, nullptr);
				EncounterActor->Tick(1.1f);
				const auto* Seat = Stage->GetPilotMesh()->GetAttachParent();
				Test->TestTrue(TEXT("The occupied seat ejects even when the boarding shot was skipped or interrupted"), Seat && !Seat->GetAttachParent());
			}
			Test->TestEqual(TEXT("Only a completed daily village boarding changes the persisted date"), ReadDate(), ExpectedDate);
		};
		if (Package == Village)
		{
			Encounter(EEncounter::Interrupt, OldDate);
			Encounter(EEncounter::Complete, Today);
			Encounter(EEncounter::Skip, Today);
		}
		else
		{
			Encounter(EEncounter::Complete, OldDate);
			SeedDate(Today);
			Encounter(EEncounter::Complete, Today);
		}
		Settings->SetDailyCombatIntroEnabled(false);
		Encounter(EEncounter::Complete, Today);
		SeedDate(OldDate);
		Encounter(EEncounter::Complete, OldDate);
		auto* Reloaded = NewObject<UTunaSweeperGameUserSettings>();
		Reloaded->LoadConfig(Reloaded->GetClass(), *Ini);
		Test->TestFalse(TEXT("A new settings instance restores the disabled preference"), Reloaded->IsDailyCombatIntroEnabled());
		Reloaded->SetDailyCombatIntroEnabled(true);
		Test->TestFalse(TEXT("The completed date survives reloading and toggling the preference"), Reloaded->ShouldShowVillageCombatIntro(OldDate));
		Test->TestTrue(TEXT("Turning the daily limit back on after an unrestricted view still allows today's first showing"),
			Reloaded->ShouldShowVillageCombatIntro(Today));
		return true;
	}
	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForTeardown, FAutomationTestBase*, Test);
	bool FWaitForTeardown::Update()
	{
		if (!GEditor->PlayWorld) return true;
		if (FPlatformTime::Seconds() - StartTime < 20.) return false;
		Test->AddError(TEXT("The daily intro fixture PIE world did not finish tearing down"));
		return true;
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FHopperDailyIntroTest, "TunaSweeper.Hopper.Play.DailyCombatIntro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FHopperDailyIntroTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	Names.Add(TEXT("Village")); Commands.Add(HopperDailyIntroTest::Village);
	Names.Add(TEXT("CombatTestMap")); Commands.Add(HopperDailyIntroTest::TestMap);
}

bool FHopperDailyIntroTest::RunTest(const FString& Package)
{
	FRequestPlaySessionParams Play;
	Play.GlobalMapOverride = Package + TEXT(".") + FPaths::GetCleanFilename(Package);
	Play.GameModeOverride = LoadClass<AGameModeBase>(nullptr, TEXT("/Game/Characters/Hopper/BP_HopperTestGameMode.BP_HopperTestGameMode_C"));
	if (!TestNotNull(TEXT("The Hopper test game mode loads"), Play.GameModeOverride.Get())) return false;
	Play.StartLocation = HopperDailyIntroTest::MechPosition - FVector(10000., 0., 0.);
	Play.StartRotation = FRotator::ZeroRotator;
	Play.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>();
	GEditor->RequestPlaySession(Play);
	ADD_LATENT_AUTOMATION_COMMAND(HopperDailyIntroTest::FCheckMap(this, Package));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(HopperDailyIntroTest::FWaitForTeardown(this));
	return true;
}

#endif
