#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "TunaSweeperLedExpressionComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLedExpressionPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression")
	FName ExpressionName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression", meta = (MultiLine = "true"))
	FString Pattern;
};

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperLedExpressionComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperLedExpressionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void OnRegister() override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	bool LoadExpressionPresetFile(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	bool RefreshExpressionPresets();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	void RebuildLedMesh();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	void SetHorizontalCurvatureDegrees(float InHorizontalCurvatureDegrees);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	bool SetExpressionByName(FName ExpressionName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	bool SetExpressionPattern(FName ExpressionName, const FString& Pattern);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	void AddOrUpdateBlueprintPreset(FName ExpressionName, const FString& Pattern);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	void ConfigureExpressionSource(const FString& InPresetFilePath, FName InDefaultExpressionName);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|LED Expression")
	void ConfigureLedAppearance(
		FLinearColor InLedColor,
		FLinearColor InOffColor,
		float InLedPitch,
		float InLedRadius);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Expression")
	bool DoesExpressionExist(FName ExpressionName) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Expression")
	FName GetCurrentExpressionName() const { return CurrentExpressionName; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|LED Expression")
	int32 GetLedCount() const { return FMath::Max(1, MatrixColumns) * FMath::Max(1, MatrixRows); }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "1", ClampMax = "256"))
	int32 MatrixColumns = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "1", ClampMax = "128"))
	int32 MatrixRows = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "0.1"))
	float LedPitch = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "0.01"))
	float LedRadius = 0.68f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "3", ClampMax = "24"))
	int32 CircleSegments = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Matrix", meta = (ClampMin = "0.0", ClampMax = "160.0", UIMin = "0.0", UIMax = "80.0"))
	float HorizontalCurvatureDegrees = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Color")
	FLinearColor LedColor = FLinearColor(1.0f, 0.78f, 0.06f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Color")
	FLinearColor OffColor = FLinearColor(0.03f, 0.03f, 0.03f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Color", meta = (ClampMin = "0.0"))
	float EmissiveIntensity = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Render", meta = (ClampMin = "0.0"))
	float ActiveLedSurfaceOffset = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Preset")
	FString PresetFilePath = TEXT("Data/LedExpressionPresets.txt");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Preset")
	FName DefaultExpressionName = TEXT("Neutral");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Preset")
	FString OnCharacters = TEXT("O*#@+Xx");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Preset")
	FString OffCharacters = TEXT(". ");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Preset")
	TArray<FTunaSweeperLedExpressionPreset> BlueprintPresets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Material")
	TSoftObjectPtr<UMaterialInterface> LedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LED Expression|Material")
	TSoftObjectPtr<UMaterialInterface> OffLedMaterial;

private:
	FString ResolvePresetFilePath() const;
	bool LoadPresetFileIntoMap();
	FString NormalizePattern(const FString& RawPattern) const;
	bool ApplyPatternToStates(const FString& Pattern);
	void RefreshVertexColors();
	void ApplyLedMaterial();
	void BuildCurvedLedVertex(float SourceY, float SourceZ, FVector& OutPosition, FVector& OutNormal, FProcMeshTangent& OutTangent) const;
	FLinearColor ResolveLedVertexColor(bool bEnabled) const;
	bool IsOnCharacter(TCHAR Character) const;

	TMap<FName, FString> ExpressionPresets;
	TArray<FVector> CachedVertices;
	TArray<FVector> CachedNormals;
	TArray<FVector2D> CachedUVs;
	TArray<FProcMeshTangent> CachedTangents;
	TArray<FLinearColor> CachedVertexColors;
	TArray<int32> CachedTriangles;
	TArray<uint8> LedStates;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicLedMaterial;
	FName CurrentExpressionName = NAME_None;
	bool bPresetFileLoaded = false;
	bool bMeshBuilt = false;
};
