#include "SogAsset.h"

#include "Math/Float16.h"
#include "Math/Float16Color.h"
#include "Misc/Compression.h"
#include "SogDecoder.h"

namespace
{
	constexpr int32 LegacyPackedSplatCacheVersion = 1;
	constexpr int32 TextureSplatCacheVersion = 3;
	constexpr int32 TransformTexelsPerSplat = 2;
	constexpr int32 TransformChannelsPerTexel = 4;
	constexpr int32 ColorChannelsPerTexel = 4;
	constexpr int32 MaxCacheTextureWidth = 2048;

	struct FLegacySogPackedSplat
	{
		FVector3f Location = FVector3f::ZeroVector;
		FQuat4f Rotation = FQuat4f::Identity;
		FVector2f Scale = FVector2f::UnitVector;
		FFloat16Color Color;
	};

	uint16 FloatToHalfBits(float Value)
	{
		FFloat16 Half;
		Half.SetClamped(Value);
		return Half.Encoded;
	}

	float HalfBitsToFloat(uint16 Value)
	{
		FFloat16 Half;
		Half.Encoded = Value;
		return Half.GetFloat();
	}

	void StoreTransformTexel(TArray<uint16>& HalfData, int32 TexelIndex, float R, float G, float B, float A)
	{
		const int32 BaseIndex = TexelIndex * TransformChannelsPerTexel;
		HalfData[BaseIndex + 0] = FloatToHalfBits(R);
		HalfData[BaseIndex + 1] = FloatToHalfBits(G);
		HalfData[BaseIndex + 2] = FloatToHalfBits(B);
		HalfData[BaseIndex + 3] = FloatToHalfBits(A);
	}

	FLinearColor LoadTransformTexel(const TArray<uint16>& HalfData, int32 TexelIndex)
	{
		const int32 BaseIndex = TexelIndex * TransformChannelsPerTexel;
		return FLinearColor(
			HalfBitsToFloat(HalfData[BaseIndex + 0]),
			HalfBitsToFloat(HalfData[BaseIndex + 1]),
			HalfBitsToFloat(HalfData[BaseIndex + 2]),
			HalfBitsToFloat(HalfData[BaseIndex + 3]));
	}

	FLinearColor LinearColorFromPackedBytes(const TArray<uint8>& Bytes, int32 TexelIndex)
	{
		const int32 BaseIndex = TexelIndex * ColorChannelsPerTexel;
		return FLinearColor(
			static_cast<float>(Bytes[BaseIndex + 0]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 1]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 2]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 3]) / 255.0f);
	}

	int32 ChooseCacheTextureWidth(int32 SplatCount)
	{
		if (SplatCount <= 0)
		{
			return 0;
		}

		const int32 SquareWidth = FMath::CeilToInt(FMath::Sqrt(static_cast<double>(SplatCount)));
		return FMath::Clamp(SquareWidth, 1, MaxCacheTextureWidth);
	}

	FVector SafeNormalizePosition(const FVector& Location, const FVector& PositionMin, const FVector& PositionExtent)
	{
		return FVector(
			PositionExtent.X > UE_SMALL_NUMBER ? (Location.X - PositionMin.X) / PositionExtent.X : 0.0,
			PositionExtent.Y > UE_SMALL_NUMBER ? (Location.Y - PositionMin.Y) / PositionExtent.Y : 0.0,
			PositionExtent.Z > UE_SMALL_NUMBER ? (Location.Z - PositionMin.Z) / PositionExtent.Z : 0.0);
	}
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
	CachedTransformTextureHalfData.Reset();
	CachedColorTextureByteData.Reset();
	CachedSplatDataVersion = 0;
	CachedSplatCount = 0;
	CachedSplatDataSizeBytes = 0;
	CachedSplatDataUncompressedSizeBytes = 0;
	CachedTransformTextureSizeBytes = 0;
	CachedColorTextureSizeBytes = 0;
	CachedTextureWidth = 0;
	CachedTextureHeight = 0;
	CachedPositionMin = FVector::ZeroVector;
	CachedPositionExtent = FVector::OneVector;

	if (Splats.IsEmpty())
	{
		return false;
	}

	FBox PositionBounds(ForceInit);
	for (const FSogSplatInstance& Splat : Splats)
	{
		PositionBounds += Splat.Transform.GetLocation();
	}

	if (!PositionBounds.IsValid)
	{
		return false;
	}

	CachedPositionMin = PositionBounds.Min;
	CachedPositionExtent = PositionBounds.Max - PositionBounds.Min;
	if (CachedPositionExtent.X <= UE_SMALL_NUMBER)
	{
		CachedPositionExtent.X = 1.0;
	}
	if (CachedPositionExtent.Y <= UE_SMALL_NUMBER)
	{
		CachedPositionExtent.Y = 1.0;
	}
	if (CachedPositionExtent.Z <= UE_SMALL_NUMBER)
	{
		CachedPositionExtent.Z = 1.0;
	}

	CachedTextureWidth = ChooseCacheTextureWidth(Splats.Num());
	CachedTextureHeight = FMath::DivideAndRoundUp(Splats.Num(), CachedTextureWidth);
	const int32 PaddedTexelCount = CachedTextureWidth * CachedTextureHeight;
	CachedTransformTextureHalfData.SetNumZeroed(PaddedTexelCount * TransformTexelsPerSplat * TransformChannelsPerTexel);
	CachedColorTextureByteData.SetNumZeroed(PaddedTexelCount * ColorChannelsPerTexel);

	for (int32 SplatIndex = 0; SplatIndex < Splats.Num(); ++SplatIndex)
	{
		const FSogSplatInstance& SourceSplat = Splats[SplatIndex];
		const FTransform& SourceTransform = SourceSplat.Transform;
		const FVector SourceLocation = SourceTransform.GetLocation();
		const FVector SourceScale = SourceTransform.GetScale3D();
		FQuat SourceRotation = SourceTransform.GetRotation().GetNormalized();
		if (SourceRotation.W < 0.0)
		{
			SourceRotation.X *= -1.0;
			SourceRotation.Y *= -1.0;
			SourceRotation.Z *= -1.0;
			SourceRotation.W *= -1.0;
		}

		const FVector NormalizedLocation = SafeNormalizePosition(SourceLocation, CachedPositionMin, CachedPositionExtent);
		const int32 SplatX = SplatIndex % CachedTextureWidth;
		const int32 SplatY = SplatIndex / CachedTextureWidth;
		const int32 SplatTexelIndex = SplatY * CachedTextureWidth + SplatX;
		const int32 TransformTexelIndex = SplatTexelIndex * TransformTexelsPerSplat;

		StoreTransformTexel(
			CachedTransformTextureHalfData,
			TransformTexelIndex,
			static_cast<float>(NormalizedLocation.X),
			static_cast<float>(NormalizedLocation.Y),
			static_cast<float>(NormalizedLocation.Z),
			static_cast<float>(SourceScale.X));
		StoreTransformTexel(
			CachedTransformTextureHalfData,
			TransformTexelIndex + 1,
			static_cast<float>(SourceRotation.X),
			static_cast<float>(SourceRotation.Y),
			static_cast<float>(SourceRotation.Z),
			static_cast<float>(SourceScale.Y));

		const FColor PackedColor = SourceSplat.Color.ToFColor(false);
		const int32 ColorBaseIndex = SplatTexelIndex * ColorChannelsPerTexel;
		CachedColorTextureByteData[ColorBaseIndex + 0] = PackedColor.R;
		CachedColorTextureByteData[ColorBaseIndex + 1] = PackedColor.G;
		CachedColorTextureByteData[ColorBaseIndex + 2] = PackedColor.B;
		CachedColorTextureByteData[ColorBaseIndex + 3] = PackedColor.A;
	}

	CachedSplatDataVersion = TextureSplatCacheVersion;
	CachedSplatCount = Splats.Num();
	CachedTransformTextureSizeBytes = CachedTransformTextureHalfData.Num() * sizeof(uint16);
	CachedColorTextureSizeBytes = CachedColorTextureByteData.Num() * sizeof(uint8);
	CachedSplatDataSizeBytes = CachedTransformTextureSizeBytes + CachedColorTextureSizeBytes;
	CachedSplatDataUncompressedSizeBytes = CachedSplatDataSizeBytes;
	return true;
}

bool USogAsset::TryLoadSplatsFromDecodedCache(FText& OutError)
{
	if (CachedSplatDataVersion == TextureSplatCacheVersion)
	{
		const int32 PaddedTexelCount = CachedTextureWidth * CachedTextureHeight;
		const int32 ExpectedTransformHalfCount = PaddedTexelCount * TransformTexelsPerSplat * TransformChannelsPerTexel;
		const int32 ExpectedColorByteCount = PaddedTexelCount * ColorChannelsPerTexel;
		if (CachedSplatCount <= 0
			|| CachedTextureWidth <= 0
			|| CachedTextureHeight <= 0
			|| CachedTextureWidth * CachedTextureHeight < CachedSplatCount
			|| CachedTransformTextureHalfData.Num() != ExpectedTransformHalfCount
			|| CachedColorTextureByteData.Num() != ExpectedColorByteCount)
		{
			OutError = NSLOCTEXT("SogAsset", "DecodedTextureCacheInvalid", "Cached SOG texture data is invalid.");
			return false;
		}

		TArray<FSogSplatInstance> DecodedSplats;
		DecodedSplats.SetNumUninitialized(CachedSplatCount);
		for (int32 SplatIndex = 0; SplatIndex < CachedSplatCount; ++SplatIndex)
		{
			const int32 SplatX = SplatIndex % CachedTextureWidth;
			const int32 SplatY = SplatIndex / CachedTextureWidth;
			const int32 SplatTexelIndex = SplatY * CachedTextureWidth + SplatX;
			const int32 TransformTexelIndex = SplatTexelIndex * TransformTexelsPerSplat;
			const FLinearColor Transform0 = LoadTransformTexel(CachedTransformTextureHalfData, TransformTexelIndex);
			const FLinearColor Transform1 = LoadTransformTexel(CachedTransformTextureHalfData, TransformTexelIndex + 1);

			const FVector Location = CachedPositionMin + FVector(Transform0.R, Transform0.G, Transform0.B) * CachedPositionExtent;
			const double QuatW = FMath::Sqrt(FMath::Max(0.0, 1.0 - Transform1.R * Transform1.R - Transform1.G * Transform1.G - Transform1.B * Transform1.B));
			FQuat Rotation(Transform1.R, Transform1.G, Transform1.B, QuatW);
			Rotation.Normalize();

			FSogSplatInstance& DecodedSplat = DecodedSplats[SplatIndex];
			DecodedSplat.Transform = FTransform(
				Rotation,
				Location,
				FVector(Transform0.A, Transform1.A, 1.0));
			DecodedSplat.Color = LinearColorFromPackedBytes(CachedColorTextureByteData, SplatTexelIndex);
		}

		SetSplats(MoveTemp(DecodedSplats));
		return true;
	}

	if (CachedSplatDataVersion != LegacyPackedSplatCacheVersion
		|| CachedSplatCount <= 0
		|| CachedSplatDataBytes.IsEmpty()
		|| CachedSplatDataUncompressedSizeBytes != CachedSplatCount * static_cast<int32>(sizeof(FLegacySogPackedSplat)))
	{
		return false;
	}

	TArray<FLegacySogPackedSplat> PackedSplats;
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
		const FLegacySogPackedSplat& PackedSplat = PackedSplats[SplatIndex];
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

#if WITH_EDITOR
	if (IsRunningCommandlet() && CachedSplatDataVersion != TextureSplatCacheVersion)
	{
		DecodeOptions.bFlipHorizontalAxis = true;

		FText CacheMigrationError;
		if (!SourceArchiveBytes.IsEmpty())
		{
			if (FSogDecoder::DecodeArchiveBytesToAsset(SourceFilePath, SourceArchiveBytes, DecodeOptions, this, CacheMigrationError))
			{
				MarkPackageDirty();
			}
		}
		else if (CachedSplatDataVersion == LegacyPackedSplatCacheVersion
			&& !CachedSplatDataBytes.IsEmpty()
			&& TryLoadSplatsFromDecodedCache(CacheMigrationError)
			&& BuildDecodedSplatCache())
		{
			MarkPackageDirty();
		}
	}
#endif

	StoredSourceSizeBytes = SourceArchiveBytes.Num();
	CachedTransformTextureSizeBytes = CachedTransformTextureHalfData.Num() * sizeof(uint16);
	CachedColorTextureSizeBytes = CachedColorTextureByteData.Num() * sizeof(uint8);
	CachedSplatDataSizeBytes = CachedSplatDataVersion == TextureSplatCacheVersion
		? CachedTransformTextureSizeBytes + CachedColorTextureSizeBytes
		: CachedSplatDataBytes.Num();
	if (CachedSplatDataVersion == TextureSplatCacheVersion)
	{
		CachedSplatDataUncompressedSizeBytes = CachedSplatDataSizeBytes;
	}
}
