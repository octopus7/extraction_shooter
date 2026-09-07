#include "AI/TunaSweeperPatternEnemyCharacter.h"

#include "Component/TunaSweeperCombatPatternComponent.h"
#include "Component/TunaSweeperFactionTypes.h"

ATunaSweeperPatternEnemyCharacter::ATunaSweeperPatternEnemyCharacter()
{
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
