#include "Vehicle/TunaSweeperATVRiderAnimInstance.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"

namespace
{
	struct FRiderProxy : FAnimInstanceProxy
	{
		using FAnimInstanceProxy::FAnimInstanceProxy;
		bool bValid = false;
		FVector Pelvis, Hands[2], Feet[2], Forward, Right, Up;
		virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
		{
			FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
			bValid = false;
			const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Instance->GetOwningActor());
			const auto* Mount = Character ? Character->GetVehicleMount() : nullptr;
			const auto* ATV = Mount ? Cast<ATunaSweeperATVActor>(Mount->GetOwner()) : nullptr;
			if (!ATV) return;
			// Resolve the attachment chain locally. World transforms can straddle the
			// physics update and otherwise make the wrists lag behind a moving ATV.
			const FTransform MeshToVehicle = Instance->GetSkelMeshComponent()->GetRelativeTransform()
				* Character->GetRootComponent()->GetRelativeTransform() * Mount->GetRelativeTransform()
				* ATV->VehicleMesh->GetSocketTransform(Mount->GetAttachSocketName(), RTS_Component);
			const FTransform Bar = ATV->VehicleMesh->GetSocketTransform(TEXT("handlebar"), RTS_Component);
			Forward = MeshToVehicle.InverseTransformVectorNoScale(Bar.GetUnitAxis(EAxis::X));
			Right = MeshToVehicle.InverseTransformVectorNoScale(Bar.GetUnitAxis(EAxis::Y));
			Up = MeshToVehicle.InverseTransformVectorNoScale(FVector::UpVector);
			// Move the hips slightly across the saddle with the steering column.
			const FVector Seat = ATV->VehicleMesh->GetSocketTransform(TEXT("seat"), RTS_Component).GetLocation();
			const FVector GripCenter = (ATV->VehicleMesh->GetSocketTransform(TEXT("grip_l"), RTS_Component).GetLocation() + ATV->VehicleMesh->GetSocketTransform(TEXT("grip_r"), RTS_Component).GetLocation()) * .5f;
			const FVector HipHorizontal = GripCenter - Bar.GetUnitAxis(EAxis::X) * 22.0f;
			const FVector Hip = HipHorizontal + FVector::UpVector * (FVector::DotProduct(Seat - HipHorizontal, FVector::UpVector) + 5.0f);
			Pelvis = MeshToVehicle.InverseTransformPosition(Hip);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const FName Grip = Side == 0 ? TEXT("grip_l") : TEXT("grip_r");
				const FName Foot = Side == 0 ? TEXT("foot_l") : TEXT("foot_r");
				// Grip markers are at the outer ends; wrists sit behind the rubber grips.
				const FVector Wrist = ATV->VehicleMesh->GetSocketTransform(Grip, RTS_Component).GetLocation() + Bar.TransformVectorNoScale(FVector(-5, Side == 0 ? 12 : -12, 3));
				Hands[Side] = MeshToVehicle.InverseTransformPosition(Wrist);
				Feet[Side] = MeshToVehicle.InverseTransformPosition(ATV->VehicleMesh->GetSocketTransform(Foot, RTS_Component).GetLocation() + FVector(0, Side == 0 ? -3 : 3, 7));
			}
			bValid = true;
		}

		virtual bool Evaluate(FPoseContext& Output) override
		{
			Output.ResetToRefPose();
			if (!bValid) return true;
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			TArray<FTransform> Pose;
			Pose.SetNum(Output.Pose.GetNumBones());
			TArray<int32> Parents;
			Parents.SetNum(Pose.Num());
			for (const FCompactPoseBoneIndex I : Output.Pose.ForEachBoneIndex())
			{
				Parents[I.GetInt()] = Bones.GetParentBoneIndex(I).GetInt();
				Pose[I.GetInt()] = Parents[I.GetInt()] < 0 ? Output.Pose[I] : Output.Pose[I] * Pose[Parents[I.GetInt()]];
			}
			const TArray<FTransform> Rest = Pose;
			auto Find = [&](FName Name)
			{
				const int32 MeshIndex = Bones.GetReferenceSkeleton().FindBoneIndex(Name);
				return MeshIndex < 0 ? INDEX_NONE : Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex)).GetInt();
			};
			auto Move = [&](int32 Bone, const FVector& Position, const FQuat& Rotation)
			{
				if (Bone < 0) return;
				const FVector Origin = Pose[Bone].GetLocation();
				const FQuat Delta = Rotation * Pose[Bone].GetRotation().Inverse();
				for (int32 I = Bone; I < Pose.Num(); ++I)
				{
					int32 Parent = I;
					while (Parent > Bone) Parent = Parents[Parent];
					if (Parent != Bone) continue;
					Pose[I].SetLocation(Position + Delta.RotateVector(Pose[I].GetLocation() - Origin));
					Pose[I].SetRotation((Delta * Pose[I].GetRotation()).GetNormalized());
				}
			};
			const int32 Hip = Find(TEXT("pelvis"));
			if (Hip < 0) return true;
			Move(Hip, Pelvis, Pose[Hip].GetRotation());
			const int32 Spine = Find(TEXT("spine_01"));
			if (Spine >= 0)
			{
				// Luna's mesh faces +Y. Convert that reference direction to the steering frame.
				const FQuat Heading = FQuat::FindBetweenNormals(FVector::RightVector, Forward);
				const FQuat Lean(Right, FMath::DegreesToRadians(35.0f));
				Move(Spine, Pose[Spine].GetLocation(), Lean * Heading * Pose[Spine].GetRotation());
				const int32 Head = Find(TEXT("head"));
				if (Head >= 0) Move(Head, Pose[Head].GetLocation(), FQuat(Right, FMath::DegreesToRadians(-25.f)) * Pose[Head].GetRotation());
			}
			auto Limb = [&](int32 A, int32 B, int32 C, FVector Target, const FVector& Pole, const FQuat& EndRotation)
			{
				if (A < 0 || B < 0 || C < 0) return;
				const FVector Start = Pose[A].GetLocation();
				const float Upper = FVector::Distance(Start, Pose[B].GetLocation());
				const float Lower = FVector::Distance(Pose[B].GetLocation(), Pose[C].GetLocation());
				const FVector Direction = (Target - Start).GetSafeNormal();
				const float Length = FMath::Clamp(float(FVector::Distance(Start, Target)), FMath::Abs(Upper-Lower)+.01f, Upper+Lower-.01f);
				Target = Start + Direction * Length;
				const float Along = (Upper*Upper + Length*Length - Lower*Lower) / (2*Length);
				const FVector Bend = FVector::VectorPlaneProject(Pole - Start, Direction).GetSafeNormal();
				const FVector Joint = Start + Direction*Along + Bend*FMath::Sqrt(FMath::Max(0.f, Upper*Upper-Along*Along));
				Move(A, Start, FQuat::FindBetweenNormals((Pose[B].GetLocation()-Start).GetSafeNormal(), (Joint-Start).GetSafeNormal()) * Pose[A].GetRotation());
				Move(B, Joint, FQuat::FindBetweenNormals((Pose[C].GetLocation()-Joint).GetSafeNormal(), (Target-Joint).GetSafeNormal()) * Pose[B].GetRotation());
				Move(C, Target, EndRotation);
			};
			for (int32 Side=0; Side<2; ++Side)
			{
				const FString Suffix = Side == 0 ? TEXT("_l") : TEXT("_r");
				const float Sign = Side == 0 ? -1.f : 1.f;
				const int32 Hand = Find(FName(*(TEXT("hand")+Suffix)));
				const int32 Foot = Find(FName(*(TEXT("foot")+Suffix)));
				const int32 Finger = Find(FName(*(TEXT("middle_01")+Suffix)));
				if (Hand < 0 || Foot < 0 || Finger < 0) continue;
				const FQuat HandRotation = FQuat::FindBetweenNormals((Rest[Finger].GetLocation()-Rest[Hand].GetLocation()).GetSafeNormal(), (Forward-Up*.15f).GetSafeNormal()) * Rest[Hand].GetRotation();
				Limb(Find(FName(*(TEXT("upperarm")+Suffix))), Find(FName(*(TEXT("lowerarm")+Suffix))), Hand, Hands[Side], Pelvis+Right*Sign*50+Up*15, HandRotation);
				Limb(Find(FName(*(TEXT("thigh")+Suffix))), Find(FName(*(TEXT("calf")+Suffix))), Foot, Feet[Side], Pelvis+Forward*20+Right*Sign*100-Up*5, Rest[Foot].GetRotation());
				for (const TCHAR* FingerName : {TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")})
				{
					for (int32 Joint=1; Joint<=3; ++Joint)
					{
						const int32 Bone = Find(FName(*FString::Printf(TEXT("%s_0%d%s"), FingerName, Joint, *Suffix)));
						if (Bone >= 0) Move(Bone, Pose[Bone].GetLocation(), FQuat(Right, FMath::DegreesToRadians(Joint==1 ? 10.f : 45.f)) * Pose[Bone].GetRotation());
					}
				}
			}
			for (const FCompactPoseBoneIndex I : Output.Pose.ForEachBoneIndex())
			{
				Output.Pose[I] = Parents[I.GetInt()] < 0 ? Pose[I.GetInt()] : Pose[I.GetInt()].GetRelativeTransform(Pose[Parents[I.GetInt()]]);
			}
			return true;
		}
	};
}
FAnimInstanceProxy* UTunaSweeperATVRiderAnimInstance::CreateAnimInstanceProxy() { return new FRiderProxy(this); }
void UTunaSweeperATVRiderAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
