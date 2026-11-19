#include "CoastalFishHabitat.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ReferenceSkeleton.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr double ProximitySquared = 10000. * 10000.;
	constexpr double TwoPi = 2. * UE_DOUBLE_PI;
	const FName JointNames[] = { TEXT("body_01"), TEXT("body_02"), TEXT("body_03"), TEXT("body_04"), TEXT("body_05"), TEXT("tail") };

	double MediumBurst(double Time)
	{
		return 0.5 + 0.5 * FMath::Cos(TwoPi * Time / 2.6);
	}

	FVector AdultPosition(bool bLarge, double Time, const FVector& Extent)
	{
		const double Rate = TwoPi / (bLarge ? 32. : 13.);
		// Integrate the periodic burst speed, keeping the coast moving in the same direction.
		const double Angle = Rate * (bLarge ? Time : Time + 0.31 * FMath::Sin(TwoPi * Time / 2.6));
		return Extent * FVector(FMath::Cos(Angle), FMath::Sin(Angle), FMath::Sin(2. * Angle + 0.3));
	}

	double JointBend(bool bLarge, int32 Joint, double Time, double Turn)
	{
		const double Along = Joint / 5.;
		const double Wave = FMath::Sin(Time * (bLarge ? 3.1 : 9.2) - Along * (bLarge ? 3.1 : 2.7));
		const double Strength = bLarge ? 1. : 0.17 + 0.83 * MediumBurst(Time);
		const double Stroke = (bLarge ? 9. : 16.) * (0.12 + 0.88 * Along) * Wave * Strength;
		// A curved spine follows the head through a turn; the caudal joints arrive last.
		return Stroke - FMath::Clamp(Turn * (bLarge ? 0.09 : 0.14), -15., 15.) * (0.25 + Along * 0.75);
	}
}

ACoastalFishHabitat::ACoastalFishHabitat()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("HabitatOrigin")));
	MediumFish = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("MediumFish"));
	LargeFish = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("LargeFish"));
	School = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FrySchool"));
	for (UPrimitiveComponent* Part : {static_cast<UPrimitiveComponent*>(MediumFish.Get()), static_cast<UPrimitiveComponent*>(LargeFish.Get()), static_cast<UPrimitiveComponent*>(School.Get())})
	{
		Part->SetupAttachment(RootComponent);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// In the shallow water pass a hard seabed shadow reads as a second fish.
		Part->SetCastShadow(false);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->PrimaryComponentTick.bCanEverTick = false;
		Part->PrimaryComponentTick.bStartWithTickEnabled = false;
	}
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MediumAsset(TEXT("/Game/Environment/CoastalFish/SK_CoastalFishMedium"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> LargeAsset(TEXT("/Game/Environment/CoastalFish/SK_CoastalFishLarge"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> FryAsset(TEXT("/Game/Environment/CoastalFish/SM_CoastalFry"));
	MediumFish->SetSkinnedAssetAndUpdate(MediumAsset.Object);
	LargeFish->SetSkinnedAssetAndUpdate(LargeAsset.Object);
	School->SetStaticMesh(FryAsset.Object);
	School->SetNumCustomDataFloats(2);
}

void ACoastalFishHabitat::ResetSchool()
{
	const int32 Count = FMath::Clamp(FryCount,20,40);
	School->SetNumCustomDataFloats(2);
	FryTransforms.Init(FTransform::Identity, Count);
	FryShaderData.SetNumZeroed(2 * Count);
	if (School->GetInstanceCount() != Count)
	{
		School->ClearInstances();
		School->AddInstances(FryTransforms, false, false, false);
	}
}

void ACoastalFishHabitat::InitializePose()
{
	auto CacheChain = [](UPoseableMeshComponent* Mesh, TArray<FJoint>& Joints)
	{
		Joints.Reset();
		const USkinnedAsset* Asset = Mesh->GetSkinnedAsset();
		if (!Asset) return;
		const FReferenceSkeleton& Skeleton = Asset->GetRefSkeleton();
		for (FName Name : JointNames)
		{
			const int32 Index = Skeleton.FindBoneIndex(Name);
			if (Index == INDEX_NONE)
			{
				Joints.Reset();
				return;
			}
			const int32 Parent = Skeleton.GetParentIndex(Index);
			const FQuat ParentOrientation = Parent == INDEX_NONE ? FQuat::Identity : Skeleton.GetBoneAbsoluteTransform(Parent).GetRotation();
			Joints.Add({ Index, Skeleton.GetRefBonePose()[Index].GetRotation(), ParentOrientation.UnrotateVector(FVector::UpVector) });
		}
	};
	CacheChain(MediumFish, MediumJoints);
	CacheChain(LargeFish, LargeJoints);
	ResetSchool();
	UpdateAdult(false);
	UpdateAdult(true);
	UpdateFry();
}

void ACoastalFishHabitat::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	InitializePose();
}

void ACoastalFishHabitat::BeginPlay()
{
	Super::BeginPlay();
	InitializePose();
	RefreshProximity();
	GetWorldTimerManager().SetTimer(ProximityTimer, this, &ThisClass::RefreshProximity, 0.25f, true);
}

void ACoastalFishHabitat::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	Super::EndPlay(Reason);
}

void ACoastalFishHabitat::RefreshProximity()
{
	TArray<FVector> Players;
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		const APawn* Pawn = Controller ? Controller->GetPawnOrSpectator() : nullptr;
		if (Pawn) Players.Add(Pawn->GetActorLocation());
	}
	ApplyPlayerLocations(Players);
}

void ACoastalFishHabitat::ApplyPlayerLocations(const TArray<FVector>& Players)
{
	auto InRange = [&](const FVector& Position)
	{
		return Players.ContainsByPredicate([&](const FVector& Player)
		{
			return FVector::DistSquared(Player, Position) <= ProximitySquared;
		});
	};
	bMediumActive = InRange(MediumFish->GetComponentLocation());
	bLargeActive = InRange(LargeFish->GetComponentLocation());
	bFryActive = !Players.IsEmpty() && FryTransforms.ContainsByPredicate([&](const FTransform& Fry)
	{
		return InRange(School->GetComponentTransform().TransformPosition(Fry.GetLocation()));
	});
	SetActorTickEnabled(bMediumActive || bLargeActive || bFryActive);
}

void ACoastalFishHabitat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bMediumActive)
	{
		MediumTime += DeltaSeconds;
		UpdateAdult(false);
	}
	if (bLargeActive)
	{
		LargeTime += DeltaSeconds;
		UpdateAdult(true);
	}
	if (bFryActive)
	{
		FryTime += DeltaSeconds;
		UpdateFry();
	}
}

void ACoastalFishHabitat::UpdateAdult(bool bLarge)
{
	const double Time = bLarge ? LargeTime : MediumTime;
	const FVector& Center = bLarge ? LargeCenter : MediumCenter;
	const FVector& Extent = bLarge ? LargeExtent : MediumExtent;
	auto* Mesh = bLarge ? LargeFish.Get() : MediumFish.Get();
	const auto Heading = [&](double T)
	{
		return (AdultPosition(bLarge,T + 0.02,Extent) - AdultPosition(bLarge,T - 0.02,Extent)).Rotation();
	};
	const FRotator Direction = Heading(Time);
	Mesh->SetRelativeLocationAndRotation(Center + AdultPosition(bLarge,Time,Extent), Direction);
	const TArray<FJoint>& Joints = bLarge ? LargeJoints : MediumJoints;
	for (int32 Index = 0; Index < Joints.Num(); ++Index)
	{
		const FJoint& Joint = Joints[Index];
		if (!Mesh->BoneSpaceTransforms.IsValidIndex(Joint.Index)) return;
		const double HeadTime = Time - Index * (bLarge ? 0.15 : 0.06);
		const double Turn = FMath::FindDeltaAngleDegrees(Heading(HeadTime - 0.12).Yaw, Heading(HeadTime + 0.12).Yaw) / 0.24;
		const FQuat Bend(Joint.BendAxis, FMath::DegreesToRadians(JointBend(bLarge,Index,Time,Turn)));
		Mesh->BoneSpaceTransforms[Joint.Index].SetRotation(Bend * Joint.RestRotation);
	}
	if (!Joints.IsEmpty()) Mesh->RefreshBoneTransforms();
}

FVector ACoastalFishHabitat::FryPosition(int32 Index, double Time) const
{
	// Row delays carry a change of direction from the leaders through the school.
	const int32 Row = Index / 4;
	const double Side = Index % 2 ? 1. : -1.;
	const double Delay = 0.26 * Row + 0.035 * (Index % 4);
	const double Angle = (Time - Delay) * TwoPi / 18.;
	const double Sine = FMath::Sin(Angle);
	const FVector Route(FryExtent.X * FMath::Sin(2.*Angle), FryExtent.Y * Sine, FryExtent.Z * FMath::Sin(Angle + 0.5));
	const FVector Tangent(2. * FryExtent.X * FMath::Cos(2.*Angle), FryExtent.Y * FMath::Cos(Angle), 0);
	const FVector Normal = FVector::CrossProduct(FVector::UpVector, Tangent).GetSafeNormal();
	const double Distance = Route.Size2D();
	const double Clearance = FMath::Max(0.f,FryObstacleRadius);
	const double Split = 1. - FMath::SmoothStep(0., FMath::Max(1.,Clearance * 2.8), Distance);
	const double Lane = 5.5 + 10. * ((Index % 4) / 2) + 1.7 * FMath::Sin(Index + Angle);
	const double Height = 2.5 * FMath::Sin(Index * 2.4 + Angle);
	return FryCenter + Route + Normal * Side * (Lane + Clearance * Split) + FVector(0,0,Height);
}

void ACoastalFishHabitat::UpdateFry()
{
	for (int32 Index = 0; Index < FryTransforms.Num(); ++Index)
	{
		const FVector Position = FryPosition(Index, FryTime);
		const FVector Forward = FryPosition(Index, FryTime + 0.035) - Position;
		const double Scale = 0.86 + 0.27 * (0.5 + 0.5 * FMath::Sin(Index * 8.3));
		FryTransforms[Index] = FTransform(Forward.Rotation(), Position, FVector(Scale));
		FryShaderData[Index*2] = FMath::Fmod(FryTime * (13. + 0.18 * (Index % 7)) + 2.4 * Index, TwoPi);
		FryShaderData[Index*2 + 1] = 0.72 + 0.22 * FMath::Sin(FryTime * 1.4 + Index);
	}
	if (!FryTransforms.IsEmpty())
	{
		School->SetCustomData(0, FryTransforms.Num()-1, FryShaderData, false);
		School->BatchUpdateInstancesTransforms(0, FryTransforms, false, true, true);
	}
}
