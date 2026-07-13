#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "AI/TunaSweeperEnemySquadTypes.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "Subsystem/TunaSweeperEnemySquadSubsystem.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace TunaSweeperEnemyCombatTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	bool TestNearlyEqual(
		FAutomationTestBase& Test,
		const FString& Description,
		float Actual,
		float Expected)
	{
		return Test.TestTrue(Description, FMath::IsNearlyEqual(Actual, Expected));
	}

	bool LoadProfile(
		FAutomationTestBase& Test,
		UTunaSweeperEnemySpawnSubsystem* SpawnSubsystem,
		const TCHAR* ProfileId,
		FTunaSweeperEnemyCombatProfile& OutProfile)
	{
		const bool bFound = SpawnSubsystem && SpawnSubsystem->TryGetEnemyCombatProfile(
			FName(ProfileId),
			OutProfile);
		Test.TestTrue(FString::Printf(TEXT("Combat profile %s exists"), ProfileId), bFound);
		return bFound;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperFactionRelationshipTest,
	"TunaSweeper.Combat.Faction.Relationships",
	TunaSweeperEnemyCombatTests::TestFlags)

bool FTunaSweeperFactionRelationshipTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UTunaSweeperFactionSubsystem* FactionSubsystem =
		GetDefault<UTunaSweeperFactionSubsystem>();
	TestNotNull(TEXT("Faction subsystem default object exists"), FactionSubsystem);
	if (!FactionSubsystem)
	{
		return false;
	}

	TestEqual(
		TEXT("Equal valid faction ids are friendly"),
		static_cast<uint8>(FactionSubsystem->GetFactionAttitudeById(10, 10)),
		static_cast<uint8>(ETunaSweeperFactionAttitude::Friendly));
	TestEqual(
		TEXT("Different valid faction ids are hostile"),
		static_cast<uint8>(FactionSubsystem->GetFactionAttitudeById(1, 10)),
		static_cast<uint8>(ETunaSweeperFactionAttitude::Hostile));
	TestEqual(
		TEXT("NoFaction source is neutral"),
		static_cast<uint8>(FactionSubsystem->GetFactionAttitudeById(TunaSweeperFactionIds::NoFaction, 10)),
		static_cast<uint8>(ETunaSweeperFactionAttitude::Neutral));
	TestEqual(
		TEXT("NoFaction target is neutral"),
		static_cast<uint8>(FactionSubsystem->GetFactionAttitudeById(10, TunaSweeperFactionIds::NoFaction)),
		static_cast<uint8>(ETunaSweeperFactionAttitude::Neutral));
	TestEqual(
		TEXT("Two NoFaction ids remain neutral"),
		static_cast<uint8>(FactionSubsystem->GetFactionAttitudeById(
			TunaSweeperFactionIds::NoFaction,
			TunaSweeperFactionIds::NoFaction)),
		static_cast<uint8>(ETunaSweeperFactionAttitude::Neutral));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperPlayerFootstepNoiseTest,
	"TunaSweeper.Combat.Noise.PlayerFootsteps",
	TunaSweeperEnemyCombatTests::TestFlags)

bool FTunaSweeperPlayerFootstepNoiseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName TestWorldName = MakeUniqueObjectName(
		GetTransientPackage(),
		UWorld::StaticClass(),
		TEXT("TunaSweeperPlayerFootstepNoiseTestWorld"));
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	TestNotNull(TEXT("Transient footstep noise world exists"), TestWorld);
	if (!TestWorld)
	{
		return false;
	}

	UTunaSweeperNoiseSubsystem* NoiseSubsystem = TestWorld->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	TestNotNull(TEXT("Noise subsystem exists"), NoiseSubsystem);
	if (!NoiseSubsystem)
	{
		TestWorld->DestroyWorld(false);
		TestWorld->RemoveFromRoot();
		return false;
	}

	int32 ReportedNoiseCount = 0;
	FTunaSweeperNoiseEvent ReportedNoise;
	const FDelegateHandle DelegateHandle = NoiseSubsystem->OnNoiseReported.AddLambda(
		[&ReportedNoiseCount, &ReportedNoise](const FTunaSweeperNoiseEvent& NoiseEvent)
		{
			++ReportedNoiseCount;
			ReportedNoise = NoiseEvent;
		});
	NoiseSubsystem->ReportNoiseAtLocation(
		FVector(1000.0f, 0.0f, 0.0f),
		0.45f,
		1600.0f,
		FName(TEXT("noise.player_footstep")));
	TestEqual(TEXT("Player footstep reports exactly one noise event"), ReportedNoiseCount, 1);
	TestEqual(TEXT("Player footstep uses its authored noise tag"), ReportedNoise.NoiseTag, FName(TEXT("noise.player_footstep")));

	FTunaSweeperHeardNoiseEvent HeardNoise;
	TestTrue(
		TEXT("Default enemy hearing detects a walking footstep at 10 metres"),
		NoiseSubsystem->CalculateHeardNoiseAtLocation(
			ReportedNoise,
			FVector::ZeroVector,
			1800.0f,
			1.0f,
			0.08f,
			HeardNoise));

	ReportedNoise.SourceLocation = FVector(1400.0f, 0.0f, 0.0f);
	TestFalse(
		TEXT("Walking footstep falls below the hearing threshold at 14 metres"),
		NoiseSubsystem->CalculateHeardNoiseAtLocation(
			ReportedNoise,
			FVector::ZeroVector,
			1800.0f,
			1.0f,
			0.08f,
			HeardNoise));

	ReportedNoise.Loudness = 0.8f;
	ReportedNoise.MaxRange = 2200.0f;
	TestTrue(
		TEXT("Default enemy hearing detects a sprinting footstep at 14 metres"),
		NoiseSubsystem->CalculateHeardNoiseAtLocation(
			ReportedNoise,
			FVector::ZeroVector,
			1800.0f,
			1.0f,
			0.08f,
			HeardNoise));

	NoiseSubsystem->OnNoiseReported.Remove(DelegateHandle);
	TestWorld->DestroyWorld(false);
	TestWorld->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperEnemyCombatDataTest,
	"TunaSweeper.Combat.Data.ProfilesAndWeapons",
	TunaSweeperEnemyCombatTests::TestFlags)

bool FTunaSweeperEnemyCombatDataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGameInstance* TestGameInstance = NewObject<UGameInstance>();
	UTunaSweeperEnemySpawnSubsystem* SpawnSubsystem =
		NewObject<UTunaSweeperEnemySpawnSubsystem>(TestGameInstance);
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem =
		NewObject<UTunaSweeperItemDataSubsystem>(TestGameInstance);
	TestNotNull(TEXT("Test game instance exists"), TestGameInstance);
	TestNotNull(TEXT("Enemy spawn subsystem exists"), SpawnSubsystem);
	TestNotNull(TEXT("Item data subsystem exists"), ItemDataSubsystem);
	if (!TestGameInstance || !SpawnSubsystem || !ItemDataSubsystem)
	{
		return false;
	}

	const bool bProfilesLoaded = SpawnSubsystem->LoadEnemyCombatProfileData(true);
	TestTrue(TEXT("Enemy combat profile JSON loads"), bProfilesLoaded);
	if (!bProfilesLoaded)
	{
		return false;
	}

	FTunaSweeperEnemyCombatProfile PistolProfile;
	FTunaSweeperEnemyCombatProfile RifleProfile;
	FTunaSweeperEnemyCombatProfile EliteProfile;
	FTunaSweeperEnemyCombatProfile MeleeProfile;
	const bool bHasAllProfiles =
		TunaSweeperEnemyCombatTests::LoadProfile(*this, SpawnSubsystem, TEXT("enemy.pistol_flanker"), PistolProfile) &
		TunaSweeperEnemyCombatTests::LoadProfile(*this, SpawnSubsystem, TEXT("enemy.rifle_anchor"), RifleProfile) &
		TunaSweeperEnemyCombatTests::LoadProfile(*this, SpawnSubsystem, TEXT("enemy.elite_rifle_anchor"), EliteProfile) &
		TunaSweeperEnemyCombatTests::LoadProfile(*this, SpawnSubsystem, TEXT("enemy.melee_lumberjack"), MeleeProfile);
	if (!bHasAllProfiles)
	{
		return false;
	}

	struct FExpectedRangedProfile
	{
		FTunaSweeperEnemyCombatProfile* Profile;
		int32 ShotCount;
		int32 OpeningShotCount;
		float ShotIntervalMin;
		float ShotIntervalMax;
		float NonFiringMin;
		float NonFiringMax;
		float TurnSpeed;
		float FacingTolerance;
		float WeaponSpreadMultiplier;
		float TrackingRange;
	};

	const FExpectedRangedProfile ExpectedProfiles[] =
	{
		{ &PistolProfile, 1, 1, 0.0f, 0.0f, 2.0f, 2.6f, 220.0f, 8.0f, 2.5f, 1050.0f },
		{ &RifleProfile, 3, 2, 0.18f, 0.23f, 2.6f, 3.5f, 180.0f, 8.0f, 2.0f, 1150.0f },
		{ &EliteProfile, 4, 3, 0.15f, 0.19f, 3.0f, 4.0f, 200.0f, 7.0f, 1.75f, 1250.0f }
	};

	for (const FExpectedRangedProfile& Expected : ExpectedProfiles)
	{
		const FTunaSweeperEnemyCombatProfile& Profile = *Expected.Profile;
		const FString ProfileName = Profile.ProfileId.ToString();
		TestEqual(*FString::Printf(TEXT("%s shot count"), *ProfileName), Profile.FiringShotCount, Expected.ShotCount);
		TestEqual(
			*FString::Printf(TEXT("%s opening shot count"), *ProfileName),
			Profile.OpeningFiringShotCount,
			Expected.OpeningShotCount);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s minimum shot interval"), *ProfileName),
			Profile.ShotIntervalSecondsMin,
			Expected.ShotIntervalMin);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s maximum shot interval"), *ProfileName),
			Profile.ShotIntervalSecondsMax,
			Expected.ShotIntervalMax);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s minimum non-firing window"), *ProfileName),
			Profile.RecoverSecondsMin + Profile.ObserveSecondsMin,
			Expected.NonFiringMin);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s maximum non-firing window"), *ProfileName),
			Profile.RecoverSecondsMax + Profile.ObserveSecondsMax,
			Expected.NonFiringMax);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s turn speed"), *ProfileName),
			Profile.TurnSpeedDegreesPerSecond,
			Expected.TurnSpeed);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s attack facing tolerance"), *ProfileName),
			Profile.AttackFacingToleranceDegrees,
			Expected.FacingTolerance);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s weapon spread multiplier"), *ProfileName),
			Profile.WeaponSpreadMultiplier,
			Expected.WeaponSpreadMultiplier);
		TunaSweeperEnemyCombatTests::TestNearlyEqual(
			*this,
			FString::Printf(TEXT("%s tracking range"), *ProfileName),
			Profile.TrackingRange,
			Expected.TrackingRange);
		TestEqual(
			*FString::Printf(TEXT("%s uses ranged attack mode"), *ProfileName),
			static_cast<uint8>(Profile.AttackMode),
			static_cast<uint8>(ETunaSweeperEnemyAttackMode::Ranged));
		TestTrue(
			*FString::Printf(TEXT("%s opening burst is bounded"), *ProfileName),
			Profile.OpeningFiringShotCount > 0 &&
			Profile.OpeningFiringShotCount <= Profile.FiringShotCount);
		TestTrue(
			*FString::Printf(TEXT("%s time ranges are ordered"), *ProfileName),
			Profile.RecoverSecondsMin <= Profile.RecoverSecondsMax &&
			Profile.ObserveSecondsMin <= Profile.ObserveSecondsMax);
		TestTrue(
			*FString::Printf(TEXT("%s distance ranges are ordered"), *ProfileName),
			Profile.DangerRange <= Profile.PreferredRangeMin &&
			Profile.PreferredRangeMin <= Profile.PreferredRangeMax &&
			Profile.RepositionDistanceMin <= Profile.RepositionDistanceMax);
	}

	TestEqual(
		TEXT("Pistol profile is the flanker"),
		static_cast<uint8>(PistolProfile.Role),
		static_cast<uint8>(ETunaSweeperEnemyCombatRole::Flanker));
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Flanker cross-reposition chance is 15 percent"),
		PistolProfile.CrossRepositionChance,
		0.15f);
	TestTrue(TEXT("Flanker cross cooldown is at least 6 seconds"), PistolProfile.CrossRepositionCooldownSeconds >= 6.0f);
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Flanker direction anchor is 1.5 metres"),
		PistolProfile.CrossRepositionOrbitRadius,
		150.0f);
	TestTrue(
		TEXT("Flanker step remains between 2 and 3 metres"),
		PistolProfile.RepositionDistanceMin >= 200.0f &&
		PistolProfile.RepositionDistanceMax <= 300.0f);
	TestTrue(
		TEXT("Flanker combat band is wider than its direction anchor"),
		PistolProfile.PreferredRangeMin > PistolProfile.CrossRepositionOrbitRadius);

	const FTunaSweeperEnemyCombatProfile* NonFlankerProfiles[] =
	{
		&RifleProfile,
		&EliteProfile,
		&MeleeProfile
	};
	for (const FTunaSweeperEnemyCombatProfile* Profile : NonFlankerProfiles)
	{
		TestTrue(
			*FString::Printf(TEXT("%s cannot cross-reposition"), *Profile->ProfileId.ToString()),
			FMath::IsNearlyZero(Profile->CrossRepositionChance));
	}
	TestEqual(
		TEXT("Melee profile uses melee mode"),
		static_cast<uint8>(MeleeProfile.AttackMode),
		static_cast<uint8>(ETunaSweeperEnemyAttackMode::Melee));
	TestEqual(TEXT("Melee profile has no firing shots"), MeleeProfile.FiringShotCount, 0);
	TestTrue(
		TEXT("Melee approach stop is below approach start"),
		MeleeProfile.MeleeApproachStopRange < MeleeProfile.MeleeApproachStartRange);
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Melee turn speed is capped at 240 degrees per second"),
		MeleeProfile.TurnSpeedDegreesPerSecond,
		240.0f);
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Melee attack requires a 14 degree facing cone"),
		MeleeProfile.AttackFacingToleranceDegrees,
		14.0f);
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Melee tracking range is 9 metres"),
		MeleeProfile.TrackingRange,
		900.0f);
	TunaSweeperEnemyCombatTests::TestNearlyEqual(
		*this,
		TEXT("Melee attack range is the global 1.5 metre constant"),
		TunaSweeperEnemyCombatConstants::MeleeAttackRange,
		150.0f);

	const bool bItemsLoaded = ItemDataSubsystem->LoadItemData(true);
	TestTrue(TEXT("Item data loads for weapon fire-mode validation"), bItemsLoaded);
	if (!bItemsLoaded)
	{
		return false;
	}

	struct FExpectedWeaponMode
	{
		int32 ItemId;
		ETunaSweeperWeaponFireMode FireMode;
	};
	const FExpectedWeaponMode ExpectedWeaponModes[] =
	{
		{ 1001, ETunaSweeperWeaponFireMode::SemiAutomatic },
		{ 1002, ETunaSweeperWeaponFireMode::Automatic },
		{ 1003, ETunaSweeperWeaponFireMode::SemiAutomatic },
		{ 1006, ETunaSweeperWeaponFireMode::Automatic },
		{ 1007, ETunaSweeperWeaponFireMode::Automatic },
		{ 1008, ETunaSweeperWeaponFireMode::Automatic }
	};
	for (const FExpectedWeaponMode& Expected : ExpectedWeaponModes)
	{
		FTunaSweeperItemDefinition ItemDefinition;
		const bool bFound = ItemDataSubsystem->TryGetItemDefinition(Expected.ItemId, ItemDefinition);
		TestTrue(*FString::Printf(TEXT("Weapon item %d exists"), Expected.ItemId), bFound);
		if (bFound)
		{
			TestEqual(
				*FString::Printf(TEXT("Weapon item %d fire mode"), Expected.ItemId),
				static_cast<uint8>(ItemDefinition.FireMode),
				static_cast<uint8>(Expected.FireMode));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperEnemyRepositionFallbackTest,
	"TunaSweeper.Combat.AI.RepositionFailureResumesFiring",
	TunaSweeperEnemyCombatTests::TestFlags)

bool FTunaSweeperEnemyRepositionFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName TestWorldName = MakeUniqueObjectName(
		GetTransientPackage(),
		UWorld::StaticClass(),
		TEXT("TunaSweeperRepositionFallbackTestWorld"));
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	TestNotNull(TEXT("Transient reposition fallback world exists"), TestWorld);
	if (!TestWorld)
	{
		return false;
	}

	ATunaSweeperEnemyAIController* Controller =
		TestWorld->SpawnActor<ATunaSweeperEnemyAIController>();
	APawn* ControlledPawn = TestWorld->SpawnActor<APawn>();
	APawn* TargetPawn = TestWorld->SpawnActor<APawn>();
	TestNotNull(TEXT("Enemy AI controller exists"), Controller);
	TestNotNull(TEXT("Controlled pawn exists"), ControlledPawn);
	TestNotNull(TEXT("Target pawn exists"), TargetPawn);
	if (!Controller || !ControlledPawn || !TargetPawn)
	{
		TestWorld->DestroyWorld(false);
		TestWorld->RemoveFromRoot();
		return false;
	}

	ControlledPawn->SetActorLocation(FVector::ZeroVector);
	TargetPawn->SetActorLocation(FVector(600.0f, 0.0f, 0.0f));
	Controller->Possess(ControlledPawn);
	Controller->CurrentTargetActor = TargetPawn;
	Controller->bHasDirectTargetSight = true;
	Controller->bIsCombatEngaged = true;
	Controller->AwarenessState = ETunaSweeperEnemyAwarenessState::Combat;
	Controller->RangedCombatState = ETunaSweeperRangedCombatState::Observe;
	Controller->FiringsAtCurrentPosition = 2;
	Controller->PositionFiringBudget = 2;
	Controller->NextAllowedFireTimeSeconds = 0.0;
	Controller->CombatProfile.PositionFiringBudgetMin = 1;
	Controller->CombatProfile.PositionFiringBudgetMax = 1;

	TestTrue(
		TEXT("Shootable solo enemy resumes combat when reposition is unavailable"),
		Controller->TryResumeFiringAfterRepositionFailure(TEXT("Automation: reposition unavailable")));
	TestEqual(
		TEXT("Reposition failure resumes Aim instead of Observe"),
		static_cast<uint8>(Controller->RangedCombatState),
		static_cast<uint8>(ETunaSweeperRangedCombatState::Aim));
	TestEqual(TEXT("Fallback clears the exhausted local firing count"), Controller->FiringsAtCurrentPosition, 0);
	TestEqual(TEXT("Fallback grants a fresh local firing budget"), Controller->PositionFiringBudget, 1);

	Controller->UnPossess();
	TestWorld->DestroyWorld(false);
	TestWorld->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperEnemySquadRoleLeaseTest,
	"TunaSweeper.Combat.Squad.RoleLeases",
	TunaSweeperEnemyCombatTests::TestFlags)

bool FTunaSweeperEnemySquadRoleLeaseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName TestWorldName = MakeUniqueObjectName(
		GetTransientPackage(),
		UWorld::StaticClass(),
		TEXT("TunaSweeperSquadRoleLeaseTestWorld"));
	UWorld* TestWorld = UWorld::CreateWorld(
		EWorldType::Game,
		false,
		TestWorldName,
		GetTransientPackage());
	TestNotNull(TEXT("Transient squad test world exists"), TestWorld);
	if (!TestWorld)
	{
		return false;
	}

	UTunaSweeperEnemySquadSubsystem* SquadSubsystem =
		TestWorld->GetSubsystem<UTunaSweeperEnemySquadSubsystem>();
	APawn* SlotZeroPawn = TestWorld->SpawnActor<APawn>();
	APawn* SlotOnePawn = TestWorld->SpawnActor<APawn>();
	APawn* DuplicateSlotPawn = TestWorld->SpawnActor<APawn>();
	TestNotNull(TEXT("Squad subsystem exists"), SquadSubsystem);
	TestNotNull(TEXT("Slot zero pawn exists"), SlotZeroPawn);
	TestNotNull(TEXT("Slot one pawn exists"), SlotOnePawn);
	TestNotNull(TEXT("Duplicate-slot pawn exists"), DuplicateSlotPawn);
	if (!SquadSubsystem || !SlotZeroPawn || !SlotOnePawn || !DuplicateSlotPawn)
	{
		TestWorld->DestroyWorld(false);
		TestWorld->RemoveFromRoot();
		return false;
	}

	FTunaSweeperEnemySquadKey SquadKey;
	SquadKey.FactionId = TunaSweeperFactionIds::Enemy;
	SquadKey.SquadId = FName(TEXT("squad.automation"));
	TestTrue(TEXT("Authored squad key is valid"), SquadKey.IsValid());

	FTunaSweeperEnemySquadState SlotZeroInitialState;
	TestTrue(
		TEXT("First member registers in slot zero"),
		SquadSubsystem->RegisterMember(SlotZeroPawn, SquadKey, 0, SlotZeroInitialState));
	TestEqual(
		TEXT("A single member remains solo"),
		static_cast<uint8>(SlotZeroInitialState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Solo));

	FTunaSweeperEnemySquadState SlotOnePairedState;
	TestTrue(
		TEXT("Second member registers in slot one"),
		SquadSubsystem->RegisterMember(SlotOnePawn, SquadKey, 1, SlotOnePairedState));
	FTunaSweeperEnemySquadState SlotZeroPairedState;
	TestTrue(
		TEXT("Slot zero paired state is available"),
		SquadSubsystem->GetMemberState(SlotZeroPawn, SlotZeroPairedState));
	TestTrue(TEXT("Pair receives a positive cycle"), SlotZeroPairedState.CycleId > 0);
	TestEqual(TEXT("Both members share one cycle"), SlotOnePairedState.CycleId, SlotZeroPairedState.CycleId);
	TestTrue(TEXT("Pair roles are never duplicated"), SlotZeroPairedState.Role != SlotOnePairedState.Role);
	TestEqual(
		TEXT("Slot zero initially suppresses"),
		static_cast<uint8>(SlotZeroPairedState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Suppress));
	TestEqual(
		TEXT("Slot one initially repositions"),
		static_cast<uint8>(SlotOnePairedState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Reposition));
	TestTrue(
		TEXT("Pair lease is active and bounded"),
		SlotZeroPairedState.LeaseRemainingSeconds > 0.0f &&
		SlotZeroPairedState.LeaseRemainingSeconds <= UTunaSweeperEnemySquadSubsystem::RoleLeaseSeconds);

	FTunaSweeperEnemySquadState DuplicateSlotState;
	TestFalse(
		TEXT("A third member cannot acquire an occupied slot"),
		SquadSubsystem->RegisterMember(DuplicateSlotPawn, SquadKey, 1, DuplicateSlotState));
	FTunaSweeperEnemySquadState IdempotentState;
	TestTrue(
		TEXT("Same member and slot registration is idempotent"),
		SquadSubsystem->RegisterMember(SlotZeroPawn, SquadKey, 0, IdempotentState));
	TestEqual(
		TEXT("Idempotent registration does not mint a second lease"),
		IdempotentState.CycleId,
		SlotZeroPairedState.CycleId);

	const int32 FirstCycleId = SlotZeroPairedState.CycleId;
	TestFalse(
		TEXT("Suppressor cannot complete the mover lease"),
		SquadSubsystem->ReportRoleCompleted(SlotZeroPawn, FirstCycleId));
	TestFalse(
		TEXT("Stale completion cannot rotate the lease"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, FirstCycleId - 1));
	TestFalse(
		TEXT("Mover cannot rotate a normal cycle before suppression finishes"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, FirstCycleId));
	TestTrue(
		TEXT("Suppressor reports a complete burst"),
		SquadSubsystem->ReportSuppressionShot(SlotZeroPawn, FirstCycleId, true, true));
	TestTrue(
		TEXT("Current mover completion rotates the lease"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, FirstCycleId));

	FTunaSweeperEnemySquadState SlotZeroRotatedState;
	FTunaSweeperEnemySquadState SlotOneRotatedState;
	TestTrue(TEXT("Slot zero rotated state exists"), SquadSubsystem->GetMemberState(SlotZeroPawn, SlotZeroRotatedState));
	TestTrue(TEXT("Slot one rotated state exists"), SquadSubsystem->GetMemberState(SlotOnePawn, SlotOneRotatedState));
	TestTrue(TEXT("Completion advances the cycle"), SlotZeroRotatedState.CycleId != FirstCycleId);
	TestTrue(TEXT("Rotated roles remain distinct"), SlotZeroRotatedState.Role != SlotOneRotatedState.Role);
	TestEqual(
		TEXT("Slot zero rotates to reposition"),
		static_cast<uint8>(SlotZeroRotatedState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Reposition));
	TestEqual(
		TEXT("Slot one rotates to suppress"),
		static_cast<uint8>(SlotOneRotatedState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Suppress));
	TestFalse(
		TEXT("Completed cycle cannot be consumed twice"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, FirstCycleId));

	const int32 RecoveryCycleId = SlotZeroRotatedState.CycleId;
	TestTrue(
		TEXT("Blocked suppressor can open line-of-fire recovery"),
		SquadSubsystem->RequestLineOfFireRecovery(SlotOnePawn, RecoveryCycleId));
	FTunaSweeperEnemySquadState RecoveryMoverState;
	TestTrue(
		TEXT("Recovery mover state is available"),
		SquadSubsystem->GetMemberState(SlotZeroPawn, RecoveryMoverState));
	TestTrue(TEXT("Recovery request is visible to the mover"), RecoveryMoverState.bLineOfFireRecoveryRequested);
	TestTrue(
		TEXT("Recovery mover gate opens without a suppression shot"),
		RecoveryMoverState.MoverStartRemainingSeconds <= KINDA_SMALL_NUMBER);
	TestTrue(
		TEXT("Recovery mover can rotate roles without a final suppression shot"),
		SquadSubsystem->ReportRoleCompleted(SlotZeroPawn, RecoveryCycleId));
	FTunaSweeperEnemySquadState PostRecoveryState;
	TestTrue(
		TEXT("Post-recovery state is available"),
		SquadSubsystem->GetMemberState(SlotZeroPawn, PostRecoveryState));
	TestTrue(TEXT("Recovery completion advances the cycle"), PostRecoveryState.CycleId != RecoveryCycleId);
	TestEqual(
		TEXT("Recovery completion returns slot zero to suppressor"),
		static_cast<uint8>(PostRecoveryState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Suppress));

	const int32 RecoveryThenFireCycleId = PostRecoveryState.CycleId;
	TestTrue(
		TEXT("Suppressor can request recovery before sight returns"),
		SquadSubsystem->RequestLineOfFireRecovery(SlotZeroPawn, RecoveryThenFireCycleId));
	TestTrue(
		TEXT("A recovered suppressor may begin a burst"),
		SquadSubsystem->ReportSuppressionShot(SlotZeroPawn, RecoveryThenFireCycleId, true, false));
	TestFalse(
		TEXT("Recovery cannot bypass the final shot after a burst begins"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, RecoveryThenFireCycleId));
	TestTrue(
		TEXT("Recovered suppressor can finish its burst"),
		SquadSubsystem->ReportSuppressionShot(SlotZeroPawn, RecoveryThenFireCycleId, false, true));
	FTunaSweeperEnemySquadState SilenceGateState;
	TestTrue(
		TEXT("Silence-gate state is available"),
		SquadSubsystem->GetMemberState(SlotOnePawn, SilenceGateState));
	TestTrue(
		TEXT("Final shot creates the 0.9-1.2 second squad silence gate"),
		SilenceGateState.NextFireRemainingSeconds >= 0.9f &&
		SilenceGateState.NextFireRemainingSeconds <= 1.2f + KINDA_SMALL_NUMBER);
	TestTrue(
		TEXT("Mover rotates after the recovered burst finishes"),
		SquadSubsystem->ReportRoleCompleted(SlotOnePawn, RecoveryThenFireCycleId));

	TestTrue(TEXT("Removing one member releases the pair"), SquadSubsystem->UnregisterMember(SlotZeroPawn));
	FTunaSweeperEnemySquadState RemainingMemberState;
	TestTrue(
		TEXT("Remaining member state is available"),
		SquadSubsystem->GetMemberState(SlotOnePawn, RemainingMemberState));
	TestEqual(
		TEXT("Remaining member falls back to solo"),
		static_cast<uint8>(RemainingMemberState.Role),
		static_cast<uint8>(ETunaSweeperEnemySquadRole::Solo));
	TestTrue(TEXT("Solo fallback releases the lease"), FMath::IsNearlyZero(RemainingMemberState.LeaseRemainingSeconds));

	TestWorld->DestroyWorld(false);
	TestWorld->RemoveFromRoot();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
