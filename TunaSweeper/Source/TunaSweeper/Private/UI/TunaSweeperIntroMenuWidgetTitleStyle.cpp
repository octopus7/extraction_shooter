#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Components/ButtonSlot.h"

namespace TunaSweeperTitleStyle
{
	// UI artwork is drawn at twice the layout resolution, with straight alpha and no font glyph icons.
	// These transient resources are owned by the menu and reused across refreshes and language changes.
	struct FArtwork
	{
		int32 Width, Height;
		TArray<FLinearColor> Pixels;
		FArtwork(int32 W, int32 H) : Width(W * 2), Height(H * 2) { Pixels.Init(FLinearColor::Transparent, Width * Height); }
		void Paint(FVector2D Min, FVector2D Max, FLinearColor Color, TFunctionRef<float(FVector2D)> Coverage)
		{
			for (int32 Y = FMath::Max(0, FMath::FloorToInt(Min.Y * 2)); Y < FMath::Min(Height, FMath::CeilToInt(Max.Y * 2)); ++Y)
			for (int32 X = FMath::Max(0, FMath::FloorToInt(Min.X * 2)); X < FMath::Min(Width, FMath::CeilToInt(Max.X * 2)); ++X)
			{
				const float Alpha = Color.A * FMath::Clamp(Coverage(FVector2D(X + 0.5f, Y + 0.5f) * 0.5f), 0.0f, 1.0f);
				FLinearColor& Dest = Pixels[Y * Width + X];
				const float OutAlpha = Alpha + Dest.A * (1.0f - Alpha);
				if (OutAlpha > SMALL_NUMBER)
				{
					Dest = (Color * Alpha + Dest * (Dest.A * (1.0f - Alpha))) / OutAlpha;
					Dest.A = OutAlpha;
				}
			}
		}
		void Line(FVector2D A, FVector2D B, float Thickness, FLinearColor Color)
		{
			const FVector2D Pad(Thickness + 1.0f);
			Paint(FVector2D(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)) - Pad,
				FVector2D(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y)) + Pad, Color, [=](FVector2D P)
			{
				const FVector2D AB = B - A;
				const float T = FMath::Clamp(float(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 0.001)), 0.0f, 1.0f);
				return (Thickness * 0.5f - (P - A - AB * T).Size()) * 2.0f + 0.5f;
			});
		}
		void Ellipse(FVector2D C, FVector2D R, FLinearColor Color)
		{
			Paint(C - R - FVector2D(1), C + R + FVector2D(1), Color, [=](FVector2D P)
			{
				return (1.0f - ((P - C) / R).Size()) * FMath::Min(R.X, R.Y) * 2.0f + 0.5f;
			});
		}
		void Fish(FVector2D C, float S, FLinearColor Color)
		{
			Ellipse(C, FVector2D(13, 6) * S, Color);
			Line(C + FVector2D(-9, 0) * S, C + FVector2D(-19, -6) * S, 3 * S, Color);
			Line(C + FVector2D(-9, 0) * S, C + FVector2D(-19, 6) * S, 3 * S, Color);
			Line(C + FVector2D(-19, -6) * S, C + FVector2D(-19, 6) * S, 2 * S, Color);
			Line(C + FVector2D(-3, -5) * S, C + FVector2D(1, -9) * S, 2 * S, Color);
			Ellipse(C + FVector2D(8, -1) * S, FVector2D(1.2f) * S, FLinearColor(0.02f, 0.07f, 0.08f, Color.A));
		}
		UTexture2D* Texture()
		{
			TArray<uint8> Bytes;
			Bytes.SetNumUninitialized(Width * Height * 4);
			for (int32 I = 0; I < Pixels.Num(); ++I)
			{
				const FColor C = Pixels[I].ToFColorSRGB();
				Bytes[I * 4] = C.B; Bytes[I * 4 + 1] = C.G; Bytes[I * 4 + 2] = C.R; Bytes[I * 4 + 3] = C.A;
			}
			UTexture2D* Result = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, NAME_None, Bytes);
			if (Result)
			{
				Result->LODGroup = TEXTUREGROUP_UI; Result->SRGB = true; Result->NeverStream = true;
				Result->Filter = TF_Bilinear; Result->AddressX = TA_Clamp; Result->AddressY = TA_Clamp;
				Result->UpdateResource();
			}
			return Result;
		}
	};

	enum class EArtwork { Play, Settings, Quit, Save, Wishlist, Laboratory };
	UTexture2D* ButtonArtwork(EArtwork Kind, int32 W, int32 H)
	{
		FArtwork Art(W, H);
		const bool Primary = Kind == EArtwork::Play;
		const bool Warm = Kind == EArtwork::Wishlist;
		const FLinearColor Fill = Primary ? FLinearColor(0.018f, 0.37f, 0.43f) : Warm
			? FLinearColor(0.43f, 0.21f, 0.085f) : FLinearColor(0.065f, 0.095f, 0.12f);
		const FLinearColor Edge = Primary ? FLinearColor(0.04f, 0.9f, 0.94f, 0.95f) : Warm
			? FLinearColor(0.84f, 0.48f, 0.22f, 0.8f) : FLinearColor(0.34f, 0.45f, 0.50f, 0.6f);
		auto Distance = [=](FVector2D P)
		{
			const FVector2D Q = (P - FVector2D(W, H) * 0.5f).GetAbs() - (FVector2D(W, H) * 0.5f - FVector2D(28));
			return FVector2D(FMath::Max(Q.X, 0.0), FMath::Max(Q.Y, 0.0)).Size() + FMath::Min(FMath::Max(Q.X, Q.Y), 0.0) - 16.0;
		};
		if (Primary)
			Art.Paint(FVector2D::ZeroVector, FVector2D(W, H), FLinearColor(0.0f, 0.65f, 0.76f, 0.32f), [=](FVector2D P)
				{ return FMath::Exp(-FMath::Square(float(FMath::Max(Distance(P), 0.0) / 5.5))); });
		Art.Paint(FVector2D::ZeroVector, FVector2D(W, H), Fill, [=](FVector2D P)
			{ return FMath::Clamp(float(0.5 - Distance(P) * 2), 0.0f, 1.0f) * FMath::Lerp(0.86f, 0.49f, float(P.X / W)); });
		Art.Paint(FVector2D::ZeroVector, FVector2D(W, H), Edge, [=](FVector2D P)
			{ return FMath::Clamp(float((0.75 - FMath::Abs(Distance(P))) * 2), 0.0f, 1.0f); });
		const FVector2D C(Primary ? 80 : 70, H * 0.5f);
		const FLinearColor Ink = Warm ? FLinearColor(1.0f, 0.52f, 0.26f, 0.95f) : FLinearColor(0.82f, 0.92f, 0.93f, Primary ? 1.0f : 0.72f);
		if (Primary)
		{
			for (int32 I = 0; I < 14; ++I) Art.Line(C + FVector2D(I - 6, -10 + I * 0.75f), C + FVector2D(I - 6, 10 - I * 0.75f), 1.5f, Ink);
			Art.Fish(FVector2D(W - 80, H * 0.5f), 1.15f, FLinearColor(0.36f, 0.85f, 0.87f, 0.28f));
			FLinearColor Ornament(0.44f, 0.94f, 0.92f, 0.48f);
			for (float Y : { 19.0f, float(H - 19) }) Art.Line(FVector2D(48, Y), FVector2D(W - 48, Y), 0.7f, Ornament);
			for (float X : { 26.0f, float(W - 26) }) for (float Y : { 28.0f, float(H - 28) })
			{
				const float SX = X < W / 2 ? 1 : -1, SY = Y < H / 2 ? 1 : -1;
				Art.Line(FVector2D(X, Y + SY * 10), FVector2D(X, Y), 0.8f, Ornament);
				Art.Line(FVector2D(X, Y), FVector2D(X + SX * 10, Y), 0.8f, Ornament);
				Art.Ellipse(FVector2D(X + SX * 4, Y + SY * 5), FVector2D(1.5f, 3), Ornament);
				Art.Ellipse(FVector2D(X + SX * 8, Y + SY * 2), FVector2D(3, 1.5f), Ornament);
			}
		}
		else if (Kind == EArtwork::Settings)
		{
			for (int32 I = 0; I < 8; ++I)
			{
				const FVector2D D(FMath::Cos(I * PI / 4), FMath::Sin(I * PI / 4));
				Art.Line(C + D * 9, C + D * 14, 5, Ink);
			}
			Art.Paint(C - FVector2D(12), C + FVector2D(12), Ink, [=](FVector2D P)
				{ return FMath::Min(float(((P - C).Size() - 5.0) * 2), float((11.0 - (P - C).Size()) * 2)); });
		}
		else if (Kind == EArtwork::Laboratory)
		{
			Art.Line(C + FVector2D(-6, -15), C + FVector2D(6, -15), 2, Ink);
			Art.Line(C + FVector2D(-4, -15), C + FVector2D(-4, -4), 2, Ink);
			Art.Line(C + FVector2D(4, -15), C + FVector2D(4, -4), 2, Ink);
			Art.Line(C + FVector2D(-4, -4), C + FVector2D(-14, 13), 2, Ink);
			Art.Line(C + FVector2D(4, -4), C + FVector2D(14, 13), 2, Ink);
			Art.Line(C + FVector2D(-14, 13), C + FVector2D(14, 13), 2, Ink);
			Art.Line(C + FVector2D(-9, 5), C + FVector2D(9, 5), 2, Ink);
		}
		else if (Kind == EArtwork::Wishlist)
		{
			Art.Paint(C - FVector2D(15), C + FVector2D(15), Ink, [=](FVector2D P)
			{
				const FVector2D Q = (P - C) / 12;
				const double X = Q.X, Y = -Q.Y;
				const double V = X * X + Y * Y - 0.7;
				return float(FMath::Clamp(0.5 - (V * V * V - X * X * Y * Y * Y) * 30, 0.0, 1.0));
			});
		}
		else
		{
			Art.Line(C + FVector2D(-3, -13), C + FVector2D(12, -13), 2, Ink);
			Art.Line(C + FVector2D(12, -13), C + FVector2D(12, 13), 2, Ink);
			Art.Line(C + FVector2D(12, 13), C + FVector2D(-3, 13), 2, Ink);
			Art.Line(C + FVector2D(-3, 13), C + FVector2D(-3, 7), 2, Ink);
			Art.Line(C + FVector2D(-13, 0), C + FVector2D(5, 0), 2, Ink);
			Art.Line(C + FVector2D(-1, -6), C + FVector2D(5, 0), 2, Ink);
			Art.Line(C + FVector2D(-1, 6), C + FVector2D(5, 0), 2, Ink);
		}
		return Art.Texture();
	}
}

void UTunaSweeperIntroMenuWidget::ApplyReferenceTitleStyle()
{
	using namespace TunaSweeperTitleStyle;
	UVerticalBox* Stack = Cast<UVerticalBox>(FindIntroWidget(TEXT("MainMenuPanel")));
	if (!Stack || !WidgetTree) return;
	// The authored stack is the nested WBP root, without a canvas slot of its own.
	Stack->SetRenderTranslation(FVector2D(-26, 18));
	auto Apply = [&](UButton* Button, const TCHAR* BoxName, EArtwork Kind, int32 W, int32 H, float Left, float Bottom)
	{
		USizeBox* Box = Cast<USizeBox>(FindIntroWidget(BoxName));
		if (!Button || !Box) return;
		Box->SetWidthOverride(W); Box->SetHeightOverride(H);
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Box->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Left); Slot->SetPadding(FMargin(Left, 0, 0, Bottom));
		}
		const FName Key(BoxName);
		TObjectPtr<UTexture2D>& Texture = TitleMenuStyleTextures.FindOrAdd(Key);
		if (!Texture) Texture = ButtonArtwork(Kind, W, H);
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image; Brush.ImageSize = FVector2D(W, H); Brush.SetResourceObject(Texture);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(Brush);
		Brush.TintColor = FLinearColor(1.24f, 1.24f, 1.24f, 1.0f); Style.SetHovered(Brush);
		Brush.TintColor = FLinearColor(0.78f, 0.78f, 0.78f, 1.0f); Style.SetPressed(Brush);
		Brush.TintColor = FLinearColor(0.6f, 0.6f, 0.6f, 0.5f); Style.SetDisabled(Brush);
		// Reserve both the action icon and the right-hand ornament, including in longer translations.
		const FMargin Padding(Kind == EArtwork::Play ? 110 : 104, 16, Kind == EArtwork::Play ? 94 : 30, 16);
		Style.SetNormalPadding(Padding); Style.SetPressedPadding(Padding + FMargin(0, 1, 0, -1));
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White);
		UTextBlock* Label = Cast<UTextBlock>(FindIntroWidget(FName(*(Button->GetName() + TEXT("Text")))));
		if (Label)
		{
			TunaSweeperUIFont::ApplyFont(Label, Kind == EArtwork::Play ? 28 : 22);
			Label->SetColorAndOpacity(FLinearColor(0.97f, 0.95f, 0.89f));
			Label->SetAutoWrapText(false);
			if (!Cast<UScaleBox>(Button->GetContent()))
			{
				UWidgetTree* Tree = Button->GetTypedOuter<UWidgetTree>();
				UScaleBox* Fit = Tree->ConstructWidget<UScaleBox>();
				Label->RemoveFromParent(); Fit->SetStretch(EStretch::ScaleToFit); Fit->SetStretchDirection(EStretchDirection::DownOnly);
				Fit->SetContent(Label); Button->SetContent(Fit);
			}
			if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Label->Slot))
			{
				TextSlot->SetHorizontalAlignment(Kind == EArtwork::Settings || Kind == EArtwork::Quit || Kind == EArtwork::Save || Kind == EArtwork::Laboratory ? HAlign_Left : HAlign_Center);
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
			if (UButtonSlot* Slot = Cast<UButtonSlot>(Button->GetContent()->Slot))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill); Slot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	};
	Apply(StartButton, TEXT("StartButtonBox"), EArtwork::Play, 454, 112, 0, 4);
	Apply(SlotSelectButton, TEXT("SlotSelectButtonBox"), EArtwork::Save, 418, 98, 12, -4);
	Apply(LaboratoryButton, TEXT("LaboratoryButtonBox"), EArtwork::Laboratory, 418, 98, 12, -4);
	Apply(SettingsButton, TEXT("SettingsButtonBox"), EArtwork::Settings, 418, 98, 12, -4);
	Apply(QuitButton, TEXT("QuitButtonBox"), EArtwork::Quit, 418, 98, 12, 0);
	Apply(SteamDemoWishlistButton, TEXT("SteamDemoWishlistButtonBox"), EArtwork::Wishlist, 418, 94, 12, 0);

	// The divider belongs to the optional Steam action, so distribution switches remove both together.
	if (SteamDemoWishlistButton)
	{
		if (UVerticalBoxSlot* WishlistSlot = Cast<UVerticalBoxSlot>(SteamDemoWishlistButtonContainer->Slot)) WishlistSlot->SetPadding(FMargin(12, 50, 0, 0));
		USizeBox* WishlistBox = Cast<USizeBox>(SteamDemoWishlistButtonContainer);
		UOverlay* Content = WishlistBox ? Cast<UOverlay>(WishlistBox->GetContent()) : nullptr;
		if (!Content && WishlistBox)
		{
			SteamDemoWishlistButton->RemoveFromParent();
			Content = WidgetTree->ConstructWidget<UOverlay>();
			UOverlaySlot* ButtonOverlaySlot = Content->AddChildToOverlay(SteamDemoWishlistButton);
			ButtonOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonOverlaySlot->SetVerticalAlignment(VAlign_Fill);
			WishlistBox->SetContent(Content);
			UImage* Divider = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleWishlistDivider"));
			TObjectPtr<UTexture2D>& Texture = TitleMenuStyleTextures.FindOrAdd(TEXT("Divider"));
			if (!Texture)
			{
				FArtwork Art(350, 28);
				const FLinearColor Ink(0.78f, 0.74f, 0.61f, 0.4f);
				Art.Line(FVector2D(0, 14), FVector2D(142, 14), 0.8f, Ink);
				Art.Line(FVector2D(208, 14), FVector2D(350, 14), 0.8f, Ink);
				Art.Fish(FVector2D(178, 14), 1.0f, Ink);
				Texture = Art.Texture();
			}
			Divider->SetBrushFromTexture(Texture);
			FSlateBrush DividerBrush = Divider->GetBrush(); DividerBrush.ImageSize = FVector2D(350, 28); Divider->SetBrush(DividerBrush);
			Divider->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* DividerSlot = Content->AddChildToOverlay(Divider);
			DividerSlot->SetHorizontalAlignment(HAlign_Center); DividerSlot->SetVerticalAlignment(VAlign_Center);
			Divider->SetRenderTranslation(FVector2D(0, -72));
		}
	}

	// The scrim is part of the menu: dissolve its right edge instead of dividing the scene with a panel.
	if (UWidget* Scrim = FindIntroWidget(TEXT("LeftScrim")))
	{
		TObjectPtr<UTexture2D>& Texture = TitleMenuStyleTextures.FindOrAdd(TEXT("Scrim"));
		if (!Texture)
		{
			FArtwork Art(512, 1);
			Art.Paint(FVector2D::ZeroVector, FVector2D(512, 1), FLinearColor(0.009f, 0.016f, 0.019f, 0.66f), [](FVector2D P)
				{ return 1.0f - FMath::SmoothStep(0.1f, 1.0f, float(P.X / 512)); });
			Texture = Art.Texture();
		}
		FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(Texture);
		if (UImage* Image = Cast<UImage>(Scrim)) { Image->SetBrush(Brush); Image->SetColorAndOpacity(FLinearColor::White); }
		if (UBorder* Border = Cast<UBorder>(Scrim)) { Border->SetBrush(Brush); Border->SetBrushColor(FLinearColor::White); }
		if (UCanvasPanelSlot* ScrimSlot = Cast<UCanvasPanelSlot>(Scrim->Slot))
		{
			ScrimSlot->SetAnchors(FAnchors(0, 0, 0, 1)); ScrimSlot->SetOffsets(FMargin(0, 0, 720, 0));
		}
	}
}
