#pragma once

#include "UI/TunaSweeperGameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperDebuffComponent.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperExtractionProgressWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudDebuffBarWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHousingPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperMapWidget.h"
#include "UI/TunaSweeperMemoWidget.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "UI/TunaSweeperReloadRingWidget.h"
#include "UI/TunaSweeperShopSellPanelWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr int32 InventoryQuickSlotFirstNumber = 3;
	constexpr int32 InventoryQuickSlotLastNumber = 8;
	constexpr float InventoryQuickSlotPanelWidth = 760.0f;
	constexpr float InventoryQuickSlotPanelHeight = 168.0f;
	constexpr float InventoryQuickSlotTileSize = 112.0f;
	constexpr float InventoryQuickSlotTileScale = 1.12f;
	constexpr float CursorDistanceRightOffset = 34.0f;
	constexpr float CursorDistanceBottomOffset = 40.0f;
	constexpr float CursorDistanceTextWidth = 46.0f;
	constexpr int32 CursorDistanceMediumMeters = 5;
	constexpr int32 CursorDistanceFarMeters = 10;
	constexpr float DebuffBarLeftOffset = 24.0f;
	constexpr float DebuffBarBottomOffset = CursorDistanceBottomOffset;
	constexpr float HudWidgetTransitionDurationSeconds = 0.18f;
	constexpr float HudWidgetTransitionDistancePadding = 36.0f;
	constexpr float HudWidgetTransitionFallbackHorizontalDistance = 420.0f;
	constexpr float HudWidgetTransitionFallbackVerticalDistance = 220.0f;
	constexpr float ShopSellPanelTransitionOffsetY = -24.0f;
	constexpr float ShopSellPanelWidth = 330.0f;
	constexpr float ShopSellPanelHeight = 240.0f;
	constexpr float UtilityPanelLeftInset = 34.0f;
	constexpr float UtilityPanelRightInset = 34.0f;
	constexpr float UtilityPanelTopOffset = 96.0f;
	constexpr float UtilityPanelBottomInset = 40.0f;
	constexpr float InventoryAreaPanelWidth = 642.0f;
	constexpr float ItemInfoPanelWidth = 429.0f;
	constexpr float ExternalPanelWidth = 780.0f;
	constexpr float InventoryWeightPanelWidth = 300.0f;
	constexpr float InventoryWeightPanelHeight = 38.0f;
	constexpr float InventoryWeightPanelBottomOffset = 2.0f;
	constexpr float QuestMenuHorizontalMargin = 250.0f;
	constexpr float QuestMenuTopMargin = 112.0f;
	constexpr float QuestMenuBottomMargin = 56.0f;
	constexpr float QuestInteractionHorizontalMargin = 200.0f;
	constexpr float QuestInteractionTopMargin = 76.0f;
	constexpr float QuestInteractionBottomMargin = 56.0f;
	constexpr int32 MaxActiveDamageNumberPopups = 64;
	constexpr float DamageNumberGrowDurationAlpha = 0.14f / 3.0f;
	constexpr float DamageNumberSettleDurationAlpha = 0.28f / 2.0f;
	const FName PistolWeaponTypeTag(TEXT("weapon.type.pistol"));
	const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));

	using TunaSweeperUiText::ResolveUiText;

	FSlateBrush MakeHudRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		float Radius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FPaintGeometry MakeHudLocalBoxGeometry(
		const FGeometry& AllottedGeometry,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		return AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
	}

	bool IsSlateVisibilityShown(ESlateVisibility Visibility)
	{
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}

	FWidgetTransform WithAddedTranslation(const FWidgetTransform& BaseTransform, const FVector2D& AddedTranslation)
	{
		FWidgetTransform Result = BaseTransform;
		Result.Translation = BaseTransform.Translation + AddedTranslation;
		return Result;
	}

	float SmoothTransitionAlpha(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return ClampedAlpha * ClampedAlpha * (3.0f - 2.0f * ClampedAlpha);
	}

	float HeadphoneNoiseHash01(int32 Seed, int32 Index, float Salt)
	{
		const float RawValue = FMath::Sin(
			(static_cast<float>(Seed) + 1.0f) * 12.9898f +
			(static_cast<float>(Index) + 1.0f) * (37.719f + Salt) +
			Salt * 78.233f) * 43758.5453f;
		return RawValue - FMath::FloorToFloat(RawValue);
	}

	float HeadphoneNoiseCenterWeightedSignedUnit(float Value01, float Exponent)
	{
		const float SignedValue = Value01 * 2.0f - 1.0f;
		const float Magnitude = FMath::Pow(FMath::Abs(SignedValue), FMath::Max(1.0f, Exponent));
		return FMath::Sign(SignedValue) * Magnitude;
	}

	float HeadphoneNoiseAngularInfluence(float NormalizedAngle)
	{
		const float ClampedAngle = FMath::Clamp(NormalizedAngle, 0.0f, 1.0f);
		return FMath::Pow(1.0f - FMath::SmoothStep(0.0f, 1.0f, ClampedAngle), 1.35f);
	}

	float EaseOutCubic(float Alpha)
	{
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		const float InverseAlpha = 1.0f - ClampedAlpha;
		return 1.0f - InverseAlpha * InverseAlpha * InverseAlpha;
	}

	FLinearColor GetDamageNumberColor(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
			return FLinearColor(1.0f, 0.78f, 0.08f, 1.0f);
		case ETunaSweeperDamageNumberType::Headshot:
			return FLinearColor(1.0f, 0.18f, 0.06f, 1.0f);
		default:
			return FLinearColor(0.95f, 0.98f, 1.0f, 1.0f);
		}
	}

	float GetDamageNumberFontSize(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
			return 60.0f;
		case ETunaSweeperDamageNumberType::Headshot:
			return 72.0f;
		default:
			return 46.0f;
		}
	}

	int32 GetDamageNumberOutlineSize(ETunaSweeperDamageNumberType DamageNumberType)
	{
		switch (DamageNumberType)
		{
		case ETunaSweeperDamageNumberType::Critical:
		case ETunaSweeperDamageNumberType::Headshot:
			return 4;
		default:
			return 3;
		}
	}

	FString FormatDamageNumber(float DamageAmount)
	{
		const float RoundedDamage = FMath::RoundToFloat(DamageAmount);
		if (FMath::IsNearlyEqual(DamageAmount, RoundedDamage, 0.05f))
		{
			return FString::Printf(TEXT("%d"), FMath::RoundToInt(DamageAmount));
		}

		return FString::Printf(TEXT("%.1f"), DamageAmount);
	}

	FLinearColor GetCursorDistanceTextColor(int32 DistanceMeters)
	{
		if (DistanceMeters >= CursorDistanceFarMeters)
		{
			return FLinearColor(1.0f, 0.20f, 0.14f, 1.0f);
		}
		if (DistanceMeters >= CursorDistanceMediumMeters)
		{
			return FLinearColor(1.0f, 0.58f, 0.16f, 1.0f);
		}
		return FLinearColor::White;
	}

	bool IsPrecisionCrosshairWeaponType(FName WeaponTypeTag)
	{
		return WeaponTypeTag == PistolWeaponTypeTag || WeaponTypeTag == RifleWeaponTypeTag;
	}

	FTunaSweeperItemStackTileData BuildQuickSlotTileData(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperInventorySlot& Slot,
		int32 SlotIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SourceIndex = SlotIndex;
		TileData.SlotReference.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SlotReference.SlotIndex = SlotIndex;
		TileData.bIsEmpty = true;

		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance || !Slot.ItemUid.IsValid() || !TunaGameInstance->TryGetItemInstance(Slot.ItemUid, ItemInstance))
		{
			return TileData;
		}

		TileData.ItemInstance = ItemInstance;
		TileData.ItemStack.ItemId = ItemInstance.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, ItemInstance.Quantity);
		TileData.bIsEmpty = false;
		const ETunaSweeperItemTextLanguage Language = TunaGameInstance
			? TunaGameInstance->GetCurrentTextLanguage()
			: ETunaSweeperItemTextLanguage::English;

		if (!ItemDataSubsystem)
		{
			TileData.DisplayName = FText::Format(
				ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
				FText::AsNumber(ItemInstance.ItemId));
			return TileData;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		if (ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			TileData.ItemDefinition = ItemDefinition;
			TileData.bHasItemDefinition = true;

			FText DisplayName;
			if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, Language, DisplayName))
			{
				TileData.DisplayName = DisplayName;
			}
			else
			{
				TileData.DisplayName = FText::Format(
					ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
					FText::AsNumber(ItemInstance.ItemId));
			}

			const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
			if (!IconObjectPath.IsEmpty())
			{
				TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
			}

			FText DescriptionText;
			if (ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, DescriptionText))
			{
				TileData.DescriptionText = DescriptionText;
			}
		}

		return TileData;
	}
}

DECLARE_LOG_CATEGORY_EXTERN(LogTunaSweeperGameHud, Log, All);

