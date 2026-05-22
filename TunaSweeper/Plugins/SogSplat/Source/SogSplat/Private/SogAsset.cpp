#include "SogAsset.h"

#include "SogDecoder.h"

int32 USogAsset::GetSplatCount() const
{
	return Splats.IsEmpty() ? ImportedSplatCount : Splats.Num();
}

bool USogAsset::EnsureSplatsDecoded(FText& OutError)
{
	if (!Splats.IsEmpty())
	{
		return true;
	}

	if (!SourceArchiveBytes.IsEmpty())
	{
		return FSogDecoder::DecodeArchiveBytesToAsset(SourceFilePath, SourceArchiveBytes, DecodeOptions, this, OutError);
	}

	if (!SourceFilePath.IsEmpty())
	{
		return FSogDecoder::DecodeFileToAsset(SourceFilePath, DecodeOptions, this, OutError);
	}

	OutError = NSLOCTEXT("SogAsset", "NoStoredSource", "SOG asset does not contain stored source data.");
	return false;
}

const TArray<FSogSplatInstance>& USogAsset::GetSplats() const
{
	return Splats;
}

void USogAsset::SetSplats(TArray<FSogSplatInstance>&& InSplats)
{
	Splats = MoveTemp(InSplats);
	ImportedSplatCount = Splats.Num();

	FBox Bounds(ForceInit);
	for (const FSogSplatInstance& Splat : Splats)
	{
		const FVector Location = Splat.Transform.GetLocation();
		const FVector Scale = Splat.Transform.GetScale3D();
		const double RadiusCm = FMath::Max3(FMath::Abs(Scale.X), FMath::Abs(Scale.Y), FMath::Abs(Scale.Z)) * 50.0;
		Bounds += Location - FVector(RadiusCm);
		Bounds += Location + FVector(RadiusCm);
	}

	LocalBounds = Bounds.IsValid ? FBoxSphereBounds(Bounds) : FBoxSphereBounds(EForceInit::ForceInit);
}

void USogAsset::SetSourceArchiveBytes(TArray<uint8>&& InSourceArchiveBytes)
{
	SourceArchiveBytes = MoveTemp(InSourceArchiveBytes);
	StoredSourceSizeBytes = SourceArchiveBytes.Num();
}

const TArray<uint8>& USogAsset::GetSourceArchiveBytes() const
{
	return SourceArchiveBytes;
}

void USogAsset::ClearSplats()
{
	Splats.Reset();
	ImportedSplatCount = 0;
	LocalBounds = FBoxSphereBounds(EForceInit::ForceInit);
}

void USogAsset::PostLoad()
{
	Super::PostLoad();
	StoredSourceSizeBytes = SourceArchiveBytes.Num();
}
