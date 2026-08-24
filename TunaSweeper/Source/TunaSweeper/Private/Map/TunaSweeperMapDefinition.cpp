#include "Map/TunaSweeperMapDefinition.h"

#include "Engine/World.h"
#include "Misc/PackageName.h"

namespace
{
	FString GetWorldPackageName(const UWorld* World)
	{
		return World && World->GetOutermost() ? World->GetOutermost()->GetName() : FString();
	}
}

bool UTunaSweeperMapDefinition::IsValidDefinition() const
{
	const FIntRect ContentRect = GetContentPixelRect();
	return DefinitionVersion > 0 &&
		!MapId.IsNone() &&
		!Texture.IsNull() &&
		CaptureWorldSize.X > 0.0 &&
		CaptureWorldSize.Y > 0.0 &&
		TextureSize == FIntPoint(FixedTextureResolution, FixedTextureResolution) &&
		ContentRect.Min.X >= 0 &&
		ContentRect.Min.Y >= 0 &&
		ContentRect.Max.X <= TextureSize.X &&
		ContentRect.Max.Y <= TextureSize.Y &&
		ContentRect.Width() > 0 &&
		ContentRect.Height() > 0;
}

FVector2D UTunaSweeperMapDefinition::WorldLocationToContentUV(const FVector& WorldLocation) const
{
	const FTransform CaptureTransform(FRotator(0.0f, CaptureYawDegrees, 0.0f), CaptureCenter);
	const FVector LocalPosition = CaptureTransform.InverseTransformPosition(WorldLocation);
	const FVector2D SafeSize(
		FMath::Max(1.0, CaptureWorldSize.X),
		FMath::Max(1.0, CaptureWorldSize.Y));

	return FVector2D(
		0.5 + LocalPosition.Y / SafeSize.Y,
		0.5 - LocalPosition.X / SafeSize.X);
}

FVector UTunaSweeperMapDefinition::ContentUVToWorldLocation(const FVector2D& ContentUV, float WorldZ) const
{
	const FVector LocalPosition(
		(0.5 - ContentUV.Y) * CaptureWorldSize.X,
		(ContentUV.X - 0.5) * CaptureWorldSize.Y,
		0.0f);
	const FTransform CaptureTransform(FRotator(0.0f, CaptureYawDegrees, 0.0f), CaptureCenter);
	FVector WorldPosition = CaptureTransform.TransformPosition(LocalPosition);
	WorldPosition.Z = WorldZ;
	return WorldPosition;
}

FVector2D UTunaSweeperMapDefinition::ContentUVToTextureUV(const FVector2D& ContentUV) const
{
	const FVector2D SafeTextureSize(
		FMath::Max(1, TextureSize.X),
		FMath::Max(1, TextureSize.Y));
	return FVector2D(
		(ContentPixelMin.X + ContentUV.X * ContentPixelSize.X) / SafeTextureSize.X,
		(ContentPixelMin.Y + ContentUV.Y * ContentPixelSize.Y) / SafeTextureSize.Y);
}

bool UTunaSweeperMapDefinition::TextureUVToContentUV(const FVector2D& TextureUV, FVector2D& OutContentUV) const
{
	const FIntRect ContentRect = GetContentPixelRect();
	if (TextureSize.X <= 0 || TextureSize.Y <= 0 ||
		ContentRect.Min.X < 0 || ContentRect.Min.Y < 0 ||
		ContentRect.Max.X > TextureSize.X || ContentRect.Max.Y > TextureSize.Y ||
		ContentRect.Width() <= 0 || ContentRect.Height() <= 0)
	{
		return false;
	}

	const FVector2D PixelPosition(TextureUV.X * TextureSize.X, TextureUV.Y * TextureSize.Y);
	OutContentUV = FVector2D(
		(PixelPosition.X - ContentPixelMin.X) / ContentPixelSize.X,
		(PixelPosition.Y - ContentPixelMin.Y) / ContentPixelSize.Y);
	const bool bInsideContent =
		OutContentUV.X >= 0.0 && OutContentUV.X <= 1.0 &&
		OutContentUV.Y >= 0.0 && OutContentUV.Y <= 1.0;
	OutContentUV.X = FMath::Clamp(OutContentUV.X, 0.0, 1.0);
	OutContentUV.Y = FMath::Clamp(OutContentUV.Y, 0.0, 1.0);
	return bInsideContent;
}

bool UTunaSweeperMapDefinition::MatchesWorld(const UWorld* InWorld) const
{
	const FString InWorldPackageName = UWorld::RemovePIEPrefix(GetWorldPackageName(InWorld));
	if (InWorldPackageName.IsEmpty())
	{
		return false;
	}

	const FString DefinitionWorldPackageName = UWorld::RemovePIEPrefix(World.ToSoftObjectPath().GetLongPackageName());
	if (!DefinitionWorldPackageName.IsEmpty() && DefinitionWorldPackageName == InWorldPackageName)
	{
		return true;
	}

	return MapId == FName(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(InWorldPackageName)));
}

FIntRect UTunaSweeperMapDefinition::GetContentPixelRect() const
{
	return FIntRect(ContentPixelMin, ContentPixelMin + ContentPixelSize);
}

FIntRect UTunaSweeperMapDefinition::CalculateCenteredContentRect(const FVector2D& InCaptureWorldSize)
{
	const double WorldHeight = FMath::Max(1.0, InCaptureWorldSize.X);
	const double WorldWidth = FMath::Max(1.0, InCaptureWorldSize.Y);
	const double Scale = FMath::Min(
		static_cast<double>(FixedTextureResolution) / WorldWidth,
		static_cast<double>(FixedTextureResolution) / WorldHeight);
	const int32 ContentWidth = FMath::Clamp(
		FMath::RoundToInt(WorldWidth * Scale),
		1,
		FixedTextureResolution);
	const int32 ContentHeight = FMath::Clamp(
		FMath::RoundToInt(WorldHeight * Scale),
		1,
		FixedTextureResolution);
	const FIntPoint ContentMin(
		(FixedTextureResolution - ContentWidth) / 2,
		(FixedTextureResolution - ContentHeight) / 2);
	return FIntRect(ContentMin, ContentMin + FIntPoint(ContentWidth, ContentHeight));
}

UTunaSweeperMapDefinition* UTunaSweeperMapRegistry::FindDefinitionForWorld(const UWorld* InWorld) const
{
	for (UTunaSweeperMapDefinition* Definition : Definitions)
	{
		if (Definition && Definition->IsValidDefinition() && Definition->MatchesWorld(InWorld))
		{
			return Definition;
		}
	}

	return nullptr;
}
