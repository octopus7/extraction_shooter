#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperMapDefinition.generated.h"

class UTexture2D;
class UWorld;

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperMapDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentDefinitionVersion = 1;
	static constexpr int32 FixedTextureResolution = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 DefinitionVersion = CurrentDefinitionVersion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName MapId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UWorld> World;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection")
	FVector CaptureCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection", meta = (ClampMin = "1.0"))
	FVector2D CaptureWorldSize = FVector2D(6000.0, 6000.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projection")
	float CaptureYawDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	FIntPoint TextureSize = FIntPoint(FixedTextureResolution, FixedTextureResolution);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	FIntPoint ContentPixelMin = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	FIntPoint ContentPixelSize = FIntPoint(FixedTextureResolution, FixedTextureResolution);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map")
	FVector2D WorldLocationToContentUV(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map")
	FVector ContentUVToWorldLocation(const FVector2D& ContentUV, float WorldZ = 0.0f) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map")
	FVector2D ContentUVToTextureUV(const FVector2D& ContentUV) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Map")
	bool TextureUVToContentUV(const FVector2D& TextureUV, FVector2D& OutContentUV) const;

	bool MatchesWorld(const UWorld* InWorld) const;
	FIntRect GetContentPixelRect() const;
	static FIntRect CalculateCenteredContentRect(const FVector2D& InCaptureWorldSize);
};

UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperMapRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Maps")
	TArray<TObjectPtr<UTunaSweeperMapDefinition>> Definitions;

	UTunaSweeperMapDefinition* FindDefinitionForWorld(const UWorld* InWorld) const;
};
