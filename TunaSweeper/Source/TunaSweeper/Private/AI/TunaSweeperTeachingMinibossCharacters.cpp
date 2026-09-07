#include "AI/TunaSweeperTeachingMinibossCharacters.h"

#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperChargeTeachingMiniboss::ATunaSweeperChargeTeachingMiniboss()
{
	EnemyId = TEXT("teaching_charge_miniboss");
	MaxHealth = 260.0f;
	ExperienceValue = 100;
	FTunaSweeperEnemyCombatProfile Profile = GetCombatProfile();
	Profile.ProfileId = EnemyId;
	Profile.MovementSpeed = 100.0f;
	Profile.AlertSeconds = 1.2f;
	Profile.MeleeAttackDamage = 0.0f;
	ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);

	UTunaSweeperCombatPatternComponent* Patterns = GetCombatPatternComponent();
	Patterns->EnabledPatterns = {ETunaSweeperCombatPattern::Charge};
	Patterns->PatternSequence = Patterns->EnabledPatterns;
	Patterns->bPatternAttacksOnly = true;
	Patterns->MissileTurretClass = nullptr;
	Patterns->RollingMinionClass = nullptr;
	Patterns->ActivationRange = 1800.0f;
	Patterns->ChargeWarningSeconds = 3.0f;
	Patterns->ChargeSpeed = 850.0f;
	Patterns->ChargeDistance = 800.0f;
	Patterns->ChargeDamage = 12.0f;
	Patterns->RecoverySeconds = 2.2f;
	Patterns->CooldownSeconds = 7.0f;
}

ATunaSweeperRobotTeachingMiniboss::ATunaSweeperRobotTeachingMiniboss()
{
	EnemyId = TEXT("teaching_robot_miniboss");
	MaxHealth = 240.0f;
	ExperienceValue = 100;
	GetCapsuleComponent()->InitCapsuleSize(65.0f, 100.0f);
	ChargeChassis->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));
	FTunaSweeperEnemyCombatProfile Profile = GetCombatProfile();
	Profile.ProfileId = EnemyId;
	Profile.MovementSpeed = 0.0f;
	Profile.AlertSeconds = 1.2f;
	Profile.MeleeAttackDamage = 0.0f;
	ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);

	UTunaSweeperCombatPatternComponent* Patterns = GetCombatPatternComponent();
	Patterns->EnabledPatterns = {ETunaSweeperCombatPattern::RollingMinions};
	Patterns->PatternSequence = Patterns->EnabledPatterns;
	Patterns->bPatternAttacksOnly = true;
	Patterns->MissileTurretClass = nullptr;
	Patterns->ChargeTelegraphClass = nullptr;
	Patterns->RollingMinionClass = ATunaSweeperTeachingRollingRobotMinion::StaticClass();
	Patterns->ActivationRange = 1800.0f;
	Patterns->MinionWarningSeconds = 2.5f;
	Patterns->MinionsPerWave = 3;
	Patterns->MaxActiveMinions = 3;
	Patterns->MinionSpawnInterval = 0.85f;
	Patterns->MinionFanAngle = 65.0f;
	Patterns->bWaitForMinionsDefeated = true;
	Patterns->RecoverySeconds = 2.5f;
	Patterns->CooldownSeconds = 9.0f;

	// Loaded robot pods distinguish the carrier from the unladen charging chassis.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PodAsset(
		TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_RobotShell.SM_CP_RobotShell"));
	LeftRobotPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRobotPod"));
	RightRobotPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRobotPod"));
	for (UStaticMeshComponent* Pod : {LeftRobotPod.Get(), RightRobotPod.Get()})
	{
		Pod->SetupAttachment(GetRootComponent());
		Pod->SetStaticMesh(PodAsset.Object);
		Pod->SetRelativeScale3D(FVector(0.6f));
		Pod->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Pod->SetGenerateOverlapEvents(false);
		Pod->SetCanEverAffectNavigation(false);
	}
	LeftRobotPod->SetRelativeLocation(FVector(-18.0f, -30.0f, 55.0f));
	RightRobotPod->SetRelativeLocation(FVector(-18.0f, 30.0f, 55.0f));
}

ATunaSweeperTeachingRollingRobotMinion::ATunaSweeperTeachingRollingRobotMinion()
{
	EnemyId = TEXT("teaching_rolling_robot_minion");
	MaxHealth = 14.0f;
	ExperienceValue = 5;
	RollSpeed = 350.0f;
	RollDurationSeconds = 3.0f;
	UnfoldDurationSeconds = 1.2f;
	LaunchUpwardSpeed = 90.0f;
	FTunaSweeperEnemyCombatProfile Profile = GetCombatProfile();
	Profile.ProfileId = EnemyId;
	Profile.MovementSpeed = 180.0f;
	Profile.AlertSeconds = 0.8f;
	Profile.MeleeAttackDamage = 4.0f;
	Profile.AttackCooldownSeconds = 1.8f;
	ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);
}
