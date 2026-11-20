#include "ForestQuadbotEnemy.h"

#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "QuadrupedComponent.h"

namespace
{
	// M_QuadbotSurface reads scorch at 0 and the three removed plate IDs at 1..3.
	constexpr int32 ScorchDataIndex = 0;
	constexpr int32 FirstPanelDataIndex = 1;
	constexpr int32 MaxEjectedPanels = 3;
	const FName ShellSlot(TEXT("Shell"));
}

AForestQuadbotEnemy::AForestQuadbotEnemy()
{
	GetCapsuleComponent()->InitCapsuleSize(61.f, 64.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -64), FRotator::ZeroRotator);
	GetQuadrupedComponent()->InitializeDefaultLegs(65.f, 59.f);
}

void AForestQuadbotEnemy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAppearance();
}

void AForestQuadbotEnemy::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyAppearance();
}

void AForestQuadbotEnemy::ApplyAppearance()
{
	if (!RobotModel) return;
	USkeletalMeshComponent* RobotMesh = GetMesh();
	if (RobotMesh->GetSkeletalMeshAsset() != RobotModel)
	{
		RobotMesh->SetSkeletalMeshAsset(RobotModel);
	}
	if (ShellMaterial) RobotMesh->SetMaterialByName(ShellSlot, ShellMaterial);
}

void AForestQuadbotEnemy::SetScorchAmount(float Amount)
{
	ScorchAmount = FMath::Clamp(Amount, 0.0f, 1.0f);
	GetMesh()->SetCustomPrimitiveDataFloat(ScorchDataIndex, ScorchAmount);
}

float AForestQuadbotEnemy::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* InstigatorController, AActor* Causer)
{
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, InstigatorController, Causer);
	if (Applied > 0.f && !IsDead())
	{
		SetScorchAmount(ScorchAmount + Applied / FMath::Max(MaxHealth, 1.f));
	}
	return Applied;
}

void AForestQuadbotEnemy::OnDeathPresentationStarted()
{
	Super::OnDeathPresentationStarted();
	SetScorchAmount(1.f);
	if (!BreakawayPanels.Num() || GetNetMode() == NM_DedicatedServer) return;
	TArray<int32, TInlineAllocator<24>> Pool;
	for (int32 I = 0; I < BreakawayPanels.Num(); ++I) Pool.Add(I);
	const int32 Count = FMath::Min(Pool.Num(), FMath::RandRange(2, MaxEjectedPanels));
	int32 Ejected = 0;
	while (Ejected < Count && !Pool.IsEmpty())
	{
		const int32 Pick = FMath::RandHelper(Pool.Num());
		const int32 PanelId = Pool[Pick];
		Pool.RemoveAtSwap(Pick);
		if (EjectPanel(BreakawayPanels[PanelId]))
		{
			GetMesh()->SetCustomPrimitiveDataFloat(FirstPanelDataIndex + Ejected, float(PanelId + 1));
			++Ejected;
		}
	}
}

bool AForestQuadbotEnemy::EjectPanel(const FForestQuadbotPanel& Panel)
{
	USkeletalMeshComponent* RobotMesh = GetMesh();
	const USkeletalMesh* Model = RobotMesh->GetSkeletalMeshAsset();
	const int32 BoneIndex = RobotMesh->GetBoneIndex(Panel.Bone);
	if (!Panel.Shard || !Model || BoneIndex == INDEX_NONE) return false;
	const FTransform Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), BoneIndex);
	const FTransform Local = FTransform(Panel.ReferenceCenter).GetRelativeTransform(Rest);
	const FTransform Pose = Local * RobotMesh->GetSocketTransform(Panel.Bone);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AStaticMeshActor* Debris = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Pose, Params);
	if (!Debris) return false;
	UStaticMeshComponent* Piece = Debris->GetStaticMeshComponent();
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetCanEverAffectNavigation(false);
	Piece->SetGenerateOverlapEvents(false);
	Piece->SetStaticMesh(Panel.Shard);
	if (ShellMaterial) Piece->SetMaterialByName(ShellSlot, ShellMaterial);
	Piece->SetCustomPrimitiveDataFloat(ScorchDataIndex, ScorchAmount);
	Piece->SetCollisionObjectType(ECC_PhysicsBody);
	Piece->SetCollisionResponseToAllChannels(ECR_Ignore);
	Piece->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Piece->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Piece->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Piece->SetSimulatePhysics(true);
	FVector Outward = (Pose.GetLocation() - GetActorLocation()).GetSafeNormal2D();
	Outward = (Outward + FMath::VRand() * 0.25f + FVector::UpVector * 0.8f).GetSafeNormal();
	Piece->AddImpulse(Outward * FMath::FRandRange(245.f, 430.f), NAME_None, true);
	Piece->SetPhysicsAngularVelocityInDegrees(FMath::VRand() * FMath::FRandRange(190.f, 520.f));
	Debris->SetLifeSpan(4.0f);
	return true;
}
