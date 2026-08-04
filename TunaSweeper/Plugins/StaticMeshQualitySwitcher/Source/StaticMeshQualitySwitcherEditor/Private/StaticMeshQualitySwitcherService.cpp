#include "StaticMeshQualitySwitcherService.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "ScopedTransaction.h"
#include "StaticMeshQualityProfile.h"

#define LOCTEXT_NAMESPACE "StaticMeshQualitySwitcherService"

namespace
{
	struct FResolvedMeshPair
	{
		UStaticMesh* OriginalMesh = nullptr;
		UStaticMesh* LowMesh = nullptr;
	};

	bool ResolvePairs(
		const UStaticMeshQualityProfile& Profile,
		TArray<FResolvedMeshPair>& OutPairs,
		TArray<FText>& OutErrors)
	{
		OutPairs.Reset();
		OutPairs.Reserve(Profile.MeshPairs.Num());
		TMap<UStaticMesh*, int32> ClaimedResolvedMeshes;

		for (int32 PairIndex = 0; PairIndex < Profile.MeshPairs.Num(); ++PairIndex)
		{
			const FStaticMeshQualityPair& Pair = Profile.MeshPairs[PairIndex];
			UStaticMesh* OriginalMesh = Pair.OriginalMesh.LoadSynchronous();
			UStaticMesh* LowMesh = Pair.LowMesh.LoadSynchronous();

			if (!OriginalMesh || !LowMesh)
			{
				OutErrors.Add(FText::Format(
					LOCTEXT("LoadPairFailed", "Pair {0} could not load both mesh assets. No components were changed."),
					FText::AsNumber(PairIndex + 1)));
				continue;
			}

			auto ClaimResolvedMesh = [&OutErrors, &ClaimedResolvedMeshes, PairIndex](
				UStaticMesh* Mesh,
				const FText& Role)
			{
				if (const int32* ExistingPairIndex = ClaimedResolvedMeshes.Find(Mesh))
				{
					OutErrors.Add(FText::Format(
						LOCTEXT(
							"ResolvedDuplicateMesh",
							"Pair {0} {1} resolves to '{2}', which is already used by pair {3}. Redirected asset paths cannot bypass one-to-one validation."),
						FText::AsNumber(PairIndex + 1),
						Role,
						FText::FromString(Mesh->GetPathName()),
						FText::AsNumber(*ExistingPairIndex + 1)));
					return;
				}
				ClaimedResolvedMeshes.Add(Mesh, PairIndex);
			};

			ClaimResolvedMesh(OriginalMesh, LOCTEXT("ResolvedOriginalRole", "Original Mesh"));
			ClaimResolvedMesh(LowMesh, LOCTEXT("ResolvedLowRole", "Low Mesh"));

			OutPairs.Add(FResolvedMeshPair{OriginalMesh, LowMesh});
		}

		return OutErrors.IsEmpty() && OutPairs.Num() == Profile.MeshPairs.Num();
	}
}

FStaticMeshQualityApplyResult FStaticMeshQualitySwitcherService::Apply(
	const UStaticMeshQualityProfile* Profile,
	const EStaticMeshQualityTarget Target,
	const EStaticMeshQualityScope Scope)
{
	FStaticMeshQualityApplyResult Result;

	if (!Profile)
	{
		Result.Errors.Add(LOCTEXT("MissingProfile", "Select a Static Mesh Quality Profile first."));
		return Result;
	}

	if (!Profile->ValidateProfile(Result.Errors))
	{
		return Result;
	}

	TArray<FResolvedMeshPair> ResolvedPairs;
	if (!ResolvePairs(*Profile, ResolvedPairs, Result.Errors))
	{
		return Result;
	}

	TMap<UStaticMesh*, UStaticMesh*> TargetByCurrentMesh;
	TargetByCurrentMesh.Reserve(ResolvedPairs.Num() * 2);
	for (const FResolvedMeshPair& Pair : ResolvedPairs)
	{
		UStaticMesh* TargetMesh = Target == EStaticMeshQualityTarget::Original
			? Pair.OriginalMesh
			: Pair.LowMesh;
		TargetByCurrentMesh.Add(Pair.OriginalMesh, TargetMesh);
		TargetByCurrentMesh.Add(Pair.LowMesh, TargetMesh);
	}

	TArray<AActor*> Actors;
	GatherActors(Scope, Actors);
	Result.InspectedActorCount = Actors.Num();

	if (Scope == EStaticMeshQualityScope::SelectedActors && Actors.IsEmpty())
	{
		Result.Errors.Add(LOCTEXT("NoSelectedActors", "No level actors are selected."));
		return Result;
	}

	struct FPendingChange
	{
		AActor* Actor = nullptr;
		UStaticMeshComponent* Component = nullptr;
		UStaticMesh* TargetMesh = nullptr;
	};

	TArray<FPendingChange> PendingChanges;
	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor) || Actor->IsTemplate())
		{
			continue;
		}

		TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
		Actor->GetComponents(MeshComponents);
		for (UStaticMeshComponent* Component : MeshComponents)
		{
			if (!IsValid(Component) || Component->IsTemplate())
			{
				continue;
			}

			++Result.InspectedComponentCount;
			UStaticMesh* CurrentMesh = Component->GetStaticMesh();
			if (!CurrentMesh)
			{
				continue;
			}

			UStaticMesh* const* TargetMesh = TargetByCurrentMesh.Find(CurrentMesh);
			if (!TargetMesh)
			{
				++Result.UnmappedComponentCount;
				continue;
			}

			if (*TargetMesh != CurrentMesh)
			{
				PendingChanges.Add(FPendingChange{Actor, Component, *TargetMesh});
			}
		}
	}

	const FText TransactionText = Target == EStaticMeshQualityTarget::Original
		? LOCTEXT("ApplyOriginalTransaction", "Apply Original Static Mesh Quality")
		: LOCTEXT("ApplyLowTransaction", "Apply Low Static Mesh Quality");
	FScopedTransaction Transaction(TransactionText);

	if (PendingChanges.IsEmpty())
	{
		Transaction.Cancel();
		Result.bSucceeded = true;
		return Result;
	}

	TSet<AActor*> ModifiedActors;
	for (const FPendingChange& Change : PendingChanges)
	{
		if (!ModifiedActors.Contains(Change.Actor))
		{
			Change.Actor->Modify();
			ModifiedActors.Add(Change.Actor);
		}

		Change.Component->Modify();
		Change.Component->SetStaticMesh(Change.TargetMesh);
		Change.Component->MarkPackageDirty();
		Change.Actor->MarkPackageDirty();
		++Result.ChangedComponentCount;
	}

	Result.bSucceeded = true;
	return Result;
}

void FStaticMeshQualitySwitcherService::GatherActors(
	const EStaticMeshQualityScope Scope,
	TArray<AActor*>& OutActors)
{
	OutActors.Reset();

	if (!GEditor)
	{
		return;
	}

	if (Scope == EStaticMeshQualityScope::SelectedActors)
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();
		for (FSelectionIterator Iterator(*SelectedActors); Iterator; ++Iterator)
		{
			if (AActor* Actor = Cast<AActor>(*Iterator))
			{
				OutActors.AddUnique(Actor);
			}
		}
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> Iterator(World); Iterator; ++Iterator)
	{
		OutActors.Add(*Iterator);
	}
}

#undef LOCTEXT_NAMESPACE
