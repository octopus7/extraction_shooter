#include "SogAsset.h"

#include "Engine/Texture2D.h"
#include "Math/Float16.h"
#include "Math/Float16Color.h"
#include "Misc/Compression.h"
#include "SogDecoder.h"
#include "TextureResource.h"

namespace
{
	constexpr int32 LegacyPackedSplatCacheVersion = 1;
	constexpr int32 TextureSplatCacheVersion = 5;
	constexpr int32 TransformTexelsPerSplat = 2;
	constexpr int32 TransformChannelsPerTexel = 4;
	constexpr int32 ColorChannelsPerTexel = 4;
	constexpr int32 MaxCacheTextureWidth = 2048;
	const FName TransformTextureObjectName(TEXT("SogTransformTexture"));
	const FName ColorTextureObjectName(TEXT("SogColorTexture"));

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

	FLinearColor LinearColorFromBgraBytes(const TArray<uint8>& Bytes, int32 TexelIndex)
	{
		const int32 BaseIndex = TexelIndex * ColorChannelsPerTexel;
		return FLinearColor(
			static_cast<float>(Bytes[BaseIndex + 2]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 1]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 0]) / 255.0f,
			static_cast<float>(Bytes[BaseIndex + 3]) / 255.0f);
	}

	int32 GetTransformTextureWidth(int32 SplatTextureWidth)
	{
		return SplatTextureWidth * TransformTexelsPerSplat;
	}

	int32 GetExpectedTransformHalfCount(int32 SplatTextureWidth, int32 SplatTextureHeight)
	{
		return SplatTextureWidth * SplatTextureHeight * TransformTexelsPerSplat * TransformChannelsPerTexel;
	}

	int32 GetExpectedColorByteCount(int32 SplatTextureWidth, int32 SplatTextureHeight)
	{
		return SplatTextureWidth * SplatTextureHeight * ColorChannelsPerTexel;
	}

	bool DecodeTextureCacheData(
		int32 SplatCount,
		int32 SplatTextureWidth,
		int32 SplatTextureHeight,
		const FVector& PositionMin,
		const FVector& PositionExtent,
		const TArray<uint16>& TransformHalfData,
		const TArray<uint8>& ColorByteData,
		TArray<FSogSplatInstance>& OutSplats)
	{
		if (SplatCount <= 0
			|| SplatTextureWidth <= 0
			|| SplatTextureHeight <= 0
			|| SplatTextureWidth * SplatTextureHeight < SplatCount
			|| TransformHalfData.Num() != GetExpectedTransformHalfCount(SplatTextureWidth, SplatTextureHeight)
			|| ColorByteData.Num() != GetExpectedColorByteCount(SplatTextureWidth, SplatTextureHeight))
		{
			return false;
		}

		OutSplats.SetNumUninitialized(SplatCount);
		for (int32 SplatIndex = 0; SplatIndex < SplatCount; ++SplatIndex)
		{
			const int32 SplatX = SplatIndex % SplatTextureWidth;
			const int32 SplatY = SplatIndex / SplatTextureWidth;
			const int32 SplatTexelIndex = SplatY * SplatTextureWidth + SplatX;
			const int32 TransformTexelIndex = SplatTexelIndex * TransformTexelsPerSplat;
			const FLinearColor Transform0 = LoadTransformTexel(TransformHalfData, TransformTexelIndex);
			const FLinearColor Transform1 = LoadTransformTexel(TransformHalfData, TransformTexelIndex + 1);

			const FVector Location = PositionMin + FVector(Transform0.R, Transform0.G, Transform0.B) * PositionExtent;
			const double QuatW = FMath::Sqrt(FMath::Max(0.0, 1.0 - Transform1.R * Transform1.R - Transform1.G * Transform1.G - Transform1.B * Transform1.B));
			FQuat Rotation(Transform1.R, Transform1.G, Transform1.B, QuatW);
			Rotation.Normalize();

			FSogSplatInstance& DecodedSplat = OutSplats[SplatIndex];
			DecodedSplat.Transform = FTransform(
				Rotation,
				Location,
				FVector(Transform0.A, Transform1.A, 1.0));
			DecodedSplat.Color = LinearColorFromBgraBytes(ColorByteData, SplatTexelIndex);
		}

		return true;
	}

#if WITH_EDITORONLY_DATA
	UTexture2D* CreateOrUpdateCacheTexture(
		UObject* Outer,
		UTexture2D* ExistingTexture,
		FName TextureName,
		int32 SizeX,
		int32 SizeY,
		ETextureSourceFormat SourceFormat,
		TextureCompressionSettings CompressionSettings,
		TextureGroup LODGroup,
		const uint8* SourceData)
	{
		if (!Outer || SizeX <= 0 || SizeY <= 0 || !SourceData)
		{
			return nullptr;
		}

		UTexture2D* Texture = ExistingTexture;
		if (!Texture || Texture->GetOuter() != Outer)
		{
			Texture = FindObject<UTexture2D>(Outer, *TextureName.ToString());
		}
		if (!Texture)
		{
			Texture = NewObject<UTexture2D>(Outer, TextureName, RF_Public | RF_Transactional);
		}

#if WITH_EDITOR
		Texture->Modify();
		Texture->PreEditChange(nullptr);
#endif
		Texture->Source.Init(SizeX, SizeY, 1, 1, SourceFormat, SourceData);
		Texture->CompressionSettings = CompressionSettings;
		Texture->CompressionNone = true;
		Texture->LossyCompressionAmount = TLCA_None;
		Texture->Filter = TF_Nearest;
		Texture->LODGroup = LODGroup;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->MipLoadOptions = ETextureMipLoadOptions::AllMips;
		Texture->SRGB = false;
		Texture->VirtualTextureStreaming = false;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
#if WITH_EDITOR
		Texture->PostEditChange();
#endif
		Texture->UpdateResource();
		Texture->MarkPackageDirty();
		return Texture;
	}

	bool ReadTextureSourceBytes(
		const UTexture2D* Texture,
		int32 ExpectedSizeX,
		int32 ExpectedSizeY,
		ETextureSourceFormat ExpectedFormat,
		int32 ExpectedByteCount,
		TArray<uint8>& OutBytes)
	{
		UTexture2D* MutableTexture = const_cast<UTexture2D*>(Texture);
		if (!MutableTexture
			|| !MutableTexture->Source.IsValid()
			|| MutableTexture->Source.GetSizeX() != ExpectedSizeX
			|| MutableTexture->Source.GetSizeY() != ExpectedSizeY
			|| MutableTexture->Source.GetFormat() != ExpectedFormat
			|| MutableTexture->Source.CalcMipSize(0) != ExpectedByteCount)
		{
			return false;
		}

		const uint8* MipData = MutableTexture->Source.LockMipReadOnly(0);
		if (!MipData)
		{
			return false;
		}

		OutBytes.SetNumUninitialized(ExpectedByteCount);
		FMemory::Memcpy(OutBytes.GetData(), MipData, ExpectedByteCount);
		MutableTexture->Source.UnlockMip(0);
		return true;
	}
#endif

	bool ReadTexturePlatformBytes(
		const UTexture2D* Texture,
		int32 ExpectedSizeX,
		int32 ExpectedSizeY,
		EPixelFormat ExpectedPixelFormat,
		int32 ExpectedByteCount,
		TArray<uint8>& OutBytes)
	{
		const FTexturePlatformData* PlatformData = Texture ? Texture->GetPlatformData() : nullptr;
		if (!PlatformData
			|| PlatformData->SizeX != ExpectedSizeX
			|| PlatformData->SizeY != ExpectedSizeY
			|| PlatformData->PixelFormat != ExpectedPixelFormat
			|| PlatformData->Mips.IsEmpty())
		{
			return false;
		}

		const FTexture2DMipMap& Mip = PlatformData->Mips[0];
		if (Mip.SizeX != ExpectedSizeX || Mip.SizeY != ExpectedSizeY)
		{
			return false;
		}

		FByteBulkData& BulkData = const_cast<FByteBulkData&>(Mip.BulkData);
		if (BulkData.GetBulkDataSize() < ExpectedByteCount)
		{
			return false;
		}

		const void* MipData = BulkData.LockReadOnly();
		if (!MipData)
		{
			return false;
		}

		OutBytes.SetNumUninitialized(ExpectedByteCount);
		FMemory::Memcpy(OutBytes.GetData(), MipData, ExpectedByteCount);
		BulkData.Unlock();
		return true;
	}

	bool ReadTextureBytes(
		const UTexture2D* Texture,
		int32 ExpectedSizeX,
		int32 ExpectedSizeY,
		ETextureSourceFormat ExpectedSourceFormat,
		EPixelFormat ExpectedPixelFormat,
		int32 ExpectedByteCount,
		TArray<uint8>& OutBytes)
	{
#if WITH_EDITORONLY_DATA
		if (ReadTextureSourceBytes(Texture, ExpectedSizeX, ExpectedSizeY, ExpectedSourceFormat, ExpectedByteCount, OutBytes))
		{
			return true;
		}
#endif
		return ReadTexturePlatformBytes(Texture, ExpectedSizeX, ExpectedSizeY, ExpectedPixelFormat, ExpectedByteCount, OutBytes);
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
	TransformTexture = nullptr;
	ColorTexture = nullptr;
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
	TArray<uint16> TransformHalfData;
	TArray<uint8> ColorByteData;
	TransformHalfData.SetNumZeroed(PaddedTexelCount * TransformTexelsPerSplat * TransformChannelsPerTexel);
	ColorByteData.SetNumZeroed(PaddedTexelCount * ColorChannelsPerTexel);

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
			TransformHalfData,
			TransformTexelIndex,
			static_cast<float>(NormalizedLocation.X),
			static_cast<float>(NormalizedLocation.Y),
			static_cast<float>(NormalizedLocation.Z),
			static_cast<float>(SourceScale.X));
		StoreTransformTexel(
			TransformHalfData,
			TransformTexelIndex + 1,
			static_cast<float>(SourceRotation.X),
			static_cast<float>(SourceRotation.Y),
			static_cast<float>(SourceRotation.Z),
			static_cast<float>(SourceScale.Y));

		const FColor PackedColor = SourceSplat.Color.ToFColor(false);
		const int32 ColorBaseIndex = SplatTexelIndex * ColorChannelsPerTexel;
		ColorByteData[ColorBaseIndex + 0] = PackedColor.B;
		ColorByteData[ColorBaseIndex + 1] = PackedColor.G;
		ColorByteData[ColorBaseIndex + 2] = PackedColor.R;
		ColorByteData[ColorBaseIndex + 3] = PackedColor.A;
	}

	CachedSplatDataVersion = TextureSplatCacheVersion;
	CachedSplatCount = Splats.Num();
	CachedTransformTextureSizeBytes = TransformHalfData.Num() * sizeof(uint16);
	CachedColorTextureSizeBytes = ColorByteData.Num() * sizeof(uint8);
	CachedSplatDataSizeBytes = CachedTransformTextureSizeBytes + CachedColorTextureSizeBytes;
	CachedSplatDataUncompressedSizeBytes = CachedSplatDataSizeBytes;

#if WITH_EDITORONLY_DATA
	const int32 TransformTextureWidth = GetTransformTextureWidth(CachedTextureWidth);
	TransformTexture = CreateOrUpdateCacheTexture(
		this,
		TransformTexture,
		TransformTextureObjectName,
		TransformTextureWidth,
		CachedTextureHeight,
		TSF_RGBA16F,
		TC_HDR,
		TEXTUREGROUP_16BitData,
		reinterpret_cast<const uint8*>(TransformHalfData.GetData()));
	ColorTexture = CreateOrUpdateCacheTexture(
		this,
		ColorTexture,
		ColorTextureObjectName,
		CachedTextureWidth,
		CachedTextureHeight,
		TSF_BGRA8,
		TC_VectorDisplacementmap,
		TEXTUREGROUP_8BitData,
		ColorByteData.GetData());

	if (!TransformTexture || !ColorTexture)
	{
		CachedTransformTextureHalfData = MoveTemp(TransformHalfData);
		CachedColorTextureByteData = MoveTemp(ColorByteData);
	}
#else
	CachedTransformTextureHalfData = MoveTemp(TransformHalfData);
	CachedColorTextureByteData = MoveTemp(ColorByteData);
#endif
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
			|| CachedTextureWidth * CachedTextureHeight < CachedSplatCount)
		{
			OutError = NSLOCTEXT("SogAsset", "DecodedTextureCacheInvalid", "Cached SOG texture data is invalid.");
			return false;
		}

		TArray<uint16> TextureTransformHalfData;
		TArray<uint8> TextureColorByteData;
		const TArray<uint16>* TransformHalfData = nullptr;
		const TArray<uint8>* ColorByteData = nullptr;

		TArray<uint8> TransformTextureBytes;
		TArray<uint8> ColorTextureBytes;
		const int32 TransformTextureWidth = GetTransformTextureWidth(CachedTextureWidth);
		if (TransformTexture
			&& ColorTexture
			&& ReadTextureBytes(TransformTexture, TransformTextureWidth, CachedTextureHeight, TSF_RGBA16F, PF_FloatRGBA, ExpectedTransformHalfCount * static_cast<int32>(sizeof(uint16)), TransformTextureBytes)
			&& ReadTextureBytes(ColorTexture, CachedTextureWidth, CachedTextureHeight, TSF_BGRA8, PF_B8G8R8A8, ExpectedColorByteCount, ColorTextureBytes))
		{
			TextureTransformHalfData.SetNumUninitialized(ExpectedTransformHalfCount);
			FMemory::Memcpy(TextureTransformHalfData.GetData(), TransformTextureBytes.GetData(), TransformTextureBytes.Num());
			TextureColorByteData = MoveTemp(ColorTextureBytes);
			TransformHalfData = &TextureTransformHalfData;
			ColorByteData = &TextureColorByteData;
		}
		else if (CachedTransformTextureHalfData.Num() == ExpectedTransformHalfCount
			&& CachedColorTextureByteData.Num() == ExpectedColorByteCount)
		{
			TransformHalfData = &CachedTransformTextureHalfData;
			ColorByteData = &CachedColorTextureByteData;
		}

		if (!TransformHalfData || !ColorByteData)
		{
			OutError = NSLOCTEXT("SogAsset", "DecodedTextureCacheMissing", "Cached SOG texture assets are missing or invalid.");
			return false;
		}

		TArray<FSogSplatInstance> DecodedSplats;
		if (!DecodeTextureCacheData(
			CachedSplatCount,
			CachedTextureWidth,
			CachedTextureHeight,
			CachedPositionMin,
			CachedPositionExtent,
			*TransformHalfData,
			*ColorByteData,
			DecodedSplats))
		{
			OutError = NSLOCTEXT("SogAsset", "DecodedTextureCacheDecodeFailed", "Failed to decode cached SOG texture assets.");
			return false;
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
	if (CachedSplatDataVersion == TextureSplatCacheVersion && CachedTextureWidth > 0 && CachedTextureHeight > 0)
	{
		CachedTransformTextureSizeBytes = GetExpectedTransformHalfCount(CachedTextureWidth, CachedTextureHeight) * static_cast<int32>(sizeof(uint16));
		CachedColorTextureSizeBytes = GetExpectedColorByteCount(CachedTextureWidth, CachedTextureHeight) * static_cast<int32>(sizeof(uint8));
	}
	else
	{
		CachedTransformTextureSizeBytes = CachedTransformTextureHalfData.Num() * sizeof(uint16);
		CachedColorTextureSizeBytes = CachedColorTextureByteData.Num() * sizeof(uint8);
	}
	CachedSplatDataSizeBytes = CachedSplatDataVersion == TextureSplatCacheVersion
		? CachedTransformTextureSizeBytes + CachedColorTextureSizeBytes
		: CachedSplatDataBytes.Num();
	if (CachedSplatDataVersion == TextureSplatCacheVersion)
	{
		CachedSplatDataUncompressedSizeBytes = CachedSplatDataSizeBytes;
	}
}
