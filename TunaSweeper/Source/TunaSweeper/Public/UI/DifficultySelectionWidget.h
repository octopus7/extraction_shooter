#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DifficultySelectionWidget.generated.h"

class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Authored by gpt-6-astra, reasoning effort medium (session metadata verified).
// All hierarchy/layout is serialized in WBP_DifficultySelection, never built here.
UCLASS()
class TUNASWEEPER_API UDifficultySelectionWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    // Culture + string key, e.g. ko.ui.difficulty.select_title.
    UPROPERTY(EditDefaultsOnly, Category="Difficulty|Localization")
    TMap<FName, TObjectPtr<UTexture2D>> LabelTextures;

    UPROPERTY(EditDefaultsOnly, Category="Difficulty|Appearance")
    TObjectPtr<UMaterialInterface> LabelMaterial;

    void Present(int32 Stage, bool bBusy, bool bAdjustment);

protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
    friend class FDifficultySelectionVisualTest;
    void SetLabel(const TCHAR* ImageName, FName Key, const FString& Culture, bool bWhite, float MaxWidth, float MaxHeight);
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> LabelInstances;
    float DriftTime = 0.f;
};
