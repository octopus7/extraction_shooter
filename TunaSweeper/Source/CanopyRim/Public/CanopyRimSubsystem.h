#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CanopyRimSubsystem.generated.h"

class FCanopyRimViewExtension;

/** Shared visible-silhouette renderer. No per-tree capture or render target. */
UCLASS()
class CANOPYRIM_API UCanopyRimSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UCanopyRimSubsystem();
    static constexpr int32 FirstId = 16; // 1-3 belong to placement / cover outlines.
    static constexpr int32 LastId = 255;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    uint8 UpdateTree(UObject* Owner, FLinearColor Color, float Strength, float WidthPixels, float Brightness);
    void RemoveTree(UObject* Owner);
    static FIntPoint MaskExtent(FIntPoint ViewSize);
private:
    void Publish();
    TWeakObjectPtr<UObject> Owners[256];
    FVector4f Styles[512]; // color/strength followed by width, indexed by stencil ID.
    TSharedPtr<FCanopyRimViewExtension, ESPMode::ThreadSafe> Extension;
};
