#include "UI/TunaSweeperStartupLogoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "GameFramework/PlayerController.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"

namespace
{
	class SStartupVideoEdge : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStartupVideoEdge) {} SLATE_END_ARGS()
		void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool) const override
		{
			// Smooth white-to-transparent inset rings hide the video's hard rectangular edge.
			// Keep this mask fixed over both images. Only the video beneath it crossfades;
			// no inherited group opacity or overlapping translucent white backgrounds.
			constexpr int32 Rings = 24;
			const FVector2D Size = Geometry.GetLocalSize();
			const float Feather = FMath::Min(Size.X, Size.Y) * 0.07f;
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			for (int32 Ring = 0; Ring <= Rings; ++Ring)
			{
				const float T = float(Ring) / Rings;
				const float Inset = Feather * T;
				FLinearColor Color = Style.GetColorAndOpacityTint();
				Color.A *= 1.0f - T * T * (3.0f - 2.0f * T);
				const FVector2f Points[] = {
					{Inset, Inset}, {float(Size.X) - Inset, Inset},
					{float(Size.X) - Inset, float(Size.Y) - Inset}, {Inset, float(Size.Y) - Inset}};
				for (const FVector2f& Point : Points)
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
						Geometry.GetAccumulatedRenderTransform(), Point, FVector2f::ZeroVector, Color.ToFColor(true)));
				if (Ring == Rings) continue;
				for (int32 Side = 0; Side < 4; ++Side)
				{
					const SlateIndex A = Ring * 4 + Side, B = Ring * 4 + (Side + 1) % 4;
					Indices.Append({A, B, SlateIndex(A + 4), B, SlateIndex(B + 4), SlateIndex(A + 4)});
				}
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			FSlateDrawElement::MakeCustomVerts(Elements, LayerId,
				FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White), Vertices, Indices, nullptr, 0, 0);
			return LayerId;
		}
	};

	void Fill(UOverlaySlot* Slot)
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	}
	void SetTexture(UImage* Image, UObject* Texture)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetResourceObject(Texture);
		Brush.SetImageSize(FVector2D(1920.0f, 1080.0f));
		Image->SetBrush(Brush);
	}
}

TSharedRef<SWidget> UTunaSweeperStartupVideoEdge::RebuildWidget()
{
	return SNew(SStartupVideoEdge);
}

UTunaSweeperStartupLogoWidget::UTunaSweeperStartupLogoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UTunaSweeperStartupLogoWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		UImage* Background = WidgetTree->ConstructWidget<UImage>();
		Background->SetColorAndOpacity(FLinearColor::White);
		Fill(Root->AddChildToOverlay(Background));
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
		Scale->SetStretch(EStretch::ScaleToFit);
		Fill(Root->AddChildToOverlay(Scale));
		USizeBox* Frame = WidgetTree->ConstructWidget<USizeBox>();
		Frame->SetWidthOverride(1920.0f);
		Frame->SetHeightOverride(1080.0f);
		Scale->AddChild(Frame);
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>();
		Frame->AddChild(Layers);
		StillImage = WidgetTree->ConstructWidget<UImage>();
		SetTexture(StillImage, LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/UI/Title/T_DevTunaLogoWhite.T_DevTunaLogoWhite")));
		StillImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		StillImage->SetRenderScale(FVector2D(0.5f, 0.5f));
		Fill(Layers->AddChildToOverlay(StillImage));
		// The still and white background stay fully opaque underneath the video.
		USizeBox* VideoFrame = WidgetTree->ConstructWidget<USizeBox>();
		VideoFrame->SetWidthOverride(960.0f);
		VideoFrame->SetHeightOverride(540.0f);
		UOverlaySlot* VideoSlot = Layers->AddChildToOverlay(VideoFrame);
		VideoSlot->SetHorizontalAlignment(HAlign_Center);
		VideoSlot->SetVerticalAlignment(VAlign_Center);
		UOverlay* VideoComposite = WidgetTree->ConstructWidget<UOverlay>();
		VideoFrame->AddChild(VideoComposite);
		VideoImage = WidgetTree->ConstructWidget<UImage>();
		Fill(VideoComposite->AddChildToOverlay(VideoImage));
		Fill(VideoComposite->AddChildToOverlay(WidgetTree->ConstructWidget<UTunaSweeperStartupVideoEdge>()));
		BlackImage = WidgetTree->ConstructWidget<UImage>();
		BlackImage->SetColorAndOpacity(FLinearColor::Black);
		Fill(Root->AddChildToOverlay(BlackImage));
	}
	return Super::RebuildWidget();
}

void UTunaSweeperStartupLogoWidget::Start()
{
	if (bStarted || bFinished) return;
	bStarted = true;
	Flow.FadeSeconds = FMath::Max(0.01f, FadeSeconds);
	Flow.StillSeconds = FMath::Max(0.0f, StillSeconds);
	Flow.BlackSeconds = FMath::Max(0.01f, BlackSeconds);
	MediaPlayer = NewObject<UMediaPlayer>(this);
	MediaPlayer->PlayOnOpen = false;
	MediaPlayer->SetLooping(false);
	MediaPlayer->OnMediaOpened.AddDynamic(this, &ThisClass::HandleOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &ThisClass::HandleFailed);
	MediaPlayer->OnEndReached.AddDynamic(this, &ThisClass::HandleEnded);
	MediaTexture = NewObject<UMediaTexture>(this);
	// Keep the final video frame underneath the still during the crossfade.
	MediaTexture->AutoClear = false;
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();
	SetTexture(VideoImage, MediaTexture);
	MediaSound = NewObject<UMediaSoundComponent>(GetOwningPlayer());
	MediaSound->SetMediaPlayer(MediaPlayer);
	MediaSound->bIsUISound = true;
	MediaSound->RegisterComponent();
	MediaSound->Start();
	MediaSource = LoadObject<UMediaSource>(nullptr, TEXT("/Game/Movies/MS_DevTunaCut.MS_DevTunaCut"));
	UE_LOG(LogTemp, Display, TEXT("StartupLogo: opening developer movie"));
	if (!MediaSource || !MediaPlayer->OpenSource(MediaSource)) HandleFailed(FString());
}

void UTunaSweeperStartupLogoWidget::HandleOpened(FString)
{
	if (Flow.Phase != TunaSweeperStartupLogo::EPhase::Opening || bFinished) return;
	if (!MediaPlayer->Play()) { HandleFailed(FString()); return; }
	Flow.Opened();
	UE_LOG(LogTemp, Display, TEXT("StartupLogo: video fade in"));
}

void UTunaSweeperStartupLogoWidget::HandleFailed(FString)
{
	if (bFinished) return;
	Flow.EndVideo(false);
	if (MediaSound) MediaSound->Stop();
	UE_LOG(LogTemp, Warning, TEXT("StartupLogo: playback unavailable; showing still"));
}

void UTunaSweeperStartupLogoWidget::HandleEnded()
{
	if (bFinished) return;
	Flow.EndVideo(true);
	UE_LOG(LogTemp, Display, TEXT("StartupLogo: video ended; crossfading to still"));
}

void UTunaSweeperStartupLogoWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	if (!bStarted || bFinished) return;
	const auto PreviousPhase = Flow.Phase;
	Flow.Tick(DeltaTime);
	BlackImage->SetRenderOpacity(Flow.BlackOpacity);
	VideoImage->SetRenderOpacity(1.0f - Flow.StillOpacity);
	if (PreviousPhase != Flow.Phase)
	{
		UE_LOG(LogTemp, Display, TEXT("StartupLogo: phase %d"), static_cast<int32>(Flow.Phase));
	}
	if (Flow.Phase == TunaSweeperStartupLogo::EPhase::Crossfade && !Flow.VideoCompleted)
	{
		// Timeout/error also stops audio and late playback, but preserves the sampled texture.
		if (MediaPlayer) MediaPlayer->Pause();
		if (MediaSound) MediaSound->Stop();
	}
	if (Flow.Phase == TunaSweeperStartupLogo::EPhase::Done)
	{
		bFinished = true;
		Stop();
		OnFinished.ExecuteIfBound(Flow.VideoCompleted);
	}
}

void UTunaSweeperStartupLogoWidget::Stop()
{
	bStarted = false;
	if (MediaPlayer)
	{
		MediaPlayer->OnMediaOpened.RemoveAll(this);
		MediaPlayer->OnMediaOpenFailed.RemoveAll(this);
		MediaPlayer->OnEndReached.RemoveAll(this);
		MediaPlayer->Close();
	}
	if (MediaSound)
	{
		MediaSound->Stop();
		MediaSound->SetMediaPlayer(nullptr);
		MediaSound->DestroyComponent();
		MediaSound = nullptr;
	}
}

void UTunaSweeperStartupLogoWidget::NativeDestruct()
{
	Stop();
	OnFinished.Unbind();
	Super::NativeDestruct();
}

// Consume the transition input so it cannot also activate a title-menu action.
FReply UTunaSweeperStartupLogoWidget::NativeOnKeyDown(const FGeometry&, const FKeyEvent&)
{
	Flow.SkipStill(); return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnKeyUp(const FGeometry&, const FKeyEvent&)
{
	Flow.SkipStill(); return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&)
{
	Flow.SkipStill(); return FReply::Handled().SetUserFocus(TakeWidget());
}
FReply UTunaSweeperStartupLogoWidget::NativeOnMouseButtonUp(const FGeometry&, const FPointerEvent&)
{
	Flow.SkipStill(); return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnMouseMove(const FGeometry&, const FPointerEvent& Event)
{
	if (!Event.GetCursorDelta().IsNearlyZero()) Flow.SkipStill();
	return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnMouseWheel(const FGeometry&, const FPointerEvent&)
{
	Flow.SkipStill(); return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event)
{
	// Ignore resting controller noise while accepting intentional stick/trigger input.
	if (FMath::Abs(Event.GetAnalogValue()) > 0.2f) Flow.SkipStill();
	return FReply::Handled();
}
FReply UTunaSweeperStartupLogoWidget::NativeOnTouchStarted(const FGeometry&, const FPointerEvent&)
{
	Flow.SkipStill(); return FReply::Handled();
}
