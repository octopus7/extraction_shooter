#include "ShoreCrustacean.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/RotationMatrix.h"

namespace
{
	const FName JointNames[] = {
		TEXT("leg_L_1"),TEXT("leg_R_1"),TEXT("leg_L_2"),TEXT("leg_R_2"),
		TEXT("leg_L_3"),TEXT("leg_R_3"),TEXT("leg_L_4"),TEXT("leg_R_4"),
		TEXT("claw_L"),TEXT("claw_R"),TEXT("feeler_L"),TEXT("feeler_R"),TEXT("tail_01"),TEXT("tail_02")
	};

	bool IsWaterCover(const UPrimitiveComponent* Component)
	{
		if (!Component) return true;
		const auto* Static = Cast<UStaticMeshComponent>(Component);
		const FString AssetName = Static && Static->GetStaticMesh() ? Static->GetStaticMesh()->GetName() : FString();
		const FString Names = Component->GetName() + TEXT(" ") + AssetName;
		return Component->ComponentHasTag(TEXT("WaterSurface")) || Names.Contains(TEXT("WaterSurface"))
			|| Names.Contains(TEXT("WaterPlane")) || Names.Contains(TEXT("WaterMask")) || Names.Contains(TEXT("ShoreFoam"));
	}
}

AShoreCrustacean::AShoreCrustacean()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("BedAnchor")));
	Shell = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("CrustaceanMesh"));
	Shell->SetupAttachment(RootComponent);
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shell->SetGenerateOverlapEvents(false);
	Shell->SetCanEverAffectNavigation(false);
	Shell->PrimaryComponentTick.bCanEverTick = false;
	CrabMesh = FSoftObjectPath(TEXT("/Game/Environment/LittoralLife/SK_ShoreCrab.SK_ShoreCrab"));
	CrayfishMesh = FSoftObjectPath(TEXT("/Game/Environment/LittoralLife/SK_Crayfish.SK_Crayfish"));
}

void AShoreCrustacean::InitializeAnimal()
{
	USkeletalMesh* Species = (bIsCrayfish ? CrayfishMesh : CrabMesh).LoadSynchronous();
	if (Shell->GetSkinnedAsset() != Species) Shell->SetSkinnedAssetAndUpdate(Species);
	Joints.Reset();
	for (const FName Joint : JointNames) Joints.Add(Shell->GetBoneIndex(Joint));
	MotionTime = FMath::Max(0.f,InitialPhaseSeconds);
	UpdatePose();
}

void AShoreCrustacean::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Shell->SetDefaultCustomPrimitiveDataFloat(0,FMath::Clamp(ShoreLiftFactor,0.f,1.f));
	InitializeAnimal();
}

void AShoreCrustacean::BeginPlay()
{
	Super::BeginPlay();
	Shell->SetCustomPrimitiveDataFloat(0,FMath::Clamp(ShoreLiftFactor,0.f,1.f));
	InitializeAnimal();
	RefreshProximity();
	GetWorldTimerManager().SetTimer(ProximityTimer,this,&ThisClass::RefreshProximity,0.25f,true);
}

void AShoreCrustacean::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(ProximityTimer);
	Super::EndPlay(Reason);
}

void AShoreCrustacean::RefreshProximity()
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

void AShoreCrustacean::ApplyPlayerLocations(const TArray<FVector>& Players)
{
	const FVector Location = Shell->GetComponentLocation();
	SetActorTickEnabled(Players.ContainsByPredicate([&](const FVector& Player)
	{
		return FVector::DistSquared(Player,Location) <= 10000. * 10000.;
	}));
}

void AShoreCrustacean::FindBed(const FVector2D& LocalXY, FVector& Position, FVector& Normal) const
{
	const FRotator Yaw(0,GetActorRotation().Yaw,0);
	Position = GetActorLocation() + Yaw.RotateVector(FVector(LocalXY,0));
	Normal = FVector::UpVector;
	if (BedHeightOffsets.Num() == 9)
	{
		// Noncolliding riverbed/shore overlays must follow the surveyed visual floor.
		const double Width = FMath::Max(1.,RouteExtent.X);
		const double Height = FMath::Max(1.,RouteExtent.Y);
		const double GridX = FMath::Clamp(LocalXY.X / Width + 1.,0.,2.);
		const double GridY = FMath::Clamp(LocalXY.Y / Height + 1.,0.,2.);
		const int32 X = FMath::Min(1,FMath::FloorToInt(GridX));
		const int32 Y = FMath::Min(1,FMath::FloorToInt(GridY));
		const double U = GridX-X;
		const double V = GridY-Y;
		const int32 Corner = Y*3 + X;
		const double A = BedHeightOffsets[Corner], B = BedHeightOffsets[Corner+1];
		const double C = BedHeightOffsets[Corner+3], D = BedHeightOffsets[Corner+4];
		Position.Z += FMath::Lerp(FMath::Lerp(A,B,U),FMath::Lerp(C,D,U),V);
		const double SlopeX = FMath::Lerp(B-A,D-C,V) / Width;
		const double SlopeY = FMath::Lerp(C-A,D-B,U) / Height;
		Normal = Yaw.RotateVector(FVector(-SlopeX,-SlopeY,1).GetSafeNormal());
	}
	else if (const UWorld* World = GetWorld())
	{
		TArray<FHitResult> Hits;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(ShoreCrustaceanBed),true,this);
		World->LineTraceMultiByObjectType(Hits,Position + FVector(0,0,GroundProbeHeight),
			Position - FVector(0,0,GroundProbeDepth),FCollisionObjectQueryParams(ECC_WorldStatic),Query);
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.ImpactNormal.Z > 0.4 && !IsWaterCover(Hit.GetComponent()))
			{
				Position = Hit.ImpactPoint;
				Normal = Hit.ImpactNormal;
				break;
			}
		}
	}
	Position += GroundOffset * Normal;
}

void AShoreCrustacean::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	MotionTime += DeltaSeconds;
	UpdatePose();
}

void AShoreCrustacean::UpdatePose()
{
	// A quarter of the loop consists of a short walk, a stop, then a smooth departure.
	const double Section = MotionTime / 4.;
	const double Fraction = Section - FMath::FloorToDouble(Section);
	const double Step = FMath::Clamp(Fraction / 0.8,0.,1.);
	const double Angle = (FMath::FloorToDouble(Section) + FMath::SmoothStep(0.,1.,Step)) * UE_DOUBLE_PI * 0.5;
	const double X = FMath::Max(1.,RouteExtent.X);
	const double Y = FMath::Max(1.,RouteExtent.Y);
	const FVector2D LocalXY(X * FMath::Sin(Angle),Y * FMath::Cos(Angle));
	const FVector2D Tangent(X * FMath::Cos(Angle),-Y * FMath::Sin(Angle));
	FVector Position, Normal;
	FindBed(LocalXY,Position,Normal);
	const FVector Facing = bIsCrayfish ? FVector(Tangent,0) : FVector(-Tangent.Y,Tangent.X,0);
	const FVector Heading = FRotator(0,GetActorRotation().Yaw,0).RotateVector(Facing);
	Shell->SetWorldLocationAndRotation(Position,FRotationMatrix::MakeFromZX(Normal,Heading).ToQuat());

	if (!Shell->GetSkinnedAsset()) return;
	const auto& Rest = Shell->GetSkinnedAsset()->GetRefSkeleton().GetRefBonePose();
	auto Rotate = [&](int32 Slot, const FRotator& Offset)
	{
		const int32 Index = Joints[Slot];
		if (Shell->BoneSpaceTransforms.IsValidIndex(Index))
		{
			Shell->BoneSpaceTransforms[Index].SetRotation(Offset.Quaternion() * Rest[Index].GetRotation());
		}
	};
	const double Moving = 4. * Step * (1. - Step);
	const double Stride = Angle * (bIsCrayfish ? 20. : 25.);
	for (int32 Leg = 0; Leg < 8; ++Leg)
	{
		const double Side = Leg % 2 ? 1. : -1.;
		const double Cycle = Stride + (Leg / 2 + Leg % 2) * UE_DOUBLE_PI;
		const double Sweep = FMath::Sin(Cycle) * Moving;
		const double Lift = FMath::Max(0.,FMath::Cos(Cycle)) * Moving;
		Rotate(Leg,FRotator(bIsCrayfish ? 3.*Sweep : -Side*7.*Sweep,
			(bIsCrayfish ? 13. : 6.)*Side*Sweep,-Side*(9.*Lift + (bIsCrayfish ? 0. : 5.*Sweep))));
	}
	for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
	{
		const double Side = SideIndex ? 1. : -1.;
		const double Wave = FMath::Sin(MotionTime*2.1 + Side*0.7);
		Rotate(8+SideIndex,FRotator(2.5*Wave,Side*(3.5*Wave + 2.*Moving),0));
		if (bIsCrayfish) Rotate(10+SideIndex,FRotator(3.*Wave,Side*9.*FMath::Sin(MotionTime*1.8 + Side),0));
	}
	if (bIsCrayfish)
	{
		Rotate(12,FRotator(2.*Moving,3.*FMath::Sin(Stride*0.25),0));
		Rotate(13,FRotator(3.*Moving,5.*FMath::Sin(Stride*0.25-0.5),0));
	}
	Shell->RefreshBoneTransforms();
}
