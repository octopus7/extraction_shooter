#include "UI/TunaSweeperStartupLogoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaSoundComponent.h"
#include "MediaTexture.h"

namespace
{
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

TSharedRef<SWidget> UTunaSweeperStartupLogoWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		UImage* Background = WidgetTree->ConstructWidget<UImage>();
		Background->SetColorAndOpacity(FLinearColor::Black);
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
		VideoImage = WidgetTree->ConstructWidget<UImage>();
		Fill(Layers->AddChildToOverlay(VideoImage));
		StillImage = WidgetTree->ConstructWidget<UImage>();
		SetTexture(StillImage, LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/UI/Title/T_DevTunaLogoWhite.T_DevTunaLogoWhite")));
		StillImage->SetRenderOpacity(0.0f);
		Fill(Layers->AddChildToOverlay(StillImage));
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
	StillImage->SetRenderOpacity(Flow.StillOpacity);
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
