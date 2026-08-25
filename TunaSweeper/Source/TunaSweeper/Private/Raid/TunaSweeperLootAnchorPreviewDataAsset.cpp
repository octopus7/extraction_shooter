#include "Raid/TunaSweeperLootAnchorPreviewDataAsset.h"

const FTunaSweeperLootAnchorPreviewDefinition* UTunaSweeperLootAnchorPreviewDataAsset::FindPreview(FName PreviewId) const
{
	if (PreviewId.IsNone())
	{
		return PreviewDefinitions.IsEmpty() ? nullptr : &PreviewDefinitions[0];
	}

	return PreviewDefinitions.FindByPredicate(
		[PreviewId](const FTunaSweeperLootAnchorPreviewDefinition& Definition)
		{
			return Definition.PreviewId == PreviewId;
		});
}
