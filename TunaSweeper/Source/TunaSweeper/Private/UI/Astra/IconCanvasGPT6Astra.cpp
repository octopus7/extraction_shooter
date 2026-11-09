// Authored by gpt-6-astra; reasoning effort: medium.
#include "UI/Astra/IconCanvasGPT6Astra.h"
#include "IconEditor/IconStorageSubsystem.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
void UIconCanvasGPT6Astra::SetPixels(const TArray<uint8> &InPixels)
{
    Pixels = InPixels;
    InvalidateLayoutAndVolatility();
}
int32 UIconCanvasGPT6Astra::NativePaint(const FPaintArgs &Args, const FGeometry &Geo, const FSlateRect &Cull,
                                        FSlateWindowElementList &Out, int32 Layer, const FWidgetStyle &Style,
                                        bool Enabled) const
{
    Layer = Super::NativePaint(Args, Geo, Cull, Out, Layer, Style, Enabled);
    const FVector2D Cell = Geo.GetLocalSize() / static_cast<double>(UIconStorageSubsystem::Width);
    const FSlateBrush *Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
    for (int32 Y = 0; Y < UIconStorageSubsystem::Height; ++Y)
        for (int32 X = 0; X < UIconStorageSubsystem::Width; ++X)
        {
            const int32 I = (Y * UIconStorageSubsystem::Width + X) * 3;
            const FColor Color = Pixels.Num() == UIconStorageSubsystem::ByteCount
                                     ? FColor(Pixels[I], Pixels[I + 1], Pixels[I + 2])
                                     : FColor::Black;
            FSlateDrawElement::MakeBox(
                Out, Layer + 1, Geo.ToPaintGeometry(Cell, FSlateLayoutTransform(FVector2D(X * Cell.X, Y * Cell.Y))),
                Brush, ESlateDrawEffect::None, FLinearColor(Color));
        }
    if (bShowGrid)
        for (int32 I = 0; I <= UIconStorageSubsystem::Width; ++I)
        {
            TArray<FVector2D> Points;
            Points.Add(FVector2D(I * Cell.X, 0));
            Points.Add(FVector2D(I * Cell.X, Geo.GetLocalSize().Y));
            FSlateDrawElement::MakeLines(Out, Layer + 2, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None,
                                         FLinearColor(0.35f, 0.45f, 0.5f, 0.32f), false, 1.0f);
            Points[0] = FVector2D(0, I * Cell.Y);
            Points[1] = FVector2D(Geo.GetLocalSize().X, I * Cell.Y);
            FSlateDrawElement::MakeLines(Out, Layer + 2, Geo.ToPaintGeometry(), Points, ESlateDrawEffect::None,
                                         FLinearColor(0.35f, 0.45f, 0.5f, 0.32f), false, 1.0f);
        }
    return Layer + 2;
}
bool UIconCanvasGPT6Astra::GetPixel(const FGeometry &G, const FPointerEvent &E, FIntPoint &P) const
{
    const FVector2D V = G.AbsoluteToLocal(E.GetScreenSpacePosition()), S = G.GetLocalSize();
    if (S.X <= 0 || S.Y <= 0 || V.X < 0 || V.Y < 0 || V.X >= S.X || V.Y >= S.Y)
        return false;
    P = FIntPoint(
        FMath::Clamp(FMath::FloorToInt(V.X * UIconStorageSubsystem::Width / S.X), 0, UIconStorageSubsystem::Width - 1),
        FMath::Clamp(FMath::FloorToInt(V.Y * UIconStorageSubsystem::Height / S.Y), 0,
                     UIconStorageSubsystem::Width - 1));
    return true;
}
void UIconCanvasGPT6Astra::StrokeTo(FIntPoint P)
{
    if (LastPixel.X < 0 || bSamplingStroke)
    {
        OnPixelInput.ExecuteIfBound(P.X, P.Y, bSamplingStroke);
        LastPixel = P;
        return;
    }
    int32 X = LastPixel.X, Y = LastPixel.Y, DX = FMath::Abs(P.X - X), DY = -FMath::Abs(P.Y - Y), SX = X < P.X ? 1 : -1,
          SY = Y < P.Y ? 1 : -1, Err = DX + DY;
    while (true)
    {
        OnPixelInput.ExecuteIfBound(X, Y, false);
        if (X == P.X && Y == P.Y)
            break;
        const int32 E = 2 * Err;
        if (E >= DY)
        {
            Err += DY;
            X += SX;
        }
        if (E <= DX)
        {
            Err += DX;
            Y += SY;
        }
    }
    LastPixel = P;
}
FReply UIconCanvasGPT6Astra::NativeOnMouseButtonDown(const FGeometry &G, const FPointerEvent &E)
{
    FIntPoint P = FIntPoint::ZeroValue;
    if (!bInteractive ||
        (E.GetEffectingButton() != EKeys::LeftMouseButton && E.GetEffectingButton() != EKeys::RightMouseButton) ||
        !GetPixel(G, E, P))
        return FReply::Unhandled();
    bDrawing = true;
    bSamplingStroke = bEyedropper || E.GetEffectingButton() == EKeys::RightMouseButton;
    LastPixel = FIntPoint(-1, -1);
    StrokeTo(P);
    return FReply::Handled().CaptureMouse(TakeWidget());
}
FReply UIconCanvasGPT6Astra::NativeOnMouseMove(const FGeometry &G, const FPointerEvent &E)
{
    if (!bDrawing)
        return FReply::Unhandled();
    FIntPoint P = FIntPoint::ZeroValue;
    if (GetPixel(G, E, P))
        StrokeTo(P);
    else
        LastPixel = FIntPoint(-1, -1);
    return FReply::Handled();
}
FReply UIconCanvasGPT6Astra::NativeOnMouseButtonUp(const FGeometry &, const FPointerEvent &E)
{
    if (bDrawing &&
        (E.GetEffectingButton() == EKeys::LeftMouseButton || E.GetEffectingButton() == EKeys::RightMouseButton))
    {
        bDrawing = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}
void UIconCanvasGPT6Astra::NativeOnMouseCaptureLost(const FCaptureLostEvent &E)
{
    bDrawing = false;
    LastPixel = FIntPoint(-1, -1);
    Super::NativeOnMouseCaptureLost(E);
}
