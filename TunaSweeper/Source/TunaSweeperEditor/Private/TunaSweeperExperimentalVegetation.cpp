#include "TunaSweeperExperimentalVegetation.h"

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialFactoryNew.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperExperimentalVegetation, Log, All);

namespace TunaSweeperExperimentalVegetation
{
	const FString VegetationAssetPath = TEXT("/Game/Prototype");
	const FString VegetationMaskTextureAssetName = TEXT("T_ExperimentalVegetationShapeAtlas");
	const FString VegetationPaintTextureAssetName = TEXT("T_ExperimentalVegetationImagegenPaint");
	const FString VegetationMaterialAssetName = TEXT("M_ExperimentalVegetation_VertexColor");
	const FString VegetationMeshAssetName = TEXT("SM_ExperimentalVegetationTuft");
	const FString VegetationAlternateMeshAssetName = TEXT("SM_ExperimentalVegetationTuft_AltBranches");
	const FString VegetationFloweringWhiteMeshAssetName = TEXT("SM_ExperimentalVegetationTuft_FloweringWhite");
	const FString VegetationFloweringYellowMeshAssetName = TEXT("SM_ExperimentalVegetationTuft_FloweringYellow");
	const FString VegetationFloweringPinkMeshAssetName = TEXT("SM_ExperimentalVegetationTuft_Flowering");

	constexpr int32 ShapeWidth = 128;
	constexpr int32 ShapeHeight = 256;
	constexpr int32 ShapePadding = 8;
	constexpr uint8 SolidAlphaThreshold = 24;

	struct FSilhouetteSlice
	{
		float V = 0.0f;
		float LeftU = 0.5f;
		float RightU = 0.5f;
		float CenterU = 0.5f;
	};

	struct FMaskRegion
	{
		FString Name;
		int32 Width = ShapeWidth;
		int32 Height = ShapeHeight;
		float MeshWidth = 20.0f;
		float MeshHeight = 100.0f;
		FLinearColor BaseColor = FLinearColor(0.10f, 0.28f, 0.07f, 1.0f);
		FLinearColor TipColor = FLinearColor(0.34f, 0.62f, 0.16f, 1.0f);
		TArray<uint8> Alpha;
		TArray<FColor> DebugPixels;
		TArray<FSilhouetteSlice> Slices;
		float AtlasU0 = 0.0f;
		float AtlasU1 = 1.0f;
		float AtlasVTop = 0.0f;
		float AtlasVBottom = 1.0f;
	};

	struct FPiecePlacement
	{
		FVector3f Base = FVector3f(0.0f, 0.0f, 0.0f);
		FVector3f Along = FVector3f(0.0f, 0.0f, 1.0f);
		FVector3f Side = FVector3f(0.0f, 1.0f, 0.0f);
		FVector3f Bend = FVector3f(1.0f, 0.0f, 0.0f);
		float LengthScale = 1.0f;
		float WidthScale = 1.0f;
		float BendAmount = 0.0f;
		float SideSway = 0.0f;
		float Phase = 0.0f;
		float ColorScale = 1.0f;
	};

	enum class EVegetationMeshVariant : uint8
	{
		Primary,
		AlternateBranches,
		FloweringWhite,
		FloweringYellow,
		FloweringPink
	};

	FString GetAssetObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	bool SavePixelsAsPng(const FString& FilePath, const TArray<FColor>& Pixels, int32 Width, int32 Height)
	{
		if (Pixels.Num() != Width * Height)
		{
			return false;
		}

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);
		const FImageView ImageView(Pixels.GetData(), Width, Height, EGammaSpace::sRGB);
		return FImageUtils::SaveImageByExtension(*FilePath, ImageView);
	}

	FString GetVegetationGeneratedImagePath(const FString& FileName)
	{
		FString Directory = FPaths::Combine(FPaths::ProjectDir(), TEXT("../GeneratedImages/Vegetation"));
		FPaths::NormalizeDirectoryName(Directory);
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(Directory, FileName));
	}

	float ClampUnit(float Value)
	{
		return FMath::Clamp(Value, 0.0f, 1.0f);
	}

	float SmoothStep(float Edge0, float Edge1, float Value)
	{
		const float Alpha = ClampUnit((Value - Edge0) / (Edge1 - Edge0));
		return Alpha * Alpha * (3.0f - 2.0f * Alpha);
	}

	FVector3f SafeNormal(const FVector3f& Value, const FVector3f& Fallback)
	{
		const float SizeSquared = Value.SizeSquared();
		return SizeSquared > KINDA_SMALL_NUMBER ? Value * FMath::InvSqrt(SizeSquared) : Fallback;
	}

	FVector3f ProjectToPlaneNormal(const FVector3f& Value, const FVector3f& Normal)
	{
		return Value - Normal * FVector3f::DotProduct(Value, Normal);
	}

	FPiecePlacement MakePiecePlacement(
		const FVector3f& Base,
		const FVector3f& Along,
		const FVector3f& SideHint,
		const FVector3f& BendHint)
	{
		FPiecePlacement Placement;
		Placement.Base = Base;
		Placement.Along = SafeNormal(Along, FVector3f(0.0f, 0.0f, 1.0f));

		FVector3f ProjectedSide = ProjectToPlaneNormal(SideHint, Placement.Along);
		if (ProjectedSide.SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			ProjectedSide = FVector3f::CrossProduct(FVector3f(0.0f, 0.0f, 1.0f), Placement.Along);
		}
		if (ProjectedSide.SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			ProjectedSide = FVector3f::CrossProduct(FVector3f(1.0f, 0.0f, 0.0f), Placement.Along);
		}

		Placement.Side = SafeNormal(ProjectedSide, FVector3f(0.0f, 1.0f, 0.0f));

		FVector3f ProjectedBend = ProjectToPlaneNormal(BendHint, Placement.Along);
		ProjectedBend = ProjectToPlaneNormal(ProjectedBend, Placement.Side);
		if (ProjectedBend.SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			ProjectedBend = FVector3f::CrossProduct(Placement.Side, Placement.Along);
		}

		Placement.Bend = SafeNormal(ProjectedBend, FVector3f(1.0f, 0.0f, 0.0f));
		return Placement;
	}

	void SetMaskAlpha(FMaskRegion& Region, int32 X, int32 Y, float Alpha)
	{
		if (X < 0 || X >= Region.Width || Y < 0 || Y >= Region.Height)
		{
			return;
		}

		uint8& PixelAlpha = Region.Alpha[Y * Region.Width + X];
		PixelAlpha = FMath::Max(PixelAlpha, static_cast<uint8>(FMath::RoundToInt(ClampUnit(Alpha) * 255.0f)));
	}

	void InitializeRegion(FMaskRegion& Region)
	{
		Region.Alpha.Init(0, Region.Width * Region.Height);
		Region.DebugPixels.Init(FColor::Transparent, Region.Width * Region.Height);
		Region.Slices.Reset();
	}

	void RasterizeTaperedStem(
		FMaskRegion& Region,
		float RootU,
		float TipU,
		float BaseHalfWidthU,
		float TipHalfWidthU,
		float CurveU,
		float StartV,
		float EndV)
	{
		for (int32 Y = 0; Y < Region.Height; ++Y)
		{
			const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);
			if (V < StartV || V > EndV)
			{
				continue;
			}

			const float LocalV = ClampUnit((V - StartV) / FMath::Max(EndV - StartV, KINDA_SMALL_NUMBER));
			const float CenterU = FMath::Lerp(RootU, TipU, LocalV) + CurveU * FMath::Sin(LocalV * PI);
			const float HalfWidthU = FMath::Lerp(BaseHalfWidthU, TipHalfWidthU, FMath::Pow(LocalV, 0.85f));
			const float EdgeSoftness = 1.2f / static_cast<float>(Region.Width);

			for (int32 X = 0; X < Region.Width; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(Region.Width - 1);
				const float Alpha = 1.0f - SmoothStep(HalfWidthU, HalfWidthU + EdgeSoftness, FMath::Abs(U - CenterU));
				SetMaskAlpha(Region, X, Y, Alpha);
			}
		}
	}

	void RasterizeLeaf(
		FMaskRegion& Region,
		float RootU,
		float TipU,
		float MaxHalfWidthU,
		float CurveU,
		float StartV,
		float EndV,
		float VeinWavePhase)
	{
		for (int32 Y = 0; Y < Region.Height; ++Y)
		{
			const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);
			if (V < StartV || V > EndV)
			{
				continue;
			}

			const float LocalV = ClampUnit((V - StartV) / FMath::Max(EndV - StartV, KINDA_SMALL_NUMBER));
			const float Rise = FMath::Pow(LocalV, 1.04f);
			const float CenterU =
				FMath::Lerp(RootU, TipU, Rise) +
				CurveU * FMath::Sin(LocalV * PI) +
				0.006f * FMath::Sin((LocalV * 2.0f + VeinWavePhase) * PI);
			const float WidthProfile = FMath::Pow(FMath::Clamp(FMath::Sin(LocalV * PI), 0.0f, 1.0f), 0.62f);
			const float BaseFade = SmoothStep(0.0f, 0.055f, LocalV);
			const float TipFade = 1.0f - SmoothStep(0.88f, 1.0f, LocalV);
			const float Serration = 1.0f + 0.06f * FMath::Sin((LocalV * 13.0f + VeinWavePhase) * PI);
			const float HalfWidthU = FMath::Max(0.004f, MaxHalfWidthU * WidthProfile * BaseFade * TipFade * Serration);
			const float EdgeSoftness = 1.6f / static_cast<float>(Region.Width);

			for (int32 X = 0; X < Region.Width; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(Region.Width - 1);
				const float Alpha = 1.0f - SmoothStep(HalfWidthU, HalfWidthU + EdgeSoftness, FMath::Abs(U - CenterU));
				SetMaskAlpha(Region, X, Y, Alpha);
			}
		}
	}

	bool IsSolid(const FMaskRegion& Region, int32 X, int32 Y)
	{
		if (X < 0 || X >= Region.Width || Y < 0 || Y >= Region.Height)
		{
			return false;
		}

		return Region.Alpha[Y * Region.Width + X] >= SolidAlphaThreshold;
	}

	bool FindSolidSpanOnRow(const FMaskRegion& Region, int32 Y, int32& OutLeftX, int32& OutRightX)
	{
		OutLeftX = Region.Width;
		OutRightX = -1;

		if (Y < 0 || Y >= Region.Height)
		{
			return false;
		}

		for (int32 X = 0; X < Region.Width; ++X)
		{
			if (IsSolid(Region, X, Y))
			{
				OutLeftX = FMath::Min(OutLeftX, X);
				OutRightX = FMath::Max(OutRightX, X);
			}
		}

		return OutLeftX <= OutRightX;
	}

	bool FindNearestSolidSpan(const FMaskRegion& Region, int32 PreferredY, int32& OutLeftX, int32& OutRightX)
	{
		if (FindSolidSpanOnRow(Region, PreferredY, OutLeftX, OutRightX))
		{
			return true;
		}

		constexpr int32 SearchRadius = 10;
		for (int32 Offset = 1; Offset <= SearchRadius; ++Offset)
		{
			if (FindSolidSpanOnRow(Region, PreferredY - Offset, OutLeftX, OutRightX) ||
				FindSolidSpanOnRow(Region, PreferredY + Offset, OutLeftX, OutRightX))
			{
				return true;
			}
		}

		return false;
	}

	void ExtractOutlineSlices(FMaskRegion& Region, int32 SegmentCount)
	{
		Region.Slices.Reset();
		Region.Slices.Reserve(SegmentCount + 1);

		for (int32 SliceIndex = 0; SliceIndex <= SegmentCount; ++SliceIndex)
		{
			const float V = static_cast<float>(SliceIndex) / static_cast<float>(SegmentCount);
			const int32 Y = FMath::RoundToInt((1.0f - V) * static_cast<float>(Region.Height - 1));

			int32 LeftX = 0;
			int32 RightX = 0;
			if (!FindNearestSolidSpan(Region, Y, LeftX, RightX))
			{
				continue;
			}

			const float PixelPadU = 0.5f / static_cast<float>(Region.Width - 1);
			const float LeftU = ClampUnit(static_cast<float>(LeftX) / static_cast<float>(Region.Width - 1) - PixelPadU);
			const float RightU = ClampUnit(static_cast<float>(RightX) / static_cast<float>(Region.Width - 1) + PixelPadU);

			FSilhouetteSlice Slice;
			Slice.V = V;
			Slice.LeftU = LeftU;
			Slice.RightU = FMath::Min(1.0f, FMath::Max(RightU, LeftU + PixelPadU));
			Slice.CenterU = (Slice.LeftU + Slice.RightU) * 0.5f;
			Region.Slices.Add(Slice);
		}
	}

	FLinearColor LerpColor(const FLinearColor& A, const FLinearColor& B, float Alpha)
	{
		return FLinearColor(
			FMath::Lerp(A.R, B.R, Alpha),
			FMath::Lerp(A.G, B.G, Alpha),
			FMath::Lerp(A.B, B.B, Alpha),
			FMath::Lerp(A.A, B.A, Alpha));
	}

	void BuildDebugPixels(FMaskRegion& Region)
	{
		for (int32 Y = 0; Y < Region.Height; ++Y)
		{
			const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);
			for (int32 X = 0; X < Region.Width; ++X)
			{
				const int32 PixelIndex = Y * Region.Width + X;
				const uint8 Alpha = Region.Alpha[PixelIndex];
				if (Alpha < SolidAlphaThreshold)
				{
					Region.DebugPixels[PixelIndex] = FColor::Transparent;
					continue;
				}

				const bool bEdge =
					!IsSolid(Region, X - 1, Y) ||
					!IsSolid(Region, X + 1, Y) ||
					!IsSolid(Region, X, Y - 1) ||
					!IsSolid(Region, X, Y + 1);
				const FLinearColor FillColor = LerpColor(Region.BaseColor, Region.TipColor, FMath::Pow(V, 0.72f));
				FLinearColor PixelColor = bEdge ? FLinearColor(0.028f, 0.046f, 0.018f, 1.0f) : FillColor;
				PixelColor.A = static_cast<float>(Alpha) / 255.0f;
				Region.DebugPixels[PixelIndex] = PixelColor.ToFColor(true);
			}
		}
	}

	void RasterizeRotatedEllipse(
		FMaskRegion& Region,
		float CenterU,
		float CenterV,
		float RadiusAlong,
		float RadiusSide,
		float AngleRadians,
		float EdgeSoftness,
		float AlphaScale = 1.0f)
	{
		const float CosAngle = FMath::Cos(AngleRadians);
		const float SinAngle = FMath::Sin(AngleRadians);
		for (int32 Y = 0; Y < Region.Height; ++Y)
		{
			const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);
			for (int32 X = 0; X < Region.Width; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(Region.Width - 1);
				const float DeltaU = U - CenterU;
				const float DeltaV = V - CenterV;
				const float Along = DeltaU * CosAngle + DeltaV * SinAngle;
				const float Side = -DeltaU * SinAngle + DeltaV * CosAngle;
				const float Distance = FMath::Sqrt(
					FMath::Square(Along / FMath::Max(RadiusAlong, KINDA_SMALL_NUMBER)) +
					FMath::Square(Side / FMath::Max(RadiusSide, KINDA_SMALL_NUMBER)));
				const float Alpha = AlphaScale * (1.0f - SmoothStep(1.0f, 1.0f + EdgeSoftness, Distance));
				SetMaskAlpha(Region, X, Y, Alpha);
			}
		}
	}

	void RasterizePetal(
		FMaskRegion& Region,
		float CenterU,
		float MaxHalfWidthU,
		float CurveU,
		float StartV,
		float EndV)
	{
		for (int32 Y = 0; Y < Region.Height; ++Y)
		{
			const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);
			if (V < StartV || V > EndV)
			{
				continue;
			}

			const float LocalV = ClampUnit((V - StartV) / FMath::Max(EndV - StartV, KINDA_SMALL_NUMBER));
			const float Center = CenterU + CurveU * FMath::Sin(LocalV * PI);
			const float YNorm = FMath::Abs(LocalV - 0.50f) / 0.50f;
			const float EdgeSoftness = 0.060f;

			for (int32 X = 0; X < Region.Width; ++X)
			{
				const float U = static_cast<float>(X) / static_cast<float>(Region.Width - 1);
				const float XNorm = FMath::Abs(U - Center) / FMath::Max(MaxHalfWidthU, KINDA_SMALL_NUMBER);
				const float SuperEllipse = FMath::Sqrt(FMath::Square(XNorm) + FMath::Square(YNorm));
				const float Alpha = 1.0f - SmoothStep(1.0f, 1.0f + EdgeSoftness, SuperEllipse);
				SetMaskAlpha(Region, X, Y, Alpha);
			}
		}
	}

	FMaskRegion MakeBranchRegion()
	{
		FMaskRegion Region;
		Region.Name = TEXT("Branch");
		Region.MeshWidth = 13.5f;
		Region.MeshHeight = 72.0f;
		Region.BaseColor = FLinearColor(0.24f, 0.15f, 0.070f, 1.0f);
		Region.TipColor = FLinearColor(0.18f, 0.30f, 0.09f, 1.0f);
		InitializeRegion(Region);

		RasterizeTaperedStem(Region, 0.50f, 0.52f, 0.105f, 0.044f, -0.020f, 0.0f, 0.985f);
		ExtractOutlineSlices(Region, 9);
		BuildDebugPixels(Region);
		return Region;
	}

	FMaskRegion MakeLeafRegion()
	{
		FMaskRegion Region;
		Region.Name = TEXT("Leaf");
		Region.MeshWidth = 26.0f;
		Region.MeshHeight = 44.0f;
		Region.BaseColor = FLinearColor(0.075f, 0.24f, 0.065f, 1.0f);
		Region.TipColor = FLinearColor(0.42f, 0.66f, 0.18f, 1.0f);
		InitializeRegion(Region);

		RasterizeLeaf(Region, 0.50f, 0.54f, 0.205f, -0.030f, 0.015f, 0.985f, 0.15f);
		ExtractOutlineSlices(Region, 8);
		BuildDebugPixels(Region);
		return Region;
	}

	FMaskRegion MakeLeafClusterRegion()
	{
		FMaskRegion Region;
		Region.Name = TEXT("LeafCluster");
		Region.MeshWidth = 54.0f;
		Region.MeshHeight = 54.0f;
		Region.BaseColor = FLinearColor(0.06f, 0.22f, 0.055f, 1.0f);
		Region.TipColor = FLinearColor(0.36f, 0.58f, 0.15f, 1.0f);
		InitializeRegion(Region);

		RasterizeLeaf(Region, 0.49f, 0.22f, 0.122f, -0.060f, 0.00f, 0.82f, 0.05f);
		RasterizeLeaf(Region, 0.49f, 0.39f, 0.142f, -0.025f, 0.02f, 0.94f, 0.24f);
		RasterizeLeaf(Region, 0.51f, 0.55f, 0.165f, 0.015f, 0.00f, 0.98f, 0.46f);
		RasterizeLeaf(Region, 0.53f, 0.70f, 0.138f, 0.045f, 0.02f, 0.90f, 0.67f);
		RasterizeLeaf(Region, 0.53f, 0.86f, 0.112f, 0.070f, 0.00f, 0.78f, 0.86f);
		ExtractOutlineSlices(Region, 9);
		BuildDebugPixels(Region);
		return Region;
	}

	FMaskRegion MakeFlowerCenterRegion()
	{
		FMaskRegion Region;
		Region.Name = TEXT("FlowerCenter");
		Region.MeshWidth = 8.0f;
		Region.MeshHeight = 8.0f;
		Region.BaseColor = FLinearColor(0.76f, 0.46f, 0.08f, 1.0f);
		Region.TipColor = FLinearColor(1.0f, 0.78f, 0.18f, 1.0f);
		InitializeRegion(Region);

		RasterizeRotatedEllipse(Region, 0.50f, 0.50f, 0.360f, 0.360f, 0.0f, 0.080f);
		ExtractOutlineSlices(Region, 6);
		BuildDebugPixels(Region);
		return Region;
	}

	FMaskRegion MakeFlowerPetalRegion(const TCHAR* RegionName, const FLinearColor& BaseColor, const FLinearColor& TipColor)
	{
		FMaskRegion Region;
		Region.Name = RegionName;
		Region.MeshWidth = 17.0f;
		Region.MeshHeight = 14.0f;
		Region.BaseColor = BaseColor;
		Region.TipColor = TipColor;
		InitializeRegion(Region);

		RasterizePetal(Region, 0.50f, 0.315f, 0.000f, 0.000f, 1.000f);
		ExtractOutlineSlices(Region, 6);
		BuildDebugPixels(Region);
		return Region;
	}

	void BuildMaskRegions(TArray<FMaskRegion>& OutRegions)
	{
		OutRegions.Reset();
		OutRegions.Add(MakeBranchRegion());
		OutRegions.Add(MakeLeafRegion());
		OutRegions.Add(MakeLeafClusterRegion());
		OutRegions.Add(MakeFlowerCenterRegion());
		OutRegions.Add(MakeFlowerPetalRegion(TEXT("FlowerPetalWhite"), FLinearColor(0.82f, 0.78f, 0.70f, 1.0f), FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)));
		OutRegions.Add(MakeFlowerPetalRegion(TEXT("FlowerPetalYellow"), FLinearColor(0.88f, 0.58f, 0.10f, 1.0f), FLinearColor(1.0f, 0.88f, 0.28f, 1.0f)));
		OutRegions.Add(MakeFlowerPetalRegion(TEXT("FlowerPetalPink"), FLinearColor(0.66f, 0.13f, 0.35f, 1.0f), FLinearColor(0.98f, 0.52f, 0.72f, 1.0f)));
	}

	void BuildAtlasPixels(TArray<FMaskRegion>& Regions, TArray<FColor>& OutPixels, int32& OutWidth, int32& OutHeight)
	{
		OutWidth = ShapePadding + Regions.Num() * (ShapeWidth + ShapePadding);
		OutHeight = ShapeHeight + ShapePadding * 2;
		OutPixels.Init(FColor::Transparent, OutWidth * OutHeight);

		for (int32 RegionIndex = 0; RegionIndex < Regions.Num(); ++RegionIndex)
		{
			FMaskRegion& Region = Regions[RegionIndex];
			const int32 OffsetX = ShapePadding + RegionIndex * (ShapeWidth + ShapePadding);
			const int32 OffsetY = ShapePadding;
			Region.AtlasU0 = static_cast<float>(OffsetX) / static_cast<float>(OutWidth - 1);
			Region.AtlasU1 = static_cast<float>(OffsetX + Region.Width - 1) / static_cast<float>(OutWidth - 1);
			Region.AtlasVTop = static_cast<float>(OffsetY) / static_cast<float>(OutHeight - 1);
			Region.AtlasVBottom = static_cast<float>(OffsetY + Region.Height - 1) / static_cast<float>(OutHeight - 1);

			for (int32 Y = 0; Y < Region.Height; ++Y)
			{
				for (int32 X = 0; X < Region.Width; ++X)
				{
					OutPixels[(OffsetY + Y) * OutWidth + OffsetX + X] = Region.DebugPixels[Y * Region.Width + X];
				}
			}
		}
	}

	FVector2f ConvertLocalUvToAtlasUv(const FMaskRegion& Region, const FVector2f& LocalUv)
	{
		return FVector2f(
			FMath::Lerp(Region.AtlasU0, Region.AtlasU1, ClampUnit(LocalUv.X)),
			FMath::Lerp(Region.AtlasVBottom, Region.AtlasVTop, ClampUnit(LocalUv.Y)));
	}

	void BuildFallbackPaintPixels(const TArray<FMaskRegion>& Regions, TArray<FColor>& OutPixels, int32 Width, int32 Height)
	{
		OutPixels.Init(FColor::Transparent, Width * Height);

		for (const FMaskRegion& Region : Regions)
		{
			const int32 OffsetX = FMath::RoundToInt(Region.AtlasU0 * static_cast<float>(Width - 1));
			const int32 OffsetY = FMath::RoundToInt(Region.AtlasVTop * static_cast<float>(Height - 1));
			const bool bBranch = Region.Name == TEXT("Branch");
			const bool bFlowerCenter = Region.Name == TEXT("FlowerCenter");
			const bool bFlowerPetal = Region.Name.StartsWith(TEXT("FlowerPetal"));

			for (int32 Y = 0; Y < Region.Height; ++Y)
			{
				int32 LeftX = 0;
				int32 RightX = 0;
				FindSolidSpanOnRow(Region, Y, LeftX, RightX);
				const float CenterX = (static_cast<float>(LeftX) + static_cast<float>(RightX)) * 0.5f;
				const float V = 1.0f - static_cast<float>(Y) / static_cast<float>(Region.Height - 1);

				for (int32 X = 0; X < Region.Width; ++X)
				{
					const int32 LocalIndex = Y * Region.Width + X;
					const uint8 Alpha = Region.Alpha[LocalIndex];
					if (Alpha < SolidAlphaThreshold)
					{
						continue;
					}

					const float U = static_cast<float>(X) / static_cast<float>(Region.Width - 1);
					const float Noise =
						0.92f +
						0.05f * FMath::Sin((U * 31.0f + V * 17.0f) * PI) +
						0.04f * FMath::Sin((U * 11.0f - V * 29.0f) * PI);
					FLinearColor Color = LerpColor(Region.BaseColor, Region.TipColor, FMath::Pow(V, 0.72f));

					if (bBranch)
					{
						const float BarkStripe = 0.5f + 0.5f * FMath::Sin((V * 26.0f + U * 7.0f) * PI);
						Color = LerpColor(
							FLinearColor(0.18f, 0.105f, 0.052f, 1.0f),
							FLinearColor(0.29f, 0.19f, 0.085f, 1.0f),
							BarkStripe);
					}
					else if (bFlowerCenter)
					{
						const float DeltaU = U - 0.50f;
						const float DeltaV = V - 0.50f;
						const float Radius = FMath::Sqrt(DeltaU * DeltaU + DeltaV * DeltaV);
						const float Grain = 0.5f + 0.5f * FMath::Sin((U * 39.0f + V * 31.0f) * PI);
						Color = LerpColor(
							FLinearColor(0.70f, 0.42f, 0.08f, 1.0f),
							FLinearColor(1.0f, 0.78f, 0.18f, 1.0f),
							1.0f - SmoothStep(0.02f, 0.18f, Radius));
						Color = LerpColor(Color, FLinearColor(0.98f, 0.90f, 0.36f, 1.0f), Grain * 0.16f);
					}
					else if (bFlowerPetal)
					{
						const float CenterDistance = FMath::Abs(U - 0.50f);
						const float Ridge = 1.0f - SmoothStep(0.0f, 0.08f, CenterDistance);
						const float TipLight = SmoothStep(0.25f, 0.94f, V);
						FLinearColor RidgeColor = LerpColor(Region.BaseColor, FLinearColor::White, 0.30f);
						if (Region.Name == TEXT("FlowerPetalWhite"))
						{
							RidgeColor = FLinearColor(1.0f, 0.98f, 0.92f, 1.0f);
						}
						else if (Region.Name == TEXT("FlowerPetalYellow"))
						{
							RidgeColor = FLinearColor(1.0f, 0.78f, 0.16f, 1.0f);
						}

						Color = LerpColor(Region.BaseColor, Region.TipColor, TipLight);
						Color = LerpColor(Color, RidgeColor, Ridge * 0.20f);
						Color = LerpColor(Color, FLinearColor(0.94f, 0.70f, 0.16f, 1.0f), 1.0f - SmoothStep(0.03f, 0.23f, V));
					}
					else
					{
						const float CenterDistance = FMath::Abs(static_cast<float>(X) - CenterX);
						if (CenterDistance <= 1.5f)
						{
							Color = LerpColor(Color, FLinearColor(0.56f, 0.78f, 0.24f, 1.0f), 0.45f);
						}

						const float VeinMask = 1.0f - SmoothStep(0.0f, 1.0f, FMath::Abs(FMath::Frac(V * 8.0f + U * 2.0f) - 0.5f) * 2.0f);
						if (VeinMask > 0.72f && V > 0.16f && V < 0.88f)
						{
							Color = LerpColor(Color, FLinearColor(0.47f, 0.68f, 0.20f, 1.0f), 0.20f);
						}
					}

					Color.R = FMath::Clamp(Color.R * Noise, 0.0f, 1.0f);
					Color.G = FMath::Clamp(Color.G * Noise, 0.0f, 1.0f);
					Color.B = FMath::Clamp(Color.B * Noise, 0.0f, 1.0f);
					Color.A = static_cast<float>(Alpha) / 255.0f;

					OutPixels[(OffsetY + Y) * Width + OffsetX + X] = Color.ToFColor(true);
				}
			}
		}
	}

	void ApplyGuideAlphaAndDilateColors(TArray<FColor>& Pixels, const TArray<FColor>& GuidePixels, int32 Width, int32 Height)
	{
		if (Pixels.Num() != Width * Height || GuidePixels.Num() != Width * Height)
		{
			return;
		}

		TArray<uint8> GuideAlpha;
		GuideAlpha.SetNumUninitialized(Pixels.Num());
		for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
		{
			GuideAlpha[PixelIndex] = GuidePixels[PixelIndex].A;
			Pixels[PixelIndex].A = GuideAlpha[PixelIndex];
		}

		constexpr int32 DilationIterations = 10;
		for (int32 Iteration = 0; Iteration < DilationIterations; ++Iteration)
		{
			TArray<FColor> DilatedPixels = Pixels;
			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					const int32 PixelIndex = Y * Width + X;
					if (GuideAlpha[PixelIndex] >= SolidAlphaThreshold)
					{
						continue;
					}

					FIntVector AccumulatedColor(0, 0, 0);
					int32 SampleCount = 0;
					for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
					{
						for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
						{
							if (OffsetX == 0 && OffsetY == 0)
							{
								continue;
							}

							const int32 NeighborX = X + OffsetX;
							const int32 NeighborY = Y + OffsetY;
							if (NeighborX < 0 || NeighborX >= Width || NeighborY < 0 || NeighborY >= Height)
							{
								continue;
							}

							const int32 NeighborIndex = NeighborY * Width + NeighborX;
							if (Pixels[NeighborIndex].A < SolidAlphaThreshold)
							{
								continue;
							}

							AccumulatedColor.X += Pixels[NeighborIndex].R;
							AccumulatedColor.Y += Pixels[NeighborIndex].G;
							AccumulatedColor.Z += Pixels[NeighborIndex].B;
							++SampleCount;
						}
					}

					if (SampleCount > 0)
					{
						DilatedPixels[PixelIndex].R = static_cast<uint8>(AccumulatedColor.X / SampleCount);
						DilatedPixels[PixelIndex].G = static_cast<uint8>(AccumulatedColor.Y / SampleCount);
						DilatedPixels[PixelIndex].B = static_cast<uint8>(AccumulatedColor.Z / SampleCount);
					}
				}
			}

			for (int32 PixelIndex = 0; PixelIndex < Pixels.Num(); ++PixelIndex)
			{
				DilatedPixels[PixelIndex].A = GuideAlpha[PixelIndex];
			}
			Pixels = MoveTemp(DilatedPixels);
		}
	}

	bool LoadAndConformPaintPixels(
		const FString& SourceFile,
		const TArray<FColor>& GuidePixels,
		int32 Width,
		int32 Height,
		TArray<FColor>& OutPixels)
	{
		FImage SourceImage;
		if (!FImageUtils::LoadImage(*SourceFile, SourceImage))
		{
			UE_LOG(LogTunaSweeperExperimentalVegetation, Error, TEXT("Failed to load Imagegen paint source %s."), *SourceFile);
			return false;
		}

		SourceImage.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		TArray<FColor> SourcePixels;
		const TArrayView64<FColor> SourceView = SourceImage.AsBGRA8();
		SourcePixels.Append(SourceView.GetData(), static_cast<int32>(SourceView.Num()));

		if (SourceImage.SizeX == Width && SourceImage.SizeY == Height)
		{
			OutPixels = MoveTemp(SourcePixels);
		}
		else
		{
			OutPixels.SetNumUninitialized(Width * Height);
			FImageUtils::ImageResize(
				SourceImage.SizeX,
				SourceImage.SizeY,
				SourcePixels,
				Width,
				Height,
				OutPixels,
				true,
				false);
		}

		ApplyGuideAlphaAndDilateColors(OutPixels, GuidePixels, Width, Height);
		return true;
	}

	UTexture2D* EnsureTextureAsset(const FString& AssetName, const TArray<FColor>& Pixels, int32 Width, int32 Height, bool bDisableMipmaps)
	{
		const FString ObjectPath = GetAssetObjectPath(VegetationAssetPath, AssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		const bool bNewAsset = Texture == nullptr;

		if (!Texture)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *VegetationAssetPath, *AssetName);
			UPackage* Package = CreatePackage(*PackageName);
			Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
			if (!Texture)
			{
				return nullptr;
			}
		}

		Texture->Modify();
		Texture->Source.Init(
			Width,
			Height,
			1,
			1,
			TSF_BGRA8,
			reinterpret_cast<const uint8*>(Pixels.GetData()));
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = bDisableMipmaps ? TMGS_NoMipmaps : TMGS_FromTextureGroup;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		if (bNewAsset)
		{
			FAssetRegistryModule::AssetCreated(Texture);
		}

		return SaveAsset(Texture) ? Texture : nullptr;
	}

	UTexture2D* ImportImagegenPaintTextureFromSourceFile(
		const FString& SourceFile,
		const TArray<FColor>& GuidePixels,
		int32 Width,
		int32 Height)
	{
		if (SourceFile.IsEmpty() || !FPaths::FileExists(SourceFile))
		{
			return nullptr;
		}

		TArray<FColor> PaintPixels;
		if (!LoadAndConformPaintPixels(SourceFile, GuidePixels, Width, Height, PaintPixels))
		{
			return nullptr;
		}

		SavePixelsAsPng(
			GetVegetationGeneratedImagePath(TEXT("ExperimentalVegetationImagegenPaintImportedMasked.png")),
			PaintPixels,
			Width,
			Height);
		return EnsureTextureAsset(VegetationPaintTextureAssetName, PaintPixels, Width, Height, false);
	}

	UTexture2D* ImportImagegenPaintTextureFromCommandLineIfRequested(
		const TArray<FColor>& GuidePixels,
		int32 Width,
		int32 Height)
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperExperimentalVegetationPaintSource="), SourceFile))
		{
			return nullptr;
		}

		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		return ImportImagegenPaintTextureFromSourceFile(SourceFile, GuidePixels, Width, Height);
	}

	UMaterial* EnsureVegetationMaterial(UTexture2D* PaintTexture, UTexture2D* GuideTexture)
	{
		const FString ObjectPath = GetAssetObjectPath(VegetationAssetPath, VegetationMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				VegetationMaterialAssetName,
				VegetationAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperExperimentalVegetation, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->TwoSided = true;
		Material->BlendMode = BLEND_Masked;
		Material->OpacityMaskClipValue = 0.18f;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperExperimentalVegetation, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -520;
		VertexColorExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -820;
		TextureCoordinateExpression->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSample* PaintTextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
		PaintTextureSample->Material = Material;
		PaintTextureSample->Texture = PaintTexture;
		PaintTextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		PaintTextureSample->MaterialExpressionEditorX = -520;
		PaintTextureSample->MaterialExpressionEditorY = 180;
		PaintTextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(PaintTextureSample);

		UMaterialExpressionTextureSample* GuideTextureSample = NewObject<UMaterialExpressionTextureSample>(Material);
		GuideTextureSample->Material = Material;
		GuideTextureSample->Texture = GuideTexture;
		GuideTextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		GuideTextureSample->MaterialExpressionEditorX = -520;
		GuideTextureSample->MaterialExpressionEditorY = 420;
		GuideTextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(GuideTextureSample);

		UMaterialExpressionMultiply* TintedTextureMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		TintedTextureMultiply->Material = Material;
		TintedTextureMultiply->A.Connect(0, PaintTextureSample);
		TintedTextureMultiply->B.Connect(0, VertexColorExpression);
		TintedTextureMultiply->MaterialExpressionEditorX = -220;
		TintedTextureMultiply->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(TintedTextureMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, TintedTextureMultiply);
		MaterialEditorOnly->OpacityMask.Connect(4, GuideTextureSample);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.86f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.24f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		return SaveAsset(Material) ? Material : nullptr;
	}

	FLinearColor GetSliceColor(const FMaskRegion& Region, float V, float ColorScale)
	{
		FLinearColor Color = LerpColor(Region.BaseColor, Region.TipColor, FMath::Pow(V, 0.72f));
		Color.R = FMath::Clamp(Color.R * ColorScale, 0.0f, 1.0f);
		Color.G = FMath::Clamp(Color.G * ColorScale, 0.0f, 1.0f);
		Color.B = FMath::Clamp(Color.B * ColorScale, 0.0f, 1.0f);
		Color.A = 1.0f;
		return Color;
	}

	FVector3f EvaluatePieceCenter(const FMaskRegion& Region, const FPiecePlacement& Placement, float V)
	{
		const float Length = Region.MeshHeight * Placement.LengthScale;
		const float Sway = Placement.SideSway * FMath::Sin(V * PI * 1.35f + Placement.Phase) * V;
		const float Bend = Placement.BendAmount * FMath::Pow(V, 1.28f);

		return Placement.Base +
			Placement.Along * (Length * V) +
			Placement.Bend * Bend +
			Placement.Side * Sway;
	}

	FVector3f ComputeVegetationPoint(
		const FMaskRegion& Region,
		const FSilhouetteSlice& Slice,
		const FPiecePlacement& Placement,
		float U)
	{
		const float Width = Region.MeshWidth * Placement.WidthScale;
		const float SourceCenterOffset = (Slice.CenterU - 0.5f) * Width;
		const float SourceSideOffset = (U - Slice.CenterU) * Width;
		return EvaluatePieceCenter(Region, Placement, Slice.V) + Placement.Side * (SourceCenterOffset + SourceSideOffset);
	}

	FVertexInstanceID AddVegetationVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector2f& UV,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = Normal;
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		VertexInstanceColors[VertexInstanceId] = FVector4f(Color.R, Color.G, Color.B, Color.A);
		return VertexInstanceId;
	}

	void AddVegetationQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& LeftBottom,
		const FVector3f& RightBottom,
		const FVector3f& RightTop,
		const FVector3f& LeftTop,
		const FVector2f& LeftBottomUv,
		const FVector2f& RightBottomUv,
		const FVector2f& RightTopUv,
		const FVector2f& LeftTopUv,
		const FLinearColor& BottomColor,
		const FLinearColor& TopColor)
	{
		FVector3f Normal = FVector3f::CrossProduct(RightBottom - LeftBottom, LeftTop - LeftBottom);
		Normal = SafeNormal(Normal, FVector3f(0.0f, -1.0f, 0.0f));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(4);
		VertexInstances.Add(AddVegetationVertex(MeshDescription, Attributes, LeftBottom, Normal, LeftBottomUv, BottomColor));
		VertexInstances.Add(AddVegetationVertex(MeshDescription, Attributes, RightBottom, Normal, RightBottomUv, BottomColor));
		VertexInstances.Add(AddVegetationVertex(MeshDescription, Attributes, RightTop, Normal, RightTopUv, TopColor));
		VertexInstances.Add(AddVegetationVertex(MeshDescription, Attributes, LeftTop, Normal, LeftTopUv, TopColor));
		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddVegetationPiece(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FMaskRegion& Region,
		const FPiecePlacement& Placement)
	{
		if (Region.Slices.Num() < 2)
		{
			return;
		}

		for (int32 SliceIndex = 0; SliceIndex < Region.Slices.Num() - 1; ++SliceIndex)
		{
			const FSilhouetteSlice& BottomSlice = Region.Slices[SliceIndex];
			const FSilhouetteSlice& TopSlice = Region.Slices[SliceIndex + 1];

			const FVector3f LeftBottom = ComputeVegetationPoint(Region, BottomSlice, Placement, BottomSlice.LeftU);
			const FVector3f RightBottom = ComputeVegetationPoint(Region, BottomSlice, Placement, BottomSlice.RightU);
			const FVector3f RightTop = ComputeVegetationPoint(Region, TopSlice, Placement, TopSlice.RightU);
			const FVector3f LeftTop = ComputeVegetationPoint(Region, TopSlice, Placement, TopSlice.LeftU);
			const FLinearColor BottomColor = GetSliceColor(Region, BottomSlice.V, Placement.ColorScale);
			const FLinearColor TopColor = GetSliceColor(Region, TopSlice.V, Placement.ColorScale);

			AddVegetationQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				LeftBottom,
				RightBottom,
				RightTop,
				LeftTop,
				ConvertLocalUvToAtlasUv(Region, FVector2f(BottomSlice.LeftU, BottomSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(BottomSlice.RightU, BottomSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(TopSlice.RightU, TopSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(TopSlice.LeftU, TopSlice.V)),
				BottomColor,
				TopColor);
		}
	}

	float RandomRange(FRandomStream& RandomStream, float Min, float Max)
	{
		return FMath::Lerp(Min, Max, RandomStream.GetFraction());
	}

	float MakeDistributedBranchAngle(
		int32 LevelIndex,
		int32 BranchIndex,
		int32 BranchesAtNode,
		FRandomStream& RandomStream,
		float PhaseOffset,
		float JitterRadians)
	{
		constexpr float GoldenAngle = 2.39996314f;
		const float NodeAngle = PhaseOffset + static_cast<float>(LevelIndex) * GoldenAngle;
		const float BranchOffset = BranchesAtNode > 1
			? static_cast<float>(BranchIndex) / static_cast<float>(BranchesAtNode) * 2.0f * PI
			: 0.0f;
		return NodeAngle + BranchOffset + RandomRange(RandomStream, -JitterRadians, JitterRadians);
	}

	bool IsFloweringVariant(EVegetationMeshVariant Variant)
	{
		return
			Variant == EVegetationMeshVariant::FloweringWhite ||
			Variant == EVegetationMeshVariant::FloweringYellow ||
			Variant == EVegetationMeshVariant::FloweringPink;
	}

	int32 GetFlowerRegionIndex(EVegetationMeshVariant Variant)
	{
		if (Variant == EVegetationMeshVariant::FloweringWhite)
		{
			return 4;
		}
		if (Variant == EVegetationMeshVariant::FloweringYellow)
		{
			return 5;
		}
		if (Variant == EVegetationMeshVariant::FloweringPink)
		{
			return 6;
		}
		return INDEX_NONE;
	}

	void AddLeafAt(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FMaskRegion& LeafRegion,
		const FVector3f& Base,
		const FVector3f& Direction,
		const FVector3f& WidthHint,
		const FVector3f& BendHint,
		float LengthScale,
		float WidthScale,
		float BendAmount,
		float ColorScale,
		float Phase)
	{
		FPiecePlacement LeafPlacement = MakePiecePlacement(Base, Direction, WidthHint, BendHint);
		LeafPlacement.LengthScale = LengthScale;
		LeafPlacement.WidthScale = WidthScale;
		LeafPlacement.BendAmount = BendAmount;
		LeafPlacement.SideSway = 3.4f;
		LeafPlacement.Phase = Phase;
		LeafPlacement.ColorScale = ColorScale;
		AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, LeafRegion, LeafPlacement);
	}

	FVector3f EvaluateSegmentedFlowerPetalCenter(const FMaskRegion& Region, const FPiecePlacement& Placement, float V)
	{
		const float Length = Region.MeshHeight * Placement.LengthScale;
		const float Sway = Placement.SideSway * FMath::Sin(V * PI * 1.20f + Placement.Phase) * V;
		float BendAlpha = 0.0f;
		if (V <= 1.0f / 3.0f)
		{
			BendAlpha = FMath::Lerp(0.0f, 0.28f, V * 3.0f);
		}
		else if (V <= 2.0f / 3.0f)
		{
			BendAlpha = FMath::Lerp(0.28f, 0.70f, (V - 1.0f / 3.0f) * 3.0f);
		}
		else
		{
			BendAlpha = FMath::Lerp(0.70f, 1.0f, (V - 2.0f / 3.0f) * 3.0f);
		}

		return Placement.Base +
			Placement.Along * (Length * V) +
			Placement.Bend * (Placement.BendAmount * BendAlpha) +
			Placement.Side * Sway;
	}

	FVector3f ComputeSegmentedFlowerPetalPoint(
		const FMaskRegion& Region,
		const FSilhouetteSlice& Slice,
		const FPiecePlacement& Placement,
		float U)
	{
		const float Width = Region.MeshWidth * Placement.WidthScale;
		const float SourceCenterOffset = (Slice.CenterU - 0.5f) * Width;
		const float SourceSideOffset = (U - Slice.CenterU) * Width;
		return EvaluateSegmentedFlowerPetalCenter(Region, Placement, Slice.V) + Placement.Side * (SourceCenterOffset + SourceSideOffset);
	}

	void AddFlowerPetalPiece(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FMaskRegion& Region,
		const FPiecePlacement& Placement)
	{
		if (Region.Slices.Num() < 2)
		{
			return;
		}

		for (int32 SliceIndex = 0; SliceIndex < Region.Slices.Num() - 1; ++SliceIndex)
		{
			const FSilhouetteSlice& BottomSlice = Region.Slices[SliceIndex];
			const FSilhouetteSlice& TopSlice = Region.Slices[SliceIndex + 1];

			const FVector3f LeftBottom = ComputeSegmentedFlowerPetalPoint(Region, BottomSlice, Placement, BottomSlice.LeftU);
			const FVector3f RightBottom = ComputeSegmentedFlowerPetalPoint(Region, BottomSlice, Placement, BottomSlice.RightU);
			const FVector3f RightTop = ComputeSegmentedFlowerPetalPoint(Region, TopSlice, Placement, TopSlice.RightU);
			const FVector3f LeftTop = ComputeSegmentedFlowerPetalPoint(Region, TopSlice, Placement, TopSlice.LeftU);
			const FLinearColor BottomColor = GetSliceColor(Region, BottomSlice.V, Placement.ColorScale);
			const FLinearColor TopColor = GetSliceColor(Region, TopSlice.V, Placement.ColorScale);

			AddVegetationQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				LeftBottom,
				RightBottom,
				RightTop,
				LeftTop,
				ConvertLocalUvToAtlasUv(Region, FVector2f(BottomSlice.LeftU, BottomSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(BottomSlice.RightU, BottomSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(TopSlice.RightU, TopSlice.V)),
				ConvertLocalUvToAtlasUv(Region, FVector2f(TopSlice.LeftU, TopSlice.V)),
				BottomColor,
				TopColor);
		}
	}

	void AddFlowerBloomAt(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FMaskRegion& FlowerCenterRegion,
		const FMaskRegion& FlowerPetalRegion,
		const FVector3f& Center,
		const FVector3f& FaceNormal,
		const FVector3f& UpHint,
		float Scale,
		float ColorScale,
		float Phase)
	{
		const FVector3f Face = SafeNormal(FaceNormal, FVector3f(1.0f, 0.0f, 0.0f));
		FVector3f Up = ProjectToPlaneNormal(UpHint, Face);
		if (Up.SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			Up = ProjectToPlaneNormal(FVector3f(0.0f, 0.0f, 1.0f), Face);
		}
		Up = SafeNormal(Up, FVector3f(0.0f, 0.0f, 1.0f));
		FVector3f Right = SafeNormal(FVector3f::CrossProduct(Face, Up), FVector3f(0.0f, 1.0f, 0.0f));

		constexpr int32 PetalCount = 5;
		const float CenterRadius = FlowerCenterRegion.MeshHeight * Scale * 0.16f;
		for (int32 PetalIndex = 0; PetalIndex < PetalCount; ++PetalIndex)
		{
			const float Angle = Phase + static_cast<float>(PetalIndex) / static_cast<float>(PetalCount) * 2.0f * PI;
			const FVector3f PetalAlong = SafeNormal(Up * FMath::Cos(Angle) + Right * FMath::Sin(Angle), Up);
			const FVector3f PetalSide = SafeNormal(FVector3f::CrossProduct(Face, PetalAlong), Right);
			FPiecePlacement PetalPlacement = MakePiecePlacement(
				Center + Face * 1.8f + PetalAlong * CenterRadius,
				PetalAlong,
				PetalSide,
				Face + PetalAlong * 0.08f);
			PetalPlacement.LengthScale = Scale * 0.78f;
			PetalPlacement.WidthScale = Scale * 1.18f;
			PetalPlacement.BendAmount = 1.9f * Scale;
			PetalPlacement.SideSway = 0.16f * Scale;
			PetalPlacement.Phase = Phase + static_cast<float>(PetalIndex) * 0.37f;
			PetalPlacement.ColorScale = ColorScale;
			AddFlowerPetalPiece(MeshDescription, Attributes, PolygonGroupId, FlowerPetalRegion, PetalPlacement);
		}

		FPiecePlacement CenterPlacement = MakePiecePlacement(
			Center + Face * 3.0f - Up * (FlowerCenterRegion.MeshHeight * Scale * 0.50f),
			Up,
			Right,
			Face);
		CenterPlacement.LengthScale = Scale * 1.05f;
		CenterPlacement.WidthScale = Scale * 1.05f;
		CenterPlacement.BendAmount = 0.0f;
		CenterPlacement.SideSway = 0.0f;
		CenterPlacement.Phase = Phase;
		CenterPlacement.ColorScale = 1.0f;
		AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, FlowerCenterRegion, CenterPlacement);
	}

	void AddBranchWithLeaves(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FMaskRegion& BranchRegion,
		const FMaskRegion& LeafRegion,
		const FMaskRegion& LeafClusterRegion,
		FRandomStream& RandomStream,
		const FVector3f& Base,
		const FVector3f& Direction,
		const FVector3f& SideHint,
		float BranchLengthScale,
		float BranchWidthScale,
		float BendAmount,
		int32 LeafCount,
		const FMaskRegion* FlowerCenterRegion = nullptr,
		const FMaskRegion* FlowerPetalRegion = nullptr,
		float FlowerChance = 0.0f)
	{
		const FVector3f NormalizedDirection = SafeNormal(Direction, FVector3f(0.0f, 0.0f, 1.0f));
		FPiecePlacement BranchPlacement = MakePiecePlacement(Base, NormalizedDirection, SideHint, FVector3f(0.0f, 0.0f, -1.0f) + NormalizedDirection * 0.25f);
		BranchPlacement.LengthScale = BranchLengthScale;
		BranchPlacement.WidthScale = BranchWidthScale;
		BranchPlacement.BendAmount = BendAmount;
		BranchPlacement.SideSway = RandomRange(RandomStream, -5.0f, 5.0f);
		BranchPlacement.Phase = RandomRange(RandomStream, 0.0f, 2.0f * PI);
		BranchPlacement.ColorScale = RandomRange(RandomStream, 0.88f, 1.10f);
		AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, BranchRegion, BranchPlacement);

		for (int32 LeafIndex = 0; LeafIndex < LeafCount; ++LeafIndex)
		{
			const float AlongAlpha = FMath::Lerp(0.38f, 0.92f, static_cast<float>(LeafIndex) / FMath::Max(static_cast<float>(LeafCount - 1), 1.0f));
			const FVector3f AttachPoint = EvaluatePieceCenter(BranchRegion, BranchPlacement, AlongAlpha);
			const float SideSign = LeafIndex % 2 == 0 ? 1.0f : -1.0f;
			const FVector3f LeafDirection = SafeNormal(
				NormalizedDirection * RandomRange(RandomStream, 0.30f, 0.48f) +
				BranchPlacement.Side * SideSign * RandomRange(RandomStream, 0.58f, 0.82f) +
				FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.16f, 0.36f)),
				NormalizedDirection);
			const FVector3f LeafBase = AttachPoint - LeafDirection * RandomRange(RandomStream, 1.5f, 5.0f);

			AddLeafAt(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				LeafRegion,
				LeafBase,
				LeafDirection,
				BranchPlacement.Side * SideSign,
				FVector3f(0.0f, 0.0f, -1.0f) + BranchPlacement.Side * SideSign * 0.2f,
				RandomRange(RandomStream, 0.78f, 1.18f),
				RandomRange(RandomStream, 0.82f, 1.18f),
				RandomRange(RandomStream, 5.0f, 12.0f),
				RandomRange(RandomStream, 0.82f, 1.15f),
				RandomRange(RandomStream, 0.0f, 2.0f * PI));

			if (FlowerCenterRegion && FlowerPetalRegion && RandomStream.GetFraction() < FlowerChance)
			{
				const FVector3f FlowerCenter =
					AttachPoint +
					LeafDirection * RandomRange(RandomStream, 18.0f, 30.0f) +
					BranchPlacement.Side * SideSign * RandomRange(RandomStream, 5.0f, 12.0f) +
					NormalizedDirection * RandomRange(RandomStream, 2.0f, 6.0f) +
					BranchPlacement.Bend * RandomRange(RandomStream, 1.6f, 3.6f);
				const FVector3f FlowerFaceNormal = SafeNormal(
					LeafDirection * 0.72f +
					BranchPlacement.Side * SideSign * 0.48f +
					FVector3f(0.0f, 0.0f, 0.18f),
					LeafDirection);
				AddFlowerBloomAt(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					*FlowerCenterRegion,
					*FlowerPetalRegion,
					FlowerCenter,
					FlowerFaceNormal,
					FVector3f(0.0f, 0.0f, 1.0f) + BranchPlacement.Side * SideSign * 0.22f,
					RandomRange(RandomStream, 0.58f, 0.78f),
					RandomRange(RandomStream, 0.92f, 1.08f),
					RandomRange(RandomStream, 0.0f, 2.0f * PI));
			}
		}

		const FVector3f TipPoint = EvaluatePieceCenter(BranchRegion, BranchPlacement, 1.0f);
		FPiecePlacement TipClusterPlacement = MakePiecePlacement(
			TipPoint - NormalizedDirection * 4.0f,
			SafeNormal(NormalizedDirection + FVector3f(0.0f, 0.0f, 0.18f), NormalizedDirection),
			BranchPlacement.Side,
			FVector3f(0.0f, 0.0f, -1.0f));
		TipClusterPlacement.LengthScale = RandomRange(RandomStream, 0.76f, 1.12f);
		TipClusterPlacement.WidthScale = RandomRange(RandomStream, 0.72f, 1.00f);
		TipClusterPlacement.BendAmount = RandomRange(RandomStream, 6.0f, 15.0f);
		TipClusterPlacement.SideSway = RandomRange(RandomStream, -4.5f, 4.5f);
		TipClusterPlacement.Phase = RandomRange(RandomStream, 0.0f, 2.0f * PI);
		TipClusterPlacement.ColorScale = RandomRange(RandomStream, 0.86f, 1.10f);
		AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, LeafClusterRegion, TipClusterPlacement);
	}

	void BuildExperimentalVegetationMeshDescription(
		FMeshDescription& MeshDescription,
		const TArray<FMaskRegion>& Regions,
		EVegetationMeshVariant Variant)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Vegetation"));

		if (Regions.Num() < 3 || (IsFloweringVariant(Variant) && Regions.Num() < 7))
		{
			return;
		}

		const FMaskRegion& BranchRegion = Regions[0];
		const FMaskRegion& LeafRegion = Regions[1];
		const FMaskRegion& LeafClusterRegion = Regions[2];

		if (Variant == EVegetationMeshVariant::AlternateBranches)
		{
			FRandomStream RandomStream(20260525);

			FPiecePlacement MainStemPlacement = MakePiecePlacement(
				FVector3f(8.0f, -6.0f, 0.0f),
				FVector3f(-0.14f, 0.10f, 1.0f),
				FVector3f(0.30f, 1.0f, 0.0f),
				FVector3f(-1.0f, 0.35f, -0.04f));
			MainStemPlacement.LengthScale = 1.50f;
			MainStemPlacement.WidthScale = 1.86f;
			MainStemPlacement.BendAmount = 24.0f;
			MainStemPlacement.SideSway = -6.0f;
			MainStemPlacement.Phase = 1.2f;
			MainStemPlacement.ColorScale = 0.96f;
			AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, BranchRegion, MainStemPlacement);

			const float BranchLevels[] = { 0.18f, 0.31f, 0.44f, 0.58f, 0.71f, 0.83f };
			for (int32 LevelIndex = 0; LevelIndex < UE_ARRAY_COUNT(BranchLevels); ++LevelIndex)
			{
				const float Level = BranchLevels[LevelIndex];
				const FVector3f BranchBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, Level);
				const int32 BranchesAtNode = LevelIndex < 5 ? 2 : 1;
				for (int32 BranchIndex = 0; BranchIndex < BranchesAtNode; ++BranchIndex)
				{
					const float SideSign = BranchIndex % 2 == 0 ? 1.0f : -1.0f;
					const float Angle = MakeDistributedBranchAngle(LevelIndex, BranchIndex, BranchesAtNode, RandomStream, 0.86f, 0.24f);
					const FVector3f Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
					const FVector3f Direction = SafeNormal(
						Outward * RandomRange(RandomStream, 1.10f, 1.42f) +
						FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.12f, 0.34f)),
						FVector3f(1.0f, 0.0f, 0.30f));
					const FVector3f SideHint = SafeNormal(FVector3f::CrossProduct(FVector3f(0.0f, 0.0f, 1.0f), Direction), FVector3f(0.0f, 1.0f, 0.0f));

					AddBranchWithLeaves(
						MeshDescription,
						Attributes,
						PolygonGroupId,
						BranchRegion,
						LeafRegion,
						LeafClusterRegion,
						RandomStream,
						BranchBase,
						Direction,
						SideHint,
						RandomRange(RandomStream, 0.82f, 1.20f) * FMath::Lerp(1.08f, 0.72f, Level),
						RandomRange(RandomStream, 0.88f, 1.18f),
						RandomRange(RandomStream, 12.0f, 28.0f),
						LevelIndex < 4 ? 4 : 3);

					if (LevelIndex < 5)
					{
						const FVector3f SecondaryBase = BranchBase + Direction * BranchRegion.MeshHeight * RandomRange(RandomStream, 0.22f, 0.38f);
						const FVector3f SecondaryDirection = SafeNormal(
							Direction * 0.48f +
							SideHint * -SideSign * RandomRange(RandomStream, 0.56f, 0.84f) +
							FVector3f(0.0f, 0.0f, RandomRange(RandomStream, -0.02f, 0.16f)),
							Direction);
						AddBranchWithLeaves(
							MeshDescription,
							Attributes,
							PolygonGroupId,
							BranchRegion,
							LeafRegion,
							LeafClusterRegion,
							RandomStream,
							SecondaryBase,
							SecondaryDirection,
							SideHint * -SideSign,
							RandomRange(RandomStream, 0.42f, 0.68f),
							RandomRange(RandomStream, 0.68f, 0.92f),
							RandomRange(RandomStream, 8.0f, 18.0f),
							2);
					}
				}
			}

			for (int32 CrownIndex = 0; CrownIndex < 5; ++CrownIndex)
			{
				const float Angle = static_cast<float>(CrownIndex) / 5.0f * 2.0f * PI + RandomRange(RandomStream, -0.18f, 0.18f);
				const FVector3f Direction = SafeNormal(
					FVector3f(FMath::Cos(Angle) * 1.06f, FMath::Sin(Angle) * 1.06f, RandomRange(RandomStream, 0.22f, 0.44f)),
					FVector3f(0.0f, 0.0f, 1.0f));
				const FVector3f CrownBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, RandomRange(RandomStream, 0.78f, 0.94f));
				AddBranchWithLeaves(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					BranchRegion,
					LeafRegion,
					LeafClusterRegion,
					RandomStream,
					CrownBase,
					Direction,
					FVector3f(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0f),
					RandomRange(RandomStream, 0.52f, 0.78f),
					RandomRange(RandomStream, 0.74f, 0.96f),
					RandomRange(RandomStream, 7.0f, 16.0f),
					2);
			}
			return;
		}

		if (IsFloweringVariant(Variant))
		{
			const int32 FlowerRegionIndex = GetFlowerRegionIndex(Variant);
			if (!Regions.IsValidIndex(FlowerRegionIndex))
			{
				return;
			}

			const FMaskRegion& FlowerCenterRegion = Regions[3];
			const FMaskRegion& FlowerPetalRegion = Regions[FlowerRegionIndex];
			FRandomStream RandomStream(20260526 + FlowerRegionIndex);

			FPiecePlacement MainStemPlacement = MakePiecePlacement(
				FVector3f(0.0f, 0.0f, 0.0f),
				FVector3f(0.08f, -0.04f, 1.0f),
				FVector3f(0.0f, 1.0f, 0.0f),
				FVector3f(1.0f, 0.25f, -0.05f));
			MainStemPlacement.LengthScale = 1.62f;
			MainStemPlacement.WidthScale = 1.95f;
			MainStemPlacement.BendAmount = 18.0f;
			MainStemPlacement.SideSway = 4.0f;
			MainStemPlacement.Phase = 0.35f;
			MainStemPlacement.ColorScale = 1.0f;
			AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, BranchRegion, MainStemPlacement);

			const float BranchLevels[] = { 0.24f, 0.38f, 0.52f, 0.66f, 0.79f, 0.90f };
			for (int32 LevelIndex = 0; LevelIndex < UE_ARRAY_COUNT(BranchLevels); ++LevelIndex)
			{
				const float Level = BranchLevels[LevelIndex];
				const FVector3f BranchBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, Level);
				const int32 BranchesAtNode = LevelIndex < 4 ? 2 : 1;
				for (int32 BranchIndex = 0; BranchIndex < BranchesAtNode; ++BranchIndex)
				{
					const float SideSign = BranchIndex % 2 == 0 ? 1.0f : -1.0f;
					const float Angle = MakeDistributedBranchAngle(LevelIndex, BranchIndex, BranchesAtNode, RandomStream, 0.34f, 0.30f);
					const FVector3f Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
					const FVector3f Direction = SafeNormal(
						Outward * RandomRange(RandomStream, 0.95f, 1.24f) +
						FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.20f, 0.42f)),
						FVector3f(1.0f, 0.0f, 0.45f));
					const FVector3f SideHint = SafeNormal(FVector3f::CrossProduct(FVector3f(0.0f, 0.0f, 1.0f), Direction), FVector3f(0.0f, 1.0f, 0.0f));

					AddBranchWithLeaves(
						MeshDescription,
						Attributes,
						PolygonGroupId,
						BranchRegion,
						LeafRegion,
						LeafClusterRegion,
						RandomStream,
						BranchBase,
						Direction,
						SideHint,
						RandomRange(RandomStream, 0.78f, 1.16f) * FMath::Lerp(1.0f, 0.76f, Level),
						RandomRange(RandomStream, 0.92f, 1.24f),
						RandomRange(RandomStream, 9.0f, 24.0f),
						LevelIndex < 4 ? 3 : 2,
						&FlowerCenterRegion,
						&FlowerPetalRegion,
						LevelIndex < 4 ? 0.34f : 0.22f);

					if (LevelIndex < 4)
					{
						const FVector3f SecondaryBase = BranchBase + Direction * BranchRegion.MeshHeight * RandomRange(RandomStream, 0.28f, 0.44f);
						const FVector3f SecondaryDirection = SafeNormal(
							Direction * 0.54f +
							SideHint * -SideSign * RandomRange(RandomStream, 0.42f, 0.68f) +
							FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.04f, 0.20f)),
							Direction);
						AddBranchWithLeaves(
							MeshDescription,
							Attributes,
							PolygonGroupId,
							BranchRegion,
							LeafRegion,
							LeafClusterRegion,
							RandomStream,
							SecondaryBase,
							SecondaryDirection,
							SideHint * -SideSign,
							RandomRange(RandomStream, 0.46f, 0.72f),
							RandomRange(RandomStream, 0.72f, 0.98f),
							RandomRange(RandomStream, 6.0f, 15.0f),
							2,
							&FlowerCenterRegion,
							&FlowerPetalRegion,
							0.28f);
					}
				}
			}

			for (int32 CrownIndex = 0; CrownIndex < 4; ++CrownIndex)
			{
				const float Angle = static_cast<float>(CrownIndex) / 4.0f * 2.0f * PI + RandomRange(RandomStream, -0.22f, 0.22f);
				const FVector3f Direction = SafeNormal(
					FVector3f(FMath::Cos(Angle), FMath::Sin(Angle), RandomRange(RandomStream, 0.34f, 0.58f)),
					FVector3f(0.0f, 0.0f, 1.0f));
				const FVector3f CrownBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, RandomRange(RandomStream, 0.86f, 0.96f));
				AddBranchWithLeaves(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					BranchRegion,
					LeafRegion,
					LeafClusterRegion,
					RandomStream,
					CrownBase,
					Direction,
					FVector3f(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0f),
					RandomRange(RandomStream, 0.56f, 0.82f),
					RandomRange(RandomStream, 0.76f, 1.02f),
					RandomRange(RandomStream, 5.0f, 13.0f),
					2,
					&FlowerCenterRegion,
					&FlowerPetalRegion,
					0.18f);
			}
			return;
		}

		FRandomStream RandomStream(20260524);

		FPiecePlacement MainStemPlacement = MakePiecePlacement(
			FVector3f(0.0f, 0.0f, 0.0f),
			FVector3f(0.08f, -0.04f, 1.0f),
			FVector3f(0.0f, 1.0f, 0.0f),
			FVector3f(1.0f, 0.25f, -0.05f));
		MainStemPlacement.LengthScale = 1.62f;
		MainStemPlacement.WidthScale = 1.95f;
		MainStemPlacement.BendAmount = 18.0f;
		MainStemPlacement.SideSway = 4.0f;
		MainStemPlacement.Phase = 0.35f;
		MainStemPlacement.ColorScale = 1.0f;
		AddVegetationPiece(MeshDescription, Attributes, PolygonGroupId, BranchRegion, MainStemPlacement);

		const float BranchLevels[] = { 0.24f, 0.38f, 0.52f, 0.66f, 0.79f, 0.90f };
		for (int32 LevelIndex = 0; LevelIndex < UE_ARRAY_COUNT(BranchLevels); ++LevelIndex)
		{
			const float Level = BranchLevels[LevelIndex];
			const FVector3f BranchBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, Level);
			const int32 BranchesAtNode = LevelIndex < 4 ? 2 : 1;
			for (int32 BranchIndex = 0; BranchIndex < BranchesAtNode; ++BranchIndex)
			{
				const float SideSign = BranchIndex % 2 == 0 ? 1.0f : -1.0f;
				const float Angle = MakeDistributedBranchAngle(LevelIndex, BranchIndex, BranchesAtNode, RandomStream, 0.12f, 0.30f);
				const FVector3f Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
				const FVector3f Direction = SafeNormal(
					Outward * RandomRange(RandomStream, 0.95f, 1.24f) +
					FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.20f, 0.42f)),
					FVector3f(1.0f, 0.0f, 0.45f));
				const FVector3f SideHint = SafeNormal(FVector3f::CrossProduct(FVector3f(0.0f, 0.0f, 1.0f), Direction), FVector3f(0.0f, 1.0f, 0.0f));

				AddBranchWithLeaves(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					BranchRegion,
					LeafRegion,
					LeafClusterRegion,
					RandomStream,
					BranchBase,
					Direction,
					SideHint,
					RandomRange(RandomStream, 0.78f, 1.16f) * FMath::Lerp(1.0f, 0.76f, Level),
					RandomRange(RandomStream, 0.92f, 1.24f),
					RandomRange(RandomStream, 9.0f, 24.0f),
					LevelIndex < 4 ? 3 : 2);

				if (LevelIndex < 4)
				{
					const FVector3f SecondaryBase = BranchBase + Direction * BranchRegion.MeshHeight * RandomRange(RandomStream, 0.28f, 0.44f);
					const FVector3f SecondaryDirection = SafeNormal(
						Direction * 0.54f +
						SideHint * -SideSign * RandomRange(RandomStream, 0.42f, 0.68f) +
						FVector3f(0.0f, 0.0f, RandomRange(RandomStream, 0.04f, 0.20f)),
						Direction);
					AddBranchWithLeaves(
						MeshDescription,
						Attributes,
						PolygonGroupId,
						BranchRegion,
						LeafRegion,
						LeafClusterRegion,
						RandomStream,
						SecondaryBase,
						SecondaryDirection,
						SideHint * -SideSign,
						RandomRange(RandomStream, 0.46f, 0.72f),
						RandomRange(RandomStream, 0.72f, 0.98f),
						RandomRange(RandomStream, 6.0f, 15.0f),
						2);
				}
			}
		}

		for (int32 CrownIndex = 0; CrownIndex < 4; ++CrownIndex)
		{
			const float Angle = static_cast<float>(CrownIndex) / 4.0f * 2.0f * PI + RandomRange(RandomStream, -0.22f, 0.22f);
			const FVector3f Direction = SafeNormal(
				FVector3f(FMath::Cos(Angle), FMath::Sin(Angle), RandomRange(RandomStream, 0.34f, 0.58f)),
				FVector3f(0.0f, 0.0f, 1.0f));
			const FVector3f CrownBase = EvaluatePieceCenter(BranchRegion, MainStemPlacement, RandomRange(RandomStream, 0.86f, 0.96f));
			AddBranchWithLeaves(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				BranchRegion,
				LeafRegion,
				LeafClusterRegion,
				RandomStream,
				CrownBase,
				Direction,
				FVector3f(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0f),
				RandomRange(RandomStream, 0.56f, 0.82f),
				RandomRange(RandomStream, 0.76f, 1.02f),
				RandomRange(RandomStream, 5.0f, 13.0f),
				2);
		}
	}

	UStaticMesh* EnsureVegetationMeshAsset(
		UMaterialInterface* VegetationMaterial,
		const TArray<FMaskRegion>& Regions,
		const FString& MeshAssetName,
		EVegetationMeshVariant Variant)
	{
		const FString ObjectPath = GetAssetObjectPath(VegetationAssetPath, MeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		const bool bNewAsset = StaticMesh == nullptr;

		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *VegetationAssetPath, *MeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			StaticMesh = NewObject<UStaticMesh>(Package, *MeshAssetName, RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildExperimentalVegetationMeshDescription(MeshDescription, Regions, Variant);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(VegetationMaterial, FName(TEXT("Vegetation"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		if (bNewAsset)
		{
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureExperimentalVegetationAssets()
	{
		TArray<FMaskRegion> Regions;
		BuildMaskRegions(Regions);

		TArray<FColor> AtlasPixels;
		int32 AtlasWidth = 0;
		int32 AtlasHeight = 0;
		BuildAtlasPixels(Regions, AtlasPixels, AtlasWidth, AtlasHeight);

		TArray<FColor> FallbackPaintPixels;
		BuildFallbackPaintPixels(Regions, FallbackPaintPixels, AtlasWidth, AtlasHeight);
		ApplyGuideAlphaAndDilateColors(FallbackPaintPixels, AtlasPixels, AtlasWidth, AtlasHeight);

		const FString GuidePngPath = GetVegetationGeneratedImagePath(TEXT("ExperimentalVegetationShapeGuide.png"));
		const FString FallbackPaintPngPath = GetVegetationGeneratedImagePath(TEXT("ExperimentalVegetationImagegenPaintFallback.png"));
		SavePixelsAsPng(GuidePngPath, AtlasPixels, AtlasWidth, AtlasHeight);
		SavePixelsAsPng(FallbackPaintPngPath, FallbackPaintPixels, AtlasWidth, AtlasHeight);

		UTexture2D* MaskTexture = EnsureTextureAsset(VegetationMaskTextureAssetName, AtlasPixels, AtlasWidth, AtlasHeight, true);
		UTexture2D* PaintTexture = ImportImagegenPaintTextureFromCommandLineIfRequested(AtlasPixels, AtlasWidth, AtlasHeight);
		if (!PaintTexture)
		{
			PaintTexture = EnsureTextureAsset(VegetationPaintTextureAssetName, FallbackPaintPixels, AtlasWidth, AtlasHeight, false);
		}

		UMaterial* VegetationMaterial = (PaintTexture && MaskTexture) ? EnsureVegetationMaterial(PaintTexture, MaskTexture) : nullptr;
		UStaticMesh* VegetationMesh = VegetationMaterial
			? EnsureVegetationMeshAsset(VegetationMaterial, Regions, VegetationMeshAssetName, EVegetationMeshVariant::Primary)
			: nullptr;
		UStaticMesh* AlternateVegetationMesh = VegetationMaterial
			? EnsureVegetationMeshAsset(VegetationMaterial, Regions, VegetationAlternateMeshAssetName, EVegetationMeshVariant::AlternateBranches)
			: nullptr;
		UStaticMesh* FloweringWhiteVegetationMesh = VegetationMaterial
			? EnsureVegetationMeshAsset(VegetationMaterial, Regions, VegetationFloweringWhiteMeshAssetName, EVegetationMeshVariant::FloweringWhite)
			: nullptr;
		UStaticMesh* FloweringYellowVegetationMesh = VegetationMaterial
			? EnsureVegetationMeshAsset(VegetationMaterial, Regions, VegetationFloweringYellowMeshAssetName, EVegetationMeshVariant::FloweringYellow)
			: nullptr;
		UStaticMesh* FloweringPinkVegetationMesh = VegetationMaterial
			? EnsureVegetationMeshAsset(VegetationMaterial, Regions, VegetationFloweringPinkMeshAssetName, EVegetationMeshVariant::FloweringPink)
			: nullptr;

		const bool bSucceeded =
			MaskTexture &&
			PaintTexture &&
			VegetationMaterial &&
			VegetationMesh &&
			AlternateVegetationMesh &&
			FloweringWhiteVegetationMesh &&
			FloweringYellowVegetationMesh &&
			FloweringPinkVegetationMesh;
		if (bSucceeded)
		{
			UE_LOG(
				LogTunaSweeperExperimentalVegetation,
				Log,
				TEXT("Experimental branching vegetation asset generation succeeded. Guide=%s Paint=%s Mesh=%s AlternateMesh=%s FloweringWhiteMesh=%s FloweringYellowMesh=%s FloweringPinkMesh=%s GuidePNG=%s"),
				*MaskTexture->GetPathName(),
				*PaintTexture->GetPathName(),
				*VegetationMesh->GetPathName(),
				*AlternateVegetationMesh->GetPathName(),
				*FloweringWhiteVegetationMesh->GetPathName(),
				*FloweringYellowVegetationMesh->GetPathName(),
				*FloweringPinkVegetationMesh->GetPathName(),
				*GuidePngPath);
		}
		else
		{
			UE_LOG(
				LogTunaSweeperExperimentalVegetation,
				Error,
				TEXT("Experimental branching vegetation asset generation failed. Guide=%s Paint=%s Mesh=%s AlternateMesh=%s FloweringWhiteMesh=%s FloweringYellowMesh=%s FloweringPinkMesh=%s"),
				MaskTexture ? *MaskTexture->GetPathName() : TEXT("<none>"),
				PaintTexture ? *PaintTexture->GetPathName() : TEXT("<none>"),
				VegetationMesh ? *VegetationMesh->GetPathName() : TEXT("<none>"),
				AlternateVegetationMesh ? *AlternateVegetationMesh->GetPathName() : TEXT("<none>"),
				FloweringWhiteVegetationMesh ? *FloweringWhiteVegetationMesh->GetPathName() : TEXT("<none>"),
				FloweringYellowVegetationMesh ? *FloweringYellowVegetationMesh->GetPathName() : TEXT("<none>"),
				FloweringPinkVegetationMesh ? *FloweringPinkVegetationMesh->GetPathName() : TEXT("<none>"));
		}
		return bSucceeded;
	}
}
