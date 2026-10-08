#pragma once

#include "SceneViewExtension.h"
#include "ScreenPass.h"

class FCanopyRimViewExtension final : public FWorldSceneViewExtension
{
public:
    FCanopyRimViewExtension(const FAutoRegister& AutoRegister, UWorld* World)
        : FWorldSceneViewExtension(AutoRegister, World) {}
    void SetStyles(TArray<FVector4f> InStyles);
    virtual void SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& View,
        FPostProcessingPassDelegateArray& Callbacks, bool bIsPassEnabled) override;
private:
    FScreenPassTexture Render(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
    TArray<FVector4f> Styles; // render thread only; contains values, never UObject pointers.
};
