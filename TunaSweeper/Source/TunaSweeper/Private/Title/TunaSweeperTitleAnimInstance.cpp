#include "Title/TunaSweeperTitleAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "UObject/ConstructorHelpers.h"

UTunaSweeperTitleAnimInstance::UTunaSweeperTitleAnimInstance()
{
	static ConstructorHelpers::FObjectFinder<UAnimSequence> C(TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_C"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> A(TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_A"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> B(TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_B"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> AB(TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_AtoB"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BA(TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_BtoA"));
	Entrance=C.Object; IdleA=A.Object; IdleB=B.Object; AtoB=AB.Object; BtoA=BA.Object;
}

void UTunaSweeperTitleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	RestartTitleAnimation(FMath::Rand());
}

void UTunaSweeperTitleAnimInstance::RestartTitleAnimation(int32 RandomSeed)
{
	Random.Initialize(RandomSeed);
	EnterMotion(ETunaSweeperTitleMotion::Entrance);
	HeadLookAlpha=0.f;
}

void UTunaSweeperTitleAnimInstance::EnterMotion(ETunaSweeperTitleMotion Motion)
{
	CurrentMotion=Motion; SequenceTime=0.; CurrentSequenceTime=0.f;
	switch (Motion)
	{
	case ETunaSweeperTitleMotion::Entrance: CurrentSequence=Entrance; break;
	case ETunaSweeperTitleMotion::IdleA: CurrentSequence=IdleA; break;
	case ETunaSweeperTitleMotion::IdleB: CurrentSequence=IdleB; break;
	case ETunaSweeperTitleMotion::AtoB: CurrentSequence=AtoB; break;
	case ETunaSweeperTitleMotion::BtoA: CurrentSequence=BtoA; break;
	}
	if (Motion==ETunaSweeperTitleMotion::IdleA || Motion==ETunaSweeperTitleMotion::IdleB)
	{
		const int32 Min=FMath::Clamp(MinimumIdleLoops,1,20);
		IdleLoopsRemaining=Random.RandRange(Min,FMath::Clamp(MaximumIdleLoops,Min,20));
	}
}

void UTunaSweeperTitleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds<0.f) return;
	// A suspended window resumes its presentation rather than fast-forwarding hours.
	double Remaining=FMath::Min(double(DeltaSeconds),60.);
	while (CurrentSequence && Remaining>0.)
	{
		const double Length=CurrentSequence->GetPlayLength();
		if (Length<=KINDA_SMALL_NUMBER) break;
		const double Advance=FMath::Min(Remaining,FMath::Max(0.,Length-SequenceTime));
		SequenceTime+=Advance; Remaining-=Advance;
		if (SequenceTime+1.e-7<Length) break;
		SequenceTime=0.;
		switch (CurrentMotion)
		{
		case ETunaSweeperTitleMotion::Entrance: EnterMotion(ETunaSweeperTitleMotion::IdleA); break;
		case ETunaSweeperTitleMotion::IdleA:
			if (--IdleLoopsRemaining<=0) EnterMotion(ETunaSweeperTitleMotion::AtoB);
			break;
		case ETunaSweeperTitleMotion::AtoB: EnterMotion(ETunaSweeperTitleMotion::IdleB); break;
		case ETunaSweeperTitleMotion::IdleB:
			if (--IdleLoopsRemaining<=0) EnterMotion(ETunaSweeperTitleMotion::BtoA);
			break;
		case ETunaSweeperTitleMotion::BtoA: EnterMotion(ETunaSweeperTitleMotion::IdleA); break;
		}
	}
	CurrentSequenceTime=float(SequenceTime);
	// Keep the authored rear-facing pose intact. Restore cursor look only near the front.
	HeadLookAlpha=CurrentMotion==ETunaSweeperTitleMotion::Entrance && Entrance
		? FMath::SmoothStep(Entrance->GetPlayLength()-.5f,Entrance->GetPlayLength(),CurrentSequenceTime) : 1.f;
}
