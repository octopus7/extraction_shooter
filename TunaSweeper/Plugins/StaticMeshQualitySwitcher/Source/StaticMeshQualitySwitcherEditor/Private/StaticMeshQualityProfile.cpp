#include "StaticMeshQualityProfile.h"

#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "StaticMeshQualityProfile"

bool UStaticMeshQualityProfile::ValidateProfile(TArray<FText>& OutErrors) const
{
	OutErrors.Reset();

	if (MeshPairs.IsEmpty())
	{
		OutErrors.Add(LOCTEXT("EmptyProfile", "The profile does not contain any mesh pairs."));
		return false;
	}

	struct FClaim
	{
		int32 PairIndex = INDEX_NONE;
		FText Role;
	};

	TMap<FSoftObjectPath, FClaim> ClaimedMeshes;

	auto ClaimMesh = [&OutErrors, &ClaimedMeshes](
		const FSoftObjectPath& MeshPath,
		const int32 PairIndex,
		const FText& Role)
	{
		if (const FClaim* ExistingClaim = ClaimedMeshes.Find(MeshPath))
		{
			OutErrors.Add(FText::Format(
				LOCTEXT(
					"DuplicateMesh",
					"Pair {0} {1} uses '{2}', which is already used by pair {3} as {4}. Every mesh may appear in exactly one slot."),
				FText::AsNumber(PairIndex + 1),
				Role,
				FText::FromString(MeshPath.ToString()),
				FText::AsNumber(ExistingClaim->PairIndex + 1),
				ExistingClaim->Role));
			return;
		}

		ClaimedMeshes.Add(MeshPath, FClaim{PairIndex, Role});
	};

	for (int32 PairIndex = 0; PairIndex < MeshPairs.Num(); ++PairIndex)
	{
		const FStaticMeshQualityPair& Pair = MeshPairs[PairIndex];
		const FSoftObjectPath OriginalPath = Pair.OriginalMesh.ToSoftObjectPath();
		const FSoftObjectPath LowPath = Pair.LowMesh.ToSoftObjectPath();

		if (OriginalPath.IsNull())
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("MissingOriginal", "Pair {0} does not have an Original Mesh."),
				FText::AsNumber(PairIndex + 1)));
		}
		else
		{
			ClaimMesh(OriginalPath, PairIndex, LOCTEXT("OriginalRole", "Original Mesh"));
		}

		if (LowPath.IsNull())
		{
			OutErrors.Add(FText::Format(
				LOCTEXT("MissingLow", "Pair {0} does not have a Low Mesh."),
				FText::AsNumber(PairIndex + 1)));
		}
		else
		{
			ClaimMesh(LowPath, PairIndex, LOCTEXT("LowRole", "Low Mesh"));
		}
	}

	return OutErrors.IsEmpty();
}

EDataValidationResult UStaticMeshQualityProfile::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);
	TArray<FText> Errors;
	if (!ValidateProfile(Errors))
	{
		for (const FText& Error : Errors)
		{
			Context.AddError(Error);
		}
		return EDataValidationResult::Invalid;
	}

	return SuperResult == EDataValidationResult::Invalid
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
