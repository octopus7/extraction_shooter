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
	LinkedActors.Reset();

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
			PrimitiveComponent->SetCastShadow(RenderState.bCastShadow);
		}
	}

	CachedPrimitiveRenderStates.Reset();
	bVisionHidden = false;
}

void UTunaSweeperVisionSubjectComponent::AddLinkedActor(AActor* Actor)
{
	if (!IsValid(Actor) || Actor == GetOwner()) return;
	LinkedActors.RemoveAll([](const TWeakObjectPtr<AActor>& Linked) { return !Linked.IsValid(); });
	LinkedActors.AddUnique(Actor);
	if (bVisionHidden) HideActorPrimitives(Actor);
}

void UTunaSweeperVisionSubjectComponent::RemoveLinkedActor(AActor* Actor)
{
	if (!Actor || Actor == GetOwner()) return;
	if (LinkedActors.RemoveAll([Actor](const TWeakObjectPtr<AActor>& Linked)
		{ return !Linked.IsValid() || Linked.Get() == Actor; }) == 0) return;
	CachedPrimitiveRenderStates.RemoveAll([Actor](const FTunaSweeperVisionSubjectPrimitiveRenderState& State)
	{
		UPrimitiveComponent* Component = State.Component.Get();
		if (!Component) return true;
		if (Component->GetOwner() != Actor) return false;
		Component->SetRenderInMainPass(State.bRenderInMainPass);
		Component->SetRenderInDepthPass(State.bRenderInDepthPass);
		Component->SetCastShadow(State.bCastShadow);
		return true;
	});
}

void UTunaSweeperVisionSubjectComponent::HideSubjectPrimitives()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		ResetVisionVisibility();
		return;
	}

	HideActorPrimitives(Owner);
	LinkedActors.RemoveAll([](const TWeakObjectPtr<AActor>& Linked) { return !Linked.IsValid(); });
	for (const TWeakObjectPtr<AActor>& Linked : LinkedActors) HideActorPrimitives(Linked.Get());
	bVisionHidden = true;
}

void UTunaSweeperVisionSubjectComponent::HideActorPrimitives(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent || !PrimitiveComponent->IsRegistered())
		{
			continue;
		}

		CachePrimitiveRenderState(PrimitiveComponent);
		PrimitiveComponent->SetRenderInMainPass(false);
		PrimitiveComponent->SetRenderInDepthPass(false);
		PrimitiveComponent->SetCastShadow(false);
	}
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
	RenderState.bCastShadow = PrimitiveComponent->CastShadow;
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
