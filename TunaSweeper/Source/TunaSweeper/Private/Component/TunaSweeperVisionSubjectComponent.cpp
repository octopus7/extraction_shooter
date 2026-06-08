#include "Component/TunaSweeperVisionSubjectComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperVisionVisibilitySubsystem.h"

UTunaSweeperVisionSubjectComponent::UTunaSweeperVisionSubjectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UTunaSweeperVisionSubjectComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperVisionVisibilitySubsystem* VisionSubsystem =
			World->GetSubsystem<UTunaSweeperVisionVisibilitySubsystem>())
		{
			VisionSubsystem->RegisterVisionSubject(this);
		}
	}
}

void UTunaSweeperVisionSubjectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetVisionVisibility();

	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperVisionVisibilitySubsystem* VisionSubsystem =
			World->GetSubsystem<UTunaSweeperVisionVisibilitySubsystem>())
		{
			VisionSubsystem->UnregisterVisionSubject(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

FVector UTunaSweeperVisionSubjectComponent::GetVisionTestLocation() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() + VisionTestLocationOffset : FVector::ZeroVector;
}

void UTunaSweeperVisionSubjectComponent::SetVisionVisibilityEnabled(bool bEnabled)
{
	if (bEnableVisionVisibility == bEnabled)
	{
		return;
	}

	bEnableVisionVisibility = bEnabled;
	if (!bEnableVisionVisibility)
	{
		ResetVisionVisibility();
	}
}

void UTunaSweeperVisionSubjectComponent::ApplyVisionVisible(bool bVisible)
{
	if (!bEnableVisionVisibility)
	{
		ResetVisionVisibility();
		return;
	}

	if (bVisible)
	{
		ResetVisionVisibility();
		return;
	}

	HideSubjectPrimitives();
}

void UTunaSweeperVisionSubjectComponent::ResetVisionVisibility()
{
	for (const FTunaSweeperVisionSubjectPrimitiveRenderState& RenderState : CachedPrimitiveRenderStates)
	{
		if (UPrimitiveComponent* PrimitiveComponent = RenderState.Component.Get())
		{
			PrimitiveComponent->SetRenderInMainPass(RenderState.bRenderInMainPass);
			PrimitiveComponent->SetRenderInDepthPass(RenderState.bRenderInDepthPass);
		}
	}

	CachedPrimitiveRenderStates.Reset();
	bVisionHidden = false;
}

void UTunaSweeperVisionSubjectComponent::HideSubjectPrimitives()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		ResetVisionVisibility();
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent || !PrimitiveComponent->IsRegistered())
		{
			continue;
		}

		CachePrimitiveRenderState(PrimitiveComponent);
		PrimitiveComponent->SetRenderInMainPass(false);
		PrimitiveComponent->SetRenderInDepthPass(false);
	}

	bVisionHidden = true;
}

void UTunaSweeperVisionSubjectComponent::CachePrimitiveRenderState(UPrimitiveComponent* PrimitiveComponent)
{
	if (!PrimitiveComponent || FindCachedRenderState(PrimitiveComponent))
	{
		return;
	}

	FTunaSweeperVisionSubjectPrimitiveRenderState RenderState;
	RenderState.Component = PrimitiveComponent;
	RenderState.bRenderInMainPass = PrimitiveComponent->bRenderInMainPass;
	RenderState.bRenderInDepthPass = PrimitiveComponent->bRenderInDepthPass;
	CachedPrimitiveRenderStates.Add(RenderState);
}

FTunaSweeperVisionSubjectPrimitiveRenderState* UTunaSweeperVisionSubjectComponent::FindCachedRenderState(
	UPrimitiveComponent* PrimitiveComponent)
{
	if (!PrimitiveComponent)
	{
		return nullptr;
	}

	for (FTunaSweeperVisionSubjectPrimitiveRenderState& RenderState : CachedPrimitiveRenderStates)
	{
		if (RenderState.Component.Get() == PrimitiveComponent)
		{
			return &RenderState;
		}
	}

	return nullptr;
}
