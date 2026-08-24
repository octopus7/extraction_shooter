#include "UI/TunaSweeperMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Map/TunaSweeperMapDefinition.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperMap
{
	const TCHAR* PlaceholderMapTexturePath = TEXT("/Game/UI/Map/T_UIMap_PlaceholderAlpha.T_UIMap_PlaceholderAlpha");
	const TCHAR* MapRegistryPath = TEXT("/Game/UI/Map/DA_UIMapRegistry.DA_UIMapRegistry");
	const TCHAR* PlayerIconTexturePath = TEXT("/Game/UI/Map/T_UIMap_PlayerLocation_Transparent.T_UIMap_PlayerLocation_Transparent");
	constexpr float MinZoom = 0.65f;
	constexpr float MaxZoom = 2.75f;
	constexpr float MarkerHitDistance = 26.0f;
	constexpr int32 FallbackMapWidth = 768;
	constexpr int32 FallbackMapHeight = 512;

	UTunaSweeperMapDefinition* ResolveRuntimeMapDefinition(const UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		UTunaSweeperMapRegistry* Registry = LoadObject<UTunaSweeperMapRegistry>(nullptr, MapRegistryPath);
		return Registry ? Registry->FindDefinitionForWorld(World) : nullptr;
	}

	FSlateBrush MakeMapBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float Radius = 6.0f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

}

void UTunaSweeperMapWidget::RefreshMapView()
{
	BuildMapWidget();
	EnsureMapTextures();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->GetMapMarkers(CachedMapMarkers);
	}
	else
	{
		CachedMapMarkers.Reset();
	}

	RefreshMapOverlayData();
	UpdatePlayerMapPosition();
	RefreshMapCanvas();
	RefreshMarkerIconButtons();
	RefreshMarkerColorButtons();
}

TSharedRef<SWidget> UTunaSweeperMapWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildMapWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	BuildMapWidget();
	EnsureMapTextures();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (ZoomSlider)
	{
		ZoomSlider->OnValueChanged.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleZoomSliderChanged);
		ZoomSlider->OnValueChanged.AddDynamic(this, &UTunaSweeperMapWidget::HandleZoomSliderChanged);
	}

	if (CircleMarkerIconButton)
	{
		CircleMarkerIconButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleCircleMarkerIconClicked);
		CircleMarkerIconButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleCircleMarkerIconClicked);
	}
	if (DiamondMarkerIconButton)
	{
		DiamondMarkerIconButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleDiamondMarkerIconClicked);
		DiamondMarkerIconButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleDiamondMarkerIconClicked);
	}
	if (TriangleMarkerIconButton)
	{
		TriangleMarkerIconButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleTriangleMarkerIconClicked);
		TriangleMarkerIconButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleTriangleMarkerIconClicked);
	}
	if (AlertMarkerIconButton)
	{
		AlertMarkerIconButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleAlertMarkerIconClicked);
		AlertMarkerIconButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleAlertMarkerIconClicked);
	}

	if (RedMarkerColorButton)
	{
		RedMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleRedMarkerColorClicked);
		RedMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleRedMarkerColorClicked);
	}
	if (AmberMarkerColorButton)
	{
		AmberMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleAmberMarkerColorClicked);
		AmberMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleAmberMarkerColorClicked);
	}
	if (GreenMarkerColorButton)
	{
		GreenMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleGreenMarkerColorClicked);
		GreenMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleGreenMarkerColorClicked);
	}
	if (CyanMarkerColorButton)
	{
		CyanMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleCyanMarkerColorClicked);
		CyanMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleCyanMarkerColorClicked);
	}
	if (VioletMarkerColorButton)
	{
		VioletMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleVioletMarkerColorClicked);
		VioletMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleVioletMarkerColorClicked);
	}
	if (WhiteMarkerColorButton)
	{
		WhiteMarkerColorButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleWhiteMarkerColorClicked);
		WhiteMarkerColorButton->OnClicked.AddDynamic(this, &UTunaSweeperMapWidget::HandleWhiteMarkerColorClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnMapMarkersChanged.RemoveAll(this);
		TunaGameInstance->OnMapMarkersChanged.AddUObject(this, &UTunaSweeperMapWidget::HandleMapMarkersChanged);
	}

	SetMapZoom(MapZoom);
	RefreshMapView();
}

void UTunaSweeperMapWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnMapMarkersChanged.RemoveAll(this);
	}

	if (ZoomSlider)
	{
		ZoomSlider->OnValueChanged.RemoveDynamic(this, &UTunaSweeperMapWidget::HandleZoomSliderChanged);
	}

	Super::NativeDestruct();
}

void UTunaSweeperMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (GetVisibility() == ESlateVisibility::Collapsed || GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}

	bool bNeedsCanvasRefresh = UpdatePlayerMapPosition();
	if (MapCanvas)
	{
		const FVector2D CurrentViewportSize = MapCanvas->GetCachedGeometry().GetLocalSize();
		if (!CurrentViewportSize.Equals(LastMapViewportSize, 0.5))
		{
			LastMapViewportSize = CurrentViewportSize;
			ClampMapPan();
			bNeedsCanvasRefresh = true;
		}
	}

	if (bNeedsCanvasRefresh)
	{
		RefreshMapCanvas();
	}
}

FReply UTunaSweeperMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D MapViewportLocalPosition;
	if (!IsMouseInsideMapViewport(InMouseEvent, &MapViewportLocalPosition))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		AddOrRemoveMarkerAtLocalPosition(MapViewportLocalPosition);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPanningMap = true;
		LastPanMouseLocalPosition = MapViewportLocalPosition;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UTunaSweeperMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPanningMap)
	{
		bIsPanningMap = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UTunaSweeperMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsPanningMap || !InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || !MapCanvas)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	const FVector2D CurrentMouseLocalPosition =
		MapCanvas->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	MapPan += CurrentMouseLocalPosition - LastPanMouseLocalPosition;
	LastPanMouseLocalPosition = CurrentMouseLocalPosition;
	ClampMapPan();
	RefreshMapCanvas();
	return FReply::Handled();
}

FReply UTunaSweeperMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D MapViewportLocalPosition;
	if (!IsMouseInsideMapViewport(InMouseEvent, &MapViewportLocalPosition))
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	const float WheelDelta = InMouseEvent.GetWheelDelta();
	if (FMath::Abs(WheelDelta) > KINDA_SMALL_NUMBER)
	{
		SetMapZoom(MapZoom + WheelDelta * 0.16f, &MapViewportLocalPosition);
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UTunaSweeperMapWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (TryCloseMapFromKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UTunaSweeperMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (TryCloseMapFromKey(InKeyEvent.GetKey()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperMapWidget::BuildMapWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MapRootOverlay"));
	BackgroundBlur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass(), TEXT("MapBackgroundBlur"));
	MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
	UCanvasPanel* ControlCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MapControlCanvas"));
	USizeBox* ZoomSliderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapZoomSliderBox"));
	ZoomSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MapZoomSlider"));
	UBorder* PaletteBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MapMarkerPaletteBackground"));
	UVerticalBox* PaletteStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MapMarkerPaletteStack"));
	UTextBlock* PaletteHelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapMarkerHelpText"));
	UHorizontalBox* PaletteRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MapMarkerPaletteRow"));
	USizeBox* PaletteGap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapMarkerPaletteGap"));

	if (!RootOverlay || !BackgroundBlur || !MapCanvas || !ControlCanvas || !ZoomSliderBox || !ZoomSlider ||
		!PaletteBackground || !PaletteStack || !PaletteHelpText || !PaletteRow || !PaletteGap)
	{
		return;
	}

	WidgetTree->RootWidget = RootOverlay;

	BackgroundBlur->SetBlurStrength(14.0f);
	UOverlaySlot* BlurSlot = RootOverlay->AddChildToOverlay(BackgroundBlur);
	if (BlurSlot)
	{
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UOverlaySlot* CanvasSlot = RootOverlay->AddChildToOverlay(MapCanvas);
	if (CanvasSlot)
	{
		CanvasSlot->SetHorizontalAlignment(HAlign_Fill);
		CanvasSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ControlCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* ControlSlot = RootOverlay->AddChildToOverlay(ControlCanvas);
	if (ControlSlot)
	{
		ControlSlot->SetHorizontalAlignment(HAlign_Fill);
		ControlSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ZoomSliderBox->SetWidthOverride(28.0f);
	ZoomSliderBox->SetContent(ZoomSlider);
	ZoomSlider->SetOrientation(Orient_Vertical);
	ZoomSlider->SetMinValue(0.0f);
	ZoomSlider->SetMaxValue(1.0f);
	ZoomSlider->SetValue((MapZoom - TunaSweeperMap::MinZoom) / (TunaSweeperMap::MaxZoom - TunaSweeperMap::MinZoom));
	UCanvasPanelSlot* SliderSlot = ControlCanvas->AddChildToCanvas(ZoomSliderBox);
	if (SliderSlot)
	{
		SliderSlot->SetAnchors(FAnchors(1.0f, 1.0f / 3.0f, 1.0f, 2.0f / 3.0f));
		SliderSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		SliderSlot->SetOffsets(FMargin(-28.0f, 0.0f, 28.0f, 0.0f));
	}

	auto AddSizedWidget = [this, PaletteRow](UWidget* Widget, float Width, float Height, float RightPadding)
	{
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!Widget || !SizeBox)
		{
			return;
		}

		SizeBox->SetWidthOverride(Width);
		SizeBox->SetHeightOverride(Height);
		SizeBox->SetContent(Widget);
		UHorizontalBoxSlot* Slot = PaletteRow->AddChildToHorizontalBox(SizeBox);
		if (Slot)
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, RightPadding, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	};

	auto MakeIconButton = [this, &AddSizedWidget](const FName& ButtonName, const FText& Glyph)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!Button || !Text)
		{
			return static_cast<UButton*>(nullptr);
		}

		Text->SetText(Glyph);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.96f, 0.98f, 1.0f)));
		TunaSweeperUIFont::ApplyFont(Text, 20, ETunaSweeperUIFontWeight::Bold);
		Button->SetContent(Text);
		AddSizedWidget(Button, 38.0f, 38.0f, 8.0f);
		return Button;
	};

	CircleMarkerIconButton = MakeIconButton(TEXT("CircleMarkerIconButton"), GetMarkerGlyph(0));
	DiamondMarkerIconButton = MakeIconButton(TEXT("DiamondMarkerIconButton"), GetMarkerGlyph(1));
	TriangleMarkerIconButton = MakeIconButton(TEXT("TriangleMarkerIconButton"), GetMarkerGlyph(2));
	AlertMarkerIconButton = MakeIconButton(TEXT("AlertMarkerIconButton"), GetMarkerGlyph(3));

	PaletteGap->SetWidthOverride(18.0f);
	if (UHorizontalBoxSlot* GapSlot = PaletteRow->AddChildToHorizontalBox(PaletteGap))
	{
		GapSlot->SetVerticalAlignment(VAlign_Fill);
	}

	auto MakeColorButton = [this, &AddSizedWidget](const FName& ButtonName)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		AddSizedWidget(Button, 34.0f, 34.0f, 8.0f);
		return Button;
	};

	RedMarkerColorButton = MakeColorButton(TEXT("RedMarkerColorButton"));
	AmberMarkerColorButton = MakeColorButton(TEXT("AmberMarkerColorButton"));
	GreenMarkerColorButton = MakeColorButton(TEXT("GreenMarkerColorButton"));
	CyanMarkerColorButton = MakeColorButton(TEXT("CyanMarkerColorButton"));
	VioletMarkerColorButton = MakeColorButton(TEXT("VioletMarkerColorButton"));
	WhiteMarkerColorButton = MakeColorButton(TEXT("WhiteMarkerColorButton"));

	PaletteHelpText->SetText(FText::FromString(TEXT("\uC6B0\uD074\uB9AD: \uB9C8\uCEE4 \uCD94\uAC00 / \uAE30\uC874 \uB9C8\uCEE4 \uC6B0\uD074\uB9AD: \uC0AD\uC81C")));
	PaletteHelpText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.96f, 0.98f, 0.88f)));
	PaletteHelpText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	PaletteHelpText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
	PaletteHelpText->SetVisibility(ESlateVisibility::HitTestInvisible);
	TunaSweeperUIFont::ApplyFont(PaletteHelpText, 13, ETunaSweeperUIFontWeight::Regular);
	if (UVerticalBoxSlot* HelpSlot = PaletteStack->AddChildToVerticalBox(PaletteHelpText))
	{
		HelpSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 6.0f));
		HelpSlot->SetHorizontalAlignment(HAlign_Left);
	}
	if (UVerticalBoxSlot* RowSlot = PaletteStack->AddChildToVerticalBox(PaletteRow))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Left);
	}

	PaletteBackground->SetBrush(TunaSweeperMap::MakeMapBoxBrush(
		FVector2D(322.0f, 72.0f),
		FLinearColor(0.006f, 0.008f, 0.012f, 0.84f),
		FLinearColor::Transparent,
		0.0f,
		6.0f));
	PaletteBackground->SetPadding(FMargin(10.0f, 8.0f, 6.0f, 8.0f));
	PaletteBackground->SetContent(PaletteStack);

	UCanvasPanelSlot* PaletteSlot = ControlCanvas->AddChildToCanvas(PaletteBackground);
	if (PaletteSlot)
	{
		PaletteSlot->SetAnchors(FAnchors(0.0f, 1.0f));
		PaletteSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		PaletteSlot->SetAutoSize(true);
		PaletteSlot->SetPosition(FVector2D(32.0f, -24.0f));
	}

	RefreshMarkerIconButtons();
	RefreshMarkerColorButtons();
}

void UTunaSweeperMapWidget::EnsureMapTextures()
{
	if (!MapDefinition || !MapDefinition->MatchesWorld(GetWorld()))
	{
		MapDefinition = TunaSweeperMap::ResolveRuntimeMapDefinition(GetWorld());
	}

	const FSoftObjectPath DesiredTextureObjectPath = MapDefinition
		? MapDefinition->Texture.ToSoftObjectPath()
		: FSoftObjectPath(TunaSweeperMap::PlaceholderMapTexturePath);
	const FString DesiredMapTexturePath = DesiredTextureObjectPath.ToString();
	if (!MapTexture || !ActiveMapTexturePath.Equals(DesiredMapTexturePath, ESearchCase::CaseSensitive))
	{
		MapTexture = Cast<UTexture2D>(DesiredTextureObjectPath.TryLoad());
		ActiveMapTexturePath = MapTexture ? DesiredMapTexturePath : FString();
	}
	if (!MapTexture)
	{
		EnsureFallbackMapTexture();
	}

	if (!PlayerIconTexture)
	{
		PlayerIconTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperMap::PlayerIconTexturePath);
	}
}

void UTunaSweeperMapWidget::EnsureFallbackMapTexture()
{
	if (MapTexture)
	{
		return;
	}

	constexpr int32 Width = TunaSweeperMap::FallbackMapWidth;
	constexpr int32 Height = TunaSweeperMap::FallbackMapHeight;
	TArray<uint8> Pixels;
	Pixels.SetNumZeroed(Width * Height * 4);

	auto SetPixel = [&Pixels](int32 X, int32 Y, const FColor& Color)
	{
		if (X < 0 || X >= TunaSweeperMap::FallbackMapWidth || Y < 0 || Y >= TunaSweeperMap::FallbackMapHeight)
		{
			return;
		}

		const int32 ByteIndex = (Y * TunaSweeperMap::FallbackMapWidth + X) * 4;
		Pixels[ByteIndex + 0] = Color.B;
		Pixels[ByteIndex + 1] = Color.G;
		Pixels[ByteIndex + 2] = Color.R;
		Pixels[ByteIndex + 3] = Color.A;
	};

	auto FillRect = [&SetPixel](int32 MinX, int32 MinY, int32 MaxX, int32 MaxY, const FColor& Color)
	{
		for (int32 Y = MinY; Y <= MaxY; ++Y)
		{
			for (int32 X = MinX; X <= MaxX; ++X)
			{
				SetPixel(X, Y, Color);
			}
		}
	};

	auto DrawRectOutline = [&FillRect](int32 MinX, int32 MinY, int32 MaxX, int32 MaxY, int32 Thickness, const FColor& Color)
	{
		FillRect(MinX, MinY, MaxX, MinY + Thickness - 1, Color);
		FillRect(MinX, MaxY - Thickness + 1, MaxX, MaxY, Color);
		FillRect(MinX, MinY, MinX + Thickness - 1, MaxY, Color);
		FillRect(MaxX - Thickness + 1, MinY, MaxX, MaxY, Color);
	};

	const FColor BodyColor(42, 52, 52, 176);
	const FColor RoomColor(64, 76, 70, 132);
	const FColor LineColor(168, 202, 190, 186);
	FillRect(64, 72, Width - 70, Height - 72, BodyColor);
	DrawRectOutline(64, 72, Width - 70, Height - 72, 3, LineColor);
	for (int32 X = 96; X < Width; X += 96)
	{
		FillRect(X, 50, X + 1, Height - 50, FColor(168, 202, 190, 82));
	}
	for (int32 Y = 96; Y < Height; Y += 96)
	{
		FillRect(50, Y, Width - 50, Y + 1, FColor(168, 202, 190, 82));
	}

	const FIntRect Rooms[] = {
		FIntRect(100, 110, 248, 220),
		FIntRect(286, 110, 424, 238),
		FIntRect(466, 126, 626, 224),
		FIntRect(118, 272, 286, 372),
		FIntRect(340, 280, 468, 396),
		FIntRect(512, 270, 666, 390)
	};
	for (const FIntRect& Room : Rooms)
	{
		FillRect(Room.Min.X, Room.Min.Y, Room.Max.X, Room.Max.Y, RoomColor);
		DrawRectOutline(Room.Min.X, Room.Min.Y, Room.Max.X, Room.Max.Y, 2, FColor(178, 212, 198, 118));
	}

	MapTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, TEXT("TunaSweeperFallbackMap"), Pixels);
	if (!MapTexture)
	{
		return;
	}

	MapTexture->CompressionSettings = TC_EditorIcon;
#if WITH_EDITORONLY_DATA
	MapTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
	MapTexture->LODGroup = TEXTUREGROUP_UI;
	MapTexture->SRGB = true;
	MapTexture->NeverStream = true;
	MapTexture->Filter = TF_Bilinear;
	MapTexture->AddressX = TA_Clamp;
	MapTexture->AddressY = TA_Clamp;
	MapTexture->UpdateResource();
}

void UTunaSweeperMapWidget::RefreshMapCanvas()
{
	if (!WidgetTree || !MapCanvas)
	{
		return;
	}

	EnsureMapTextures();
	MapCanvas->ClearChildren();

	const FVector2D ViewportSize = MapCanvas ? MapCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (ViewportSize.X <= 1.0 || ViewportSize.Y <= 1.0)
	{
		return;
	}

	const FVector2D MapTopLeft = GetMapDrawTopLeft();
	const FVector2D MapDrawSize = GetMapScaledDrawSize();

	MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (MapImage && MapTexture)
	{
		MapImage->SetBrushFromTexture(MapTexture, false);
		MapImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.94f));
		MapImage->SetVisibility(ESlateVisibility::HitTestInvisible);

		UCanvasPanelSlot* MapImageSlot = MapCanvas->AddChildToCanvas(MapImage);
		if (MapImageSlot)
		{
			MapImageSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			MapImageSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			MapImageSlot->SetPosition(MapTopLeft);
			MapImageSlot->SetSize(MapDrawSize);
		}
	}

	for (const FTunaSweeperMapMarkerSaveData& Marker : CachedMapMarkers)
	{
		UTextBlock* MarkerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!MarkerText)
		{
			continue;
		}

		const FVector2D MarkerLocalPosition = MapPositionToLocal(Marker.MapPosition);
		const FVector2D MarkerSize(38.0f, 38.0f);
		MarkerText->SetText(GetMarkerGlyph(Marker.MarkerIconIndex));
		MarkerText->SetColorAndOpacity(FSlateColor(GetMarkerColor(Marker.MarkerColorIndex)));
		MarkerText->SetJustification(ETextJustify::Center);
		MarkerText->SetShadowOffset(FVector2D(0.0f, 0.0f));
		MarkerText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
		MarkerText->SetVisibility(ESlateVisibility::HitTestInvisible);
		TunaSweeperUIFont::ApplyFont(MarkerText, 30, ETunaSweeperUIFontWeight::Bold);

		UCanvasPanelSlot* MarkerSlot = MapCanvas->AddChildToCanvas(MarkerText);
		if (MarkerSlot)
		{
			MarkerSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetPosition(MarkerLocalPosition);
			MarkerSlot->SetSize(MarkerSize);
		}
	}

	for (const FTunaSweeperMapOverlayDefinition& MapOverlay : CachedMapOverlays)
	{
		AddMapOverlayToCanvas(MapOverlay);
	}

	if (bHasPlayerMapPosition)
	{
		const FVector2D PlayerLocalPosition = MapPositionToLocal(CachedPlayerMapPosition);
		const float PlayerIconSize = FMath::Clamp(46.0f * FMath::Sqrt(MapZoom), 36.0f, 64.0f);
		if (PlayerIconTexture)
		{
			UImage* PlayerIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			if (PlayerIconImage)
			{
				PlayerIconImage->SetBrushFromTexture(PlayerIconTexture, true);
				PlayerIconImage->SetColorAndOpacity(FLinearColor::White);
				PlayerIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
				UCanvasPanelSlot* PlayerIconSlot = MapCanvas->AddChildToCanvas(PlayerIconImage);
				if (PlayerIconSlot)
				{
					PlayerIconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
					PlayerIconSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					PlayerIconSlot->SetPosition(PlayerLocalPosition);
					PlayerIconSlot->SetSize(FVector2D(PlayerIconSize, PlayerIconSize));
				}
			}
		}
		else
		{
			UTextBlock* PlayerFallbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (PlayerFallbackText)
			{
				PlayerFallbackText->SetText(FText::FromString(TEXT("\u25B2")));
				PlayerFallbackText->SetColorAndOpacity(FSlateColor(FLinearColor(0.12f, 0.82f, 1.0f, 1.0f)));
				PlayerFallbackText->SetJustification(ETextJustify::Center);
				PlayerFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
				TunaSweeperUIFont::ApplyFont(PlayerFallbackText, 34, ETunaSweeperUIFontWeight::Bold);
				UCanvasPanelSlot* PlayerFallbackSlot = MapCanvas->AddChildToCanvas(PlayerFallbackText);
				if (PlayerFallbackSlot)
				{
					PlayerFallbackSlot->SetAnchors(FAnchors(0.0f, 0.0f));
					PlayerFallbackSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					PlayerFallbackSlot->SetPosition(PlayerLocalPosition);
					PlayerFallbackSlot->SetSize(FVector2D(PlayerIconSize, PlayerIconSize));
				}
			}
		}
	}
}

void UTunaSweeperMapWidget::RefreshMapOverlayData()
{
	CachedMapOverlays.Reset();

	UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperEnemySpawnSubsystem* SpawnSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>()
		: nullptr;
	if (!SpawnSubsystem)
	{
		return;
	}

	SpawnSubsystem->GetMapOverlaysForWorld(GetWorld(), CachedMapOverlays);
}

void UTunaSweeperMapWidget::AddMapOverlayToCanvas(const FTunaSweeperMapOverlayDefinition& MapOverlay)
{
	if (!WidgetTree || !MapCanvas)
	{
		return;
	}

	const FVector2D AnchorLocalPosition = MapPositionToLocal(ProjectWorldLocationToMapPosition(MapOverlay.WorldLocation));
	if (!MapOverlay.IconId.IsNone())
	{
		UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (IconText)
		{
			const FVector2D IconSize(42.0f, 42.0f);
			IconText->SetText(GetMapOverlayIconGlyph(MapOverlay.IconId));
			IconText->SetColorAndOpacity(FSlateColor(GetMapOverlayIconColor(MapOverlay.IconId)));
			IconText->SetJustification(ETextJustify::Center);
			IconText->SetShadowOffset(FVector2D(1.0f, 1.0f));
			IconText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
			IconText->SetVisibility(ESlateVisibility::HitTestInvisible);
			TunaSweeperUIFont::ApplyFont(IconText, 34, ETunaSweeperUIFontWeight::Bold);

			if (UCanvasPanelSlot* IconSlot = MapCanvas->AddChildToCanvas(IconText))
			{
				IconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				IconSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				IconSlot->SetPosition(AnchorLocalPosition + MapOverlay.IconOffset);
				IconSlot->SetSize(IconSize);
			}
		}
	}

	if (!MapOverlay.TextStringKey.IsNone())
	{
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (LabelText)
		{
			const bool bHasIcon = !MapOverlay.IconId.IsNone();
			const FVector2D LabelSize(220.0f, 34.0f);
			LabelText->SetText(ResolveMapOverlayText(MapOverlay));
			LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 1.0f, 0.92f, 1.0f)));
			LabelText->SetJustification(ETextJustify::Center);
			LabelText->SetShadowOffset(FVector2D(1.0f, 1.0f));
			LabelText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.88f));
			LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
			TunaSweeperUIFont::ApplyFont(LabelText, 17, ETunaSweeperUIFontWeight::Bold);

			if (UCanvasPanelSlot* LabelSlot = MapCanvas->AddChildToCanvas(LabelText))
			{
				LabelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				LabelSlot->SetAlignment(bHasIcon ? FVector2D(0.5f, 1.0f) : FVector2D(0.5f, 0.5f));
				LabelSlot->SetPosition(AnchorLocalPosition + MapOverlay.TextOffset);
				LabelSlot->SetSize(LabelSize);
			}
		}
	}
}

void UTunaSweeperMapWidget::RefreshMarkerIconButtons()
{
	const FLinearColor ActiveOutline(0.30f, 0.92f, 0.86f, 1.0f);
	const FLinearColor IdleOutline(0.18f, 0.24f, 0.25f, 0.72f);
	const FLinearColor Fill(0.035f, 0.044f, 0.048f, 0.96f);

	ConfigureChoiceButton(CircleMarkerIconButton, Fill, SelectedMarkerIconIndex == 0 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(DiamondMarkerIconButton, Fill, SelectedMarkerIconIndex == 1 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(TriangleMarkerIconButton, Fill, SelectedMarkerIconIndex == 2 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(AlertMarkerIconButton, Fill, SelectedMarkerIconIndex == 3 ? ActiveOutline : IdleOutline);
}

void UTunaSweeperMapWidget::RefreshMarkerColorButtons()
{
	const FLinearColor ActiveOutline(0.90f, 0.98f, 1.0f, 1.0f);
	const FLinearColor IdleOutline(0.10f, 0.13f, 0.14f, 0.88f);

	ConfigureChoiceButton(RedMarkerColorButton, GetMarkerColor(0), SelectedMarkerColorIndex == 0 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(AmberMarkerColorButton, GetMarkerColor(1), SelectedMarkerColorIndex == 1 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(GreenMarkerColorButton, GetMarkerColor(2), SelectedMarkerColorIndex == 2 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(CyanMarkerColorButton, GetMarkerColor(3), SelectedMarkerColorIndex == 3 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(VioletMarkerColorButton, GetMarkerColor(4), SelectedMarkerColorIndex == 4 ? ActiveOutline : IdleOutline);
	ConfigureChoiceButton(WhiteMarkerColorButton, GetMarkerColor(5), SelectedMarkerColorIndex == 5 ? ActiveOutline : IdleOutline);
}

void UTunaSweeperMapWidget::SetMarkerIconIndex(int32 InMarkerIconIndex)
{
	SelectedMarkerIconIndex = FMath::Clamp(InMarkerIconIndex, 0, 3);
	RefreshMarkerIconButtons();
}

void UTunaSweeperMapWidget::SetMarkerColorIndex(int32 InMarkerColorIndex)
{
	SelectedMarkerColorIndex = FMath::Clamp(InMarkerColorIndex, 0, 5);
	RefreshMarkerColorButtons();
}

void UTunaSweeperMapWidget::SetMapZoom(float InMapZoom, const FVector2D* ZoomAnchorLocalPosition)
{
	const float NewZoom = FMath::Clamp(InMapZoom, TunaSweeperMap::MinZoom, TunaSweeperMap::MaxZoom);
	if (FMath::IsNearlyEqual(MapZoom, NewZoom, 0.001f))
	{
		return;
	}

	FVector2D AnchorMapPosition(0.5f, 0.5f);
	if (ZoomAnchorLocalPosition)
	{
		TryGetMapPositionFromLocal(*ZoomAnchorLocalPosition, AnchorMapPosition);
	}

	MapZoom = NewZoom;
	if (ZoomAnchorLocalPosition)
	{
		const FVector2D ViewportSize = MapCanvas ? MapCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
		const FVector2D NewScaledSize = GetMapScaledDrawSize();
		const FVector2D NewBaseTopLeft = (ViewportSize - NewScaledSize) * 0.5;
		const FVector2D AnchorTextureUV = MapPositionToTextureUV(AnchorMapPosition);
		MapPan = *ZoomAnchorLocalPosition - FVector2D(AnchorTextureUV.X * NewScaledSize.X, AnchorTextureUV.Y * NewScaledSize.Y) - NewBaseTopLeft;
	}

	ClampMapPan();
	if (ZoomSlider && !bIsUpdatingZoomSlider)
	{
		bIsUpdatingZoomSlider = true;
		ZoomSlider->SetValue((MapZoom - TunaSweeperMap::MinZoom) / (TunaSweeperMap::MaxZoom - TunaSweeperMap::MinZoom));
		bIsUpdatingZoomSlider = false;
	}

	RefreshMapCanvas();
}

void UTunaSweeperMapWidget::ClampMapPan()
{
	// Intentionally unbounded: the map can move past the visible viewport and is clipped only by the screen.
}

bool UTunaSweeperMapWidget::TryCloseMapFromKey(const FKey& Key)
{
	if (Key != EKeys::M && Key != EKeys::Tab)
	{
		return false;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		if (UTunaSweeperGameHudWidget* GameHudWidget = TunaPlayerController->GetGameHudWidget())
		{
			GameHudWidget->SetHudMode(ETunaSweeperHudMode::None);
		}

		TunaPlayerController->ApplyDefaultGameInputMode();
	}

	return true;
}

void UTunaSweeperMapWidget::AddOrRemoveMarkerAtLocalPosition(const FVector2D& MapViewportLocalPosition)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	int32 NearestMarkerId = INDEX_NONE;
	float NearestDistanceSquared = FMath::Square(TunaSweeperMap::MarkerHitDistance);
	for (const FTunaSweeperMapMarkerSaveData& Marker : CachedMapMarkers)
	{
		const float DistanceSquared = FVector2D::DistSquared(MapViewportLocalPosition, MapPositionToLocal(Marker.MapPosition));
		if (DistanceSquared <= NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestMarkerId = Marker.MarkerId;
		}
	}

	if (NearestMarkerId > 0)
	{
		TunaGameInstance->RemoveMapMarker(NearestMarkerId, false);
		return;
	}

	FVector2D MapPosition;
	if (TryGetMapPositionFromLocal(MapViewportLocalPosition, MapPosition))
	{
		TunaGameInstance->AddMapMarker(MapPosition, SelectedMarkerIconIndex, SelectedMarkerColorIndex, false);
	}
}

bool UTunaSweeperMapWidget::TryGetMapPositionFromLocal(const FVector2D& MapViewportLocalPosition, FVector2D& OutMapPosition) const
{
	const FVector2D TopLeft = GetMapDrawTopLeft();
	const FVector2D ScaledSize = GetMapScaledDrawSize();
	if (ScaledSize.X <= 1.0 || ScaledSize.Y <= 1.0)
	{
		return false;
	}

	const FVector2D TextureUV(
		(MapViewportLocalPosition.X - TopLeft.X) / ScaledSize.X,
		(MapViewportLocalPosition.Y - TopLeft.Y) / ScaledSize.Y);
	if (TextureUV.X < 0.0 || TextureUV.X > 1.0 || TextureUV.Y < 0.0 || TextureUV.Y > 1.0)
	{
		return false;
	}

	if (MapDefinition)
	{
		return MapDefinition->TextureUVToContentUV(TextureUV, OutMapPosition);
	}

	OutMapPosition = TextureUV;
	return true;
}

FVector2D UTunaSweeperMapWidget::MapPositionToLocal(const FVector2D& MapPosition) const
{
	const FVector2D TopLeft = GetMapDrawTopLeft();
	const FVector2D ScaledSize = GetMapScaledDrawSize();
	const FVector2D TextureUV = MapPositionToTextureUV(MapPosition);
	return TopLeft + FVector2D(TextureUV.X * ScaledSize.X, TextureUV.Y * ScaledSize.Y);
}

FVector2D UTunaSweeperMapWidget::MapPositionToTextureUV(const FVector2D& MapPosition) const
{
	return MapDefinition ? MapDefinition->ContentUVToTextureUV(MapPosition) : MapPosition;
}

FVector2D UTunaSweeperMapWidget::GetMapBaseDrawSize() const
{
	const FVector2D ViewportSize = MapCanvas ? MapCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	if (ViewportSize.X <= 1.0 || ViewportSize.Y <= 1.0)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D TextureSize(1536.0, 1024.0);
	if (MapTexture && MapTexture->GetSizeX() > 0 && MapTexture->GetSizeY() > 0)
	{
		TextureSize = FVector2D(MapTexture->GetSizeX(), MapTexture->GetSizeY());
	}

	const double FitScale = FMath::Min(ViewportSize.X / TextureSize.X, ViewportSize.Y / TextureSize.Y);
	return TextureSize * FitScale;
}

FVector2D UTunaSweeperMapWidget::GetMapScaledDrawSize() const
{
	return GetMapBaseDrawSize() * MapZoom;
}

FVector2D UTunaSweeperMapWidget::GetMapDrawTopLeft() const
{
	const FVector2D ViewportSize = MapCanvas ? MapCanvas->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
	const FVector2D ScaledSize = GetMapScaledDrawSize();
	return (ViewportSize - ScaledSize) * 0.5 + MapPan;
}

bool UTunaSweeperMapWidget::IsMouseInsideMapViewport(const FPointerEvent& InMouseEvent, FVector2D* OutMapViewportLocalPosition) const
{
	if (!MapCanvas)
	{
		return false;
	}

	const FGeometry& ViewportGeometry = MapCanvas->GetCachedGeometry();
	const FVector2D LocalPosition = ViewportGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D LocalSize = ViewportGeometry.GetLocalSize();
	const bool bInside =
		LocalPosition.X >= 0.0 && LocalPosition.Y >= 0.0 &&
		LocalPosition.X <= LocalSize.X && LocalPosition.Y <= LocalSize.Y;
	if (bInside && OutMapViewportLocalPosition)
	{
		*OutMapViewportLocalPosition = LocalPosition;
	}
	return bInside;
}

bool UTunaSweeperMapWidget::UpdatePlayerMapPosition()
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		const bool bHadPlayerPosition = bHasPlayerMapPosition;
		bHasPlayerMapPosition = false;
		return bHadPlayerPosition;
	}

	const FVector2D NewPlayerMapPosition = ProjectWorldLocationToMapPosition(Pawn->GetActorLocation());
	const bool bChanged = !bHasPlayerMapPosition || !CachedPlayerMapPosition.Equals(NewPlayerMapPosition, 0.001);
	CachedPlayerMapPosition = NewPlayerMapPosition;
	bHasPlayerMapPosition = true;
	return bChanged;
}

FVector2D UTunaSweeperMapWidget::ProjectWorldLocationToMapPosition(const FVector& WorldLocation) const
{
	if (MapDefinition)
	{
		const FVector2D ContentUV = MapDefinition->WorldLocationToContentUV(WorldLocation);
		return FVector2D(
			FMath::Clamp(ContentUV.X, 0.0, 1.0),
			FMath::Clamp(ContentUV.Y, 0.0, 1.0));
	}

	const FVector2D WorldMin(-3000.0, -3000.0);
	const FVector2D WorldMax(3000.0, 3000.0);
	const double WorldWidth = FMath::Max(1.0, WorldMax.Y - WorldMin.Y);
	const double WorldHeight = FMath::Max(1.0, WorldMax.X - WorldMin.X);
	return FVector2D(
		FMath::Clamp((WorldLocation.Y - WorldMin.Y) / WorldWidth, 0.0, 1.0),
		FMath::Clamp(1.0 - ((WorldLocation.X - WorldMin.X) / WorldHeight), 0.0, 1.0));
}

void UTunaSweeperMapWidget::HandleMapMarkersChanged()
{
	RefreshMapView();
}

FText UTunaSweeperMapWidget::ResolveMapOverlayText(const FTunaSweeperMapOverlayDefinition& MapOverlay) const
{
	if (MapOverlay.TextStringKey.IsNone())
	{
		return FText::GetEmpty();
	}

	const FText FallbackText = FText::FromString(MapOverlay.TextStringKey.ToString());
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(MapOverlay.TextStringKey, FallbackText)
		: FallbackText;
}

FText UTunaSweeperMapWidget::GetMapOverlayIconGlyph(FName IconId) const
{
	FString NormalizedIconId = IconId.ToString().TrimStartAndEnd().ToLower();
	NormalizedIconId.ReplaceInline(TEXT("-"), TEXT("_"));

	if (NormalizedIconId == TEXT("green_inverted_triangle") ||
		NormalizedIconId == TEXT("inverted_triangle") ||
		NormalizedIconId == TEXT("down_triangle"))
	{
		return FText::FromString(TEXT("\u25BC"));
	}

	return FText::FromString(TEXT("\u25CF"));
}

FLinearColor UTunaSweeperMapWidget::GetMapOverlayIconColor(FName IconId) const
{
	FString NormalizedIconId = IconId.ToString().TrimStartAndEnd().ToLower();
	NormalizedIconId.ReplaceInline(TEXT("-"), TEXT("_"));

	if (NormalizedIconId == TEXT("green_inverted_triangle") ||
		NormalizedIconId == TEXT("inverted_triangle") ||
		NormalizedIconId == TEXT("down_triangle"))
	{
		return FLinearColor(0.22f, 0.96f, 0.34f, 1.0f);
	}

	return FLinearColor(0.94f, 0.96f, 0.92f, 1.0f);
}

FText UTunaSweeperMapWidget::GetMarkerGlyph(int32 MarkerIconIndex) const
{
	switch (MarkerIconIndex)
	{
	case 1:
		return FText::FromString(TEXT("\u25C6"));
	case 2:
		return FText::FromString(TEXT("\u25B2"));
	case 3:
		return FText::FromString(TEXT("!"));
	case 0:
	default:
		return FText::FromString(TEXT("\u25CF"));
	}
}

FLinearColor UTunaSweeperMapWidget::GetMarkerColor(int32 MarkerColorIndex) const
{
	switch (MarkerColorIndex)
	{
	case 0:
		return FLinearColor(0.98f, 0.22f, 0.24f, 1.0f);
	case 1:
		return FLinearColor(1.0f, 0.68f, 0.18f, 1.0f);
	case 2:
		return FLinearColor(0.38f, 0.92f, 0.42f, 1.0f);
	case 3:
		return FLinearColor(0.12f, 0.82f, 1.0f, 1.0f);
	case 4:
		return FLinearColor(0.74f, 0.42f, 1.0f, 1.0f);
	case 5:
	default:
		return FLinearColor(0.94f, 0.96f, 0.92f, 1.0f);
	}
}

void UTunaSweeperMapWidget::ConfigureChoiceButton(UButton* Button, const FLinearColor& FillColor, const FLinearColor& OutlineColor)
{
	if (!Button)
	{
		return;
	}

	Button->SetRenderOpacity(OutlineColor.A >= 0.95f ? 1.0f : 0.72f);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(TunaSweeperMap::MakeMapBoxBrush(FVector2D(38.0f, 38.0f), FillColor, FLinearColor::Transparent, 0.0f, 4.0f));
	ButtonStyle.SetHovered(TunaSweeperMap::MakeMapBoxBrush(
		FVector2D(38.0f, 38.0f),
		FLinearColor(
			FMath::Min(FillColor.R + 0.08f, 1.0f),
			FMath::Min(FillColor.G + 0.08f, 1.0f),
			FMath::Min(FillColor.B + 0.08f, 1.0f),
			FillColor.A),
		FLinearColor::Transparent,
		0.0f,
		4.0f));
	ButtonStyle.SetPressed(TunaSweeperMap::MakeMapBoxBrush(
		FVector2D(38.0f, 38.0f),
		FLinearColor(
			FMath::Max(FillColor.R - 0.05f, 0.0f),
			FMath::Max(FillColor.G - 0.05f, 0.0f),
			FMath::Max(FillColor.B - 0.05f, 0.0f),
			FillColor.A),
		FLinearColor::Transparent,
		0.0f,
		4.0f));
	Button->SetStyle(ButtonStyle);
}

void UTunaSweeperMapWidget::HandleZoomSliderChanged(float InValue)
{
	if (bIsUpdatingZoomSlider)
	{
		return;
	}

	SetMapZoom(FMath::Lerp(TunaSweeperMap::MinZoom, TunaSweeperMap::MaxZoom, InValue));
}

void UTunaSweeperMapWidget::HandleCircleMarkerIconClicked()
{
	SetMarkerIconIndex(0);
}

void UTunaSweeperMapWidget::HandleDiamondMarkerIconClicked()
{
	SetMarkerIconIndex(1);
}

void UTunaSweeperMapWidget::HandleTriangleMarkerIconClicked()
{
	SetMarkerIconIndex(2);
}

void UTunaSweeperMapWidget::HandleAlertMarkerIconClicked()
{
	SetMarkerIconIndex(3);
}

void UTunaSweeperMapWidget::HandleRedMarkerColorClicked()
{
	SetMarkerColorIndex(0);
}

void UTunaSweeperMapWidget::HandleAmberMarkerColorClicked()
{
	SetMarkerColorIndex(1);
}

void UTunaSweeperMapWidget::HandleGreenMarkerColorClicked()
{
	SetMarkerColorIndex(2);
}

void UTunaSweeperMapWidget::HandleCyanMarkerColorClicked()
{
	SetMarkerColorIndex(3);
}

void UTunaSweeperMapWidget::HandleVioletMarkerColorClicked()
{
	SetMarkerColorIndex(4);
}

void UTunaSweeperMapWidget::HandleWhiteMarkerColorClicked()
{
	SetMarkerColorIndex(5);
}
