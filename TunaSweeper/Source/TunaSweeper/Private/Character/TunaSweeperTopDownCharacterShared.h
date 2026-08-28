#pragma once

#include "Character/TunaSweeperTopDownCharacter.h"

#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Component/TunaSweeperDebuffComponent.h"
#include "Component/TunaSweeperHeadphoneListenerComponent.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Effect/TunaSweeperMeleeImpactBurstActor.h"
#include "Effect/TunaSweeperMeleeSwingTrailActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MediaSource.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperCollisionChannels.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/TunaSweeperStaminaGaugeWidget.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperWeapon.h"

namespace TunaSweeperEquippedWeaponVisual
{
	inline const FName GunCategoryTag(TEXT("item.category.weapon.gun"));
	inline const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	inline const FName TacticalAttachmentSlotTag(TEXT("attachment.slot.tactical"));
	inline const FSoftObjectPath AssaultRifleClassPath(TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle_C"));
	constexpr int32 LaserSightItemId = 2006;
	constexpr int32 BaseballBatItemId = 1005;
	inline const FSoftObjectPath BaseballBatMeshPath(TEXT("/Game/Weapons/SM_BaseballBat.SM_BaseballBat"));
	inline const FSoftObjectPath BaseballBatMaterialPath(TEXT("/Game/Weapons/M_BaseballBat_Wood.M_BaseballBat_Wood"));
}

namespace TunaSweeperStaminaGauge
{
	constexpr float DrawSize = 70.0f;
	const FVector RelativeLocation(0.0f, 0.0f, -92.0f);
}

