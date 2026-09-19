#include "Vehicle/TunaSweeperATVAnimInstance.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"

namespace
{
	struct FATVWheelPose { float Steering = 0, Rotation = 0, Travel = 0; };
	struct FATVAnimProxy : FAnimInstanceProxy
	{
		using FAnimInstanceProxy::FAnimInstanceProxy;
		FATVWheelPose Wheels[4];
		virtual void PreUpdate(UAnimInstance* Instance, float DeltaSeconds) override
		{
			FAnimInstanceProxy::PreUpdate(Instance, DeltaSeconds);
			const auto* ATV = Cast<ATunaSweeperATVActor>(Instance->GetOwningActor());
			for (int32 Index = 0; Index < 4; ++Index)
			{
				Wheels[Index] = {};
				if (ATV && ATV->VehicleMovement->Wheels.IsValidIndex(Index))
				{
					const auto* Wheel = ATV->VehicleMovement->Wheels[Index].Get();
					Wheels[Index] = {Wheel->GetSteerAngle(), Wheel->GetRotationAngle(), Wheel->GetSuspensionOffset()};
				}
			}
		}
		virtual bool Evaluate(FPoseContext& Output) override
		{
			Output.ResetToRefPose();
			const FBoneContainer& Bones = Output.Pose.GetBoneContainer();
			const FReferenceSkeleton& Ref = Bones.GetReferenceSkeleton();
			auto Find = [&](const FString& Name) { return Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Ref.FindBoneIndex(FName(*Name)))); };
			auto Aim = [&](FCompactPoseBoneIndex Bone, const FVector& Start, const FVector& End, bool bStretch, float RestLength)
			{
				if (Bone.GetInt() == INDEX_NONE) return;
				FTransform& Pose = Output.Pose[Bone];
				const FVector Direction = End - Start;
				Pose.SetRotation(FQuat::FindBetweenNormals(Pose.GetRotation().GetAxisX(), Direction.GetSafeNormal()) * Pose.GetRotation());
				Pose.SetTranslation(Start);
				if (bStretch && RestLength > SMALL_NUMBER) Pose.SetScale3D(FVector(Direction.Size() / RestLength, 1, 1));
			};
			const TCHAR* Suffixes[] = {TEXT("FL"), TEXT("FR"), TEXT("RL"), TEXT("RR")};
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const FString Suffix = Suffixes[Index];
				const auto Wheel = Find(TEXT("wheel_") + Suffix);
				if (Wheel.GetInt() == INDEX_NONE) continue;
				const auto& Data = Wheels[Index];
				const FVector Travel(0,0,Data.Travel);
				const FQuat Steer(FVector::UpVector, FMath::DegreesToRadians(Data.Steering));
				FTransform& WheelPose = Output.Pose[Wheel];
				WheelPose.AddToTranslation(Travel);
				WheelPose.SetRotation(Steer * FRotator(Data.Rotation, 0, 0).Quaternion() * WheelPose.GetRotation());
				const auto Knuckle = Find(TEXT("knuckle_") + Suffix);
				if (Knuckle.GetInt() != INDEX_NONE)
				{
					Output.Pose[Knuckle].AddToTranslation(Travel);
					Output.Pose[Knuckle].SetRotation(Steer * Output.Pose[Knuckle].GetRotation());
				}
				for (const TCHAR* ArmName : {TEXT("lower_arm_"), TEXT("upper_arm_")})
				{
					const auto Arm = Find(FString(ArmName) + Suffix);
					if (Arm.GetInt() == INDEX_NONE) continue;
					const FTransform Rest = Output.Pose[Arm];
					const float Side = Index % 2 == 0 ? -1.0f : 1.0f;
					const FVector Hub = Bones.GetRefPoseTransform(Wheel).GetTranslation();
					const FVector End(Hub.X, Hub.Y - Side * 11.0f, Hub.Z + (FString(ArmName).StartsWith(TEXT("upper")) ? 11.0f : -7.0f));
					Aim(Arm, Rest.GetTranslation(), End + Travel, true, FVector::Distance(Rest.GetTranslation(), End));
				}
				const auto Upper = Find(TEXT("shock_upper_") + Suffix);
				const auto Lower = Find(TEXT("shock_lower_") + Suffix);
				const auto Spring = Find(TEXT("spring_") + Suffix);
				if (Upper.GetInt() != INDEX_NONE && Lower.GetInt() != INDEX_NONE)
				{
					const FVector Top = Output.Pose[Upper].GetTranslation();
					const FVector Bottom = Output.Pose[Lower].GetTranslation();
					const float Length = FVector::Distance(Top, Bottom);
					Aim(Upper, Top, Bottom + Travel, false, Length);
					Aim(Lower, Bottom + Travel, Top, false, Length);
					Aim(Spring, Top, Bottom + Travel, true, Length);
				}
			}
			const auto Handlebar = Find(TEXT("handlebar"));
			if (Handlebar.GetInt() != INDEX_NONE)
			{
				FTransform& Pose = Output.Pose[Handlebar];
				Pose.SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians((Wheels[0].Steering + Wheels[1].Steering) * 0.5f)) * Pose.GetRotation());
			}
			return true;
		}
	};
}
FAnimInstanceProxy* UTunaSweeperATVAnimInstance::CreateAnimInstanceProxy() { return new FATVAnimProxy(this); }
void UTunaSweeperATVAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
