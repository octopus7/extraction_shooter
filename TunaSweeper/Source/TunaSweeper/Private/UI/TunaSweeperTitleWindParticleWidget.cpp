#include "UI/TunaSweeperTitleWindParticleWidget.h"

#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"

namespace TunaSweeperTitleWindParticles
{
	constexpr int32 ParticleCount = 48;
	constexpr int32 ParticleTextureSize = 64;

	float Hash01(int32 Seed, float Salt)
	{
		return FMath::Frac(FMath::Sin((static_cast<float>(Seed) + 1.0f) * (12.9898f + Salt) + Salt * 78.233f) * 43758.5453f);
	}

	FLinearColor GetParticleColor(float Blend, float Alpha)
	{
		const FLinearColor Cyan(0.02f, 0.94f, 0.86f, Alpha);
		const FLinearColor Green(0.22f, 0.92f, 0.36f, Alpha);
		return FMath::Lerp(Cyan, Green, Blend);
	}
}

void UTunaSweeperTitleWindParticleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureParticleTexture();
}

void UTunaSweeperTitleWindParticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AnimationSeconds += FMath::Max(0.0f, InDeltaTime);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UTunaSweeperTitleWindParticleWidget::EnsureParticleTexture()
{
	if (ParticleTexture)
	{
		return;
	}

	constexpr int32 TextureSize = TunaSweeperTitleWindParticles::ParticleTextureSize;
	TArray<uint8> Pixels;
	Pixels.SetNumZeroed(TextureSize * TextureSize * 4);

	const FVector2f Center((TextureSize - 1) * 0.5f, (TextureSize - 1) * 0.5f);
	const float Radius = TextureSize * 0.5f;
	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			const FVector2f PixelCenter(static_cast<float>(X), static_cast<float>(Y));
			const float NormalizedDistance = FVector2f::Distance(PixelCenter, Center) / Radius;
			const float Core = 1.0f - FMath::SmoothStep(0.12f, 0.96f, NormalizedDistance);
			const uint8 Alpha = static_cast<uint8>(FMath::Clamp(Core, 0.0f, 1.0f) * 255.0f);
			const int32 ByteIndex = (Y * TextureSize + X) * 4;
			Pixels[ByteIndex + 0] = 255;
			Pixels[ByteIndex + 1] = 255;
			Pixels[ByteIndex + 2] = 255;
			Pixels[ByteIndex + 3] = Alpha;
		}
	}

	ParticleTexture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8, TEXT("TunaSweeperTitleParticle"), Pixels);
	if (!ParticleTexture)
	{
		return;
	}

	ParticleTexture->CompressionSettings = TC_EditorIcon;
#if WITH_EDITORONLY_DATA
	ParticleTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
	ParticleTexture->LODGroup = TEXTUREGROUP_UI;
	ParticleTexture->SRGB = true;
	ParticleTexture->NeverStream = true;
	ParticleTexture->Filter = TF_Bilinear;
	ParticleTexture->AddressX = TA_Clamp;
	ParticleTexture->AddressY = TA_Clamp;
	ParticleTexture->UpdateResource();

	ParticleBrush.DrawAs = ESlateBrushDrawType::Image;
	ParticleBrush.ImageSize = FVector2D(TextureSize, TextureSize);
	ParticleBrush.TintColor = FSlateColor(FLinearColor::White);
	ParticleBrush.SetResourceObject(ParticleTexture);
}

int32 UTunaSweeperTitleWindParticleWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
	{
		return BaseLayerId;
	}
	if (!ParticleTexture)
	{
		return BaseLayerId;
	}

	for (int32 Index = 0; Index < TunaSweeperTitleWindParticles::ParticleCount; ++Index)
	{
		const float BaseX = TunaSweeperTitleWindParticles::Hash01(Index, 0.13f);
		const float BaseY = TunaSweeperTitleWindParticles::Hash01(Index, 1.71f);
		const float Speed = FMath::Lerp(0.018f, 0.048f, TunaSweeperTitleWindParticles::Hash01(Index, 2.31f));
		const float Depth = TunaSweeperTitleWindParticles::Hash01(Index, 3.37f);
		const float ParticleScale = FMath::Lerp(1.0f, 5.0f, TunaSweeperTitleWindParticles::Hash01(Index, 5.17f));
		const float Travel = FMath::Frac(BaseX + AnimationSeconds * Speed);
		const float SwayPhase = AnimationSeconds * FMath::Lerp(0.7f, 1.65f, Depth) + BaseX * 2.0f * PI;
		const float Sway = FMath::Sin(SwayPhase) * FMath::Lerp(16.0f, 52.0f, Depth);

		const FVector2f Head(
			static_cast<float>(FMath::Lerp(-140.0, LocalSize.X + 140.0, Travel)),
			static_cast<float>(BaseY * LocalSize.Y + Sway - Travel * 110.0f));
		const float Diameter = FMath::Lerp(3.0f, 8.0f, Depth) * ParticleScale;
		const FVector2D ParticleSize(Diameter, Diameter);
		const FVector2D ParticlePosition(
			static_cast<double>(Head.X - Diameter * 0.5f),
			static_cast<double>(Head.Y - Diameter * 0.5f));

		const float Pulse = 0.72f + 0.28f * FMath::Sin(SwayPhase * 1.7f);
		const float Alpha = FMath::Lerp(0.16f, 0.42f, Depth) * Pulse;
		const FLinearColor ParticleColor = TunaSweeperTitleWindParticles::GetParticleColor(
			TunaSweeperTitleWindParticles::Hash01(Index, 4.03f),
			Alpha);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			BaseLayerId + 1,
			AllottedGeometry.ToPaintGeometry(ParticleSize, FSlateLayoutTransform(ParticlePosition)),
			&ParticleBrush,
			ESlateDrawEffect::None,
			ParticleColor);
	}

	return BaseLayerId + 1;
}
