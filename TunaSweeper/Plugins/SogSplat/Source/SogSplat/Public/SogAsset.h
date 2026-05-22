#pragma once

#include "CoreMinimal.h"
#include "SogSplatTypes.h"
#include "UObject/Object.h"
#include "SogAsset.generated.h"

class UMaterialInterface;
class UTexture2D;

UCLASS(BlueprintType)
class SOGSPLAT_API USogAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FString SourceFilePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 SourceGaussianCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 ImportedSplatCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 StoredSourceSizeBytes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedSplatDataSizeBytes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedSplatDataUncompressedSizeBytes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedTransformTextureSizeBytes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedColorTextureSizeBytes = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedTextureWidth = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	int32 CachedTextureHeight = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FSogDecodeOptions DecodeOptions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG")
	FBoxSphereBounds LocalBounds = FBoxSphereBounds(EForceInit::ForceInit);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SOG|Rendering")
	TSoftObjectPtr<UMaterialInterface> DefaultMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG|Cache")
	TObjectPtr<UTexture2D> TransformTexture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG|Cache")
	TObjectPtr<UTexture2D> ColorTexture;

	UFUNCTION(BlueprintPure, Category = "SOG")
	int32 GetSplatCount() const;

	bool EnsureSplatsDecoded(FText& OutError);
	const TArray<FSogSplatInstance>& GetSplats() const;
	void SetSplats(TArray<FSogSplatInstance>&& InSplats);
	bool BuildDecodedSplatCache();
	bool TryLoadSplatsFromDecodedCache(FText& OutError);
	void SetSourceArchiveBytes(TArray<uint8>&& InSourceArchiveBytes);
	const TArray<uint8>& GetSourceArchiveBytes() const;
	void ClearSplats();

	virtual void PostLoad() override;

private:
	UPROPERTY()
	TArray<uint8> SourceArchiveBytes;

	UPROPERTY()
	TArray<uint8> CachedSplatDataBytes;

	UPROPERTY()
	TArray<uint16> CachedTransformTextureHalfData;

	UPROPERTY()
	TArray<uint8> CachedColorTextureByteData;

	UPROPERTY()
	int32 CachedSplatDataVersion = 0;

	UPROPERTY()
	int32 CachedSplatCount = 0;

	UPROPERTY()
	FVector CachedPositionMin = FVector::ZeroVector;

	UPROPERTY()
	FVector CachedPositionExtent = FVector::OneVector;

	UPROPERTY(Transient)
	TArray<FSogSplatInstance> Splats;
};
