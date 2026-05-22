#include "SogAsset.h"

#include "Math/Float16Color.h"
#include "Misc/Compression.h"
#include "SogDecoder.h"

namespace
{
	constexpr int32 DecodedSplatCacheVersion = 1;

	struct FSogPackedSplat
	{
		FVector3f Location = FVector3f::ZeroVector;
		FQuat4f Rotation = FQuat4f::Identity;
		FVector2f Scale = FVector2f::UnitVector;
		FFloat16Color Color;
	};
}

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

	if (TryLoadSplatsFromDecodedCache(OutError))
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

bool USogAsset::BuildDecodedSplatCache()
{
	CachedSplatDataBytes.Reset();
	CachedSplatDataVersion = 0;
	CachedSplatCount = 0;
	CachedSplatDataSizeBytes = 0;
	CachedSplatDataUncompressedSizeBytes = 0;

	if (Splats.IsEmpty())
	{
		return false;
	}

	TArray<FSogPackedSplat> PackedSplats;
	PackedSplats.SetNumUninitialized(Splats.Num());
	for (int32 SplatIndex = 0; SplatIndex < Splats.Num(); ++SplatIndex)
	{
		const FSogSplatInstance& SourceSplat = Splats[SplatIndex];
		const FTransform& SourceTransform = SourceSplat.Transform;
		const FVector SourceLocation = SourceTransform.GetLocation();
		const FVector SourceScale = SourceTransform.GetScale3D();
		const FQuat SourceRotation = SourceTransform.GetRotation();

		FSogPackedSplat& PackedSplat = PackedSplats[SplatIndex];
		PackedSplat.Location = FVector3f(
			static_cast<float>(SourceLocation.X),
			static_cast<float>(SourceLocation.Y),
			static_cast<float>(SourceLocation.Z));
		PackedSplat.Rotation = FQuat4f(
			static_cast<float>(SourceRotation.X),
			static_cast<float>(SourceRotation.Y),
			static_cast<float>(SourceRotation.Z),
			static_cast<float>(SourceRotation.W));
		PackedSplat.Scale = FVector2f(
			static_cast<float>(SourceScale.X),
			static_cast<float>(SourceScale.Y));
		PackedSplat.Color = FFloat16Color(SourceSplat.Color);
	}

	const int32 UncompressedSizeBytes = PackedSplats.Num() * sizeof(FSogPackedSplat);
	int32 CompressedSizeBytes = FCompression::CompressMemoryBound(NAME_Zlib, UncompressedSizeBytes, COMPRESS_BiasSpeed);
	if (CompressedSizeBytes <= 0)
	{
		return false;
	}

	CachedSplatDataBytes.SetNumUninitialized(CompressedSizeBytes);
	if (!FCompression::CompressMemory(
		NAME_Zlib,
		CachedSplatDataBytes.GetData(),
		CompressedSizeBytes,
		PackedSplats.GetData(),
		UncompressedSizeBytes,
		COMPRESS_BiasSpeed))
	{
		CachedSplatDataBytes.Reset();
		return false;
	}

	CachedSplatDataBytes.SetNum(CompressedSizeBytes);
	CachedSplatDataVersion = DecodedSplatCacheVersion;
	CachedSplatCount = Splats.Num();
	CachedSplatDataSizeBytes = CachedSplatDataBytes.Num();
	CachedSplatDataUncompressedSizeBytes = UncompressedSizeBytes;
	return true;
}

bool USogAsset::TryLoadSplatsFromDecodedCache(FText& OutError)
{
	if (CachedSplatDataVersion != DecodedSplatCacheVersion
		|| CachedSplatCount <= 0
		|| CachedSplatDataBytes.IsEmpty()
		|| CachedSplatDataUncompressedSizeBytes != CachedSplatCount * static_cast<int32>(sizeof(FSogPackedSplat)))
	{
		return false;
	}

	TArray<FSogPackedSplat> PackedSplats;
	PackedSplats.SetNumUninitialized(CachedSplatCount);
	if (!FCompression::UncompressMemory(
		NAME_Zlib,
		PackedSplats.GetData(),
		CachedSplatDataUncompressedSizeBytes,
		CachedSplatDataBytes.GetData(),
		CachedSplatDataBytes.Num()))
	{
		OutError = NSLOCTEXT("SogAsset", "DecodedCacheDecompressFailed", "Failed to decompress cached SOG splat data.");
		return false;
	}

	TArray<FSogSplatInstance> DecodedSplats;
	DecodedSplats.SetNumUninitialized(CachedSplatCount);
	for (int32 SplatIndex = 0; SplatIndex < CachedSplatCount; ++SplatIndex)
	{
		const FSogPackedSplat& PackedSplat = PackedSplats[SplatIndex];
		FSogSplatInstance& DecodedSplat = DecodedSplats[SplatIndex];
		DecodedSplat.Transform = FTransform(
			FQuat(
				static_cast<double>(PackedSplat.Rotation.X),
				static_cast<double>(PackedSplat.Rotation.Y),
				static_cast<double>(PackedSplat.Rotation.Z),
				static_cast<double>(PackedSplat.Rotation.W)),
			FVector(PackedSplat.Location),
			FVector(PackedSplat.Scale.X, PackedSplat.Scale.Y, 1.0));
		DecodedSplat.Color = PackedSplat.Color.GetFloats();
	}

	SetSplats(MoveTemp(DecodedSplats));
	return true;
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
	CachedSplatDataSizeBytes = CachedSplatDataBytes.Num();
}
