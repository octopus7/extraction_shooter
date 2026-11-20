#include "ForestScatterMask.h"

#if WITH_EDITOR
#include "Components/DecalComponent.h"
#include "Engine/DecalActor.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "ImageCore.h"
#include "Materials/MaterialInstance.h"

namespace
{
	constexpr TCHAR LayoutMaterialPath[] = TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/M_LocalLayoutPreview.M_LocalLayoutPreview");
	constexpr TCHAR ForestGuidePath[] = TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/MI_Layout_Forest.MI_Layout_Forest");
	const FName PreviewTextureName(TEXT("PreviewTexture"));
	const FName PreviewOpacityName(TEXT("PreviewOpacity"));

	bool IsPlantingGuide(const UDecalComponent& Decal, bool bExplicitSelection)
	{
		UMaterialInterface* GuideMaterial = Decal.GetDecalMaterial();
		bool bIsForestGuide = false;
		for (UMaterialInterface* Material = GuideMaterial; Material;)
		{
			const FString Path = Material->GetPathName();
			if (Path == LayoutMaterialPath)
			{
				// RaidForest hides its sketch in the viewport but still uses the texture for planting.
				if (bIsForestGuide)
				{
					return true;
				}
				float Opacity = 0;
				return bExplicitSelection && Decal.GetVisibleFlag()
					&& GuideMaterial->GetScalarParameterValue(PreviewOpacityName, Opacity) && Opacity > 0;
			}
			bIsForestGuide |= Path == ForestGuidePath;
			const UMaterialInstance* Instance = Cast<UMaterialInstance>(Material);
			Material = Instance ? Instance->Parent.Get() : nullptr;
		}
		return false;
	}

	bool IsGrassColor(const FColor Color)
	{
		// Forest guide sRGB swatches include grass (123,175,110), road (222,193,110) and water (88,165,201).
		const int32 OverRed = int32(Color.G) - Color.R;
		const int32 OverBlue = int32(Color.G) - Color.B;
		return Color.A >= 128 && OverRed >= 16 && OverBlue >= 16
			&& FMath::Max(OverRed, OverBlue) * 5 >= Color.G
			&& OverRed * 3 >= OverBlue && OverBlue * 2 >= OverRed;
	}

	enum class EGrayMark : uint8 { Ordinary, UnvisitedShade, UnvisitedGray, Visited, GrassMarker, ExcludedGray };

	EGrayMark IdentifyGrayPigment(const FColor Color)
	{
		const int32 Chroma = FMath::Max3(Color.R, Color.G, Color.B) - FMath::Min3(Color.R, Color.G, Color.B);
		// The guide's cliffs alternate gray (129,141,131) and cool shadow (69,104,75). Include their blends,
		// and the soft transition from a neutral (139,139,139) marker to the surrounding painted green.
		const bool bMutedShadow = Color.G <= 151 && Color.G >= Color.R && Color.G >= Color.B && Chroma <= 48;
		if (Color.A < 128 || FMath::Abs(int32(Color.R) - Color.B) > 14 || (Chroma > 36 && !bMutedShadow))
		{
			return EGrayMark::Ordinary;
		}
		return Chroma <= 16 && Color.G >= 76 && Color.G <= 221 ? EGrayMark::UnvisitedGray : EGrayMark::UnvisitedShade;
	}

	bool IsUnvisitedGray(EGrayMark Mark)
	{
		return Mark == EGrayMark::UnvisitedShade || Mark == EGrayMark::UnvisitedGray;
	}

	int32 AddressTexel(int32 Index, int32 Size, TextureAddress Mode)
	{
		// UVs are already clipped to [0,1]. Only the two taps just outside the texture need addressing.
		if (Mode == TA_Wrap)
		{
			return (Index + Size) % Size;
		}
		return FMath::Clamp(Index, 0, Size - 1);
	}
}

struct FForestScatterMask::FGuideImage
{
	FImage Pixels;
	TArray<EGrayMark> GrayMarks;
	TextureAddress AddressX = TA_Clamp;
	TextureAddress AddressY = TA_Clamp;
	bool bNearest = false;

	bool Read(UTexture2D* Texture)
	{
		if (!Texture || !Texture->Source.IsValid() || !Texture->Source.GetMipImage(Pixels, 0)
			|| Pixels.SizeX <= 0 || Pixels.SizeY <= 0 || Pixels.NumSlices != 1
			|| int64(Pixels.SizeX) * Pixels.SizeY > MAX_int32)
		{
			return false;
		}
		Pixels.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		AddressX = Texture->AddressX;
		AddressY = Texture->AddressY;
		bNearest = Texture->Filter == TF_Nearest;
		ClassifyGrayAnnotations();
		return true;
	}

	void ClassifyGrayAnnotations()
	{
		const TArrayView64<FColor> Colors = Pixels.AsBGRA8();
		GrayMarks.SetNumUninitialized(int32(Colors.Num()));
		for (int32 Index = 0; Index < GrayMarks.Num(); ++Index)
		{
			GrayMarks[Index] = IdentifyGrayPigment(Colors[Index]);
		}
		// The five marks span 13–20 texels on the 400×400 forest guide. Scale the cut-off for reimports.
		const int32 MaxDiameter = FMath::CeilToInt(0.055 * FMath::Min(Pixels.SizeX, Pixels.SizeY));
		TArray<int32> Component;
		for (int32 Index = 0; Index < GrayMarks.Num(); ++Index)
		{
			if (IsUnvisitedGray(GrayMarks[Index]))
			{
				ClassifyGrayComponent(Index, MaxDiameter, Colors, Component);
			}
		}
	}

	void ClassifyGrayComponent(int32 Start, int32 MaxDiameter, TArrayView64<FColor> Colors, TArray<int32>& Component)
	{
		Component.Reset();
		int32 GrayCoreCount = 0;
		auto Enqueue = [&](int32 Index)
		{
			GrayCoreCount += GrayMarks[Index] == EGrayMark::UnvisitedGray;
			GrayMarks[Index] = EGrayMark::Visited;
			Component.Add(Index);
		};
		Enqueue(Start);
		FIntPoint Min(Start % Pixels.SizeX, Start / Pixels.SizeX), Max = Min;
		bool bSurroundedByGreen = true;
		FColor BorderGrass = FColor::Black;
		for (int32 Head = 0; Head < Component.Num(); ++Head)
		{
			const int32 X = Component[Head] % Pixels.SizeX, Y = Component[Head] / Pixels.SizeX;
			Min = Min.ComponentMin(FIntPoint(X, Y));
			Max = Max.ComponentMax(FIntPoint(X, Y));
			if (X == 0 || Y == 0 || X == Pixels.SizeX - 1 || Y == Pixels.SizeY - 1)
			{
				bSurroundedByGreen = false;
			}
			for (int32 NY = FMath::Max(0, Y - 1); NY <= FMath::Min(Pixels.SizeY - 1, Y + 1); ++NY)
			{
				for (int32 NX = FMath::Max(0, X - 1); NX <= FMath::Min(Pixels.SizeX - 1, X + 1); ++NX)
				{
					const int32 Neighbor = NY * Pixels.SizeX + NX;
					const EGrayMark Mark = GrayMarks[Neighbor];
					if (IsUnvisitedGray(Mark))
					{
						Enqueue(Neighbor);
					}
					else if (Mark == EGrayMark::Ordinary && bSurroundedByGreen)
					{
						BorderGrass = Colors[Neighbor];
						bSurroundedByGreen = IsGrassColor(BorderGrass);
					}
				}
			}
		}

		const int32 Width = Max.X - Min.X + 1, Height = Max.Y - Min.Y + 1;
		const int32 LongSide = FMath::Max(Width, Height), ShortSide = FMath::Min(Width, Height);
		const double Fill = double(Component.Num()) / (double(Width) * Height);
		const bool bRoundMarker = LongSide <= MaxDiameter && ShortSide >= 3 && ShortSide >= 0.70 * LongSide
			&& Fill >= 0.60 && Fill <= 0.92 && int64(GrayCoreCount) * 2 >= Component.Num();
		const bool bRestoreGrass = bRoundMarker && bSurroundedByGreen;
		// The forest's striped cliffs contain 9–50% neutral gray. Keep separate cool green patches as grass.
		if (!bRestoreGrass && int64(GrayCoreCount) * 16 < Component.Num())
		{
			return;
		}
		for (int32 Index : Component)
		{
			GrayMarks[Index] = bRestoreGrass ? EGrayMark::GrassMarker : EGrayMark::ExcludedGray;
			if (bRestoreGrass)
			{
				Colors[Index] = BorderGrass;
			}
		}
	}

	int32 TexelIndex(int32 X, int32 Y) const
	{
		return AddressTexel(Y, Pixels.SizeY, AddressY) * Pixels.SizeX + AddressTexel(X, Pixels.SizeX, AddressX);
	}

	bool IsGrass(double U, double V) const
	{
		const double X = U * Pixels.SizeX;
		const double Y = V * Pixels.SizeY;
		const int32 Nearest = TexelIndex(FMath::FloorToInt(X), FMath::FloorToInt(Y));
		const EGrayMark Mark = GrayMarks[Nearest];
		if (Mark == EGrayMark::GrassMarker || Mark == EGrayMark::ExcludedGray)
		{
			return Mark == EGrayMark::GrassMarker;
		}
		const TArrayView64<const FColor> Colors = Pixels.AsBGRA8();
		if (bNearest)
		{
			return IsGrassColor(Colors[Nearest]);
		}
		// Texture samples are centered halfway through a texel. Filter sRGB textures in linear light.
		const int32 Left = FMath::FloorToInt(X - 0.5);
		const int32 Top = FMath::FloorToInt(Y - 0.5);
		const float Fx = float(X - 0.5 - Left);
		const float Fy = float(Y - 0.5 - Top);
		const FLinearColor Color = FMath::BiLerp(
			FLinearColor(Colors[TexelIndex(Left, Top)]), FLinearColor(Colors[TexelIndex(Left + 1, Top)]),
			FLinearColor(Colors[TexelIndex(Left, Top + 1)]), FLinearColor(Colors[TexelIndex(Left + 1, Top + 1)]), Fx, Fy);
		return IsGrassColor(Color.ToFColorSRGB());
	}
};
#endif

bool FForestScatterMask::Build(UWorld* World, const TArray<TObjectPtr<ADecalActor>>& ExplicitDecals)
{
#if WITH_EDITOR
	check(IsInGameThread());
	Projections.Reset();
	if (!World)
	{
		return false;
	}

	const bool bExplicitSelection = !ExplicitDecals.IsEmpty();
	TArray<ADecalActor*> Candidates;
	auto AddCandidate = [&](ADecalActor* Actor)
	{
		if (IsValid(Actor) && Actor->GetWorld() == World)
		{
			const UDecalComponent* Decal = Actor->GetDecal();
			if (Decal && IsPlantingGuide(*Decal, bExplicitSelection))
			{
				Candidates.AddUnique(Actor);
			}
		}
	};
	if (bExplicitSelection)
	{
		for (ADecalActor* Actor : ExplicitDecals)
		{
			AddCandidate(Actor);
		}
	}
	else
	{
		for (TActorIterator<ADecalActor> It(World); It; ++It)
		{
			AddCandidate(*It);
		}
	}
	Candidates.Sort([](const ADecalActor& A, const ADecalActor& B)
	{
		const int32 OrderA = A.GetDecal()->SortOrder;
		const int32 OrderB = B.GetDecal()->SortOrder;
		return OrderA != OrderB ? OrderA > OrderB : A.GetPathName() < B.GetPathName();
	});

	TMap<UTexture2D*, TSharedPtr<FGuideImage>> Images;
	bool bHasReadableGuide = false;
	for (const ADecalActor* Actor : Candidates)
	{
		const UDecalComponent* Decal = Actor->GetDecal();
		const FTransform Transform = Decal->GetTransformIncludingDecalSize();
		if (Transform.ContainsNaN() || Transform.GetScale3D().GetAbsMin() <= UE_SMALL_NUMBER)
		{
			continue;
		}
		UMaterialInterface* Material = Decal->GetDecalMaterial();
		UTexture* TextureValue = nullptr;
		Material->GetTextureParameterValue(PreviewTextureName, TextureValue);
		UTexture2D* Texture = Cast<UTexture2D>(TextureValue);
		const TSharedPtr<FGuideImage>* Cached = Images.Find(Texture);
		if (!Cached)
		{
			TSharedPtr<FGuideImage> Image = MakeShared<FGuideImage>();
			if (!Image->Read(Texture))
			{
				Image.Reset();
			}
			Cached = &Images.Add(Texture, MoveTemp(Image));
		}
		// An unreadable top guide still covers its projection; its underlying green cannot be assumed.
		Projections.Add({Transform, *Cached});
		bHasReadableGuide |= Cached->IsValid();
	}
	return bHasReadableGuide;
#else
	return false;
#endif
}

bool FForestScatterMask::IsGreen(const FVector& WorldPosition) const
{
#if WITH_EDITOR
	if (!WorldPosition.ContainsNaN())
	{
		for (const FProjection& Projection : Projections)
		{
			const FVector Local = Projection.DecalToWorld.InverseTransformPosition(WorldPosition);
			if (FMath::Abs(Local.X) >= 1.0 || FMath::Abs(Local.Y) > 1.0 || FMath::Abs(Local.Z) > 1.0)
			{
				continue;
			}
			// UE 5.7 DeferredDecal.usf maps normalized local Z to U and local Y to V.
			return Projection.Image && Projection.Image->IsGrass(Local.Z * 0.5 + 0.5, Local.Y * 0.5 + 0.5);
		}
	}
#endif
	return false;
}
