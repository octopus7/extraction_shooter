#include "AI/TunaSweeperPatternEnemyCharacter.h"

#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperPatternEnemyCharacter::ATunaSweeperPatternEnemyCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(55.0f, 88.0f);
	ChargeChassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChargeChassis"));
	ChargeChassis->SetupAttachment(GetRootComponent());
	ChargeChassis->SetRelativeLocation(FVector(0, 0, -88));
	ChargeChassis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChargeChassis->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChassisAsset(TEXT("/Game/Characters/CombatPatterns/Meshes/SM_CP_ChargeChassis.SM_CP_ChargeChassis"));
	ChargeChassis->SetStaticMesh(ChassisAsset.Object);
	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);
	ForwardMarkerMesh->SetVisibility(false);
	ForwardMarkerMesh->SetHiddenInGame(true);
	MaxHealth = 450.0f;
	EnemyId = TEXT("pattern_enemy");
	MovementSpeedRandomOffset = FVector2D::ZeroVector;
	FTunaSweeperEnemyCombatProfile Profile;
	Profile.AttackMode = ETunaSweeperEnemyAttackMode::Melee;
	Profile.Role = ETunaSweeperEnemyCombatRole::Melee;
	Profile.MovementSpeed = 210.0f;
	Profile.TrackingRange = 2600.0f;
	Profile.MeleeAttackDamage = 12.0f;
	Profile.AttackCooldownSeconds = 1.5f;
	ConfigureCombatProfile(Profile, TunaSweeperFactionIds::Enemy, NAME_None, INDEX_NONE);
	GetCombatPatternComponent()->bAutomaticPatterns = true;
}

void ATunaSweeperPatternEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);
	ForwardMarkerMesh->SetVisibility(false);
	ForwardMarkerMesh->SetHiddenInGame(true);
	ChargeChassis->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
}
