#include "QuadrupedRigProfile.h"

UQuadrupedRigProfile::UQuadrupedRigProfile()
{
	ResetToStandardTwoLinkRobot();
}

void UQuadrupedRigProfile::ResetToStandardTwoLinkRobot()
{
	MeshRootBone = TEXT("root");
	BodyBone = TEXT("body");
	Limbs.Reset(4);

	auto AddLimb = [this](
		EQuadrupedLegSlot Slot,
		const TCHAR* UpperBone,
		const TCHAR* LowerBone,
		const TCHAR* FootBone,
		const FVector& FallbackPoleDirection)
	{
		FQuadrupedLimbRigBinding& Limb = Limbs.AddDefaulted_GetRef();
		Limb.Slot = Slot;
		Limb.UpperBone = FName(UpperBone);
		Limb.LowerBone = FName(LowerBone);
		Limb.FootBone = FName(FootBone);
		Limb.FallbackPoleDirection = FallbackPoleDirection;
		Limb.PoleDistance = 75.0f;
		Limb.bDerivePoleFromInputPose = true;
	};

	AddLimb(EQuadrupedLegSlot::FrontLeft, TEXT("leg_f_01_l"), TEXT("leg_f_02_l"), TEXT("leg_f_03_l"), FVector::BackwardVector);
	AddLimb(EQuadrupedLegSlot::FrontRight, TEXT("leg_f_01_r"), TEXT("leg_f_02_r"), TEXT("leg_f_03_r"), FVector::BackwardVector);
	AddLimb(EQuadrupedLegSlot::BackLeft, TEXT("leg_b_01_l"), TEXT("leg_b_02_l"), TEXT("leg_b_03_l"), FVector::ForwardVector);
	AddLimb(EQuadrupedLegSlot::BackRight, TEXT("leg_b_01_r"), TEXT("leg_b_02_r"), TEXT("leg_b_03_r"), FVector::ForwardVector);
}

const FQuadrupedLimbRigBinding* UQuadrupedRigProfile::FindLimb(EQuadrupedLegSlot Slot) const
{
	return Limbs.FindByPredicate([Slot](const FQuadrupedLimbRigBinding& Limb)
	{
		return Limb.Slot == Slot;
	});
}

bool UQuadrupedRigProfile::HasCompleteFourLegBinding() const
{
	if (Limbs.Num() != 4)
	{
		return false;
	}

	for (uint8 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		const FQuadrupedLimbRigBinding* Limb = FindLimb(static_cast<EQuadrupedLegSlot>(SlotIndex));
		if (!Limb || Limb->UpperBone.IsNone() || Limb->LowerBone.IsNone() || Limb->FootBone.IsNone())
		{
			return false;
		}
	}

	return true;
}
