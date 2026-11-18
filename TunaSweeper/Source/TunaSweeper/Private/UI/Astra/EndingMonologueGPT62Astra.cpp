#include "UI/Astra/EndingMonologueGPT62Astra.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
    constexpr double BlackHold = 1.1;
    constexpr double AppearSeconds = 0.8;
    constexpr double ReadSeconds = 5.0;
}

UEndingMonologueGPT62Astra::UEndingMonologueGPT62Astra(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

void UEndingMonologueGPT62Astra::PlayLine(const FText& Line, FSimpleDelegate OnContinue)
{
    check(MonologueText);
    TunaSweeperUIFont::ApplyFont(MonologueText, MonologueText->GetFont().Size, ETunaSweeperUIFontWeight::Regular);
    MonologueText->SetText(Line);
    MonologueText->SetRenderOpacity(0.f);
    bWaitingForRetry = false;
    StartedAt = FPlatformTime::Seconds();
    ContinueAction = MoveTemp(OnContinue);
}

void UEndingMonologueGPT62Astra::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    if (bWaitingForRetry || !ContinueAction.IsBound()) return;
    const double Elapsed = FPlatformTime::Seconds() - StartedAt;
    MonologueText->SetRenderOpacity(FMath::SmoothStep(0.0, AppearSeconds, Elapsed - BlackHold));
    if (Elapsed >= BlackHold + AppearSeconds + ReadSeconds) Continue();
}

void UEndingMonologueGPT62Astra::ShowSaveRetry(const FText& Message, FSimpleDelegate OnRetry)
{
    MonologueText->SetText(Message);
    MonologueText->SetRenderOpacity(1.f);
    bWaitingForRetry = true;
    ContinueAction = MoveTemp(OnRetry);
    SetKeyboardFocus();
}

void UEndingMonologueGPT62Astra::Continue()
{
    FSimpleDelegate Action = MoveTemp(ContinueAction);
    ContinueAction.Unbind();
    Action.ExecuteIfBound();
}

FReply UEndingMonologueGPT62Astra::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bWaitingForRetry && !Event.IsRepeat()) Continue();
    return FReply::Handled();
}

FReply UEndingMonologueGPT62Astra::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (bWaitingForRetry && Event.GetEffectingButton() == EKeys::LeftMouseButton) Continue();
    return FReply::Handled();
}
