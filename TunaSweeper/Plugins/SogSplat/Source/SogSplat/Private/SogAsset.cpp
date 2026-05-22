#include "SogAsset.h"

int32 USogAsset::GetSplatCount() const
{
	return Splats.Num();
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

void USogAsset::ClearSplats()
{
	Splats.Reset();
	ImportedSplatCount = 0;
	LocalBounds = FBoxSphereBounds(EForceInit::ForceInit);
}
