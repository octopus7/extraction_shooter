#if WITH_DEV_AUTOMATION_TESTS
#include "UI/TunaSweeperDeliveryIndicator.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "RHI.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif
#include "Slate/WidgetRenderer.h"
#include "Widgets/SLeafWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    class SDeliveryIndicatorPreview : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SDeliveryIndicatorPreview) {} SLATE_END_ARGS()
        void Construct(const FArguments&) {}
        UTexture2D* Icon = nullptr;
        UTexture2D* OtherIcon = nullptr;
        float Seconds = 0;
        virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280, 720); }
        virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
            FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
        {
            // The actual game renderer, with deterministic projected destinations for visual QA.
            int32 Index = 0;
            for (const FVector2D Position : {FVector2D(2500,360), FVector2D(-1000,120),
                FVector2D(1000,-600), FVector2D(1000,1600), FVector2D(640,360)})
            {
                TunaSweeperDeliveryIndicator::FPlacement Placement;
                if (TunaSweeperDeliveryIndicator::ComputePlacement(Geometry.GetLocalSize(), Position, true, Seconds, Placement))
                    TunaSweeperDeliveryIndicator::DrawMarker(Placement, Index++ % 2 == 0 ? Icon : OtherIcon, Geometry, Elements, Layer);
            }
            return Layer;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperDeliveryIndicatorPaintTest,
    "TunaSweeper.UI.DeliveryIndicator.Paint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperDeliveryIndicatorPaintTest::RunTest(const FString& Parameters)
{
    if (GUsingNullRHI) { AddWarning(TEXT("Delivery marker render test requires an RHI run.")); return true; }
    TStrongObjectPtr<UTexture2D> Icon(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Icons/T_UIIcon_ToadJar.T_UIIcon_ToadJar")));
    TStrongObjectPtr<UTexture2D> OtherIcon(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Icons/T_UIIcon_ModelBoat.T_UIIcon_ModelBoat")));
    if (!TestNotNull(TEXT("Parcel icon loads"), Icon.Get()) || !TestNotNull(TEXT("Other parcel icon loads"), OtherIcon.Get())) return false;
#if WITH_EDITOR
    FAssetCompilingManager::Get().FinishAllCompilation();
#endif
    Icon->WaitForPendingInitOrStreaming();
    OtherIcon->WaitForPendingInitOrStreaming();
    auto Preview = SNew(SDeliveryIndicatorPreview);
    Preview->Icon = Icon.Get();
    Preview->OtherIcon = OtherIcon.Get();
    FWidgetRenderer Renderer(true);
    const FString Output = FPaths::ProjectSavedDir() / TEXT("Tests/DeliveryIndicator");
    IFileManager::Get().MakeDirectory(*Output, true);
    for (int32 Frame = 0; Frame < 2; ++Frame)
    {
        Preview->Seconds = Frame * 0.24f;
        Preview->Invalidate(EInvalidateWidgetReason::Paint);
        TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Preview, FVector2D(1280,720)));
        FlushRenderingCommands();
        if (!TestNotNull(TEXT("Marker render target"), Target.Get())) return false;
#if WITH_EDITOR
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
#endif
        FlushRenderingCommands();
        Preview->Invalidate(EInvalidateWidgetReason::Paint);
        Renderer.DrawWidget(Target.Get(), Preview, FVector2D(1280,720), 0.f);
        FlushRenderingCommands();
        TArray<FColor> Pixels;
        Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        int32 PaintedPixels = 0;
        bool bClearBorder = true;
        for (int32 Y = 0; Y < 720; ++Y)
        for (int32 X = 0; X < 1280; ++X)
        {
            const bool bPainted = Pixels[Y * 1280 + X].A > 32;
            if (bPainted) ++PaintedPixels;
            if ((X < 32 || Y < 32 || X >= 1248 || Y >= 688) && bPainted) bClearBorder = false;
        }
        TestTrue(TEXT("Markers are actually rendered"), PaintedPixels > 1000);
        TSet<uint32> IconColors;
        for (int32 Y = 344; Y < 376; ++Y)
        for (int32 X = 624; X < 656; ++X) IconColors.Add(Pixels[Y * 1280 + X].ToPackedARGB());
        TestTrue(TEXT("Item texture is visible inside the badge"), IconColors.Num() > 20);
        TestTrue(TEXT("Arrows and icons keep clear of physical border"), bClearBorder);
        FImage Image;
        if (FImageUtils::GetRenderTargetImage(Target.Get(), Image))
            TestTrue(TEXT("Save render evidence"), FImageUtils::SaveImageByExtension(
                *(Output / FString::Printf(TEXT("marker_%d.png"), Frame)), Image));
    }
    return true;
}
#endif
