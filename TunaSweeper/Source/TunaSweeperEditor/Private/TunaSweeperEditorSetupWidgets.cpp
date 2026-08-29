#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
	void RegisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget)
	{
		if (!WidgetBlueprint || !Widget)
		{
			return;
		}

		Widget->bIsVariable = true;
		if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			WidgetBlueprint->OnVariableAdded(Widget->GetFName());
		}
	}

	void UnregisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, const FName& VariableName)
	{
		if (WidgetBlueprint && WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(VariableName))
		{
			WidgetBlueprint->OnVariableRemoved(VariableName);
		}
	}

	void SyncWidgetVariableGuidsToSource(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint)
		{
			return;
		}

		TSet<FName> SourceVariableNames;
		WidgetBlueprint->ForEachSourceWidget([&SourceVariableNames](UWidget* Widget)
		{
			if (Widget)
			{
				SourceVariableNames.Add(Widget->GetFName());
			}
		});

		for (UWidgetAnimation* Animation : WidgetBlueprint->Animations)
		{
			if (Animation)
			{
				SourceVariableNames.Add(Animation->GetFName());
			}
		}

		bool bModified = false;
		auto ModifyIfNeeded = [&WidgetBlueprint, &bModified]()
		{
			if (!bModified)
			{
				WidgetBlueprint->Modify();
				bModified = true;
			}
		};

		TSet<FGuid> SeenGuids;
		for (auto It = WidgetBlueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!SourceVariableNames.Contains(It.Key()))
			{
				ModifyIfNeeded();
				It.RemoveCurrent();
				continue;
			}

			if (!It.Value().IsValid() || SeenGuids.Contains(It.Value()))
			{
				ModifyIfNeeded();
				It.Value() = FGuid::NewGuid();
			}

			SeenGuids.Add(It.Value());
		}

		for (const FName& SourceVariableName : SourceVariableNames)
		{
			if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(SourceVariableName))
			{
				ModifyIfNeeded();
				WidgetBlueprint->WidgetVariableNameToGuidMap.Add(SourceVariableName, FGuid::NewGuid());
			}
		}
	}

	void RegisterAllWidgetsInTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> Widgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			RegisterWidgetVariable(WidgetBlueprint, Widget);
		}

		SyncWidgetVariableGuidsToSource(WidgetBlueprint);
	}

	void ClearWidgetTreeForRebuild(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> ExistingWidgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(ExistingWidgets);

		TArray<FName> ExistingVariableNames;
		WidgetBlueprint->WidgetVariableNameToGuidMap.GenerateKeyArray(ExistingVariableNames);
		for (const FName& ExistingVariableName : ExistingVariableNames)
		{
			UnregisterWidgetVariable(WidgetBlueprint, ExistingVariableName);
		}
		WidgetBlueprint->WidgetVariableNameToGuidMap.Empty();

		if (WidgetBlueprint->WidgetTree->RootWidget)
		{
			WidgetBlueprint->WidgetTree->RemoveWidget(WidgetBlueprint->WidgetTree->RootWidget);
			WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		}

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (ExistingWidget)
			{
				ExistingWidget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
	}

	void ConfigureTextBlock(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TunaSweeperUIFont::ApplyFont(TextBlock, FontSize);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(ETextJustify::Center);
	}

	void ConfigureTextBlockLeft(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize)
	{
		ConfigureTextBlock(TextBlock, Text, Color, FontSize);
		if (TextBlock)
		{
			TextBlock->SetJustification(ETextJustify::Left);
		}
	}

	FSlateBrush MakeRoundedBoxBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(5.0f, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float CornerRadius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(CornerRadius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush MakeCircularBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FScrollBarStyle MakeItemContainerScrollBarStyle()
	{
		const FSlateBrush TrackBrush = MakeRoundedBoxBrush(
			FVector2D(8.0f, 8.0f),
			FLinearColor(0.012f, 0.016f, 0.018f, 0.18f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f,
			3.0f);
		const FSlateBrush ThumbBrush = MakeRoundedBoxBrush(
			FVector2D(8.0f, 8.0f),
			FLinearColor(0.34f, 0.42f, 0.45f, 0.42f),
			FLinearColor(0.55f, 0.68f, 0.72f, 0.20f),
			0.8f,
			3.0f);
		const FSlateBrush HoveredThumbBrush = MakeRoundedBoxBrush(
			FVector2D(8.0f, 8.0f),
			FLinearColor(0.46f, 0.56f, 0.60f, 0.66f),
			FLinearColor(0.70f, 0.82f, 0.86f, 0.34f),
			1.0f,
			3.0f);
		const FSlateBrush DraggedThumbBrush = MakeRoundedBoxBrush(
			FVector2D(8.0f, 8.0f),
			FLinearColor(0.54f, 0.66f, 0.70f, 0.78f),
			FLinearColor(0.82f, 0.92f, 0.96f, 0.48f),
			1.0f,
			3.0f);

		FScrollBarStyle ScrollBarStyle;
		ScrollBarStyle.SetHorizontalBackgroundImage(TrackBrush)
			.SetVerticalBackgroundImage(TrackBrush)
			.SetVerticalTopSlotImage(TrackBrush)
			.SetVerticalBottomSlotImage(TrackBrush)
			.SetHorizontalTopSlotImage(TrackBrush)
			.SetHorizontalBottomSlotImage(TrackBrush)
			.SetNormalThumbImage(ThumbBrush)
			.SetHoveredThumbImage(HoveredThumbBrush)
			.SetDraggedThumbImage(DraggedThumbBrush)
			.SetThickness(8.0f);
		return ScrollBarStyle;
	}

	void ApplyItemContainerScrollBarStyle(UTileView* TileView)
	{
		if (!TileView)
		{
			return;
		}

		static const FName ScrollBarStylePropertyName(TEXT("ScrollBarStyle"));
		if (FStructProperty* ScrollBarStyleProperty =
			FindFProperty<FStructProperty>(UListView::StaticClass(), ScrollBarStylePropertyName))
		{
			if (ScrollBarStyleProperty->Struct == FScrollBarStyle::StaticStruct())
			{
				if (FScrollBarStyle* ScrollBarStyle =
					ScrollBarStyleProperty->ContainerPtrToValuePtr<FScrollBarStyle>(TileView))
				{
					*ScrollBarStyle = MakeItemContainerScrollBarStyle();
				}
			}
		}

		TileView->SetScrollBarPadding(FMargin(8.0f, 0.0f, 2.0f, 0.0f));
	}

	bool BuildIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UVerticalBox* MenuStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuStack"));
		UVerticalBox* MainMenuPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuPanel"));
		UHorizontalBox* MainMenuRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainMenuRow"));
		USizeBox* CurrentSaveSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CurrentSaveSlotBox"));
		UBorder* CurrentSaveSlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrentSaveSlotBorder"));
		UTextBlock* CurrentSaveSlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentSaveSlotText"));
		USizeBox* StartButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonBox"));
		UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
		USizeBox* SlotSelectButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSelectButtonBox"));
		UButton* SlotSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotSelectButton"));
		UTextBlock* SlotSelectButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotSelectButtonText"));
		UVerticalBox* SaveSlotPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotPanel"));
		UHorizontalBox* SaveSlotButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveSlotButtonRow"));
		USizeBox* SaveSlot1ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot1ButtonBox"));
		UButton* SaveSlot1Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot1Button"));
		UTextBlock* SaveSlot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot1Text"));
		USizeBox* SaveSlot2ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot2ButtonBox"));
		UButton* SaveSlot2Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot2Button"));
		UTextBlock* SaveSlot2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot2Text"));
		USizeBox* SaveSlot3ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot3ButtonBox"));
		UButton* SaveSlot3Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot3Button"));
		UTextBlock* SaveSlot3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot3Text"));
		UHorizontalBox* SaveSlotActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveSlotActionRow"));
		USizeBox* PrimarySaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrimarySaveSlotButtonBox"));
		UButton* PrimarySaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PrimarySaveSlotButton"));
		UTextBlock* PrimarySaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimarySaveSlotButtonText"));
		USizeBox* DeleteSaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeleteSaveSlotButtonBox"));
		UButton* DeleteSaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteSaveSlotButton"));
		UTextBlock* DeleteSaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteSaveSlotButtonText"));
		USizeBox* QuitButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuitButtonBox"));
		UButton* QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
		UTextBlock* QuitButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitButtonText"));

		if (!RootCanvas || !MenuStack || !MainMenuPanel || !MainMenuRow || !CurrentSaveSlotBox ||
			!CurrentSaveSlotBorder || !CurrentSaveSlotText || !StartButtonBox || !StartButton || !StartButtonText ||
			!SlotSelectButtonBox || !SlotSelectButton || !SlotSelectButtonText ||
			!SaveSlotPanel || !SaveSlotButtonRow || !SaveSlot1ButtonBox || !SaveSlot1Button || !SaveSlot1Text ||
			!SaveSlot2ButtonBox || !SaveSlot2Button || !SaveSlot2Text || !SaveSlot3ButtonBox || !SaveSlot3Button ||
			!SaveSlot3Text || !SaveSlotActionRow || !PrimarySaveSlotButtonBox || !PrimarySaveSlotButton ||
			!PrimarySaveSlotButtonText || !DeleteSaveSlotButtonBox || !DeleteSaveSlotButton || !DeleteSaveSlotButtonText ||
			!QuitButtonBox || !QuitButton || !QuitButtonText)
		{
			return false;
		}

		auto ConfigureMenuButton = [](UButton* Button, const FVector2D& ButtonSize, const FLinearColor& FillColor, const FLinearColor& HoveredColor)
		{
			if (!Button)
			{
				return;
			}

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.78f, 0.84f, 0.90f, 0.95f), 1.5f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), 2.0f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.78f, FLinearColor(0.68f, 0.75f, 0.84f, 1.0f), 1.0f));
			ButtonStyle.SetNormalPadding(FMargin(10.0f, 4.0f));
			ButtonStyle.SetPressedPadding(FMargin(10.0f, 5.0f, 10.0f, 3.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* MenuStackSlot = RootCanvas->AddChildToCanvas(MenuStack);
		if (MenuStackSlot)
		{
			MenuStackSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			MenuStackSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MenuStackSlot->SetPosition(FVector2D(0.0f, 80.0f));
			MenuStackSlot->SetSize(FVector2D(920.0f, 340.0f));
		}

		CurrentSaveSlotBox->SetWidthOverride(330.0f);
		CurrentSaveSlotBox->SetHeightOverride(120.0f);
		CurrentSaveSlotBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(330.0f, 120.0f),
			FLinearColor(0.035f, 0.045f, 0.052f, 0.96f),
			FLinearColor(0.28f, 0.34f, 0.38f, 1.0f),
			1.2f));
		CurrentSaveSlotBorder->SetPadding(FMargin(16.0f, 10.0f));
		ConfigureTextBlock(
			CurrentSaveSlotText,
			FText::FromString(TEXT("\uC2AC\uB86F 1\n\uBE48 \uC2AC\uB86F")),
			FLinearColor(0.82f, 0.88f, 0.92f, 1.0f),
			19);
		CurrentSaveSlotText->SetAutoWrapText(true);
		CurrentSaveSlotBorder->SetContent(CurrentSaveSlotText);
		CurrentSaveSlotBox->SetContent(CurrentSaveSlotBorder);

		StartButtonBox->SetWidthOverride(230.0f);
		StartButtonBox->SetHeightOverride(120.0f);
		StartButtonBox->SetContent(StartButton);
		ConfigureMenuButton(
			StartButton,
			FVector2D(230.0f, 120.0f),
			FLinearColor(0.10f, 0.18f, 0.22f, 0.96f),
			FLinearColor(0.14f, 0.27f, 0.33f, 0.98f));
		ConfigureTextBlock(StartButtonText, FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30")), FLinearColor::White, 26);
		StartButton->SetContent(StartButtonText);

		SlotSelectButtonBox->SetWidthOverride(210.0f);
		SlotSelectButtonBox->SetHeightOverride(120.0f);
		SlotSelectButtonBox->SetContent(SlotSelectButton);
		ConfigureMenuButton(
			SlotSelectButton,
			FVector2D(210.0f, 120.0f),
			FLinearColor(0.055f, 0.075f, 0.085f, 0.96f),
			FLinearColor(0.10f, 0.16f, 0.19f, 0.98f));
		ConfigureTextBlock(SlotSelectButtonText, FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 24);
		SlotSelectButton->SetContent(SlotSelectButtonText);

		for (UWidget* MainMenuItem : { static_cast<UWidget*>(CurrentSaveSlotBox), static_cast<UWidget*>(StartButtonBox), static_cast<UWidget*>(SlotSelectButtonBox) })
		{
			UHorizontalBoxSlot* MainMenuItemSlot = MainMenuRow->AddChildToHorizontalBox(MainMenuItem);
			if (MainMenuItemSlot)
			{
				MainMenuItemSlot->SetHorizontalAlignment(HAlign_Center);
				MainMenuItemSlot->SetVerticalAlignment(VAlign_Center);
				MainMenuItemSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* MainMenuRowSlot = MainMenuPanel->AddChildToVerticalBox(MainMenuRow);
		if (MainMenuRowSlot)
		{
			MainMenuRowSlot->SetHorizontalAlignment(HAlign_Center);
			MainMenuRowSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* MainMenuPanelSlot = MenuStack->AddChildToVerticalBox(MainMenuPanel);
		if (MainMenuPanelSlot)
		{
			MainMenuPanelSlot->SetHorizontalAlignment(HAlign_Center);
			MainMenuPanelSlot->SetVerticalAlignment(VAlign_Center);
			MainMenuPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		auto ConfigureSaveSlotButton = [&ConfigureMenuButton](USizeBox* ButtonBox, UButton* Button, UTextBlock* TextBlock, int32 SlotIndex)
		{
			ButtonBox->SetWidthOverride(260.0f);
			ButtonBox->SetHeightOverride(120.0f);
			ButtonBox->SetContent(Button);
			ConfigureMenuButton(
				Button,
				FVector2D(260.0f, 120.0f),
				FLinearColor(0.055f, 0.075f, 0.085f, 0.96f),
				FLinearColor(0.10f, 0.16f, 0.19f, 0.98f));
			ConfigureTextBlock(
				TextBlock,
				FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F"), SlotIndex)),
				FLinearColor(0.82f, 0.88f, 0.92f, 1.0f),
				18);
			TextBlock->SetAutoWrapText(true);
			Button->SetContent(TextBlock);
		};

		ConfigureSaveSlotButton(SaveSlot1ButtonBox, SaveSlot1Button, SaveSlot1Text, 1);
		ConfigureSaveSlotButton(SaveSlot2ButtonBox, SaveSlot2Button, SaveSlot2Text, 2);
		ConfigureSaveSlotButton(SaveSlot3ButtonBox, SaveSlot3Button, SaveSlot3Text, 3);

		for (USizeBox* SlotButtonBox : { SaveSlot1ButtonBox, SaveSlot2ButtonBox, SaveSlot3ButtonBox })
		{
			UHorizontalBoxSlot* SlotButtonSlot = SaveSlotButtonRow->AddChildToHorizontalBox(SlotButtonBox);
			if (SlotButtonSlot)
			{
				SlotButtonSlot->SetHorizontalAlignment(HAlign_Center);
				SlotButtonSlot->SetVerticalAlignment(VAlign_Center);
				SlotButtonSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* SaveSlotButtonRowSlot = SaveSlotPanel->AddChildToVerticalBox(SaveSlotButtonRow);
		if (SaveSlotButtonRowSlot)
		{
			SaveSlotButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
			SaveSlotButtonRowSlot->SetVerticalAlignment(VAlign_Center);
		}

		PrimarySaveSlotButtonBox->SetWidthOverride(250.0f);
		PrimarySaveSlotButtonBox->SetHeightOverride(56.0f);
		PrimarySaveSlotButtonBox->SetContent(PrimarySaveSlotButton);
		ConfigureMenuButton(
			PrimarySaveSlotButton,
			FVector2D(250.0f, 56.0f),
			FLinearColor(0.10f, 0.18f, 0.22f, 0.96f),
			FLinearColor(0.14f, 0.27f, 0.33f, 0.98f));
		ConfigureTextBlock(
			PrimarySaveSlotButtonText,
			FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD")),
			FLinearColor::White,
			22);
		PrimarySaveSlotButton->SetContent(PrimarySaveSlotButtonText);

		DeleteSaveSlotButtonBox->SetWidthOverride(190.0f);
		DeleteSaveSlotButtonBox->SetHeightOverride(56.0f);
		DeleteSaveSlotButtonBox->SetContent(DeleteSaveSlotButton);
		ConfigureMenuButton(
			DeleteSaveSlotButton,
			FVector2D(190.0f, 56.0f),
			FLinearColor(0.42f, 0.045f, 0.04f, 0.96f),
			FLinearColor(0.62f, 0.07f, 0.06f, 0.98f));
		ConfigureTextBlock(
			DeleteSaveSlotButtonText,
			FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30")),
			FLinearColor::White,
			22);
		DeleteSaveSlotButton->SetContent(DeleteSaveSlotButtonText);

		UHorizontalBoxSlot* PrimaryActionSlot = SaveSlotActionRow->AddChildToHorizontalBox(PrimarySaveSlotButtonBox);
		if (PrimaryActionSlot)
		{
			PrimaryActionSlot->SetHorizontalAlignment(HAlign_Center);
			PrimaryActionSlot->SetVerticalAlignment(VAlign_Center);
			PrimaryActionSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		}

		UHorizontalBoxSlot* DeleteActionSlot = SaveSlotActionRow->AddChildToHorizontalBox(DeleteSaveSlotButtonBox);
		if (DeleteActionSlot)
		{
			DeleteActionSlot->SetHorizontalAlignment(HAlign_Center);
			DeleteActionSlot->SetVerticalAlignment(VAlign_Center);
		}
		SaveSlotActionRow->SetVisibility(ESlateVisibility::Collapsed);

		UVerticalBoxSlot* ActionRowSlot = SaveSlotPanel->AddChildToVerticalBox(SaveSlotActionRow);
		if (ActionRowSlot)
		{
			ActionRowSlot->SetHorizontalAlignment(HAlign_Center);
			ActionRowSlot->SetVerticalAlignment(VAlign_Center);
			ActionRowSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
		}

		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* SaveSlotPanelSlot = MenuStack->AddChildToVerticalBox(SaveSlotPanel);
		if (SaveSlotPanelSlot)
		{
			SaveSlotPanelSlot->SetHorizontalAlignment(HAlign_Center);
			SaveSlotPanelSlot->SetVerticalAlignment(VAlign_Center);
			SaveSlotPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		QuitButtonBox->SetWidthOverride(220.0f);
		QuitButtonBox->SetHeightOverride(54.0f);
		QuitButtonBox->SetContent(QuitButton);
		ConfigureMenuButton(
			QuitButton,
			FVector2D(220.0f, 54.0f),
			FLinearColor(0.055f, 0.065f, 0.075f, 0.92f),
			FLinearColor(0.12f, 0.14f, 0.16f, 0.96f));
		ConfigureTextBlock(QuitButtonText, FText::FromString(TEXT("\uC885\uB8CC")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 20);
		QuitButton->SetContent(QuitButtonText);

		UVerticalBoxSlot* QuitSlot = MenuStack->AddChildToVerticalBox(QuitButtonBox);
		if (QuitSlot)
		{
			QuitSlot->SetHorizontalAlignment(HAlign_Center);
			QuitSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildTitleIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UTexture2D* LogoTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UITitleTextureAssetPath, TitleLogoTextureAssetName));

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UBorder* LeftScrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftScrim"));
		UImage* LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoImage"));
		UVerticalBox* MainMenuPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuPanel"));
		USizeBox* StartButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonBox"));
		UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
		USizeBox* CurrentSaveSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CurrentSaveSlotBox"));
		UBorder* CurrentSaveSlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrentSaveSlotBorder"));
		UTextBlock* CurrentSaveSlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentSaveSlotText"));
		USizeBox* SlotSelectButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSelectButtonBox"));
		UButton* SlotSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotSelectButton"));
		UTextBlock* SlotSelectButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotSelectButtonText"));
		USizeBox* SettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsButtonBox"));
		UButton* SettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsButton"));
		UTextBlock* SettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsButtonText"));
		USizeBox* CreditsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsButtonBox"));
		UButton* CreditsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreditsButton"));
		UTextBlock* CreditsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsButtonText"));
		USizeBox* QuitButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuitButtonBox"));
		UButton* QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
		UTextBlock* QuitButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitButtonText"));
		UCanvasPanel* SaveSlotPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SaveSlotPanel"));
		UBorder* SaveSlotBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotBackdrop"));
		UBorder* SaveSlotContentBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotContentBackground"));
		UVerticalBox* SaveSlotContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotContentStack"));
		UTextBlock* SaveSlotPanelTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlotPanelTitleText"));
		USizeBox* SaveSlot1ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot1ButtonBox"));
		UButton* SaveSlot1Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot1Button"));
		UTextBlock* SaveSlot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot1Text"));
		USizeBox* SaveSlot2ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot2ButtonBox"));
		UButton* SaveSlot2Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot2Button"));
		UTextBlock* SaveSlot2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot2Text"));
		USizeBox* SaveSlot3ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot3ButtonBox"));
		UButton* SaveSlot3Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot3Button"));
		UTextBlock* SaveSlot3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot3Text"));
		UVerticalBox* SaveSlotActionRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotActionRow"));
		USizeBox* PrimarySaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrimarySaveSlotButtonBox"));
		UButton* PrimarySaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PrimarySaveSlotButton"));
		UTextBlock* PrimarySaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimarySaveSlotButtonText"));
		USizeBox* DeleteSaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeleteSaveSlotButtonBox"));
		UButton* DeleteSaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteSaveSlotButton"));
		UTextBlock* DeleteSaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteSaveSlotButtonText"));
		UOverlay* DeleteSaveSlotButtonContent = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DeleteSaveSlotButtonContent"));
		UImage* DeleteSaveSlotHoldProgressFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeleteSaveSlotHoldProgressFill"));
		USizeBox* BackToMainMenuButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackToMainMenuButtonBox"));
		UButton* BackToMainMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackToMainMenuButton"));
		UTextBlock* BackToMainMenuButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackToMainMenuButtonText"));
		UBorder* DeleteConfirmPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeleteConfirmPanel"));
		UVerticalBox* DeleteConfirmStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeleteConfirmStack"));
		UTextBlock* DeleteConfirmTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteConfirmTitleText"));
		UTextBlock* DeleteConfirmMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteConfirmMessageText"));
		UHorizontalBox* DeleteConfirmButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DeleteConfirmButtonRow"));
		USizeBox* ConfirmDeleteButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmDeleteButtonBox"));
		UButton* ConfirmDeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmDeleteButton"));
		UTextBlock* ConfirmDeleteButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmDeleteButtonText"));
		USizeBox* CancelDeleteButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CancelDeleteButtonBox"));
		UButton* CancelDeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelDeleteButton"));
		UTextBlock* CancelDeleteButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelDeleteButtonText"));
		UCanvasPanel* SettingsPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsPanel"));
		UBorder* SettingsBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackdrop"));
		UBorder* SettingsContentBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsContentBackground"));
		UVerticalBox* SettingsContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContentStack"));
		UTextBlock* SettingsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitleText"));
		UTextBlock* SettingsStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsStatusText"));
		UHorizontalBox* SettingsTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsTabRow"));
		USizeBox* GraphicsTabButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GraphicsTabButtonBox"));
		UButton* SettingsGraphicsTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsGraphicsTabButton"));
		UTextBlock* SettingsGraphicsTabButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsGraphicsTabButtonText"));
		USizeBox* InterfaceTabButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InterfaceTabButtonBox"));
		UButton* SettingsInterfaceTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsInterfaceTabButton"));
		UTextBlock* SettingsInterfaceTabButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsInterfaceTabButtonText"));
		USizeBox* DevelopmentTabButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DevelopmentTabButtonBox"));
		UButton* SettingsDevelopmentTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsDevelopmentTabButton"));
		UTextBlock* SettingsDevelopmentTabButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsDevelopmentTabButtonText"));
		UVerticalBox* GraphicsSettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsPanel"));
		UVerticalBox* InterfaceSettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InterfaceSettingsPanel"));
		UVerticalBox* DevelopmentSettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DevelopmentSettingsPanel"));
		UBorder* EnemyCombatDebugSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemyCombatDebugSection"));
		UVerticalBox* EnemyCombatDebugSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EnemyCombatDebugSectionStack"));
		UTextBlock* EnemyCombatDebugLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyCombatDebugLabelText"));
		USizeBox* EnemyCombatDebugToggleButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EnemyCombatDebugToggleButtonBox"));
		UButton* EnemyCombatDebugToggleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EnemyCombatDebugToggleButton"));
		UTextBlock* EnemyCombatDebugToggleButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyCombatDebugToggleButtonText"));
		UBorder* DebugDisplayLanguageSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DebugDisplayLanguageSection"));
		UVerticalBox* DebugDisplayLanguageSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DebugDisplayLanguageSectionStack"));
		UTextBlock* DebugDisplayLanguageLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DebugDisplayLanguageLabelText"));
		UVerticalBox* DebugDisplayLanguageButtonStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DebugDisplayLanguageButtonStack"));
		USizeBox* DebugDisplayLanguageKoreanButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DebugDisplayLanguageKoreanButtonBox"));
		UButton* DebugDisplayLanguageKoreanButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DebugDisplayLanguageKoreanButton"));
		UTextBlock* DebugDisplayLanguageKoreanButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DebugDisplayLanguageKoreanButtonText"));
		USizeBox* DebugDisplayLanguageEnglishButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DebugDisplayLanguageEnglishButtonBox"));
		UButton* DebugDisplayLanguageEnglishButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DebugDisplayLanguageEnglishButton"));
		UTextBlock* DebugDisplayLanguageEnglishButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DebugDisplayLanguageEnglishButtonText"));
		UBorder* SettingsWindowModeSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsWindowModeSection"));
		UVerticalBox* SettingsWindowModeSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsWindowModeSectionStack"));
		UTextBlock* WindowModeLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WindowModeLabelText"));
		UHorizontalBox* WindowModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WindowModeRow"));
		USizeBox* WindowedModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowedModeButtonBox"));
		UButton* WindowedModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WindowedModeButton"));
		UTextBlock* WindowedModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WindowedModeButtonText"));
		USizeBox* BorderlessWindowModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BorderlessWindowModeButtonBox"));
		UButton* BorderlessWindowModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BorderlessWindowModeButton"));
		UTextBlock* BorderlessWindowModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BorderlessWindowModeButtonText"));
		USizeBox* FullscreenModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FullscreenModeButtonBox"));
		UButton* FullscreenModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FullscreenModeButton"));
		UTextBlock* FullscreenModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FullscreenModeButtonText"));
		UBorder* SettingsResolutionSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsResolutionSection"));
		UVerticalBox* SettingsResolutionSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsResolutionSectionStack"));
		UTextBlock* ResolutionLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResolutionLabelText"));
		UVerticalBox* ResolutionButtonStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResolutionButtonStack"));
		USizeBox* Resolution1280ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1280ButtonBox"));
		UButton* Resolution1280Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1280Button"));
		UTextBlock* Resolution1280ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1280ButtonText"));
		USizeBox* Resolution1600ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1600ButtonBox"));
		UButton* Resolution1600Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1600Button"));
		UTextBlock* Resolution1600ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1600ButtonText"));
		USizeBox* Resolution1920ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1920ButtonBox"));
		UButton* Resolution1920Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1920Button"));
		UTextBlock* Resolution1920ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1920ButtonText"));
		USizeBox* Resolution2560ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution2560ButtonBox"));
		UButton* Resolution2560Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution2560Button"));
		UTextBlock* Resolution2560ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution2560ButtonText"));
		USizeBox* Resolution3840ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution3840ButtonBox"));
		UButton* Resolution3840Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution3840Button"));
		UTextBlock* Resolution3840ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution3840ButtonText"));
		UBorder* SettingsDLSSSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsDLSSSection"));
		UVerticalBox* SettingsDLSSSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsDLSSSectionStack"));
		UTextBlock* DLSSLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSLabelText"));
		UHorizontalBox* DLSSButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DLSSButtonRow"));
		USizeBox* DLSSOffButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSOffButtonBox"));
		UButton* DLSSOffButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSOffButton"));
		UTextBlock* DLSSOffButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSOffButtonText"));
		USizeBox* DLSSQualityButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSQualityButtonBox"));
		UButton* DLSSQualityButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSQualityButton"));
		UTextBlock* DLSSQualityButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSQualityButtonText"));
		USizeBox* DLSSBalancedButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSBalancedButtonBox"));
		UButton* DLSSBalancedButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSBalancedButton"));
		UTextBlock* DLSSBalancedButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSBalancedButtonText"));
		USizeBox* DLSSPerformanceButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSPerformanceButtonBox"));
		UButton* DLSSPerformanceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSPerformanceButton"));
		UTextBlock* DLSSPerformanceButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSPerformanceButtonText"));
		USizeBox* BackFromSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackFromSettingsButtonBox"));
		UButton* BackFromSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackFromSettingsButton"));
		UTextBlock* BackFromSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackFromSettingsButtonText"));
		UBorder* SettingsLanguageSection = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsLanguageSection"));
		UVerticalBox* SettingsLanguageSectionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsLanguageSectionStack"));
		UTextBlock* LanguageLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageLabelText"));
		UVerticalBox* LanguageButtonStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LanguageButtonStack"));
		USizeBox* LanguageEnglishButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageEnglishButtonBox"));
		UButton* LanguageEnglishButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageEnglishButton"));
		UTextBlock* LanguageEnglishButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageEnglishButtonText"));
		USizeBox* LanguageKoreanButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageKoreanButtonBox"));
		UButton* LanguageKoreanButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageKoreanButton"));
		UTextBlock* LanguageKoreanButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageKoreanButtonText"));
		USizeBox* LanguageJapaneseButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageJapaneseButtonBox"));
		UButton* LanguageJapaneseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageJapaneseButton"));
		UTextBlock* LanguageJapaneseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageJapaneseButtonText"));
		UHorizontalBox* InterfaceActionButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InterfaceActionButtonRow"));
		USizeBox* ConfirmInterfaceSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmInterfaceSettingsButtonBox"));
		UButton* ConfirmInterfaceSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmInterfaceSettingsButton"));
		UTextBlock* ConfirmInterfaceSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmInterfaceSettingsButtonText"));
		USizeBox* CancelInterfaceSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CancelInterfaceSettingsButtonBox"));
		UButton* CancelInterfaceSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelInterfaceSettingsButton"));
		UTextBlock* CancelInterfaceSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelInterfaceSettingsButtonText"));
		UCanvasPanel* CreditsPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CreditsPanel"));
		UBorder* CreditsBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreditsBackdrop"));
		UVerticalBox* CreditsContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsContentStack"));
		UTextBlock* CreditsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsTitleText"));
		UHorizontalBox* CreditsColumnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreditsColumnRow"));
		USizeBox* CreditsScrollBoxFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame"));
		UScrollBox* CreditsScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox"));
		UTextBlock* CreditsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText"));
		USizeBox* CreditsScrollBoxFrame2 = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame2"));
		UScrollBox* CreditsScrollBox2 = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox2"));
		UTextBlock* CreditsText2 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText2"));
		USizeBox* CreditsScrollBoxFrame3 = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame3"));
		UScrollBox* CreditsScrollBox3 = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox3"));
		UTextBlock* CreditsText3 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText3"));
		USizeBox* BackFromCreditsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackFromCreditsButtonBox"));
		UButton* BackFromCreditsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackFromCreditsButton"));
		UTextBlock* BackFromCreditsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackFromCreditsButtonText"));
		UTextBlock* VersionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VersionText"));

		if (!RootCanvas || !LeftScrim || !LogoImage || !MainMenuPanel || !StartButtonBox ||
			!StartButton || !StartButtonText || !CurrentSaveSlotBox || !CurrentSaveSlotBorder || !CurrentSaveSlotText ||
			!SlotSelectButtonBox || !SlotSelectButton || !SlotSelectButtonText || !SettingsButtonBox || !SettingsButton ||
			!SettingsButtonText || !CreditsButtonBox || !CreditsButton || !CreditsButtonText || !QuitButtonBox ||
			!QuitButton || !QuitButtonText || !SaveSlotPanel || !SaveSlotBackdrop || !SaveSlotContentBackground ||
			!SaveSlotContentStack || !SaveSlotPanelTitleText || !SaveSlot1ButtonBox || !SaveSlot1Button || !SaveSlot1Text ||
			!SaveSlot2ButtonBox || !SaveSlot2Button || !SaveSlot2Text || !SaveSlot3ButtonBox || !SaveSlot3Button ||
			!SaveSlot3Text || !SaveSlotActionRow || !PrimarySaveSlotButtonBox ||
			!PrimarySaveSlotButton || !PrimarySaveSlotButtonText || !DeleteSaveSlotButtonBox || !DeleteSaveSlotButton ||
			!DeleteSaveSlotButtonText || !DeleteSaveSlotButtonContent || !DeleteSaveSlotHoldProgressFill ||
			!BackToMainMenuButtonBox || !BackToMainMenuButton ||
			!BackToMainMenuButtonText || !DeleteConfirmPanel || !DeleteConfirmStack || !DeleteConfirmTitleText ||
			!DeleteConfirmMessageText || !DeleteConfirmButtonRow || !ConfirmDeleteButtonBox || !ConfirmDeleteButton ||
			!ConfirmDeleteButtonText || !CancelDeleteButtonBox || !CancelDeleteButton || !CancelDeleteButtonText ||
			!SettingsPanel || !SettingsBackdrop || !SettingsContentBackground || !SettingsContentStack ||
			!SettingsTitleText || !SettingsStatusText || !SettingsTabRow || !GraphicsTabButtonBox ||
			!SettingsGraphicsTabButton || !SettingsGraphicsTabButtonText || !InterfaceTabButtonBox ||
			!SettingsInterfaceTabButton || !SettingsInterfaceTabButtonText || !DevelopmentTabButtonBox ||
			!SettingsDevelopmentTabButton || !SettingsDevelopmentTabButtonText || !GraphicsSettingsPanel ||
			!InterfaceSettingsPanel || !DevelopmentSettingsPanel || !EnemyCombatDebugSection ||
			!EnemyCombatDebugSectionStack || !EnemyCombatDebugLabelText || !EnemyCombatDebugToggleButtonBox ||
			!EnemyCombatDebugToggleButton || !EnemyCombatDebugToggleButtonText || !DebugDisplayLanguageSection ||
			!DebugDisplayLanguageSectionStack || !DebugDisplayLanguageLabelText || !DebugDisplayLanguageButtonStack ||
			!DebugDisplayLanguageKoreanButtonBox || !DebugDisplayLanguageKoreanButton || !DebugDisplayLanguageKoreanButtonText ||
			!DebugDisplayLanguageEnglishButtonBox || !DebugDisplayLanguageEnglishButton || !DebugDisplayLanguageEnglishButtonText ||
			!SettingsWindowModeSection || !SettingsWindowModeSectionStack ||
			!WindowModeLabelText || !WindowModeRow ||
			!WindowedModeButtonBox || !WindowedModeButton || !WindowedModeButtonText ||
			!BorderlessWindowModeButtonBox || !BorderlessWindowModeButton || !BorderlessWindowModeButtonText ||
			!FullscreenModeButtonBox || !FullscreenModeButton || !FullscreenModeButtonText ||
			!SettingsResolutionSection || !SettingsResolutionSectionStack ||
			!ResolutionLabelText || !ResolutionButtonStack || !Resolution1280ButtonBox || !Resolution1280Button ||
			!Resolution1280ButtonText || !Resolution1600ButtonBox || !Resolution1600Button || !Resolution1600ButtonText ||
			!Resolution1920ButtonBox || !Resolution1920Button || !Resolution1920ButtonText || !Resolution2560ButtonBox ||
			!Resolution2560Button || !Resolution2560ButtonText || !Resolution3840ButtonBox || !Resolution3840Button ||
			!Resolution3840ButtonText || !SettingsDLSSSection || !SettingsDLSSSectionStack ||
			!DLSSLabelText || !DLSSButtonRow || !DLSSOffButtonBox || !DLSSOffButton ||
			!DLSSOffButtonText || !DLSSQualityButtonBox || !DLSSQualityButton || !DLSSQualityButtonText ||
			!DLSSBalancedButtonBox || !DLSSBalancedButton || !DLSSBalancedButtonText ||
			!DLSSPerformanceButtonBox || !DLSSPerformanceButton || !DLSSPerformanceButtonText ||
			!BackFromSettingsButtonBox || !BackFromSettingsButton ||
			!BackFromSettingsButtonText || !SettingsLanguageSection || !SettingsLanguageSectionStack ||
			!LanguageLabelText || !LanguageButtonStack || !LanguageEnglishButtonBox ||
			!LanguageEnglishButton || !LanguageEnglishButtonText || !LanguageKoreanButtonBox || !LanguageKoreanButton ||
			!LanguageKoreanButtonText || !LanguageJapaneseButtonBox || !LanguageJapaneseButton || !LanguageJapaneseButtonText ||
			!InterfaceActionButtonRow || !ConfirmInterfaceSettingsButtonBox || !ConfirmInterfaceSettingsButton ||
			!ConfirmInterfaceSettingsButtonText || !CancelInterfaceSettingsButtonBox || !CancelInterfaceSettingsButton ||
			!CancelInterfaceSettingsButtonText || !CreditsPanel || !CreditsBackdrop || !CreditsContentStack || !CreditsTitleText ||
			!CreditsColumnRow || !CreditsScrollBoxFrame || !CreditsScrollBox || !CreditsText ||
			!CreditsScrollBoxFrame2 || !CreditsScrollBox2 || !CreditsText2 || !CreditsScrollBoxFrame3 ||
			!CreditsScrollBox3 || !CreditsText3 ||
			!BackFromCreditsButtonBox || !BackFromCreditsButton || !BackFromCreditsButtonText || !VersionText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		auto FillCanvas = [](UCanvasPanelSlot* Slot)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D::ZeroVector);
		};

		auto ConfigureButtonStyle = [](UButton* Button, const FVector2D& ButtonSize, bool bPrimary)
		{
			const FLinearColor FillColor = bPrimary
				? FLinearColor(0.025f, 0.045f, 0.050f, 0.76f)
				: FLinearColor(0.025f, 0.045f, 0.050f, 0.56f);
			const FLinearColor HoveredColor = bPrimary
				? FLinearColor(0.075f, 0.13f, 0.14f, 0.88f)
				: FLinearColor(0.055f, 0.095f, 0.105f, 0.76f);
			const float CornerRadius = bPrimary ? 14.0f : 11.0f;

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.78f, 0.84f, 0.82f, 0.88f), 1.3f, CornerRadius));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(0.96f, 0.98f, 0.95f, 1.0f), 1.7f, CornerRadius));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.75f, FLinearColor(0.60f, 0.68f, 0.68f, 0.90f), 1.0f, CornerRadius));
			ButtonStyle.SetNormalPadding(FMargin(0.0f));
			ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		auto MakeButtonContent = [WidgetTree](
			const FString& NamePrefix,
			const FText& Icon,
			UTextBlock* LabelText,
			const FText& Label,
			int32 LabelFontSize,
			int32 IconFontSize)
		{
			UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(),
				FName(*(NamePrefix + TEXT("Content"))));
			USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(NamePrefix + TEXT("IconBox"))));
			UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(NamePrefix + TEXT("IconText"))));
			USizeBox* BalanceBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(NamePrefix + TEXT("BalanceBox"))));

			if (!Content || !IconBox || !IconText || !BalanceBox)
			{
				return static_cast<UWidget*>(LabelText);
			}

			ConfigureTextBlock(IconText, Icon, FLinearColor(0.94f, 0.92f, 0.84f, 1.0f), IconFontSize);
			ConfigureTextBlock(LabelText, Label, FLinearColor(0.94f, 0.92f, 0.84f, 1.0f), LabelFontSize);

			const float IconLaneWidth = IconFontSize >= 28 ? 58.0f : 46.0f;
			IconBox->SetWidthOverride(IconLaneWidth);
			IconBox->SetHeightOverride(IconFontSize + 8.0f);
			IconBox->SetContent(IconText);
			BalanceBox->SetWidthOverride(IconLaneWidth);
			BalanceBox->SetHeightOverride(IconFontSize + 8.0f);

			UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox);
			if (IconSlot)
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText);
			if (LabelSlot)
			{
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			UHorizontalBoxSlot* BalanceSlot = Content->AddChildToHorizontalBox(BalanceBox);
			if (BalanceSlot)
			{
				BalanceSlot->SetHorizontalAlignment(HAlign_Center);
				BalanceSlot->SetVerticalAlignment(VAlign_Center);
				BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			return static_cast<UWidget*>(Content);
		};

		auto ConfigureMenuButton = [&ConfigureButtonStyle, &MakeButtonContent](
			USizeBox* ButtonBox,
			UButton* Button,
			UTextBlock* ButtonText,
			const FString& NamePrefix,
			const FText& Icon,
			const FText& Label,
			const FVector2D& ButtonSize,
			bool bPrimary)
		{
			ButtonBox->SetWidthOverride(ButtonSize.X);
			ButtonBox->SetHeightOverride(ButtonSize.Y);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, ButtonSize, bPrimary);
			Button->SetContent(MakeButtonContent(
				NamePrefix,
				Icon,
				ButtonText,
				Label,
				bPrimary ? 28 : 20,
				bPrimary ? 28 : 20));
		};

		auto ConfigurePlainButton = [&ConfigureButtonStyle](
			USizeBox* ButtonBox,
			UButton* Button,
			UTextBlock* ButtonText,
			const FText& Label,
			const FVector2D& ButtonSize,
			bool bPrimary)
		{
			ButtonBox->SetWidthOverride(ButtonSize.X);
			ButtonBox->SetHeightOverride(ButtonSize.Y);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, ButtonSize, bPrimary);
			ConfigureTextBlock(ButtonText, Label, FLinearColor::White, bPrimary ? 19 : 17);
			Button->SetContent(ButtonText);
		};

		auto ConfigureSettingsSection = [](
			UBorder* SectionBorder,
			UVerticalBox* SectionStack,
			UVerticalBox* Parent,
			UWidget* LabelWidget,
			UWidget* ControlWidget,
			const FVector2D& BrushSize,
			const FMargin& OuterPadding)
		{
			SectionBorder->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 16.0f));
			SectionBorder->SetBrush(MakeRoundedBoxBrush(
				BrushSize,
				FLinearColor(0.018f, 0.038f, 0.044f, 0.64f),
				FLinearColor(0.46f, 0.58f, 0.58f, 0.40f),
				1.0f,
				8.0f));
			SectionBorder->SetContent(SectionStack);

			UVerticalBoxSlot* LabelSlot = SectionStack->AddChildToVerticalBox(LabelWidget);
			if (LabelSlot)
			{
				LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
				LabelSlot->SetHorizontalAlignment(HAlign_Fill);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
			}

			UVerticalBoxSlot* ControlSlot = SectionStack->AddChildToVerticalBox(ControlWidget);
			if (ControlSlot)
			{
				ControlSlot->SetHorizontalAlignment(HAlign_Fill);
				ControlSlot->SetVerticalAlignment(VAlign_Center);
			}

			UVerticalBoxSlot* SectionSlot = Parent->AddChildToVerticalBox(SectionBorder);
			if (SectionSlot)
			{
				SectionSlot->SetPadding(OuterPadding);
				SectionSlot->SetHorizontalAlignment(HAlign_Fill);
				SectionSlot->SetVerticalAlignment(VAlign_Center);
			}
		};

		FSlateBrush ScrimBrush;
		ScrimBrush.DrawAs = ESlateBrushDrawType::Box;
		ScrimBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
		LeftScrim->SetBrush(ScrimBrush);
		UCanvasPanelSlot* ScrimSlot = RootCanvas->AddChildToCanvas(LeftScrim);
		if (ScrimSlot)
		{
			ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			ScrimSlot->SetAlignment(FVector2D::ZeroVector);
			ScrimSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ScrimSlot->SetSize(FVector2D(720.0f, 1080.0f));
		}

		if (LogoTexture)
		{
			LogoImage->SetBrushFromTexture(LogoTexture, false);
			FSlateBrush LogoBrush = LogoImage->GetBrush();
			LogoBrush.SetImageSize(FVector2D(400.0f, 162.0f));
			LogoImage->SetBrush(LogoBrush);
		}
		LogoImage->SetColorAndOpacity(FLinearColor::White);
		UCanvasPanelSlot* LogoSlot = RootCanvas->AddChildToCanvas(LogoImage);
		if (LogoSlot)
		{
			LogoSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			LogoSlot->SetAlignment(FVector2D::ZeroVector);
			LogoSlot->SetPosition(FVector2D(80.0f, 86.0f));
			LogoSlot->SetSize(FVector2D(400.0f, 162.0f));
		}

		ConfigureMenuButton(
			StartButtonBox,
			StartButton,
			StartButtonText,
			TEXT("StartButton"),
			FText::FromString(TEXT("\u25B6")),
			FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30")),
			FVector2D(440.0f, 72.0f),
			true);

		CurrentSaveSlotBox->SetWidthOverride(260.0f);
		CurrentSaveSlotBox->SetHeightOverride(48.0f);
		CurrentSaveSlotBox->SetContent(CurrentSaveSlotBorder);
		CurrentSaveSlotBorder->SetPadding(FMargin(14.0f, 6.0f));
		CurrentSaveSlotBorder->SetHorizontalAlignment(HAlign_Center);
		CurrentSaveSlotBorder->SetVerticalAlignment(VAlign_Center);
		CurrentSaveSlotBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(260.0f, 48.0f),
			FLinearColor(0.025f, 0.045f, 0.050f, 0.52f),
			FLinearColor(0.78f, 0.84f, 0.82f, 0.74f),
			1.0f,
			8.0f));
		ConfigureTextBlock(
			CurrentSaveSlotText,
			FText::FromString(TEXT("\uC2AC\uB86F 1 - \uBE48 \uC2AC\uB86F")),
			FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
			16);
		CurrentSaveSlotBorder->SetContent(CurrentSaveSlotText);

		ConfigureMenuButton(
			SlotSelectButtonBox,
			SlotSelectButton,
			SlotSelectButtonText,
			TEXT("SlotSelectButton"),
			FText::FromString(TEXT("\u25A6")),
			FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			SettingsButtonBox,
			SettingsButton,
			SettingsButtonText,
			TEXT("SettingsButton"),
			FText::FromString(TEXT("\u2699")),
			FText::FromString(TEXT("\uC124\uC815")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			CreditsButtonBox,
			CreditsButton,
			CreditsButtonText,
			TEXT("CreditsButton"),
			FText::FromString(TEXT("\u24D8")),
			FText::FromString(TEXT("\uD06C\uB808\uB527")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			QuitButtonBox,
			QuitButton,
			QuitButtonText,
			TEXT("QuitButton"),
			FText::FromString(TEXT("\u00D7")),
			FText::FromString(TEXT("\uC885\uB8CC")),
			FVector2D(380.0f, 52.0f),
			false);

		for (UWidget* MenuItem : {
				static_cast<UWidget*>(StartButtonBox),
				static_cast<UWidget*>(CurrentSaveSlotBox),
				static_cast<UWidget*>(SlotSelectButtonBox),
				static_cast<UWidget*>(SettingsButtonBox),
				static_cast<UWidget*>(CreditsButtonBox),
				static_cast<UWidget*>(QuitButtonBox) })
		{
			UVerticalBoxSlot* ItemSlot = MainMenuPanel->AddChildToVerticalBox(MenuItem);
			if (ItemSlot)
			{
				ItemSlot->SetHorizontalAlignment(HAlign_Left);
				ItemSlot->SetVerticalAlignment(VAlign_Center);
				ItemSlot->SetPadding(MenuItem == CurrentSaveSlotBox ? FMargin(0.0f, 12.0f, 0.0f, 22.0f) : FMargin(0.0f, 0.0f, 0.0f, 12.0f));
			}
		}

		UCanvasPanelSlot* MainMenuSlot = RootCanvas->AddChildToCanvas(MainMenuPanel);
		if (MainMenuSlot)
		{
			MainMenuSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			MainMenuSlot->SetAlignment(FVector2D::ZeroVector);
			MainMenuSlot->SetPosition(FVector2D(92.0f, 310.0f));
			MainMenuSlot->SetSize(FVector2D(460.0f, 440.0f));
		}

		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(SaveSlotPanel));

		SaveSlotBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.58f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(SaveSlotPanel->AddChildToCanvas(SaveSlotBackdrop));

		SaveSlotContentBackground->SetPadding(FMargin(36.0f, 32.0f));
		SaveSlotContentBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(780.0f, 710.0f),
			FLinearColor(0.018f, 0.030f, 0.034f, 0.88f),
			FLinearColor(0.70f, 0.78f, 0.76f, 0.72f),
			1.2f,
			16.0f));
		SaveSlotContentBackground->SetContent(SaveSlotContentStack);
		UCanvasPanelSlot* SaveContentSlot = SaveSlotPanel->AddChildToCanvas(SaveSlotContentBackground);
		if (SaveContentSlot)
		{
			SaveContentSlot->SetAnchors(FAnchors(0.0f, 0.5f));
			SaveContentSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			SaveContentSlot->SetPosition(FVector2D(88.0f, 14.0f));
			SaveContentSlot->SetSize(FVector2D(780.0f, 710.0f));
		}

		ConfigureTextBlockLeft(
			SaveSlotPanelTitleText,
			FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")),
			FLinearColor(0.94f, 0.92f, 0.84f, 1.0f),
			30);
		UVerticalBoxSlot* SaveTitleSlot = SaveSlotContentStack->AddChildToVerticalBox(SaveSlotPanelTitleText);
		if (SaveTitleSlot)
		{
			SaveTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}

		auto ConfigureSaveSlotButton = [&ConfigureButtonStyle, WidgetTree](
			USizeBox* ButtonBox,
			UButton* Button,
			UTextBlock* TextBlock,
			int32 SlotIndex)
		{
			UOverlay* SlotContent = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				*FString::Printf(TEXT("SaveSlot%dContent"), SlotIndex));
			UImage* SelectionRingImage = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				*FString::Printf(TEXT("SaveSlot%dSelectionRingImage"), SlotIndex));

			ButtonBox->SetWidthOverride(700.0f);
			ButtonBox->SetHeightOverride(112.0f);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, FVector2D(700.0f, 112.0f), false);

			ConfigureTextBlock(
				TextBlock,
				FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F\n\uC0C8 \uAC8C\uC784 \uC2DC\uC791"), SlotIndex)),
				FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
				18);
			TextBlock->SetMargin(FMargin(0.0f));
			TextBlock->SetAutoWrapText(true);

			if (SlotContent)
			{
				SlotContent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				SlotContent->SetClipping(EWidgetClipping::ClipToBounds);

				if (SelectionRingImage)
				{
					SelectionRingImage->SetVisibility(ESlateVisibility::Collapsed);
					UOverlaySlot* RingSlot = SlotContent->AddChildToOverlay(SelectionRingImage);
					if (RingSlot)
					{
						RingSlot->SetHorizontalAlignment(HAlign_Right);
						RingSlot->SetVerticalAlignment(VAlign_Center);
						RingSlot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
					}
				}

				UOverlaySlot* TextSlot = SlotContent->AddChildToOverlay(TextBlock);
				if (TextSlot)
				{
					TextSlot->SetHorizontalAlignment(HAlign_Fill);
					TextSlot->SetVerticalAlignment(VAlign_Center);
					TextSlot->SetPadding(FMargin(56.0f, 0.0f));
				}

				Button->SetContent(SlotContent);
			}
		};

		ConfigureSaveSlotButton(SaveSlot1ButtonBox, SaveSlot1Button, SaveSlot1Text, 1);
		ConfigureSaveSlotButton(SaveSlot2ButtonBox, SaveSlot2Button, SaveSlot2Text, 2);
		ConfigureSaveSlotButton(SaveSlot3ButtonBox, SaveSlot3Button, SaveSlot3Text, 3);

		for (UWidget* SlotButtonBox : { static_cast<UWidget*>(SaveSlot1ButtonBox), static_cast<UWidget*>(SaveSlot2ButtonBox), static_cast<UWidget*>(SaveSlot3ButtonBox) })
		{
			UVerticalBoxSlot* SlotButtonSlot = SaveSlotContentStack->AddChildToVerticalBox(SlotButtonBox);
			if (SlotButtonSlot)
			{
				SlotButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
			}
		}

		PrimarySaveSlotButtonBox->SetWidthOverride(420.0f);
		PrimarySaveSlotButtonBox->SetHeightOverride(56.0f);
		PrimarySaveSlotButtonBox->SetContent(PrimarySaveSlotButton);
		ConfigureButtonStyle(PrimarySaveSlotButton, FVector2D(420.0f, 56.0f), true);
		ConfigureTextBlock(PrimarySaveSlotButtonText, FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")), FLinearColor::White, 19);
		PrimarySaveSlotButton->SetContent(PrimarySaveSlotButtonText);

		DeleteSaveSlotButtonBox->SetWidthOverride(420.0f);
		DeleteSaveSlotButtonBox->SetHeightOverride(56.0f);
		DeleteSaveSlotButtonBox->SetContent(DeleteSaveSlotButtonContent);
		ConfigureButtonStyle(DeleteSaveSlotButton, FVector2D(420.0f, 56.0f), false);

		FSlateBrush DeleteProgressFillBrush;
		DeleteProgressFillBrush.DrawAs = ESlateBrushDrawType::Box;
		DeleteProgressFillBrush.TintColor = FSlateColor(FLinearColor(0.86f, 0.26f, 0.18f, 0.50f));
		DeleteSaveSlotHoldProgressFill->SetBrush(DeleteProgressFillBrush);
		DeleteSaveSlotHoldProgressFill->SetVisibility(ESlateVisibility::Collapsed);
		DeleteSaveSlotHoldProgressFill->SetRenderOpacity(0.0f);
		DeleteSaveSlotHoldProgressFill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
		DeleteSaveSlotHoldProgressFill->SetRenderScale(FVector2D(0.0f, 1.0f));

		ConfigureTextBlock(
			DeleteSaveSlotButtonText,
			FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30")),
			FLinearColor::White,
			18);
		DeleteSaveSlotButtonText->SetMargin(FMargin(0.0f));
		DeleteSaveSlotButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (UOverlaySlot* DeleteButtonSlot = DeleteSaveSlotButtonContent->AddChildToOverlay(DeleteSaveSlotButton))
		{
			DeleteButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* DeleteProgressFillSlot = DeleteSaveSlotButtonContent->AddChildToOverlay(DeleteSaveSlotHoldProgressFill))
		{
			DeleteProgressFillSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteProgressFillSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* DeleteTextSlot = DeleteSaveSlotButtonContent->AddChildToOverlay(DeleteSaveSlotButtonText))
		{
			DeleteTextSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		SaveSlotActionRow->SetVisibility(ESlateVisibility::Collapsed);
		for (UWidget* ActionButton : { static_cast<UWidget*>(PrimarySaveSlotButtonBox), static_cast<UWidget*>(DeleteSaveSlotButtonBox) })
		{
			UVerticalBoxSlot* ActionButtonSlot = SaveSlotActionRow->AddChildToVerticalBox(ActionButton);
			if (ActionButtonSlot)
			{
				ActionButtonSlot->SetHorizontalAlignment(HAlign_Left);
				ActionButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
			}
		}
		UVerticalBoxSlot* ActionSlot = SaveSlotContentStack->AddChildToVerticalBox(SaveSlotActionRow);
		if (ActionSlot)
		{
			ActionSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 2.0f));
		}

		BackToMainMenuButtonBox->SetWidthOverride(420.0f);
		BackToMainMenuButtonBox->SetHeightOverride(54.0f);
		BackToMainMenuButtonBox->SetContent(BackToMainMenuButton);
		ConfigureButtonStyle(BackToMainMenuButton, FVector2D(420.0f, 54.0f), false);
		ConfigureTextBlock(BackToMainMenuButtonText, FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")), FLinearColor::White, 18);
		BackToMainMenuButton->SetContent(BackToMainMenuButtonText);
		UVerticalBoxSlot* BackSlot = SaveSlotContentStack->AddChildToVerticalBox(BackToMainMenuButtonBox);
		if (BackSlot)
		{
			BackSlot->SetHorizontalAlignment(HAlign_Left);
			BackSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}

		DeleteConfirmPanel->SetPadding(FMargin(30.0f, 26.0f));
		DeleteConfirmPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(520.0f, 220.0f),
			FLinearColor(0.012f, 0.018f, 0.022f, 0.96f),
			FLinearColor(0.92f, 0.36f, 0.30f, 0.88f),
			1.4f,
			14.0f));
		DeleteConfirmPanel->SetContent(DeleteConfirmStack);
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureTextBlockLeft(
			DeleteConfirmTitleText,
			FText::FromString(TEXT("\uC2AC\uB86F \uC0AD\uC81C")),
			FLinearColor::White,
			24);
		ConfigureTextBlockLeft(
			DeleteConfirmMessageText,
			FText::FromString(TEXT("\uC120\uD0DD\uD55C \uC800\uC7A5 \uB370\uC774\uD130\uB97C \uC0AD\uC81C\uD560\uAE4C\uC694?")),
			FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
			17);
		DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmTitleText);
		UVerticalBoxSlot* ConfirmMessageSlot = DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmMessageText);
		if (ConfirmMessageSlot)
		{
			ConfirmMessageSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 22.0f));
		}

		ConfirmDeleteButtonBox->SetWidthOverride(188.0f);
		ConfirmDeleteButtonBox->SetHeightOverride(48.0f);
		ConfirmDeleteButtonBox->SetContent(ConfirmDeleteButton);
		ConfigureButtonStyle(ConfirmDeleteButton, FVector2D(188.0f, 48.0f), true);
		ConfigureTextBlock(ConfirmDeleteButtonText, FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30")), FLinearColor::White, 17);
		ConfirmDeleteButton->SetContent(ConfirmDeleteButtonText);

		CancelDeleteButtonBox->SetWidthOverride(188.0f);
		CancelDeleteButtonBox->SetHeightOverride(48.0f);
		CancelDeleteButtonBox->SetContent(CancelDeleteButton);
		ConfigureButtonStyle(CancelDeleteButton, FVector2D(188.0f, 48.0f), false);
		ConfigureTextBlock(CancelDeleteButtonText, FText::FromString(TEXT("\uCDE8\uC18C")), FLinearColor::White, 17);
		CancelDeleteButton->SetContent(CancelDeleteButtonText);

		UHorizontalBoxSlot* ConfirmButtonSlot = DeleteConfirmButtonRow->AddChildToHorizontalBox(ConfirmDeleteButtonBox);
		if (ConfirmButtonSlot)
		{
			ConfirmButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
		}
		DeleteConfirmButtonRow->AddChildToHorizontalBox(CancelDeleteButtonBox);
		DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmButtonRow);

		UCanvasPanelSlot* ConfirmSlot = SaveSlotPanel->AddChildToCanvas(DeleteConfirmPanel);
		if (ConfirmSlot)
		{
			ConfirmSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ConfirmSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ConfirmSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ConfirmSlot->SetSize(FVector2D(520.0f, 220.0f));
		}

		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(SettingsPanel));
		SettingsBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.006f, 0.010f, 0.012f, 0.58f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(SettingsPanel->AddChildToCanvas(SettingsBackdrop));

		SettingsContentBackground->SetPadding(FMargin(32.0f, 28.0f, 32.0f, 24.0f));
		SettingsContentBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(840.0f, 780.0f),
			FLinearColor(0.015f, 0.025f, 0.030f, 0.86f),
			FLinearColor(0.58f, 0.70f, 0.70f, 0.62f),
			1.2f,
			8.0f));
		SettingsContentBackground->SetContent(SettingsContentStack);
		UCanvasPanelSlot* SettingsContentSlot = SettingsPanel->AddChildToCanvas(SettingsContentBackground);
		if (SettingsContentSlot)
		{
			SettingsContentSlot->SetAnchors(FAnchors(0.0f, 0.5f));
			SettingsContentSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			SettingsContentSlot->SetPosition(FVector2D(164.0f, 0.0f));
			SettingsContentSlot->SetSize(FVector2D(840.0f, 780.0f));
		}

		ConfigurePlainButton(BackFromSettingsButtonBox, BackFromSettingsButton, BackFromSettingsButtonText, FText::FromString(TEXT("\u2190")), FVector2D(52.0f, 52.0f), false);
		UCanvasPanelSlot* SettingsBackSlot = SettingsPanel->AddChildToCanvas(BackFromSettingsButtonBox);
		if (SettingsBackSlot)
		{
			SettingsBackSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			SettingsBackSlot->SetAlignment(FVector2D::ZeroVector);
			SettingsBackSlot->SetPosition(FVector2D(34.0f, 24.0f));
			SettingsBackSlot->SetSize(FVector2D(52.0f, 52.0f));
			SettingsBackSlot->SetZOrder(2);
		}

		ConfigureTextBlockLeft(SettingsTitleText, FText::FromString(TEXT("\uC124\uC815")), FLinearColor::White, 32);
		UVerticalBoxSlot* SettingsTitleSlot = SettingsContentStack->AddChildToVerticalBox(SettingsTitleText);
		if (SettingsTitleSlot)
		{
			SettingsTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		ConfigureTextBlockLeft(SettingsStatusText, FText::FromString(TEXT("\uD604\uC7AC: --")), FLinearColor(0.70f, 0.80f, 0.79f, 1.0f), 15);
		UVerticalBoxSlot* SettingsStatusSlot = SettingsContentStack->AddChildToVerticalBox(SettingsStatusText);
		if (SettingsStatusSlot)
		{
			SettingsStatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		ConfigurePlainButton(GraphicsTabButtonBox, SettingsGraphicsTabButton, SettingsGraphicsTabButtonText, FText::FromString(TEXT("\uADF8\uB798\uD53D")), FVector2D(142.0f, 38.0f), true);
		ConfigurePlainButton(InterfaceTabButtonBox, SettingsInterfaceTabButton, SettingsInterfaceTabButtonText, FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4")), FVector2D(158.0f, 38.0f), false);
		ConfigurePlainButton(DevelopmentTabButtonBox, SettingsDevelopmentTabButton, SettingsDevelopmentTabButtonText, FText::FromString(TEXT("\uAC1C\uBC1C")), FVector2D(102.0f, 38.0f), false);
		for (UWidget* TabButtonBox : {
				static_cast<UWidget*>(GraphicsTabButtonBox),
				static_cast<UWidget*>(InterfaceTabButtonBox),
				static_cast<UWidget*>(DevelopmentTabButtonBox) })
		{
			UHorizontalBoxSlot* TabButtonSlot = SettingsTabRow->AddChildToHorizontalBox(TabButtonBox);
			if (TabButtonSlot)
			{
				TabButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		UVerticalBoxSlot* SettingsTabSlot = SettingsContentStack->AddChildToVerticalBox(SettingsTabRow);
		if (SettingsTabSlot)
		{
			SettingsTabSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}
		UVerticalBoxSlot* GraphicsSettingsSlot = SettingsContentStack->AddChildToVerticalBox(GraphicsSettingsPanel);
		if (GraphicsSettingsSlot)
		{
			GraphicsSettingsSlot->SetPadding(FMargin(0.0f));
			GraphicsSettingsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ConfigureTextBlockLeft(WindowModeLabelText, FText::FromString(TEXT("\uD654\uBA74 \uBAA8\uB4DC")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		ConfigurePlainButton(WindowedModeButtonBox, WindowedModeButton, WindowedModeButtonText, FText::FromString(TEXT("\uCC3D\uBAA8\uB4DC")), FVector2D(160.0f, 44.0f), false);
		ConfigurePlainButton(BorderlessWindowModeButtonBox, BorderlessWindowModeButton, BorderlessWindowModeButtonText, FText::FromString(TEXT("\uD14C\uB450\uB9AC \uC5C6\uB294 \uCC3D\uBAA8\uB4DC")), FVector2D(236.0f, 44.0f), false);
		ConfigurePlainButton(FullscreenModeButtonBox, FullscreenModeButton, FullscreenModeButtonText, FText::FromString(TEXT("\uC804\uCCB4\uD654\uBA74\uBAA8\uB4DC")), FVector2D(184.0f, 44.0f), false);
		for (UWidget* WindowModeButtonBox : {
				static_cast<UWidget*>(WindowedModeButtonBox),
				static_cast<UWidget*>(BorderlessWindowModeButtonBox),
				static_cast<UWidget*>(FullscreenModeButtonBox) })
		{
			UHorizontalBoxSlot* ModeSlot = WindowModeRow->AddChildToHorizontalBox(WindowModeButtonBox);
			if (ModeSlot)
			{
				ModeSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		ConfigureSettingsSection(
			SettingsWindowModeSection,
			SettingsWindowModeSectionStack,
			GraphicsSettingsPanel,
			WindowModeLabelText,
			WindowModeRow,
			FVector2D(760.0f, 104.0f),
			FMargin(0.0f, 0.0f, 0.0f, 12.0f));

		ConfigureTextBlockLeft(ResolutionLabelText, FText::FromString(TEXT("\uD574\uC0C1\uB3C4")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		ConfigurePlainButton(Resolution1280ButtonBox, Resolution1280Button, Resolution1280ButtonText, FText::FromString(TEXT("1280 x 720")), FVector2D(660.0f, 42.0f), false);
		ConfigurePlainButton(Resolution1600ButtonBox, Resolution1600Button, Resolution1600ButtonText, FText::FromString(TEXT("1600 x 900")), FVector2D(660.0f, 42.0f), false);
		ConfigurePlainButton(Resolution1920ButtonBox, Resolution1920Button, Resolution1920ButtonText, FText::FromString(TEXT("1920 x 1080")), FVector2D(660.0f, 42.0f), false);
		ConfigurePlainButton(Resolution2560ButtonBox, Resolution2560Button, Resolution2560ButtonText, FText::FromString(TEXT("2560 x 1440")), FVector2D(660.0f, 42.0f), false);
		ConfigurePlainButton(Resolution3840ButtonBox, Resolution3840Button, Resolution3840ButtonText, FText::FromString(TEXT("3840 x 2160")), FVector2D(660.0f, 42.0f), false);
		for (UWidget* ResolutionButtonBox : {
				static_cast<UWidget*>(Resolution1280ButtonBox),
				static_cast<UWidget*>(Resolution1600ButtonBox),
				static_cast<UWidget*>(Resolution1920ButtonBox),
				static_cast<UWidget*>(Resolution2560ButtonBox),
				static_cast<UWidget*>(Resolution3840ButtonBox) })
		{
			UVerticalBoxSlot* ResolutionSlot = ResolutionButtonStack->AddChildToVerticalBox(ResolutionButtonBox);
			if (ResolutionSlot)
			{
				ResolutionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}
		ConfigureSettingsSection(
			SettingsResolutionSection,
			SettingsResolutionSectionStack,
			GraphicsSettingsPanel,
			ResolutionLabelText,
			ResolutionButtonStack,
			FVector2D(760.0f, 286.0f),
			FMargin(0.0f, 0.0f, 0.0f, 12.0f));

		ConfigureTextBlockLeft(DLSSLabelText, FText::FromString(TEXT("DLSS")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		ConfigurePlainButton(DLSSOffButtonBox, DLSSOffButton, DLSSOffButtonText, FText::FromString(TEXT("\uB044\uAE30")), FVector2D(146.0f, 42.0f), false);
		ConfigurePlainButton(DLSSQualityButtonBox, DLSSQualityButton, DLSSQualityButtonText, FText::FromString(TEXT("\uD488\uC9C8")), FVector2D(146.0f, 42.0f), false);
		ConfigurePlainButton(DLSSBalancedButtonBox, DLSSBalancedButton, DLSSBalancedButtonText, FText::FromString(TEXT("\uADE0\uD615")), FVector2D(146.0f, 42.0f), false);
		ConfigurePlainButton(DLSSPerformanceButtonBox, DLSSPerformanceButton, DLSSPerformanceButtonText, FText::FromString(TEXT("\uC131\uB2A5")), FVector2D(146.0f, 42.0f), false);
		for (UWidget* DLSSButtonBox : {
				static_cast<UWidget*>(DLSSOffButtonBox),
				static_cast<UWidget*>(DLSSQualityButtonBox),
				static_cast<UWidget*>(DLSSBalancedButtonBox),
				static_cast<UWidget*>(DLSSPerformanceButtonBox) })
		{
			UHorizontalBoxSlot* DLSSSlot = DLSSButtonRow->AddChildToHorizontalBox(DLSSButtonBox);
			if (DLSSSlot)
			{
				DLSSSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		ConfigureSettingsSection(
			SettingsDLSSSection,
			SettingsDLSSSectionStack,
			GraphicsSettingsPanel,
			DLSSLabelText,
			DLSSButtonRow,
			FVector2D(760.0f, 104.0f),
			FMargin(0.0f));

		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* InterfaceSettingsSlot = SettingsContentStack->AddChildToVerticalBox(InterfaceSettingsPanel);
		if (InterfaceSettingsSlot)
		{
			InterfaceSettingsSlot->SetPadding(FMargin(0.0f));
		}

		ConfigureTextBlockLeft(LanguageLabelText, FText::FromString(TEXT("\uC5B8\uC5B4")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		ConfigurePlainButton(LanguageEnglishButtonBox, LanguageEnglishButton, LanguageEnglishButtonText, FText::FromString(TEXT("[x] English")), FVector2D(660.0f, 46.0f), false);
		ConfigurePlainButton(LanguageKoreanButtonBox, LanguageKoreanButton, LanguageKoreanButtonText, FText::FromString(TEXT("[ ] \uD55C\uAD6D\uC5B4")), FVector2D(660.0f, 46.0f), false);
		ConfigurePlainButton(LanguageJapaneseButtonBox, LanguageJapaneseButton, LanguageJapaneseButtonText, FText::FromString(TEXT("[ ] \u65E5\u672C\u8A9E")), FVector2D(660.0f, 46.0f), false);
		for (UWidget* LanguageButtonBox : {
				static_cast<UWidget*>(LanguageEnglishButtonBox),
				static_cast<UWidget*>(LanguageKoreanButtonBox),
				static_cast<UWidget*>(LanguageJapaneseButtonBox) })
		{
			UVerticalBoxSlot* LanguageButtonSlot = LanguageButtonStack->AddChildToVerticalBox(LanguageButtonBox);
			if (LanguageButtonSlot)
			{
				LanguageButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
			}
		}
		ConfigureSettingsSection(
			SettingsLanguageSection,
			SettingsLanguageSectionStack,
			InterfaceSettingsPanel,
			LanguageLabelText,
			LanguageButtonStack,
			FVector2D(760.0f, 220.0f),
			FMargin(0.0f, 0.0f, 0.0f, 18.0f));

		ConfigurePlainButton(ConfirmInterfaceSettingsButtonBox, ConfirmInterfaceSettingsButton, ConfirmInterfaceSettingsButtonText, FText::FromString(TEXT("\uACB0\uC815")), FVector2D(160.0f, 46.0f), true);
		ConfigurePlainButton(CancelInterfaceSettingsButtonBox, CancelInterfaceSettingsButton, CancelInterfaceSettingsButtonText, FText::FromString(TEXT("\uCDE8\uC18C")), FVector2D(160.0f, 46.0f), false);
		UHorizontalBoxSlot* ConfirmInterfaceSlot = InterfaceActionButtonRow->AddChildToHorizontalBox(ConfirmInterfaceSettingsButtonBox);
		if (ConfirmInterfaceSlot)
		{
			ConfirmInterfaceSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		}
		InterfaceActionButtonRow->AddChildToHorizontalBox(CancelInterfaceSettingsButtonBox);
		UVerticalBoxSlot* InterfaceActionSlot = InterfaceSettingsPanel->AddChildToVerticalBox(InterfaceActionButtonRow);
		if (InterfaceActionSlot)
		{
			InterfaceActionSlot->SetHorizontalAlignment(HAlign_Left);
		}

		DevelopmentSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* DevelopmentSettingsSlot = SettingsContentStack->AddChildToVerticalBox(DevelopmentSettingsPanel);
		if (DevelopmentSettingsSlot)
		{
			DevelopmentSettingsSlot->SetPadding(FMargin(0.0f));
		}

		ConfigureTextBlockLeft(EnemyCombatDebugLabelText, FText::FromString(TEXT("\uC804\uD22C \uB514\uBC84\uADF8")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		ConfigurePlainButton(
			EnemyCombatDebugToggleButtonBox,
			EnemyCombatDebugToggleButton,
			EnemyCombatDebugToggleButtonText,
			FText::FromString(TEXT("\uC801 \uC804\uD22C \uB514\uBC84\uADF8 \uD45C\uC2DC")),
			FVector2D(660.0f, 46.0f),
			false);
		ConfigureSettingsSection(
			EnemyCombatDebugSection,
			EnemyCombatDebugSectionStack,
			DevelopmentSettingsPanel,
			EnemyCombatDebugLabelText,
			EnemyCombatDebugToggleButtonBox,
			FVector2D(760.0f, 104.0f),
			FMargin(0.0f));

		ConfigureTextBlockLeft(
			DebugDisplayLanguageLabelText,
			FText::FromString(TEXT("\uB514\uBC84\uADF8 \uD45C\uAE30 \uC5B8\uC5B4")),
			FLinearColor(0.72f, 0.80f, 0.78f, 1.0f),
			15);
		ConfigurePlainButton(
			DebugDisplayLanguageKoreanButtonBox,
			DebugDisplayLanguageKoreanButton,
			DebugDisplayLanguageKoreanButtonText,
			FText::FromString(TEXT("\uD55C\uAD6D\uC5B4")),
			FVector2D(660.0f, 46.0f),
			false);
		ConfigurePlainButton(
			DebugDisplayLanguageEnglishButtonBox,
			DebugDisplayLanguageEnglishButton,
			DebugDisplayLanguageEnglishButtonText,
			FText::FromString(TEXT("English")),
			FVector2D(660.0f, 46.0f),
			false);
		if (UVerticalBoxSlot* DebugLanguageKoreanSlot = DebugDisplayLanguageButtonStack->AddChildToVerticalBox(DebugDisplayLanguageKoreanButtonBox))
		{
			DebugLanguageKoreanSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		DebugDisplayLanguageButtonStack->AddChildToVerticalBox(DebugDisplayLanguageEnglishButtonBox);
		ConfigureSettingsSection(
			DebugDisplayLanguageSection,
			DebugDisplayLanguageSectionStack,
			DevelopmentSettingsPanel,
			DebugDisplayLanguageLabelText,
			DebugDisplayLanguageButtonStack,
			FVector2D(760.0f, 164.0f),
			FMargin(0.0f, 12.0f, 0.0f, 0.0f));

		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(CreditsPanel));
		CreditsBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.46f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(CreditsPanel->AddChildToCanvas(CreditsBackdrop));

		CreditsContentStack->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* LegacyCreditsStackSlot = CreditsPanel->AddChildToCanvas(CreditsContentStack);
		if (LegacyCreditsStackSlot)
		{
			LegacyCreditsStackSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			LegacyCreditsStackSlot->SetAlignment(FVector2D::ZeroVector);
			LegacyCreditsStackSlot->SetPosition(FVector2D::ZeroVector);
			LegacyCreditsStackSlot->SetSize(FVector2D::ZeroVector);
		}

		ConfigurePlainButton(BackFromCreditsButtonBox, BackFromCreditsButton, BackFromCreditsButtonText, FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")), FVector2D(380.0f, 52.0f), false);
		UCanvasPanelSlot* BackFromCreditsSlot = CreditsPanel->AddChildToCanvas(BackFromCreditsButtonBox);
		if (BackFromCreditsSlot)
		{
			BackFromCreditsSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			BackFromCreditsSlot->SetAlignment(FVector2D::ZeroVector);
			BackFromCreditsSlot->SetPosition(FVector2D(92.0f, 310.0f));
			BackFromCreditsSlot->SetSize(FVector2D(380.0f, 52.0f));
		}

		ConfigureTextBlockLeft(CreditsTitleText, FText::FromString(TEXT("\uD06C\uB808\uB527")), FLinearColor::White, 30);
		UCanvasPanelSlot* CreditsTitleSlot = CreditsPanel->AddChildToCanvas(CreditsTitleText);
		if (CreditsTitleSlot)
		{
			CreditsTitleSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CreditsTitleSlot->SetAlignment(FVector2D::ZeroVector);
			CreditsTitleSlot->SetPosition(FVector2D(560.0f, 116.0f));
			CreditsTitleSlot->SetSize(FVector2D(1180.0f, 42.0f));
		}

		auto ConfigureCreditsColumn = [](USizeBox* Frame, UScrollBox* ScrollBox, UTextBlock* TextBlock, const FText& PreviewText)
		{
			Frame->SetWidthOverride(380.0f);
			Frame->SetHeightOverride(700.0f);
			Frame->SetContent(ScrollBox);
			ConfigureTextBlock(
				TextBlock,
				PreviewText,
				FLinearColor(0.90f, 0.94f, 0.92f, 1.0f),
				17);
			TextBlock->SetAutoWrapText(true);
			TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
			TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
			ScrollBox->AddChild(TextBlock);
		};

		ConfigureCreditsColumn(CreditsScrollBoxFrame, CreditsScrollBox, CreditsText, FText::FromString(TEXT("Tuna Sweeper\n\nBlenG")));
		ConfigureCreditsColumn(CreditsScrollBoxFrame2, CreditsScrollBox2, CreditsText2, FText::FromString(TEXT("BlenG")));
		ConfigureCreditsColumn(CreditsScrollBoxFrame3, CreditsScrollBox3, CreditsText3, FText::FromString(TEXT("BlenG")));

		for (UWidget* CreditsColumn : {
				static_cast<UWidget*>(CreditsScrollBoxFrame),
				static_cast<UWidget*>(CreditsScrollBoxFrame2),
				static_cast<UWidget*>(CreditsScrollBoxFrame3) })
		{
			UHorizontalBoxSlot* ColumnSlot = CreditsColumnRow->AddChildToHorizontalBox(CreditsColumn);
			if (ColumnSlot)
			{
				ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
				ColumnSlot->SetVerticalAlignment(VAlign_Fill);
				ColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 32.0f, 0.0f));
				ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
		UCanvasPanelSlot* CreditsColumnSlot = CreditsPanel->AddChildToCanvas(CreditsColumnRow);
		if (CreditsColumnSlot)
		{
			CreditsColumnSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CreditsColumnSlot->SetAlignment(FVector2D::ZeroVector);
			CreditsColumnSlot->SetPosition(FVector2D(560.0f, 176.0f));
			CreditsColumnSlot->SetSize(FVector2D(1240.0f, 720.0f));
		}

		ConfigureTextBlock(VersionText, FText::FromString(TEXT("v0.1")), FLinearColor(1.0f, 1.0f, 1.0f, 0.86f), 14);
		UCanvasPanelSlot* VersionSlot = RootCanvas->AddChildToCanvas(VersionText);
		if (VersionSlot)
		{
			VersionSlot->SetAnchors(FAnchors(1.0f, 1.0f));
			VersionSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			VersionSlot->SetPosition(FVector2D(-28.0f, -22.0f));
			VersionSlot->SetSize(FVector2D(80.0f, 24.0f));
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildLevelTransitionVideoWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UImage* VideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VideoImage"));
		UBorder* LetterboxTopPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LetterboxTopPanel"));
		UBorder* LetterboxBottomPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LetterboxBottomPanel"));
		UBorder* MessageBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MessageBackground"));
		UTextBlock* TransitionMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TransitionMessageText"));
		UBorder* BlackFadePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackFadePanel"));

		if (!RootCanvas || !VideoImage || !LetterboxTopPanel || !LetterboxBottomPanel || !MessageBackground || !TransitionMessageText || !BlackFadePanel)
		{
			return false;
		}

		auto FillCanvas = [](UCanvasPanelSlot* Slot)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D(0.0f, 0.0f));
		};

		auto ConfigureLetterboxSlot = [](UCanvasPanelSlot* Slot, bool bTop)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(bTop
				? FAnchors(0.0f, 0.0f, 1.0f, 0.10f)
				: FAnchors(0.0f, 0.90f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D(0.0f, 0.0f));
			Slot->SetZOrder(10);
		};

		FSlateBrush VideoBrush;
		VideoBrush.DrawAs = ESlateBrushDrawType::Image;
		VideoBrush.TintColor = FSlateColor(FLinearColor::White);
		VideoBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));

		FSlateBrush BlackBrush;
		BlackBrush.DrawAs = ESlateBrushDrawType::Box;
		BlackBrush.TintColor = FSlateColor(FLinearColor::Black);
		BlackBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));

		WidgetTree->RootWidget = RootCanvas;
		VideoImage->SetBrush(VideoBrush);
		VideoImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* VideoSlot = RootCanvas->AddChildToCanvas(VideoImage))
		{
			FillCanvas(VideoSlot);
			VideoSlot->SetZOrder(0);
		}

		LetterboxTopPanel->SetBrush(BlackBrush);
		LetterboxTopPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxTopPanel), true);

		LetterboxBottomPanel->SetBrush(BlackBrush);
		LetterboxBottomPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxBottomPanel), false);

		MessageBackground->SetPadding(FMargin(18.0f, 7.0f));
		MessageBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(520.0f, 48.0f),
			FLinearColor(0.015f, 0.018f, 0.022f, 0.72f),
			FLinearColor(0.65f, 0.72f, 0.78f, 0.45f),
			1.0f));
		MessageBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ConfigureTextBlock(TransitionMessageText, FText::GetEmpty(), FLinearColor(0.88f, 0.92f, 0.95f, 1.0f), 18);
		MessageBackground->SetContent(TransitionMessageText);

		UCanvasPanelSlot* MessageSlot = RootCanvas->AddChildToCanvas(MessageBackground);
		if (MessageSlot)
		{
			MessageSlot->SetAnchors(FAnchors(0.5f, 1.0f));
			MessageSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			MessageSlot->SetPosition(FVector2D(0.0f, -58.0f));
			MessageSlot->SetSize(FVector2D(520.0f, 48.0f));
			MessageSlot->SetZOrder(20);
		}

		BlackFadePanel->SetBrush(BlackBrush);
		BlackFadePanel->SetRenderOpacity(0.0f);
		BlackFadePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* FadeSlot = RootCanvas->AddChildToCanvas(BlackFadePanel))
		{
			FillCanvas(FadeSlot);
			FadeSlot->SetZOrder(30);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildSpeechBubbleWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UBorder* BubbleTail = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleTail"));
		UBorder* BubbleBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleBackground"));
		UTextBlock* BubbleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BubbleText"));

		if (!RootCanvas || !BubbleTail || !BubbleBackground || !BubbleText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		const FLinearColor FillColor(0.96f, 0.98f, 1.0f, 0.96f);
		const FLinearColor OutlineColor(0.08f, 0.10f, 0.12f, 0.92f);
		BubbleTail->SetBrush(MakeRoundedBoxBrush(FVector2D(18.0f, 18.0f), FillColor, OutlineColor, 1.0f));
		BubbleTail->SetRenderTransformAngle(45.0f);

		UCanvasPanelSlot* TailSlot = RootCanvas->AddChildToCanvas(BubbleTail);
		if (TailSlot)
		{
			TailSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			TailSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			TailSlot->SetPosition(FVector2D(0.0f, 40.0f));
			TailSlot->SetSize(FVector2D(18.0f, 18.0f));
		}

		BubbleBackground->SetPadding(FMargin(12.0f, 6.0f));
		BubbleBackground->SetBrush(MakeRoundedBoxBrush(FVector2D(160.0f, 48.0f), FillColor, OutlineColor, 1.5f));
		ConfigureTextBlock(BubbleText, FText::FromString(TEXT("3")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 28);
		BubbleBackground->SetContent(BubbleText);

		UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BubbleBackground);
		if (BackgroundSlot)
		{
			BackgroundSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			BackgroundSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			BackgroundSlot->SetPosition(FVector2D(0.0f, 0.0f));
			BackgroundSlot->SetSize(FVector2D(160.0f, 48.0f));
		}

		RegisterWidgetVariable(WidgetBlueprint, BubbleText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildQuestWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		UTextBlock* QuestTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTitleText"));
		UButton* QuestCloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestCloseButton"));
		UTextBlock* QuestCloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestCloseButtonText"));
		UTextBlock* QuestDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDescriptionText"));
		UTextBlock* QuestObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestObjectiveText"));
		UTextBlock* QuestRewardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestRewardText"));
		UTextBlock* QuestStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestStateText"));
		USizeBox* QuestPrimaryButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestPrimaryButtonBox"));
		UButton* QuestPrimaryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestPrimaryButton"));
		UTextBlock* QuestPrimaryButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestPrimaryButtonText"));

		if (!RootCanvas || !PanelBackground || !PanelStack || !HeaderRow || !QuestTitleText || !QuestCloseButton ||
			!QuestCloseButtonText || !QuestDescriptionText || !QuestObjectiveText || !QuestRewardText ||
			!QuestStateText || !QuestPrimaryButtonBox || !QuestPrimaryButton || !QuestPrimaryButtonText)
		{
			return false;
		}

		auto ConfigureQuestButton = [](UButton* Button, const FVector2D& ButtonSize, const FLinearColor& FillColor, const FLinearColor& HoveredColor)
		{
			if (!Button)
			{
				return;
			}

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.64f, 0.72f, 0.76f, 0.90f), 1.2f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(0.92f, 0.96f, 1.0f, 0.96f), 1.8f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.78f, FLinearColor(0.56f, 0.64f, 0.70f, 1.0f), 1.0f));
			ButtonStyle.SetNormalPadding(FMargin(10.0f, 4.0f));
			ButtonStyle.SetPressedPadding(FMargin(10.0f, 5.0f, 10.0f, 3.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBackground);
		if (PanelSlot)
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
			PanelSlot->SetSize(FVector2D(1180.0f, 640.0f));
		}

		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1180.0f, 640.0f),
			FLinearColor(0.055f, 0.065f, 0.075f, 0.96f),
			FLinearColor(0.40f, 0.48f, 0.54f, 0.85f),
			1.5f));
		PanelBackground->SetPadding(FMargin(24.0f, 20.0f));
		PanelBackground->SetContent(PanelStack);

		UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}

		ConfigureTextBlockLeft(QuestTitleText, FText::FromString(TEXT("\uCCAB \uC678\uCD9C")), FLinearColor::White, 28);
		UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(QuestTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureQuestButton(
			QuestCloseButton,
			FVector2D(78.0f, 38.0f),
			FLinearColor(0.12f, 0.14f, 0.16f, 0.94f),
			FLinearColor(0.18f, 0.21f, 0.24f, 0.98f));
		ConfigureTextBlock(QuestCloseButtonText, FText::FromString(TEXT("\uB2EB\uAE30")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 15);
		QuestCloseButton->SetContent(QuestCloseButtonText);
		UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(QuestCloseButton);
		if (CloseSlot)
		{
			CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlockLeft(QuestDescriptionText, FText::FromString(TEXT("\uC774\uC81C \uB4E4\uC5B4\uC654\uC73C\uB2C8 \uB098\uAC00\uC11C \uD55C\uBC88 \uC0B0\uCC45\uD558\uACE0 \uB4E4\uC5B4\uC640")), FLinearColor(0.83f, 0.88f, 0.91f, 1.0f), 18);
		QuestDescriptionText->SetAutoWrapText(true);
		QuestDescriptionText->SetWrapTextAt(0.0f);
		UVerticalBoxSlot* DescriptionSlot = PanelStack->AddChildToVerticalBox(QuestDescriptionText);
		if (DescriptionSlot)
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
		}

		ConfigureTextBlockLeft(QuestObjectiveText, FText::FromString(TEXT("\uBAA9\uD45C: \uBC99\uCEE4 \uBC16\uC73C\uB85C \uC774\uB3D9")), FLinearColor(0.97f, 0.91f, 0.72f, 1.0f), 18);
		UVerticalBoxSlot* ObjectiveSlot = PanelStack->AddChildToVerticalBox(QuestObjectiveText);
		if (ObjectiveSlot)
		{
			ObjectiveSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		ConfigureTextBlockLeft(QuestRewardText, FText::FromString(TEXT("\uBCF4\uC0C1: \uCF54\uC778 100")), FLinearColor(0.95f, 0.78f, 0.36f, 1.0f), 17);
		UVerticalBoxSlot* RewardSlot = PanelStack->AddChildToVerticalBox(QuestRewardText);
		if (RewardSlot)
		{
			RewardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		ConfigureTextBlockLeft(QuestStateText, FText::FromString(TEXT("\uC0C1\uD0DC: \uBC1B\uAE30 \uAC00\uB2A5")), FLinearColor(0.72f, 0.80f, 0.86f, 1.0f), 16);
		UVerticalBoxSlot* StateSlot = PanelStack->AddChildToVerticalBox(QuestStateText);
		if (StateSlot)
		{
			StateSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
		}

		QuestPrimaryButtonBox->SetWidthOverride(220.0f);
		QuestPrimaryButtonBox->SetHeightOverride(54.0f);
		QuestPrimaryButtonBox->SetContent(QuestPrimaryButton);
		ConfigureQuestButton(
			QuestPrimaryButton,
			FVector2D(220.0f, 54.0f),
			FLinearColor(0.14f, 0.27f, 0.22f, 0.96f),
			FLinearColor(0.18f, 0.37f, 0.30f, 0.98f));
		ConfigureTextBlock(QuestPrimaryButtonText, FText::FromString(TEXT("\uC218\uB77D")), FLinearColor::White, 20);
		QuestPrimaryButton->SetContent(QuestPrimaryButtonText);

		UVerticalBoxSlot* PrimaryButtonSlot = PanelStack->AddChildToVerticalBox(QuestPrimaryButtonBox);
		if (PrimaryButtonSlot)
		{
			PrimaryButtonSlot->SetHorizontalAlignment(HAlign_Right);
			PrimaryButtonSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	void SetListViewEntryWidgetClass(UListViewBase* ListViewBase, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!ListViewBase || !EntryWidgetClass)
		{
			return;
		}

		if (FClassProperty* EntryWidgetClassProperty = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
		{
			EntryWidgetClassProperty->SetPropertyValue_InContainer(ListViewBase, EntryWidgetClass);
		}
	}

	UWidgetBlueprint* EnsureWidgetBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (UWidgetBlueprint* ExistingBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on %s."), *ObjectPath, *GetNameSafe(ParentClass));
				return nullptr;
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			return ExistingBlueprint;
		}

		UWidgetBlueprintFactory* WidgetBlueprintFactory = NewObject<UWidgetBlueprintFactory>();
		WidgetBlueprintFactory->ParentClass = ParentClass;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			AssetName,
			AssetPath,
			UWidgetBlueprint::StaticClass(),
			WidgetBlueprintFactory);

		UWidgetBlueprint* CreatedBlueprint = Cast<UWidgetBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();
		return CreatedBlueprint;
	}

	bool BuildItemThumbnailSlotWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UVerticalBox* RootStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootStack"));
		UTextBlock* EquipmentSlotNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EquipmentSlotNameText"));
		USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSizeBox"));
		UBorder* SlotBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBackground"));
		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SlotOverlay"));
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconBox"));
		UImage* ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
		UVerticalBox* SlotLabelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotLabelStack"));
		UBorder* ItemQuantityPlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemQuantityPlate"));
		UTextBlock* ItemQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemQuantityText"));
		UTextBlock* AttachmentSlotIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("AttachmentSlotIndicatorText"));
		UBorder* ItemNamePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemNamePlate"));
		UTextBlock* ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
		UBorder* ItemPricePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemPricePlate"));
		UHorizontalBox* ItemPriceRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ItemPriceRow"));
		USizeBox* ItemPriceCoinSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ItemPriceCoinSizeBox"));
		UImage* ItemPriceCoinImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemPriceCoinImage"));
		UTextBlock* ItemPriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemPriceText"));

		if (!RootSizeBox || !RootStack || !EquipmentSlotNameText || !SlotSizeBox || !SlotBackground || !SlotOverlay ||
			!IconBox || !ItemIconImage || !SlotLabelStack || !ItemQuantityPlate || !ItemQuantityText ||
			!AttachmentSlotIndicatorText || !ItemNamePlate || !ItemNameText || !ItemPricePlate ||
			!ItemPriceRow || !ItemPriceCoinSizeBox || !ItemPriceCoinImage || !ItemPriceText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(96.0f);
		RootSizeBox->SetContent(RootStack);

		ConfigureTextBlockLeft(
			EquipmentSlotNameText,
			FText::FromString(TEXT("Slot")),
			FLinearColor(0.72f, 0.80f, 0.86f, 1.0f),
			12);
		EquipmentSlotNameText->SetAutoWrapText(false);
		EquipmentSlotNameText->SetJustification(ETextJustify::Center);
		EquipmentSlotNameText->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* EquipmentSlotNameSlot = RootStack->AddChildToVerticalBox(EquipmentSlotNameText);
		if (EquipmentSlotNameSlot)
		{
			EquipmentSlotNameSlot->SetHorizontalAlignment(HAlign_Fill);
			EquipmentSlotNameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
		}

		SlotSizeBox->SetWidthOverride(92.0f);
		SlotSizeBox->SetHeightOverride(92.0f);
		SlotSizeBox->SetContent(SlotBackground);
		UVerticalBoxSlot* SlotSizeSlot = RootStack->AddChildToVerticalBox(SlotSizeBox);
		if (SlotSizeSlot)
		{
			SlotSizeSlot->SetHorizontalAlignment(HAlign_Center);
			SlotSizeSlot->SetVerticalAlignment(VAlign_Top);
			SlotSizeSlot->SetPadding(FMargin(2.0f));
		}

		SlotBackground->SetPadding(FMargin(1.0f));
		SlotBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(92.0f, 92.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.24f, 0.27f, 0.31f, 0.95f),
			1.0f));
		SlotBackground->SetContent(SlotOverlay);

		IconBox->SetWidthOverride(86.0f);
		IconBox->SetHeightOverride(86.0f);
		IconBox->SetContent(ItemIconImage);
		ItemIconImage->SetColorAndOpacity(FLinearColor::White);

		UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(IconBox);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(AttachmentSlotIndicatorText, FText::GetEmpty(), FLinearColor::White, 13);
		UOverlaySlot* AttachmentIndicatorSlot = SlotOverlay->AddChildToOverlay(AttachmentSlotIndicatorText);
		if (AttachmentIndicatorSlot)
		{
			AttachmentIndicatorSlot->SetHorizontalAlignment(HAlign_Left);
			AttachmentIndicatorSlot->SetVerticalAlignment(VAlign_Top);
		}

		UOverlaySlot* LabelStackSlot = SlotOverlay->AddChildToOverlay(SlotLabelStack);
		if (LabelStackSlot)
		{
			LabelStackSlot->SetHorizontalAlignment(HAlign_Right);
			LabelStackSlot->SetVerticalAlignment(VAlign_Bottom);
			LabelStackSlot->SetPadding(FMargin(0.0f, 0.0f, -2.0f, -2.0f));
		}

		ItemQuantityPlate->SetPadding(FMargin(2.0f, 0.0f));
		ItemQuantityPlate->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 14.0f),
			FLinearColor(0.36f, 0.38f, 0.40f, 0.50f),
			FLinearColor::Transparent,
			0.0f));
		ConfigureTextBlock(ItemQuantityText, FText::FromString(TEXT("1")), FLinearColor::White, 12);
		ItemQuantityText->SetJustification(ETextJustify::Right);
		ItemQuantityText->SetAutoWrapText(false);
		ItemQuantityText->SetLineHeightPercentage(0.72f);
		ItemQuantityPlate->SetContent(ItemQuantityText);
		UVerticalBoxSlot* QuantitySlot = SlotLabelStack->AddChildToVerticalBox(ItemQuantityPlate);
		if (QuantitySlot)
		{
			QuantitySlot->SetHorizontalAlignment(HAlign_Right);
			QuantitySlot->SetVerticalAlignment(VAlign_Bottom);
		}

		ItemNamePlate->SetPadding(FMargin(2.0f, 0.0f));
		ItemNamePlate->SetBrush(MakeRoundedBoxBrush(
			FVector2D(64.0f, 14.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.50f),
			FLinearColor::Transparent,
			0.0f));
		ConfigureTextBlock(ItemNameText, FText::FromString(TEXT("Item")), FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), 10);
		ItemNameText->SetJustification(ETextJustify::Right);
		ItemNameText->SetAutoWrapText(false);
		ItemNamePlate->SetContent(ItemNameText);
		UVerticalBoxSlot* NameSlot = SlotLabelStack->AddChildToVerticalBox(ItemNamePlate);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Right);
			NameSlot->SetVerticalAlignment(VAlign_Bottom);
			NameSlot->SetPadding(FMargin(0.0f));
		}

		ItemPricePlate->SetPadding(FMargin(0.0f, 1.0f));
		ItemPricePlate->SetBrush(MakeRoundedBoxBrush(
			FVector2D(86.0f, 18.0f),
			FLinearColor::Transparent,
			FLinearColor::Transparent,
			0.0f));
		ConfigureTextBlock(ItemPriceText, FText::FromString(TEXT("0")), FLinearColor(0.92f, 0.96f, 0.92f, 1.0f), 12);
		ItemPriceText->SetJustification(ETextJustify::Right);
		ItemPriceText->SetAutoWrapText(false);
		ItemPriceCoinSizeBox->SetWidthOverride(13.0f);
		ItemPriceCoinSizeBox->SetHeightOverride(13.0f);
		ItemPriceCoinImage->SetBrushFromTexture(UTunaSweeperCurrencyDisplayWidget::LoadCurrencyCoinIconTexture(), true);
		ItemPriceCoinImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		ItemPriceCoinSizeBox->SetContent(ItemPriceCoinImage);
		UHorizontalBoxSlot* PriceIconSlot = ItemPriceRow->AddChildToHorizontalBox(ItemPriceCoinSizeBox);
		if (PriceIconSlot)
		{
			PriceIconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PriceIconSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* PriceTextSlot = ItemPriceRow->AddChildToHorizontalBox(ItemPriceText);
		if (PriceTextSlot)
		{
			PriceTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			PriceTextSlot->SetVerticalAlignment(VAlign_Center);
			PriceTextSlot->SetPadding(FMargin(3.0f, 0.0f, 0.0f, 0.0f));
		}
		ItemPricePlate->SetContent(ItemPriceRow);
		ItemPricePlate->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* PriceSlot = RootStack->AddChildToVerticalBox(ItemPricePlate);
		if (PriceSlot)
		{
			PriceSlot->SetHorizontalAlignment(HAlign_Right);
			PriceSlot->SetVerticalAlignment(VAlign_Top);
			PriceSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, SlotSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, SlotBackground);
		RegisterWidgetVariable(WidgetBlueprint, IconBox);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentSlotNameText);
		RegisterWidgetVariable(WidgetBlueprint, ItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, ItemQuantityPlate);
		RegisterWidgetVariable(WidgetBlueprint, ItemQuantityText);
		RegisterWidgetVariable(WidgetBlueprint, AttachmentSlotIndicatorText);
		RegisterWidgetVariable(WidgetBlueprint, ItemNamePlate);
		RegisterWidgetVariable(WidgetBlueprint, ItemNameText);
		RegisterWidgetVariable(WidgetBlueprint, ItemPricePlate);
		RegisterWidgetVariable(WidgetBlueprint, ItemPriceCoinImage);
		RegisterWidgetVariable(WidgetBlueprint, ItemPriceText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudTopReserveWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* ReservedBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ReservedBackground"));
		UHorizontalBox* ModeTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeTabRow"));
		UButton* InventoryModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InventoryModeButton"));
		UButton* QuestModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestModeButton"));
		UButton* MapModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MapModeButton"));
		UButton* MemoModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MemoModeButton"));
		UButton* ResearchModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResearchModeButton"));
		USizeBox* InventoryModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryModeButtonFrame"));
		USizeBox* QuestModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestModeButtonFrame"));
		USizeBox* MapModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapModeButtonFrame"));
		USizeBox* MemoModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MemoModeButtonFrame"));
		USizeBox* ResearchModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ResearchModeButtonFrame"));
		UImage* InventoryModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InventoryModeIcon"));
		UImage* QuestModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("QuestModeIcon"));
		UImage* MapModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MapModeIcon"));
		UImage* MemoModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MemoModeIcon"));
		UImage* ResearchModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ResearchModeIcon"));
		if (!RootSizeBox || !ReservedBackground || !ModeTabRow ||
			!InventoryModeButton || !QuestModeButton || !MapModeButton || !MemoModeButton || !ResearchModeButton ||
			!InventoryModeButtonFrame || !QuestModeButtonFrame || !MapModeButtonFrame || !MemoModeButtonFrame || !ResearchModeButtonFrame ||
			!InventoryModeIcon || !QuestModeIcon || !MapModeIcon || !MemoModeIcon || !ResearchModeIcon)
		{
			return false;
		}

		UTexture2D* InventoryModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeInventoryIconAssetName));
		UTexture2D* QuestModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeQuestIconAssetName));
		UTexture2D* MapModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeMapIconAssetName));
		UTexture2D* MemoModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeMemoIconAssetName));
		UTexture2D* ResearchModeTexture = MemoModeTexture;

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(HudTopModeTabPanelWidth);
		RootSizeBox->SetHeightOverride(HudTopModeTabPanelHeight);
		RootSizeBox->SetContent(ReservedBackground);

		ReservedBackground->SetPadding(FMargin(HudTopModeTabPaddingX, HudTopModeTabPaddingY));
		ReservedBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(HudTopModeTabPanelWidth, HudTopModeTabPanelHeight),
			FLinearColor(0.005f, 0.006f, 0.008f, 0.48f),
			FLinearColor(0.15f, 0.17f, 0.19f, 0.42f),
			1.0f,
			5.0f));
		ReservedBackground->SetContent(ModeTabRow);

		auto ConfigureModeButton = [=](UButton* Button)
		{
			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.030f, 0.036f, 0.038f, 0.76f),
				FLinearColor(0.20f, 0.24f, 0.25f, 0.82f),
				1.0f,
				4.0f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.070f, 0.085f, 0.083f, 0.90f),
				FLinearColor(0.58f, 0.70f, 0.62f, 0.92f),
				1.5f,
				4.0f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.020f, 0.026f, 0.028f, 0.96f),
				FLinearColor(0.48f, 0.64f, 0.54f, 0.94f),
				1.0f,
				4.0f));
			Button->SetStyle(ButtonStyle);
		};

		auto ConfigureModeIcon = [](UImage* Icon, UTexture2D* Texture)
		{
			if (Texture)
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			Icon->SetDesiredSizeOverride(FVector2D(28.0f, 28.0f));
			Icon->SetColorAndOpacity(FLinearColor(0.74f, 0.80f, 0.82f, 1.0f));
		};

		auto AddModeTab = [&ModeTabRow, &ConfigureModeButton, &ConfigureModeIcon](
			USizeBox* Frame,
			UButton* Button,
			UImage* Icon,
			UTexture2D* Texture,
			bool bFirst)
		{
			Frame->SetWidthOverride(HudTopModeTabButtonWidth);
			Frame->SetHeightOverride(HudTopModeTabButtonHeight);
			Frame->SetContent(Button);
			ConfigureModeButton(Button);
			ConfigureModeIcon(Icon, Texture);
			Button->SetContent(Icon);
			if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Icon->Slot))
			{
				ButtonSlot->SetHorizontalAlignment(HAlign_Center);
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
				ButtonSlot->SetPadding(FMargin(0.0f));
			}
			UHorizontalBoxSlot* Slot = ModeTabRow->AddChildToHorizontalBox(Frame);
			if (Slot)
			{
				Slot->SetPadding(FMargin(bFirst ? 0.0f : HudTopModeTabGap, 0.0f, 0.0f, 0.0f));
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddModeTab(InventoryModeButtonFrame, InventoryModeButton, InventoryModeIcon, InventoryModeTexture, true);
		AddModeTab(QuestModeButtonFrame, QuestModeButton, QuestModeIcon, QuestModeTexture, false);
		AddModeTab(MapModeButtonFrame, MapModeButton, MapModeIcon, MapModeTexture, false);
		AddModeTab(MemoModeButtonFrame, MemoModeButton, MemoModeIcon, MemoModeTexture, false);
		AddModeTab(ResearchModeButtonFrame, ResearchModeButton, ResearchModeIcon, ResearchModeTexture, false);

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ReservedBackground);
		RegisterWidgetVariable(WidgetBlueprint, ModeTabRow);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeButton);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeButton);
		RegisterWidgetVariable(WidgetBlueprint, MapModeButton);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeButton);
		RegisterWidgetVariable(WidgetBlueprint, ResearchModeButton);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, MapModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, ResearchModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, MapModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, ResearchModeIcon);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudBottomStatusWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
		UHorizontalBox* VitalsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VitalsRow"));
		USizeBox* HealthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HealthBox"));
		UOverlay* HealthOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HealthOverlay"));
		UBorder* HealthBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HealthBackdrop"));
		UProgressBar* HealthGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthGauge"));
		UBorder* HealthIconBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HealthIconBackdrop"));
		USizeBox* HealthIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HealthIconBox"));
		UImage* HealthIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HealthIcon"));
		UTextBlock* HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
		USizeBox* ScratchBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ScratchBox"));
		UOverlay* ScratchOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ScratchOverlay"));
		UBorder* ScratchBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ScratchBackdrop"));
		UProgressBar* ScratchGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ScratchGauge"));
		USizeBox* HydrationBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HydrationBox"));
		UOverlay* HydrationOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HydrationOverlay"));
		UBorder* HydrationBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HydrationBackdrop"));
		UTunaSweeperHudStatusRingWidget* HydrationRing = WidgetTree->ConstructWidget<UTunaSweeperHudStatusRingWidget>(
			UTunaSweeperHudStatusRingWidget::StaticClass(),
			TEXT("HydrationRing"));
		USizeBox* HydrationIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HydrationIconBox"));
		UImage* HydrationIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HydrationIcon"));
		USizeBox* HungerBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HungerBox"));
		UOverlay* HungerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HungerOverlay"));
		UBorder* HungerBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HungerBackdrop"));
		UTunaSweeperHudStatusRingWidget* HungerRing = WidgetTree->ConstructWidget<UTunaSweeperHudStatusRingWidget>(
			UTunaSweeperHudStatusRingWidget::StaticClass(),
			TEXT("HungerRing"));
		USizeBox* HungerIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HungerIconBox"));
		UImage* HungerIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HungerIcon"));

		if (!RootSizeBox || !RootOverlay || !VitalsRow ||
			!HealthBox || !HealthOverlay || !HealthBackdrop || !HealthGauge || !HealthIconBackdrop || !HealthIconBox || !HealthIcon || !HealthText ||
			!ScratchBox || !ScratchOverlay || !ScratchBackdrop || !ScratchGauge ||
			!HydrationBox || !HydrationOverlay || !HydrationBackdrop || !HydrationRing || !HydrationIconBox || !HydrationIcon ||
			!HungerBox || !HungerOverlay || !HungerBackdrop || !HungerRing || !HungerIconBox || !HungerIcon)
		{
			return false;
		}

		UTexture2D* HealthIconTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudStatusHeartIconAssetName));
		UTexture2D* HydrationIconTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudStatusWaterIconAssetName));
		UTexture2D* HungerIconTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudStatusMeatIconAssetName));
		UMaterialInterface* ScratchGaugeMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			*GetAssetObjectPath(UIAssetPath, ScratchGaugeMaterialAssetName));

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(GameplayBottomStatusWidth);
		RootSizeBox->SetHeightOverride(GameplayBottomStatusHeight);
		RootSizeBox->SetContent(RootOverlay);

		UOverlaySlot* VitalsRowSlot = RootOverlay->AddChildToOverlay(VitalsRow);
		if (VitalsRowSlot)
		{
			VitalsRowSlot->SetHorizontalAlignment(HAlign_Center);
			VitalsRowSlot->SetVerticalAlignment(VAlign_Center);
		}

		HealthBox->SetWidthOverride(154.0f);
		HealthBox->SetHeightOverride(42.0f);
		HealthBox->SetContent(HealthOverlay);

		HealthBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(154.0f, 42.0f),
			FLinearColor(0.012f, 0.015f, 0.018f, 0.54f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.42f),
			1.0f,
			12.0f));
		UOverlaySlot* HealthBackdropSlot = HealthOverlay->AddChildToOverlay(HealthBackdrop);
		if (HealthBackdropSlot)
		{
			HealthBackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			HealthBackdropSlot->SetVerticalAlignment(VAlign_Fill);
		}

		FProgressBarStyle HealthGaugeStyle;
		HealthGaugeStyle.SetBackgroundImage(MakeRoundedBoxBrush(
			FVector2D(146.0f, 26.0f),
			FLinearColor(0.02f, 0.022f, 0.026f, 0.72f),
			FLinearColor(0.02f, 0.022f, 0.026f, 0.72f),
			0.0f,
			10.0f));
		HealthGaugeStyle.SetFillImage(MakeRoundedBoxBrush(
			FVector2D(146.0f, 26.0f),
			FLinearColor(0.96f, 0.28f, 0.34f, 1.0f),
			FLinearColor(1.0f, 0.55f, 0.60f, 0.88f),
			1.0f,
			10.0f));
		HealthGauge->SetWidgetStyle(HealthGaugeStyle);
		HealthGauge->SetBarFillType(EProgressBarFillType::LeftToRight);
		HealthGauge->SetFillColorAndOpacity(FLinearColor::White);
		HealthGauge->SetPercent(1.0f);
		UOverlaySlot* HealthGaugeSlot = HealthOverlay->AddChildToOverlay(HealthGauge);
		if (HealthGaugeSlot)
		{
			HealthGaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			HealthGaugeSlot->SetVerticalAlignment(VAlign_Fill);
			HealthGaugeSlot->SetPadding(FMargin(4.0f, 8.0f, 4.0f, 8.0f));
		}

		HealthIconBackdrop->SetPadding(FMargin(4.0f));
		HealthIconBackdrop->SetBrush(MakeCircularBrush(
			FVector2D(30.0f, 30.0f),
			FLinearColor(0.18f, 0.018f, 0.035f, 0.96f),
			FLinearColor(1.0f, 0.45f, 0.55f, 0.95f),
			1.0f));
		HealthIconBox->SetWidthOverride(20.0f);
		HealthIconBox->SetHeightOverride(20.0f);
		HealthIconBox->SetContent(HealthIcon);
		if (HealthIconTexture)
		{
			HealthIcon->SetBrushFromTexture(HealthIconTexture, true);
			FSlateBrush HealthIconBrush = HealthIcon->GetBrush();
			HealthIconBrush.SetImageSize(FVector2D(20.0f, 20.0f));
			HealthIcon->SetBrush(HealthIconBrush);
			HealthIcon->SetColorAndOpacity(FLinearColor::White);
		}
		HealthIconBackdrop->SetContent(HealthIconBox);
		UOverlaySlot* HealthIconBackdropSlot = HealthOverlay->AddChildToOverlay(HealthIconBackdrop);
		if (HealthIconBackdropSlot)
		{
			HealthIconBackdropSlot->SetHorizontalAlignment(HAlign_Left);
			HealthIconBackdropSlot->SetVerticalAlignment(VAlign_Center);
			HealthIconBackdropSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
		}

		ConfigureTextBlock(HealthText, FText::FromString(TEXT("40 / 40")), FLinearColor(0.98f, 0.98f, 0.98f, 1.0f), 11);
		TunaSweeperUIFont::ApplyFont(HealthText, 11.0f, ETunaSweeperUIFontWeight::Bold);
		HealthText->SetShadowOffset(FVector2D(0.0f, 1.0f));
		HealthText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
		UOverlaySlot* HealthTextSlot = HealthOverlay->AddChildToOverlay(HealthText);
		if (HealthTextSlot)
		{
			HealthTextSlot->SetHorizontalAlignment(HAlign_Center);
			HealthTextSlot->SetVerticalAlignment(VAlign_Center);
			HealthTextSlot->SetPadding(FMargin(26.0f, 0.0f, 6.0f, 0.0f));
		}

		ScratchBox->SetWidthOverride(14.0f);
		ScratchBox->SetHeightOverride(44.0f);
		ScratchBox->SetContent(ScratchOverlay);
		ScratchBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(14.0f, 44.0f),
			FLinearColor(0.012f, 0.015f, 0.02f, 0.9f),
			FLinearColor(0.68f, 0.72f, 0.82f, 0.5f),
			1.0f,
			5.0f));
		if (UOverlaySlot* ScratchBackdropSlot = ScratchOverlay->AddChildToOverlay(ScratchBackdrop))
		{
			ScratchBackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			ScratchBackdropSlot->SetVerticalAlignment(VAlign_Fill);
		}

		FProgressBarStyle ScratchGaugeStyle;
		ScratchGaugeStyle.SetBackgroundImage(MakeRoundedBoxBrush(
			FVector2D(8.0f, 38.0f),
			FLinearColor::Transparent,
			FLinearColor::Transparent,
			0.0f,
			3.0f));
		if (ScratchGaugeMaterial)
		{
			FSlateBrush ScratchFillBrush;
			ScratchFillBrush.DrawAs = ESlateBrushDrawType::Image;
			ScratchFillBrush.SetImageSize(FVector2D(8.0f, 38.0f));
			ScratchFillBrush.SetResourceObject(ScratchGaugeMaterial);
			ScratchFillBrush.TintColor = FSlateColor(FLinearColor::White);
			ScratchGaugeStyle.SetFillImage(ScratchFillBrush);
		}
		else
		{
			ScratchGaugeStyle.SetFillImage(MakeRoundedBoxBrush(
				FVector2D(8.0f, 38.0f),
				FLinearColor(0.62f, 0.9f, 1.0f, 1.0f),
				FLinearColor::White,
				0.0f,
				3.0f));
		}
		ScratchGauge->SetWidgetStyle(ScratchGaugeStyle);
		ScratchGauge->SetBarFillType(EProgressBarFillType::BottomToTop);
		ScratchGauge->SetFillColorAndOpacity(FLinearColor::White);
		ScratchGauge->SetPercent(0.0f);
		if (UOverlaySlot* ScratchGaugeSlot = ScratchOverlay->AddChildToOverlay(ScratchGauge))
		{
			ScratchGaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			ScratchGaugeSlot->SetVerticalAlignment(VAlign_Fill);
			ScratchGaugeSlot->SetPadding(FMargin(3.0f));
		}

		auto ConfigureRingStatusSlot = [](
			USizeBox* Box,
			UOverlay* Overlay,
			UBorder* Backdrop,
			UTunaSweeperHudStatusRingWidget* Ring,
			USizeBox* IconBox,
			UImage* Icon,
			UTexture2D* IconTexture,
			const FLinearColor& FillColor)
		{
			const FLinearColor TrackColor(0.015f, 0.018f, 0.022f, 0.92f);
			Box->SetWidthOverride(52.0f);
			Box->SetHeightOverride(52.0f);
			Box->SetContent(Overlay);

			Backdrop->SetBrush(MakeCircularBrush(
				FVector2D(52.0f, 52.0f),
				FLinearColor(0.008f, 0.010f, 0.013f, 0.72f),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.84f),
				1.0f));
			UOverlaySlot* BackdropSlot = Overlay->AddChildToOverlay(Backdrop);
			if (BackdropSlot)
			{
				BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
				BackdropSlot->SetVerticalAlignment(VAlign_Fill);
			}

			Ring->SetRingColors(TrackColor, FillColor);
			Ring->SetStatusPercent(1.0f);
			UOverlaySlot* RingSlot = Overlay->AddChildToOverlay(Ring);
			if (RingSlot)
			{
				RingSlot->SetHorizontalAlignment(HAlign_Fill);
				RingSlot->SetVerticalAlignment(VAlign_Fill);
			}

			IconBox->SetWidthOverride(26.0f);
			IconBox->SetHeightOverride(26.0f);
			IconBox->SetContent(Icon);
			if (IconTexture)
			{
				Icon->SetBrushFromTexture(IconTexture, true);
				FSlateBrush IconBrush = Icon->GetBrush();
				IconBrush.SetImageSize(FVector2D(26.0f, 26.0f));
				Icon->SetBrush(IconBrush);
				Icon->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				Icon->SetBrush(MakeCircularBrush(
					FVector2D(30.0f, 30.0f),
					FillColor,
					FLinearColor::Transparent,
					0.0f));
			}

			UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(IconBox);
			if (IconSlot)
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
			}
		};

		ConfigureRingStatusSlot(
			HydrationBox,
			HydrationOverlay,
			HydrationBackdrop,
			HydrationRing,
			HydrationIconBox,
			HydrationIcon,
			HydrationIconTexture,
			FLinearColor(0.23f, 0.63f, 1.0f, 1.0f));
		ConfigureRingStatusSlot(
			HungerBox,
			HungerOverlay,
			HungerBackdrop,
			HungerRing,
			HungerIconBox,
			HungerIcon,
			HungerIconTexture,
			FLinearColor(0.96f, 0.68f, 0.22f, 1.0f));

		UHorizontalBoxSlot* HealthBoxSlot = VitalsRow->AddChildToHorizontalBox(HealthBox);
		if (HealthBoxSlot)
		{
			HealthBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* ScratchBoxSlot = VitalsRow->AddChildToHorizontalBox(ScratchBox);
		if (ScratchBoxSlot)
		{
			ScratchBoxSlot->SetPadding(FMargin(5.0f, 0.0f, 0.0f, 0.0f));
			ScratchBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* HydrationBoxSlot = VitalsRow->AddChildToHorizontalBox(HydrationBox);
		if (HydrationBoxSlot)
		{
			HydrationBoxSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			HydrationBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* HungerBoxSlot = VitalsRow->AddChildToHorizontalBox(HungerBox);
		if (HungerBoxSlot)
		{
			HungerBoxSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			HungerBoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterWidgetVariable(WidgetBlueprint, HealthGauge);
		RegisterWidgetVariable(WidgetBlueprint, ScratchGauge);
		RegisterWidgetVariable(WidgetBlueprint, HealthText);
		RegisterWidgetVariable(WidgetBlueprint, HungerRing);
		RegisterWidgetVariable(WidgetBlueprint, HydrationRing);
		RegisterWidgetVariable(WidgetBlueprint, HealthIcon);
		RegisterWidgetVariable(WidgetBlueprint, HungerIcon);
		RegisterWidgetVariable(WidgetBlueprint, HydrationIcon);
		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, RootOverlay);
		RegisterWidgetVariable(WidgetBlueprint, VitalsRow);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudDebuffBarWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UHorizontalBox* DebuffRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("DebuffRowRoot"));
		if (!DebuffRow)
		{
			return false;
		}

		WidgetTree->RootWidget = DebuffRow;
		DebuffRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

		RegisterWidgetVariable(WidgetBlueprint, DebuffRow);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudQuickSlotBarWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UHorizontalBox* AmmoSelectorPanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AmmoSelectorPanel"));
		UBorder* AmmoSelectorPromptBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AmmoSelectorPromptBackground"));
		UTextBlock* AmmoSelectorPromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSelectorPromptText"));
		UBorder* AmmoSelectorKeyBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AmmoSelectorKeyBackground"));
		UTextBlock* AmmoSelectorKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSelectorKeyText"));
		UHorizontalBox* CancelableActionPromptRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CancelableActionPromptRoot"));
		UBorder* CancelableActionCancelKeyBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CancelableActionCancelKeyBackground"));
		UTextBlock* CancelableActionCancelKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelableActionCancelKeyText"));
		UTextBlock* CancelableActionCancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelableActionCancelText"));
		USizeBox* CancelableActionProgressPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CancelableActionProgressPanel"));
		UProgressBar* CancelableActionProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CancelableActionProgressBar"));
		UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRow"));
		if (!RootSizeBox || !RootCanvas || !AmmoSelectorPanel || !AmmoSelectorPromptBackground || !AmmoSelectorPromptText ||
			!AmmoSelectorKeyBackground || !AmmoSelectorKeyText ||
			!CancelableActionPromptRoot || !CancelableActionCancelKeyBackground || !CancelableActionCancelKeyText || !CancelableActionCancelText ||
			!CancelableActionProgressPanel || !CancelableActionProgressBar || !SlotRow)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(GameplayBottomQuickSlotWidth);
		RootSizeBox->SetHeightOverride(GameplayBottomQuickSlotHeight);
		RootSizeBox->SetContent(RootCanvas);

		AmmoSelectorPanel->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* AmmoSelectorSlot = RootCanvas->AddChildToCanvas(AmmoSelectorPanel);
		if (AmmoSelectorSlot)
		{
			AmmoSelectorSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			AmmoSelectorSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			AmmoSelectorSlot->SetPosition(FVector2D(0.0f, 28.0f));
			AmmoSelectorSlot->SetAutoSize(true);
			AmmoSelectorSlot->SetSize(FVector2D(0.0f, 26.0f));
		}

		AmmoSelectorPromptBackground->SetPadding(FMargin(8.0f, 3.0f));
		AmmoSelectorPromptBackground->SetVisibility(ESlateVisibility::Collapsed);
		AmmoSelectorPromptBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(98.0f, 24.0f),
			FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
			FLinearColor(0.7f, 0.85f, 0.55f, 1.0f),
			1.0f));
		ConfigureTextBlock(AmmoSelectorPromptText, FText::FromString(TEXT("탄약 미지정")), FLinearColor(0.9f, 0.96f, 0.88f, 1.0f), 11);
		AmmoSelectorPromptBackground->SetContent(AmmoSelectorPromptText);
		UHorizontalBoxSlot* PromptSlot = AmmoSelectorPanel->AddChildToHorizontalBox(AmmoSelectorPromptBackground);
		if (PromptSlot)
		{
			PromptSlot->SetVerticalAlignment(VAlign_Center);
		}

		for (int32 OptionNumber = 1; OptionNumber <= 6; ++OptionNumber)
		{
			UBorder* OptionBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("AmmoOption%dBackground"), OptionNumber)));
			UTextBlock* OptionText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*FString::Printf(TEXT("AmmoOption%dText"), OptionNumber)));
			if (!OptionBackground || !OptionText)
			{
				return false;
			}

			OptionBackground->SetPadding(FMargin(8.0f, 3.0f));
			OptionBackground->SetVisibility(ESlateVisibility::Collapsed);
			OptionBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(96.0f, 24.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.7f, 0.85f, 0.55f, 1.0f),
				1.0f));
			ConfigureTextBlock(OptionText, FText::GetEmpty(), FLinearColor(0.9f, 0.96f, 0.88f, 1.0f), 11);
			OptionBackground->SetContent(OptionText);

			UHorizontalBoxSlot* OptionSlot = AmmoSelectorPanel->AddChildToHorizontalBox(OptionBackground);
			if (OptionSlot)
			{
				OptionSlot->SetPadding(FMargin(OptionNumber == 1 ? 0.0f : 4.0f, 0.0f, 0.0f, 0.0f));
				OptionSlot->SetVerticalAlignment(VAlign_Center);
			}

			RegisterWidgetVariable(WidgetBlueprint, OptionBackground);
			RegisterWidgetVariable(WidgetBlueprint, OptionText);
		}

		AmmoSelectorKeyBackground->SetPadding(FMargin(7.0f, 2.0f));
		AmmoSelectorKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
		AmmoSelectorKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(22.0f, 22.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			0.0f));
		ConfigureTextBlock(AmmoSelectorKeyText, FText::FromString(TEXT("T")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 11);
		AmmoSelectorKeyBackground->SetContent(AmmoSelectorKeyText);
		UHorizontalBoxSlot* KeySlot = AmmoSelectorPanel->AddChildToHorizontalBox(AmmoSelectorKeyBackground);
		if (KeySlot)
		{
			KeySlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		CancelableActionPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CancelableActionPromptSlot = RootCanvas->AddChildToCanvas(CancelableActionPromptRoot);
		if (CancelableActionPromptSlot)
		{
			CancelableActionPromptSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			CancelableActionPromptSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CancelableActionPromptSlot->SetPosition(FVector2D(0.0f, 6.0f));
			CancelableActionPromptSlot->SetAutoSize(true);
			CancelableActionPromptSlot->SetZOrder(12);
		}

		CancelableActionCancelKeyBackground->SetPadding(FMargin(8.0f, 2.0f));
		CancelableActionCancelKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
		CancelableActionCancelKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 24.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f,
			7.0f));
		ConfigureTextBlock(CancelableActionCancelKeyText, FText::FromString(TEXT("X")), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f), 12);
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelKeyText, 12.0f, ETunaSweeperUIFontWeight::Bold);
		CancelableActionCancelKeyText->SetVisibility(ESlateVisibility::Collapsed);
		CancelableActionCancelKeyBackground->SetContent(CancelableActionCancelKeyText);
		UHorizontalBoxSlot* CancelableActionCancelKeySlot = CancelableActionPromptRoot->AddChildToHorizontalBox(CancelableActionCancelKeyBackground);
		if (CancelableActionCancelKeySlot)
		{
			CancelableActionCancelKeySlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(CancelableActionCancelText, FText::FromString(TEXT("\uB3D9\uC791 \uCDE8\uC18C")), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), 14);
		TunaSweeperUIFont::ApplyFont(CancelableActionCancelText, 14.0f, ETunaSweeperUIFontWeight::Bold);
		CancelableActionCancelText->SetShadowOffset(FVector2D(0.0f, 1.0f));
		CancelableActionCancelText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		CancelableActionCancelText->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* CancelableActionCancelTextSlot = CancelableActionPromptRoot->AddChildToHorizontalBox(CancelableActionCancelText);
		if (CancelableActionCancelTextSlot)
		{
			CancelableActionCancelTextSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CancelableActionCancelTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		CancelableActionProgressPanel->SetWidthOverride(210.0f);
		CancelableActionProgressPanel->SetHeightOverride(18.0f);
		CancelableActionProgressPanel->SetVisibility(ESlateVisibility::Collapsed);
		CancelableActionProgressBar->SetPercent(0.0f);
		FProgressBarStyle CancelableActionProgressStyle;
		CancelableActionProgressStyle.SetBackgroundImage(MakeRoundedBoxBrush(
			FVector2D(210.0f, 18.0f),
			FLinearColor(0.012f, 0.016f, 0.020f, 0.88f),
			FLinearColor(0.90f, 1.0f, 0.88f, 0.42f),
			1.0f,
			9.0f));
		CancelableActionProgressStyle.SetFillImage(MakeRoundedBoxBrush(
			FVector2D(210.0f, 18.0f),
			FLinearColor(0.50f, 1.0f, 0.68f, 1.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f,
			9.0f));
		CancelableActionProgressStyle.SetMarqueeImage(MakeRoundedBoxBrush(
			FVector2D(210.0f, 18.0f),
			FLinearColor(0.64f, 1.0f, 0.78f, 1.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			0.0f,
			9.0f));
		CancelableActionProgressBar->SetWidgetStyle(CancelableActionProgressStyle);
		CancelableActionProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
		CancelableActionProgressBar->SetFillColorAndOpacity(FLinearColor::White);
		CancelableActionProgressPanel->SetContent(CancelableActionProgressBar);
		UCanvasPanelSlot* CancelableActionProgressSlot = RootCanvas->AddChildToCanvas(CancelableActionProgressPanel);
		if (CancelableActionProgressSlot)
		{
			CancelableActionProgressSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			CancelableActionProgressSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			CancelableActionProgressSlot->SetPosition(FVector2D(0.0f, 36.0f));
			CancelableActionProgressSlot->SetSize(FVector2D(210.0f, 18.0f));
			CancelableActionProgressSlot->SetZOrder(11);
		}

		UCanvasPanelSlot* SlotRowCanvasSlot = RootCanvas->AddChildToCanvas(SlotRow);
		if (SlotRowCanvasSlot)
		{
			SlotRowCanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			SlotRowCanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			SlotRowCanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
			SlotRowCanvasSlot->SetSize(FVector2D(690.0f, 146.0f));
		}

		const FString DefaultIconPaths[8] = {
			TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol"),
			TEXT("/Game/UI/Icons/T_UIIcon_Rifle.T_UIIcon_Rifle"),
			TEXT("/Game/UI/Icons/T_UIIcon_Bandage.T_UIIcon_Bandage"),
			TEXT("/Game/UI/Icons/T_UIIcon_FirstAidKit.T_UIIcon_FirstAidKit"),
			TEXT("/Game/UI/Icons/T_UIIcon_CannedFood.T_UIIcon_CannedFood"),
			TEXT("/Game/UI/Icons/T_UIIcon_WaterBottle.T_UIIcon_WaterBottle"),
			TEXT("/Game/UI/Icons/T_UIIcon_Painkillers.T_UIIcon_Painkillers"),
			TEXT("/Game/UI/Icons/T_UIIcon_EnergyBar.T_UIIcon_EnergyBar")
		};
		const FString MeleeDefaultIconPath = TEXT("/Game/UI/Icons/T_UIIcon_CombatKnife.T_UIIcon_CombatKnife");

		for (int32 DisplaySlotIndex = 0; DisplaySlotIndex < 9; ++DisplaySlotIndex)
		{
			const bool bMeleeSlot = DisplaySlotIndex == 2;
			const int32 SlotNumber = bMeleeSlot ? INDEX_NONE : (DisplaySlotIndex < 2 ? DisplaySlotIndex + 1 : DisplaySlotIndex);
			const FString SlotWidgetPrefix = bMeleeSlot
				? FString(TEXT("QuickSlotMelee"))
				: FString::Printf(TEXT("QuickSlot%d"), SlotNumber);
			const bool bWeaponSlot = !bMeleeSlot && SlotNumber <= 2;
			const float SlotSize = bWeaponSlot ? 82.0f : 66.0f;
			const float IconSize = bWeaponSlot ? 68.0f : 54.0f;
			const FString DefaultIconPath = bMeleeSlot ? MeleeDefaultIconPath : DefaultIconPaths[SlotNumber - 1];
			const FText SlotLabelText = bMeleeSlot ? FText::FromString(TEXT("V")) : FText::AsNumber(SlotNumber);

			UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Stack"))));
			USizeBox* SlotAmmoTypeContainer = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeContainer"))));
			UHorizontalBox* SlotAmmoTypeRow = WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeRow"))));
			UBorder* SlotAmmoTypeBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeBackground"))));
			UTextBlock* SlotAmmoTypeText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeText"))));
			UBorder* SlotAmmoKeyBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoKeyBackground"))));
			UTextBlock* SlotAmmoKeyText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoKeyText"))));
			USizeBox* SlotAmmoTextContainer = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTextContainer"))));
			UBorder* SlotAmmoTextBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTextBackground"))));
			UTextBlock* SlotAmmoText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoText"))));
			USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("SizeBox"))));
			UBorder* SlotBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Background"))));
			UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Overlay"))));
			UImage* SlotIcon = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Icon"))));
			UBorder* SlotNumberBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("NumberBackground"))));
			UTextBlock* SlotNumberText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("NumberText"))));
			UBorder* SelectionFrame = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("SelectionFrame"))));

			if (!SlotStack || !SlotAmmoTypeContainer || !SlotAmmoTypeRow || !SlotAmmoTypeBackground || !SlotAmmoTypeText ||
				!SlotAmmoKeyBackground || !SlotAmmoKeyText || !SlotAmmoTextContainer || !SlotAmmoTextBackground || !SlotAmmoText ||
				!SlotSizeBox || !SlotBackground || !SlotOverlay || !SlotIcon || !SlotNumberBackground || !SlotNumberText || !SelectionFrame)
			{
				return false;
			}

			SlotAmmoTypeContainer->SetWidthOverride(SlotSize);
			SlotAmmoTypeContainer->SetHeightOverride(18.0f);
			SlotAmmoTypeContainer->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTypeContainer->SetClipping(EWidgetClipping::ClipToBounds);
			SlotAmmoTypeBackground->SetPadding(FMargin(5.0f, 2.0f));
			SlotAmmoTypeBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(62.0f, 17.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				0.0f));
			ConfigureTextBlock(SlotAmmoTypeText, FText::GetEmpty(), FLinearColor(0.92f, 0.96f, 0.88f, 1.0f), 8);
			SlotAmmoTypeText->SetShadowOffset(FVector2D(0.0f, 1.0f));
			SlotAmmoTypeText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
			SlotAmmoTypeText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTypeBackground->SetContent(SlotAmmoTypeText);
			UHorizontalBoxSlot* AmmoTypeTextSlot = SlotAmmoTypeRow->AddChildToHorizontalBox(SlotAmmoTypeBackground);
			if (AmmoTypeTextSlot)
			{
				AmmoTypeTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				AmmoTypeTextSlot->SetVerticalAlignment(VAlign_Center);
			}
			SlotAmmoKeyBackground->SetPadding(FMargin(4.0f, 1.0f));
			SlotAmmoKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoKeyBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(15.0f, 15.0f),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
				0.0f));
			ConfigureTextBlock(SlotAmmoKeyText, FText::FromString(TEXT("T")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 8);
			SlotAmmoKeyText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoKeyBackground->SetContent(SlotAmmoKeyText);
			UHorizontalBoxSlot* AmmoKeySlot = SlotAmmoTypeRow->AddChildToHorizontalBox(SlotAmmoKeyBackground);
			if (AmmoKeySlot)
			{
				AmmoKeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				AmmoKeySlot->SetVerticalAlignment(VAlign_Center);
				AmmoKeySlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
			}
			SlotAmmoTypeContainer->SetContent(SlotAmmoTypeRow);
			UVerticalBoxSlot* AmmoTypeSlot = SlotStack->AddChildToVerticalBox(SlotAmmoTypeContainer);
			if (AmmoTypeSlot)
			{
				AmmoTypeSlot->SetHorizontalAlignment(HAlign_Center);
				AmmoTypeSlot->SetPadding(FMargin(0.0f));
			}

			SlotAmmoTextContainer->SetWidthOverride(bWeaponSlot ? 64.0f : SlotSize);
			SlotAmmoTextContainer->SetHeightOverride(18.0f);
			SlotAmmoTextContainer->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTextContainer->SetClipping(EWidgetClipping::ClipToBounds);
			SlotAmmoTextBackground->SetPadding(FMargin(6.0f, 2.0f));
			SlotAmmoTextBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(64.0f, 18.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				0.0f));
			ConfigureTextBlock(SlotAmmoText, FText::GetEmpty(), FLinearColor(0.95f, 0.97f, 1.0f, 1.0f), 11);
			SlotAmmoText->SetShadowOffset(FVector2D(0.0f, 1.0f));
			SlotAmmoText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
			SlotAmmoText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTextBackground->SetContent(SlotAmmoText);
			SlotAmmoTextContainer->SetContent(SlotAmmoTextBackground);
			UVerticalBoxSlot* AmmoTextSlot = SlotStack->AddChildToVerticalBox(SlotAmmoTextContainer);
			if (AmmoTextSlot)
			{
				AmmoTextSlot->SetHorizontalAlignment(HAlign_Center);
				AmmoTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
			}

			SlotSizeBox->SetWidthOverride(SlotSize);
			SlotSizeBox->SetHeightOverride(SlotSize);
			SlotSizeBox->SetContent(SlotBackground);

			SlotBackground->SetPadding(FMargin(4.0f));
			SlotBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(SlotSize, SlotSize),
				FLinearColor(0.012f, 0.014f, 0.016f, 0.88f),
				FLinearColor(0.22f, 0.25f, 0.3f, 0.95f),
				1.0f));
			SlotBackground->SetContent(SlotOverlay);

			if (UTexture2D* DefaultIcon = LoadObject<UTexture2D>(nullptr, *DefaultIconPath))
			{
				SlotIcon->SetBrushFromTexture(DefaultIcon, true);
			}
			SlotIcon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
			SlotIcon->SetColorAndOpacity(FLinearColor::White);

			UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(SlotIcon);
			if (IconSlot)
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin((SlotSize - IconSize) * 0.25f));
			}

			SelectionFrame->SetVisibility(!bMeleeSlot && SlotNumber == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			SelectionFrame->SetBrush(MakeRoundedBoxBrush(
				FVector2D(SlotSize, SlotSize),
				FLinearColor::Transparent,
				FLinearColor(0.98f, 0.82f, 0.22f, 1.0f),
				2.0f));
			UOverlaySlot* SelectionSlot = SlotOverlay->AddChildToOverlay(SelectionFrame);
			if (SelectionSlot)
			{
				SelectionSlot->SetHorizontalAlignment(HAlign_Fill);
				SelectionSlot->SetVerticalAlignment(VAlign_Fill);
			}

			UVerticalBoxSlot* SlotBoxStackSlot = SlotStack->AddChildToVerticalBox(SlotSizeBox);
			if (SlotBoxStackSlot)
			{
				SlotBoxStackSlot->SetHorizontalAlignment(HAlign_Center);
			}

			SlotNumberBackground->SetPadding(FMargin(7.0f, 1.0f));
			SlotNumberBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(24.0f, 18.0f),
				FLinearColor(0.98f, 0.98f, 0.96f, 0.96f),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.78f),
				1.0f,
				4.0f));
			ConfigureTextBlock(SlotNumberText, SlotLabelText, FLinearColor(0.02f, 0.024f, 0.028f, 1.0f), 11);
			TunaSweeperUIFont::ApplyFont(SlotNumberText, 11.0f, ETunaSweeperUIFontWeight::Bold);
			SlotNumberBackground->SetContent(SlotNumberText);
			UVerticalBoxSlot* SlotNumberStackSlot = SlotStack->AddChildToVerticalBox(SlotNumberBackground);
			if (SlotNumberStackSlot)
			{
				SlotNumberStackSlot->SetHorizontalAlignment(HAlign_Center);
				SlotNumberStackSlot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
			}

			UHorizontalBoxSlot* SlotRowSlot = SlotRow->AddChildToHorizontalBox(SlotStack);
			if (SlotRowSlot)
			{
				SlotRowSlot->SetPadding(FMargin(DisplaySlotIndex == 0 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
				SlotRowSlot->SetVerticalAlignment(VAlign_Bottom);
			}

			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeContainer);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeRow);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeText);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoKeyBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoKeyText);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTextContainer);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTextBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoText);
			RegisterWidgetVariable(WidgetBlueprint, SlotIcon);
			RegisterWidgetVariable(WidgetBlueprint, SelectionFrame);
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPanel);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPromptBackground);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPromptText);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorKeyBackground);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorKeyText);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionPromptRoot);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionCancelKeyBackground);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionCancelKeyText);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionCancelText);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionProgressPanel);
		RegisterWidgetVariable(WidgetBlueprint, CancelableActionProgressBar);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	UBorder* BuildHudSimplePanel(
		UWidgetTree* WidgetTree,
		const FName& PanelName,
		const FText& Title,
		const FVector2D& PanelSize,
		const FLinearColor& AccentColor)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), PanelName);
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*(PanelName.ToString() + TEXT("Stack"))));
		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(PanelName.ToString() + TEXT("TitleText"))));
		if (!Panel || !PanelStack || !TitleText)
		{
			return nullptr;
		}

		Panel->SetPadding(FMargin(14.0f));
		Panel->SetBrush(MakeRoundedBoxBrush(
			PanelSize,
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			AccentColor,
			1.0f));
		Panel->SetContent(PanelStack);

		ConfigureTextBlockLeft(TitleText, Title, FLinearColor::White, 18);
		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		return Panel;
	}

	bool BuildHudInventoryAreaWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootRow"));
		USizeBox* MainInventorySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainInventorySizeBox"));
		UBorder* InventoryPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryPanel"));
		UVerticalBox* InventoryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryStack"));
		UHorizontalBox* InventoryHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryHeaderRow"));
		UTextBlock* InventoryTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryTitleText"));
		UTunaSweeperCurrencyDisplayWidget* CurrencyDisplayWidget =
			WidgetTree->ConstructWidget<UTunaSweeperCurrencyDisplayWidget>(
				UTunaSweeperCurrencyDisplayWidget::StaticClass(),
				TEXT("CurrencyDisplayWidget"));
		UButton* SortInventoryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SortInventoryButton"));
		UTextBlock* SortInventoryButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SortInventoryButtonText"));
		USizeBox* InventorySortControlArea = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventorySortControlArea"));
		UHorizontalBox* InventorySortControlRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventorySortControlRow"));
		USizeBox* InventorySortControlSpacer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventorySortControlSpacer"));
		USizeBox* EquipmentReserveSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EquipmentReserveSizeBox"));
		UTileView* EquipmentReserveTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("EquipmentReserveTileView"));
		USizeBox* AuxiliaryBagPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AuxiliaryBagPanel"));
		UBorder* AuxiliaryBagBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AuxiliaryBagBackground"));
		UTileView* AuxiliaryBagTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("AuxiliaryBagTileView"));
		UTileView* InventoryTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("InventoryTileView"));
		UBorder* InventoryWeightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryWeightPanel"));
		UHorizontalBox* InventoryWeightRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryWeightRow"));
		UTextBlock* InventoryWeightLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightLabelText"));
		USizeBox* InventoryWeightGaugeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryWeightGaugeBox"));
		UOverlay* InventoryWeightGaugeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("InventoryWeightGaugeOverlay"));
		UProgressBar* InventoryWeightGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("InventoryWeightGauge"));
		UCanvasPanel* InventoryWeightMarkerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InventoryWeightMarkerCanvas"));
		UTextBlock* InventoryWeightOverweightMarker = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightOverweightMarker"));
		UTextBlock* InventoryWeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightText"));
		UBorder* InventoryWeightWarningIcon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryWeightWarningIcon"));
		UTextBlock* InventoryWeightWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightWarningText"));

		if (!RootSizeBox || !RootRow || !MainInventorySizeBox || !InventoryPanel || !InventoryStack || !InventoryHeaderRow ||
			!InventoryTitleText || !CurrencyDisplayWidget || !SortInventoryButton || !SortInventoryButtonText ||
			!InventorySortControlArea || !InventorySortControlRow || !InventorySortControlSpacer || !EquipmentReserveSizeBox ||
			!EquipmentReserveTileView || !AuxiliaryBagPanel || !AuxiliaryBagBackground || !AuxiliaryBagTileView || !InventoryTileView ||
			!InventoryWeightPanel || !InventoryWeightRow || !InventoryWeightLabelText || !InventoryWeightGaugeBox ||
			!InventoryWeightGaugeOverlay || !InventoryWeightGauge || !InventoryWeightMarkerCanvas || !InventoryWeightOverweightMarker ||
			!InventoryWeightText || !InventoryWeightWarningIcon || !InventoryWeightWarningText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(InventoryAreaPanelWidth);
		RootSizeBox->SetContent(RootRow);

		MainInventorySizeBox->SetWidthOverride(InventoryPanelWidth);
		MainInventorySizeBox->SetContent(InventoryPanel);
		UHorizontalBoxSlot* MainInventorySlot = RootRow->AddChildToHorizontalBox(MainInventorySizeBox);
		if (MainInventorySlot)
		{
			MainInventorySlot->SetVerticalAlignment(VAlign_Fill);
		}

		InventoryPanel->SetPadding(FMargin(InventoryPanelPadding));
		InventoryPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(InventoryPanelWidth, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.28f, 0.36f, 0.44f, 1.0f),
			1.0f));
		InventoryPanel->SetContent(InventoryStack);

		ConfigureTextBlockLeft(InventoryTitleText, FText::FromString(TEXT("Inventory")), FLinearColor::White, 18);
		UHorizontalBoxSlot* TitleSlot = InventoryHeaderRow->AddChildToHorizontalBox(InventoryTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UHorizontalBoxSlot* CurrencySlot = InventoryHeaderRow->AddChildToHorizontalBox(CurrencyDisplayWidget);
		if (CurrencySlot)
		{
			CurrencySlot->SetHorizontalAlignment(HAlign_Right);
			CurrencySlot->SetVerticalAlignment(VAlign_Center);
			CurrencySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CurrencySlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}

		FButtonStyle SortButtonStyle;
		SortButtonStyle.SetNormal(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.10f, 0.17f, 0.20f, 0.96f),
			FLinearColor(0.34f, 0.48f, 0.56f, 1.0f),
			1.0f));
		SortButtonStyle.SetHovered(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.14f, 0.25f, 0.29f, 0.98f),
			FLinearColor(0.56f, 0.74f, 0.80f, 1.0f),
			1.5f));
		SortButtonStyle.SetPressed(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.07f, 0.12f, 0.15f, 1.0f),
			FLinearColor(0.28f, 0.40f, 0.48f, 1.0f),
			1.0f));
		SortButtonStyle.SetNormalPadding(FMargin(8.0f, 3.0f));
		SortButtonStyle.SetPressedPadding(FMargin(8.0f, 4.0f, 8.0f, 2.0f));
		SortInventoryButton->SetStyle(SortButtonStyle);
		SortInventoryButton->SetClickMethod(EButtonClickMethod::DownAndUp);
		ConfigureTextBlock(SortInventoryButtonText, FText::FromString(TEXT("\uC815\uB9AC")), FLinearColor::White, 14);
		SortInventoryButton->SetContent(SortInventoryButtonText);

		UVerticalBoxSlot* HeaderSlot = InventoryStack->AddChildToVerticalBox(InventoryHeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetVerticalAlignment(VAlign_Top);
		}

		EquipmentReserveTileView->SetEntryWidth(EquipmentReserveEntryWidth);
		EquipmentReserveTileView->SetEntryHeight(EquipmentReserveEntryHeight);
		SetListViewEntryWidgetClass(EquipmentReserveTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(EquipmentReserveTileView);
		EquipmentReserveSizeBox->SetWidthOverride(EquipmentReserveWidth);
		EquipmentReserveSizeBox->SetHeightOverride(EquipmentReserveHeight);
		EquipmentReserveSizeBox->SetContent(EquipmentReserveTileView);
		UVerticalBoxSlot* ReserveRowSlot = InventoryStack->AddChildToVerticalBox(EquipmentReserveSizeBox);
		if (ReserveRowSlot)
		{
			ReserveRowSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			ReserveRowSlot->SetHorizontalAlignment(HAlign_Left);
			ReserveRowSlot->SetVerticalAlignment(VAlign_Top);
		}

		InventorySortControlArea->SetHeightOverride(InventorySortControlAreaHeight);
		InventorySortControlArea->SetContent(InventorySortControlRow);
		UHorizontalBoxSlot* SortSpacerSlot = InventorySortControlRow->AddChildToHorizontalBox(InventorySortControlSpacer);
		if (SortSpacerSlot)
		{
			SortSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SortSpacerSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* SortButtonSlot = InventorySortControlRow->AddChildToHorizontalBox(SortInventoryButton);
		if (SortButtonSlot)
		{
			SortButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			SortButtonSlot->SetHorizontalAlignment(HAlign_Right);
			SortButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
		UVerticalBoxSlot* SortControlSlot = InventoryStack->AddChildToVerticalBox(InventorySortControlArea);
		if (SortControlSlot)
		{
			SortControlSlot->SetHorizontalAlignment(HAlign_Fill);
			SortControlSlot->SetVerticalAlignment(VAlign_Top);
		}

		AuxiliaryBagPanel->SetWidthOverride(AuxiliaryBagPanelWidth);
		AuxiliaryBagPanel->SetHeightOverride(AuxiliaryBagPanelHeight);
		AuxiliaryBagPanel->SetContent(AuxiliaryBagBackground);
		AuxiliaryBagBackground->SetPadding(FMargin(AuxiliaryBagPanelPadding));
		AuxiliaryBagBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(AuxiliaryBagPanelWidth, AuxiliaryBagPanelHeight),
			FLinearColor(0.02f, 0.025f, 0.03f, 0.88f),
			FLinearColor(0.28f, 0.44f, 0.36f, 1.0f),
			1.0f));
		AuxiliaryBagTileView->SetEntryWidth(InventoryTileWidth);
		AuxiliaryBagTileView->SetEntryHeight(InventoryTileHeight);
		SetListViewEntryWidgetClass(AuxiliaryBagTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(AuxiliaryBagTileView);
		AuxiliaryBagBackground->SetContent(AuxiliaryBagTileView);
		UHorizontalBoxSlot* BagSlot = RootRow->AddChildToHorizontalBox(AuxiliaryBagPanel);
		if (BagSlot)
		{
			BagSlot->SetPadding(FMargin(AuxiliaryBagPanelGap, 0.0f, 0.0f, 0.0f));
			BagSlot->SetVerticalAlignment(VAlign_Top);
		}

		InventoryTileView->SetEntryWidth(InventoryTileWidth);
		InventoryTileView->SetEntryHeight(InventoryTileHeight);
		SetListViewEntryWidgetClass(InventoryTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(InventoryTileView);
		UVerticalBoxSlot* InventoryTileSlot = InventoryStack->AddChildToVerticalBox(InventoryTileView);
		if (InventoryTileSlot)
		{
			InventoryTileSlot->SetHorizontalAlignment(HAlign_Fill);
			InventoryTileSlot->SetVerticalAlignment(VAlign_Fill);
			InventoryTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		InventoryWeightPanel->SetPadding(FMargin(10.0f, 6.0f));
		InventoryWeightPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(InventoryTileViewWidth, 36.0f),
			FLinearColor(0.005f, 0.008f, 0.010f, 0.72f),
			FLinearColor(0.18f, 0.24f, 0.26f, 0.85f),
			1.0f));
		InventoryWeightPanel->SetContent(InventoryWeightRow);

		ConfigureTextBlockLeft(InventoryWeightLabelText, FText::FromString(TEXT("소지 중량")), FLinearColor(0.92f, 0.96f, 0.94f, 1.0f), 13);
		UHorizontalBoxSlot* WeightLabelSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightLabelText);
		if (WeightLabelSlot)
		{
			WeightLabelSlot->SetVerticalAlignment(VAlign_Center);
			WeightLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		InventoryWeightGauge->SetPercent(0.0f);
		InventoryWeightGauge->SetFillColorAndOpacity(FLinearColor(0.60f, 0.84f, 0.36f, 1.0f));
		InventoryWeightGaugeBox->SetHeightOverride(16.0f);
		InventoryWeightGaugeBox->SetContent(InventoryWeightGaugeOverlay);
		UOverlaySlot* WeightGaugeOverlaySlot = InventoryWeightGaugeOverlay->AddChildToOverlay(InventoryWeightGauge);
		if (WeightGaugeOverlaySlot)
		{
			WeightGaugeOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			WeightGaugeOverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		UOverlaySlot* WeightMarkerCanvasSlot = InventoryWeightGaugeOverlay->AddChildToOverlay(InventoryWeightMarkerCanvas);
		if (WeightMarkerCanvasSlot)
		{
			WeightMarkerCanvasSlot->SetHorizontalAlignment(HAlign_Fill);
			WeightMarkerCanvasSlot->SetVerticalAlignment(VAlign_Fill);
		}
		ConfigureTextBlock(InventoryWeightOverweightMarker, FText::FromString(TEXT("\u25B2")), FLinearColor(1.0f, 0.90f, 0.30f, 1.0f), 13);
		InventoryWeightOverweightMarker->SetJustification(ETextJustify::Center);
		InventoryWeightOverweightMarker->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		InventoryWeightOverweightMarker->SetShadowOffset(FVector2D(1.0f, 1.0f));
		UCanvasPanelSlot* WeightMarkerSlot = Cast<UCanvasPanelSlot>(
			InventoryWeightMarkerCanvas->AddChild(InventoryWeightOverweightMarker));
		if (WeightMarkerSlot)
		{
			WeightMarkerSlot->SetAnchors(FAnchors(0.7f, 1.0f, 0.7f, 1.0f));
			WeightMarkerSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			WeightMarkerSlot->SetPosition(FVector2D::ZeroVector);
			WeightMarkerSlot->SetAutoSize(true);
		}
		UHorizontalBoxSlot* WeightGaugeSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightGaugeBox);
		if (WeightGaugeSlot)
		{
			WeightGaugeSlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f));
			WeightGaugeSlot->SetVerticalAlignment(VAlign_Center);
			WeightGaugeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ConfigureTextBlock(InventoryWeightText, FText::FromString(TEXT("0/50kg")), FLinearColor::White, 13);
		UHorizontalBoxSlot* WeightTextSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightText);
		if (WeightTextSlot)
		{
			WeightTextSlot->SetVerticalAlignment(VAlign_Center);
			WeightTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		InventoryWeightWarningIcon->SetVisibility(ESlateVisibility::Hidden);
		InventoryWeightWarningIcon->SetPadding(FMargin(6.0f, 2.0f));
		InventoryWeightWarningIcon->SetBrush(MakeRoundedBoxBrush(
			FVector2D(58.0f, 22.0f),
			FLinearColor(0.86f, 0.18f, 0.08f, 0.95f),
			FLinearColor(1.0f, 0.70f, 0.20f, 1.0f),
			1.0f));
		ConfigureTextBlock(InventoryWeightWarningText, FText::FromString(TEXT("과중량")), FLinearColor::White, 11);
		InventoryWeightWarningIcon->SetContent(InventoryWeightWarningText);
		UHorizontalBoxSlot* WeightWarningSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightWarningIcon);
		if (WeightWarningSlot)
		{
			WeightWarningSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			WeightWarningSlot->SetVerticalAlignment(VAlign_Center);
			WeightWarningSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		InventoryWeightPanel->SetVisibility(ESlateVisibility::Collapsed);

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, RootRow);
		RegisterWidgetVariable(WidgetBlueprint, MainInventorySizeBox);
		RegisterWidgetVariable(WidgetBlueprint, InventoryPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryStack);
		RegisterWidgetVariable(WidgetBlueprint, InventoryHeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, InventoryTitleText);
		RegisterWidgetVariable(WidgetBlueprint, CurrencyDisplayWidget);
		RegisterWidgetVariable(WidgetBlueprint, SortInventoryButton);
		RegisterWidgetVariable(WidgetBlueprint, SortInventoryButtonText);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentReserveSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagPanel);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagBackground);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentReserveTileView);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagTileView);
		RegisterWidgetVariable(WidgetBlueprint, InventoryTileView);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightText);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightGaugeBox);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightGaugeOverlay);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightGauge);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightMarkerCanvas);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightOverweightMarker);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightWarningIcon);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudItemInfoPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		USizeBox* SelectedItemIconContainer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedItemIconContainer"));
		UImage* SelectedItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedItemIconImage"));
		UTextBlock* SelectedItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemNameText"));
		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
		UTextBlock* SelectedItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemDescriptionText"));
		UBorder* ModdingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModdingPanel"));
		UVerticalBox* ModdingStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModdingStack"));
		UTextBlock* ModdingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModdingText"));
		UTileView* AttachmentSlotTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("AttachmentSlotTileView"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !HeaderRow ||
			!SelectedItemIconContainer || !SelectedItemIconImage || !SelectedItemNameText || !CloseButton || !CloseButtonText ||
			!SelectedItemDescriptionText || !ModdingPanel || !ModdingStack || !ModdingText ||
			!AttachmentSlotTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(429.0f);
		RootSizeBox->SetMaxDesiredHeight(620.0f);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(LootContainerPanelPadding));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(429.0f, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.36f, 0.34f, 0.54f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(SelectedItemNameText, FText::FromString(TEXT("No Item")), FLinearColor::White, 20);
		UHorizontalBoxSlot* NameSlot = HeaderRow->AddChildToHorizontalBox(SelectedItemNameText);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Center);
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		FButtonStyle CloseButtonStyle;
		CloseButtonStyle.SetNormal(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.10f, 0.12f, 0.16f, 0.96f),
			FLinearColor(0.42f, 0.42f, 0.58f, 1.0f),
			1.0f));
		CloseButtonStyle.SetHovered(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.18f, 0.20f, 0.28f, 0.98f),
			FLinearColor(0.72f, 0.72f, 0.90f, 1.0f),
			1.5f));
		CloseButtonStyle.SetPressed(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.07f, 0.08f, 0.12f, 1.0f),
			FLinearColor(0.34f, 0.34f, 0.48f, 1.0f),
			1.0f));
		CloseButtonStyle.SetNormalPadding(FMargin(8.0f, 2.0f));
		CloseButtonStyle.SetPressedPadding(FMargin(8.0f, 3.0f, 8.0f, 1.0f));
		CloseButton->SetStyle(CloseButtonStyle);
		CloseButton->SetClickMethod(EButtonClickMethod::DownAndUp);
		ConfigureTextBlock(CloseButtonText, FText::FromString(TEXT("\uB2EB\uAE30")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 13);
		CloseButton->SetContent(CloseButtonText);
		UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(CloseButton);
		if (CloseSlot)
		{
			CloseSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CloseSlot->SetHorizontalAlignment(HAlign_Right);
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetVerticalAlignment(VAlign_Top);
		}

		SelectedItemIconContainer->SetWidthOverride(132.0f);
		SelectedItemIconContainer->SetHeightOverride(132.0f);
		SelectedItemIconImage->SetOpacity(0.0f);
		SelectedItemIconContainer->SetContent(SelectedItemIconImage);
		UVerticalBoxSlot* IconSlot = PanelStack->AddChildToVerticalBox(SelectedItemIconContainer);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Top);
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			IconSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 8.0f));
		}

		ConfigureTextBlockLeft(SelectedItemDescriptionText, FText::GetEmpty(), FLinearColor(0.75f, 0.8f, 0.86f, 1.0f), 15);
		SelectedItemDescriptionText->SetAutoWrapText(true);
		UVerticalBoxSlot* DescriptionSlot = PanelStack->AddChildToVerticalBox(SelectedItemDescriptionText);
		if (DescriptionSlot)
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
			DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		ModdingPanel->SetVisibility(ESlateVisibility::Collapsed);
		ModdingPanel->SetPadding(FMargin(10.0f));
		ModdingPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(300.0f, 150.0f),
			FLinearColor(0.03f, 0.034f, 0.04f, 0.92f),
			FLinearColor(0.56f, 0.50f, 0.78f, 1.0f),
			1.0f));
		ConfigureTextBlockLeft(ModdingText, FText::FromString(TEXT("Modding")), FLinearColor::White, 15);
		ModdingPanel->SetContent(ModdingStack);
		UVerticalBoxSlot* ModdingTextSlot = ModdingStack->AddChildToVerticalBox(ModdingText);
		if (ModdingTextSlot)
		{
			ModdingTextSlot->SetHorizontalAlignment(HAlign_Fill);
			ModdingTextSlot->SetVerticalAlignment(VAlign_Top);
		}
		AttachmentSlotTileView->SetEntryWidth(96.0f);
		AttachmentSlotTileView->SetEntryHeight(96.0f);
		SetListViewEntryWidgetClass(AttachmentSlotTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(AttachmentSlotTileView);
		UVerticalBoxSlot* AttachmentSlot = ModdingStack->AddChildToVerticalBox(AttachmentSlotTileView);
		if (AttachmentSlot)
		{
			AttachmentSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			AttachmentSlot->SetHorizontalAlignment(HAlign_Fill);
			AttachmentSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBoxSlot* ModdingSlot = PanelStack->AddChildToVerticalBox(ModdingPanel);
		if (ModdingSlot)
		{
			ModdingSlot->SetHorizontalAlignment(HAlign_Fill);
			ModdingSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, PanelStack);
		RegisterWidgetVariable(WidgetBlueprint, HeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemIconContainer);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemNameText);
		RegisterWidgetVariable(WidgetBlueprint, CloseButton);
		RegisterWidgetVariable(WidgetBlueprint, CloseButtonText);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemDescriptionText);
		RegisterWidgetVariable(WidgetBlueprint, ModdingPanel);
		RegisterWidgetVariable(WidgetBlueprint, ModdingText);
		RegisterWidgetVariable(WidgetBlueprint, AttachmentSlotTileView);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildLootContainerWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UHorizontalBox* ContainerHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContainerHeaderRow"));
		UHorizontalBox* StorageFilterTabsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StorageFilterTabsRow"));
		UTextBlock* ContainerTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContainerTitleText"));
		UTextBlock* ContainerOccupancyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContainerOccupancyText"));
		UTunaSweeperCurrencyDisplayWidget* ShopCurrencyDisplayWidget =
			WidgetTree->ConstructWidget<UTunaSweeperCurrencyDisplayWidget>(
				UTunaSweeperCurrencyDisplayWidget::StaticClass(),
				TEXT("ShopCurrencyDisplayWidget"));
		USizeBox* ShopRefreshStockButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ShopRefreshStockButtonBox"));
		UButton* ShopRefreshStockButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ShopRefreshStockButton"));
		UTextBlock* ShopRefreshStockButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShopRefreshStockButtonText"));
		UTileView* ContainerTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("ContainerTileView"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !ContainerHeaderRow || !StorageFilterTabsRow ||
			!ContainerTitleText || !ContainerOccupancyText || !ShopCurrencyDisplayWidget || !ShopRefreshStockButtonBox ||
			!ShopRefreshStockButton || !ShopRefreshStockButtonText || !ContainerTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(LootContainerPanelWidth);
		RootSizeBox->SetHeightOverride(LootContainerPanelHeaderHeight + 2.0f * LootContainerTileHeight);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(14.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(LootContainerPanelWidth, LootContainerPanelHeaderHeight + 2.0f * LootContainerTileHeight),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.44f, 0.34f, 0.26f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(ContainerTitleText, FText::FromString(TEXT("Container")), FLinearColor::White, 18);
		ConfigureTextBlockLeft(ContainerOccupancyText, FText::FromString(TEXT("(0/0)")), FLinearColor(0.92f, 0.94f, 0.96f, 1.0f), 18);
		ConfigureTextBlock(ShopRefreshStockButtonText, FText::FromString(TEXT("\uAC31\uC2E0")), FLinearColor(0.02f, 0.03f, 0.035f, 1.0f), 14);
		ShopRefreshStockButton->SetContent(ShopRefreshStockButtonText);
		ShopRefreshStockButton->SetBackgroundColor(FLinearColor(0.46f, 0.72f, 0.86f, 1.0f));
		ShopRefreshStockButton->SetVisibility(ESlateVisibility::Collapsed);
		ShopRefreshStockButtonBox->SetWidthOverride(62.0f);
		ShopRefreshStockButtonBox->SetHeightOverride(30.0f);
		ShopRefreshStockButtonBox->SetContent(ShopRefreshStockButton);
		ShopCurrencyDisplayWidget->SetVisibility(ESlateVisibility::Collapsed);

		UHorizontalBoxSlot* TitleTextSlot = ContainerHeaderRow->AddChildToHorizontalBox(ContainerTitleText);
		if (TitleTextSlot)
		{
			TitleTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleTextSlot->SetHorizontalAlignment(HAlign_Left);
			TitleTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBoxSlot* OccupancyTextSlot = ContainerHeaderRow->AddChildToHorizontalBox(ContainerOccupancyText);
		if (OccupancyTextSlot)
		{
			OccupancyTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			OccupancyTextSlot->SetHorizontalAlignment(HAlign_Left);
			OccupancyTextSlot->SetVerticalAlignment(VAlign_Center);
			OccupancyTextSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		}

		UHorizontalBoxSlot* CurrencySlot = ContainerHeaderRow->AddChildToHorizontalBox(ShopCurrencyDisplayWidget);
		if (CurrencySlot)
		{
			CurrencySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CurrencySlot->SetHorizontalAlignment(HAlign_Right);
			CurrencySlot->SetVerticalAlignment(VAlign_Center);
			CurrencySlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}

		UHorizontalBoxSlot* ShopRefreshButtonSlot = ContainerHeaderRow->AddChildToHorizontalBox(ShopRefreshStockButtonBox);
		if (ShopRefreshButtonSlot)
		{
			ShopRefreshButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			ShopRefreshButtonSlot->SetHorizontalAlignment(HAlign_Right);
			ShopRefreshButtonSlot->SetVerticalAlignment(VAlign_Center);
			ShopRefreshButtonSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		}

		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(ContainerHeaderRow);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		StorageFilterTabsRow->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* FilterTabsSlot = PanelStack->AddChildToVerticalBox(StorageFilterTabsRow);
		if (FilterTabsSlot)
		{
			FilterTabsSlot->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 5.0f));
			FilterTabsSlot->SetHorizontalAlignment(HAlign_Fill);
			FilterTabsSlot->SetVerticalAlignment(VAlign_Center);
		}

		ContainerTileView->SetEntryWidth(LootContainerTileWidth);
		ContainerTileView->SetEntryHeight(LootContainerTileHeight);
		SetListViewEntryWidgetClass(ContainerTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(ContainerTileView);
		UVerticalBoxSlot* TileViewSlot = PanelStack->AddChildToVerticalBox(ContainerTileView);
		if (TileViewSlot)
		{
			TileViewSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			TileViewSlot->SetHorizontalAlignment(HAlign_Fill);
			TileViewSlot->SetVerticalAlignment(VAlign_Fill);
			TileViewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ContainerHeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, StorageFilterTabsRow);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTitleText);
		RegisterWidgetVariable(WidgetBlueprint, ContainerOccupancyText);
		RegisterWidgetVariable(WidgetBlueprint, ShopCurrencyDisplayWidget);
		RegisterWidgetVariable(WidgetBlueprint, ShopRefreshStockButton);
		RegisterWidgetVariable(WidgetBlueprint, ShopRefreshStockButtonText);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTileView);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildWorkbenchRecipeListEntryWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UBorder* RowBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBackground"));
		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RecipeRowBox"));
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RecipeIconBox"));
		UImage* RecipeIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RecipeIconImage"));
		UTextBlock* RecipeNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeNameText"));

		if (!RowBackground || !RowBox || !IconBox || !RecipeIconImage || !RecipeNameText)
		{
			return false;
		}

		WidgetTree->RootWidget = RowBackground;
		RowBackground->SetPadding(FMargin(8.0f, 5.0f));
		RowBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(318.0f, 52.0f),
			FLinearColor(0.025f, 0.030f, 0.034f, 0.72f),
			FLinearColor(0.14f, 0.17f, 0.20f, 0.80f),
			1.0f,
			4.0f));
		RowBackground->SetContent(RowBox);

		IconBox->SetWidthOverride(34.0f);
		IconBox->SetHeightOverride(34.0f);
		IconBox->SetContent(RecipeIconImage);
		UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(IconBox);
		if (IconSlot)
		{
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			IconSlot->SetHorizontalAlignment(HAlign_Left);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlockLeft(RecipeNameText, FText::GetEmpty(), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), 16);
		RecipeNameText->SetAutoWrapText(false);
		UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(RecipeNameText);
		if (NameSlot)
		{
			NameSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterWidgetVariable(WidgetBlueprint, RowBackground);
		RegisterWidgetVariable(WidgetBlueprint, RecipeIconImage);
		RegisterWidgetVariable(WidgetBlueprint, RecipeNameText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildWorkbenchPanelWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> EntryWidgetClass,
		TSubclassOf<UUserWidget> WorkbenchRecipeEntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass || !WorkbenchRecipeEntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UTextBlock* WorkbenchTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WorkbenchTitleText"));
		UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyRow"));
		UBorder* LeftPanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBackground"));
		UOverlay* LeftModeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LeftModeOverlay"));
		UBorder* RightPanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBackground"));
		UOverlay* RightModeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RightModeOverlay"));

		UVerticalBox* CraftLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftLeftStack"));
		UListView* CraftRecipeListView = WidgetTree->ConstructWidget<UListView>(UListView::StaticClass(), TEXT("CraftRecipeListView"));
		UVerticalBox* DismantleLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DismantleLeftStack"));
		UTextBlock* DismantleInventoryHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleInventoryHeaderText"));
		UTileView* DismantleInventoryTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("DismantleInventoryTileView"));
		UTextBlock* DismantleStorageHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleStorageHeaderText"));
		UTileView* DismantleStorageTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("DismantleStorageTileView"));
		UVerticalBox* BlueprintLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BlueprintLeftStack"));
		UTileView* BlueprintItemTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("BlueprintItemTileView"));

		UVerticalBox* CraftRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftRightStack"));
		UTextBlock* CraftMaterialsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftMaterialsTitleText"));
		UVerticalBox* CraftIngredientList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftIngredientList"));
		UTextBlock* CraftArrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftArrowText"));
		UHorizontalBox* CraftOutputRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CraftOutputRow"));
		USizeBox* CraftOutputImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CraftOutputImageBox"));
		UImage* CraftOutputImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CraftOutputImage"));
		UTextBlock* CraftOutputText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftOutputText"));
		UButton* CraftButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CraftButton"));
		UTextBlock* CraftButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftButtonText"));

		UVerticalBox* DismantleRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DismantleRightStack"));
		UTextBlock* DismantleSelectedItemTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleSelectedItemTitleText"));
		UBorder* DismantleSelectedItemDropZone = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DismantleSelectedItemDropZone"));
		USizeBox* DismantleSelectedItemBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DismantleSelectedItemBox"));
		UTileView* DismantleSelectedItemTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("DismantleSelectedItemTileView"));
		UTextBlock* DismantleResultTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleResultTitleText"));
		UTextBlock* DismantleResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleResultText"));
		UButton* DismantleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DismantleButton"));
		UTextBlock* DismantleButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleButtonText"));

		UVerticalBox* BlueprintRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BlueprintRightStack"));
		UTextBlock* BlueprintGuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintGuideText"));
		UTextBlock* BlueprintSelectedItemTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintSelectedItemTitleText"));
		UBorder* BlueprintSelectedItemDropZone = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlueprintSelectedItemDropZone"));
		USizeBox* BlueprintSelectedItemBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BlueprintSelectedItemBox"));
		UTileView* BlueprintSelectedItemTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("BlueprintSelectedItemTileView"));
		UTextBlock* BlueprintRegisterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintRegisterText"));
		UButton* BlueprintRegisterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BlueprintRegisterButton"));
		UTextBlock* BlueprintRegisterButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintRegisterButtonText"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !WorkbenchTitleText || !BodyRow ||
			!LeftPanelBackground || !LeftModeOverlay || !RightPanelBackground || !RightModeOverlay ||
			!CraftLeftStack || !CraftRecipeListView || !DismantleLeftStack || !DismantleInventoryHeaderText ||
			!DismantleInventoryTileView || !DismantleStorageHeaderText || !DismantleStorageTileView ||
			!BlueprintLeftStack || !BlueprintItemTileView || !CraftRightStack || !CraftMaterialsTitleText ||
			!CraftIngredientList || !CraftArrowText || !CraftOutputRow || !CraftOutputImageBox || !CraftOutputImage ||
			!CraftOutputText || !CraftButton || !CraftButtonText || !DismantleRightStack || !DismantleSelectedItemTitleText ||
			!DismantleSelectedItemDropZone || !DismantleSelectedItemBox || !DismantleSelectedItemTileView ||
			!DismantleResultTitleText || !DismantleResultText || !DismantleButton || !DismantleButtonText || !BlueprintRightStack ||
			!BlueprintGuideText || !BlueprintSelectedItemTitleText || !BlueprintSelectedItemDropZone || !BlueprintSelectedItemBox ||
			!BlueprintSelectedItemTileView || !BlueprintRegisterText || !BlueprintRegisterButton || !BlueprintRegisterButtonText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(WorkbenchPanelWidth);
		RootSizeBox->SetHeightOverride(WorkbenchPanelHeight);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(WorkbenchPanelPadding));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchPanelWidth, WorkbenchPanelHeight),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.92f),
			FLinearColor(0.22f, 0.42f, 0.56f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(WorkbenchTitleText, FText::FromString(TEXT("\uC81C\uC870")), FLinearColor::White, 22);
		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(WorkbenchTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBoxSlot* BodySlot = PanelStack->AddChildToVerticalBox(BodyRow);
		if (BodySlot)
		{
			BodySlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			BodySlot->SetHorizontalAlignment(HAlign_Fill);
			BodySlot->SetVerticalAlignment(VAlign_Fill);
			BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		LeftPanelBackground->SetPadding(FMargin(10.0f));
		LeftPanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchLeftPanelWidth, WorkbenchPanelHeight - 72.0f),
			FLinearColor(0.025f, 0.030f, 0.034f, 0.94f),
			FLinearColor(0.18f, 0.22f, 0.26f, 1.0f),
			1.0f));
		LeftPanelBackground->SetContent(LeftModeOverlay);

		RightPanelBackground->SetPadding(FMargin(14.0f));
		RightPanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchPanelWidth - WorkbenchLeftPanelWidth - 54.0f, WorkbenchPanelHeight - 72.0f),
			FLinearColor(0.020f, 0.024f, 0.028f, 0.94f),
			FLinearColor(0.18f, 0.22f, 0.26f, 1.0f),
			1.0f));
		RightPanelBackground->SetContent(RightModeOverlay);

		UHorizontalBoxSlot* LeftPanelSlot = BodyRow->AddChildToHorizontalBox(LeftPanelBackground);
		if (LeftPanelSlot)
		{
			LeftPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			LeftPanelSlot->SetHorizontalAlignment(HAlign_Left);
			LeftPanelSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBoxSlot* RightPanelSlot = BodyRow->AddChildToHorizontalBox(RightPanelBackground);
		if (RightPanelSlot)
		{
			RightPanelSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
			RightPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RightPanelSlot->SetHorizontalAlignment(HAlign_Fill);
			RightPanelSlot->SetVerticalAlignment(VAlign_Fill);
		}

		auto ConfigureWorkbenchTileView = [EntryWidgetClass](UTileView* TileView, float Height)
		{
			TileView->SetEntryWidth(WorkbenchTileWidth);
			TileView->SetEntryHeight(WorkbenchTileHeight);
			TileView->SetWheelScrollMultiplier(0.55f);
			SetListViewEntryWidgetClass(TileView, EntryWidgetClass);
			ApplyItemContainerScrollBarStyle(TileView);
			if (USizeBox* TileViewSizeBox = Cast<USizeBox>(TileView->GetParent()))
			{
				TileViewSizeBox->SetWidthOverride(WorkbenchTileViewWidth);
				TileViewSizeBox->SetHeightOverride(Height);
			}
		};

		auto ConfigureWorkbenchRecipeListView = [](UListView* ListView)
		{
			ListView->SetVerticalEntrySpacing(4.0f);
			ListView->SetWheelScrollMultiplier(0.55f);
		};

		auto AddModeStackToOverlay = [](UOverlay* Overlay, UWidget* Stack)
		{
			UOverlaySlot* StackSlot = Overlay->AddChildToOverlay(Stack);
			if (StackSlot)
			{
				StackSlot->SetHorizontalAlignment(HAlign_Fill);
				StackSlot->SetVerticalAlignment(VAlign_Fill);
			}
		};

		USizeBox* CraftRecipeListBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CraftRecipeListBox"));
		CraftRecipeListBox->SetWidthOverride(WorkbenchTileViewWidth);
		CraftRecipeListBox->SetHeightOverride(WorkbenchTileViewHeight);
		CraftRecipeListBox->SetContent(CraftRecipeListView);
		UVerticalBoxSlot* CraftListSlot = CraftLeftStack->AddChildToVerticalBox(CraftRecipeListBox);
		if (CraftListSlot)
		{
			CraftListSlot->SetHorizontalAlignment(HAlign_Fill);
			CraftListSlot->SetVerticalAlignment(VAlign_Fill);
			CraftListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		ConfigureWorkbenchRecipeListView(CraftRecipeListView);
		SetListViewEntryWidgetClass(CraftRecipeListView, WorkbenchRecipeEntryWidgetClass);

		ConfigureTextBlockLeft(DismantleInventoryHeaderText, FText::FromString(TEXT("\uC778\uBCA4\uD1A0\uB9AC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 16);
		ConfigureTextBlockLeft(DismantleStorageHeaderText, FText::FromString(TEXT("\uCC3D\uACE0")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 16);
		DismantleLeftStack->AddChildToVerticalBox(DismantleInventoryHeaderText);
		USizeBox* DismantleInventoryBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DismantleInventoryBox"));
		DismantleInventoryBox->SetContent(DismantleInventoryTileView);
		UVerticalBoxSlot* DismantleInventorySlot = DismantleLeftStack->AddChildToVerticalBox(DismantleInventoryBox);
		if (DismantleInventorySlot)
		{
			DismantleInventorySlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 12.0f));
			DismantleInventorySlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleInventorySlot->SetVerticalAlignment(VAlign_Top);
		}
		DismantleLeftStack->AddChildToVerticalBox(DismantleStorageHeaderText);
		USizeBox* DismantleStorageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DismantleStorageBox"));
		DismantleStorageBox->SetContent(DismantleStorageTileView);
		UVerticalBoxSlot* DismantleStorageSlot = DismantleLeftStack->AddChildToVerticalBox(DismantleStorageBox);
		if (DismantleStorageSlot)
		{
			DismantleStorageSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
			DismantleStorageSlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleStorageSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureWorkbenchTileView(DismantleInventoryTileView, 204.0f);
		ConfigureWorkbenchTileView(DismantleStorageTileView, 204.0f);

		USizeBox* BlueprintTileViewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BlueprintTileViewBox"));
		BlueprintTileViewBox->SetContent(BlueprintItemTileView);
		UVerticalBoxSlot* BlueprintTileSlot = BlueprintLeftStack->AddChildToVerticalBox(BlueprintTileViewBox);
		if (BlueprintTileSlot)
		{
			BlueprintTileSlot->SetHorizontalAlignment(HAlign_Fill);
			BlueprintTileSlot->SetVerticalAlignment(VAlign_Fill);
			BlueprintTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		ConfigureWorkbenchTileView(BlueprintItemTileView, WorkbenchTileViewHeight);

		AddModeStackToOverlay(LeftModeOverlay, CraftLeftStack);

		auto ConfigureActionButton = [](UButton* Button, UTextBlock* ButtonText, const FText& Text)
		{
			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.05f, 0.33f, 0.78f, 1.0f), FLinearColor(0.45f, 0.68f, 0.95f, 1.0f), 1.0f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.08f, 0.42f, 0.92f, 1.0f), FLinearColor(0.70f, 0.86f, 1.0f, 1.0f), 1.5f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.04f, 0.24f, 0.62f, 1.0f), FLinearColor(0.36f, 0.56f, 0.84f, 1.0f), 1.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetContent(ButtonText);
			ConfigureTextBlock(ButtonText, Text, FLinearColor::White, 17);
		};

		ConfigureTextBlockLeft(CraftMaterialsTitleText, FText::FromString(TEXT("\uC7AC\uB8CC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		CraftRightStack->AddChildToVerticalBox(CraftMaterialsTitleText);
		UVerticalBoxSlot* IngredientSlot = CraftRightStack->AddChildToVerticalBox(CraftIngredientList);
		if (IngredientSlot)
		{
			IngredientSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
			IngredientSlot->SetHorizontalAlignment(HAlign_Fill);
			IngredientSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureTextBlock(CraftArrowText, FText::FromString(TEXT("\u2193")), FLinearColor(0.68f, 0.82f, 0.96f, 1.0f), 34);
		UVerticalBoxSlot* ArrowSlot = CraftRightStack->AddChildToVerticalBox(CraftArrowText);
		if (ArrowSlot)
		{
			ArrowSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 12.0f));
			ArrowSlot->SetHorizontalAlignment(HAlign_Center);
			ArrowSlot->SetVerticalAlignment(VAlign_Top);
		}
		CraftOutputImageBox->SetWidthOverride(84.0f);
		CraftOutputImageBox->SetHeightOverride(84.0f);
		CraftOutputImageBox->SetContent(CraftOutputImage);
		CraftOutputImage->SetOpacity(0.0f);
		UHorizontalBoxSlot* OutputImageSlot = CraftOutputRow->AddChildToHorizontalBox(CraftOutputImageBox);
		if (OutputImageSlot)
		{
			OutputImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			OutputImageSlot->SetHorizontalAlignment(HAlign_Left);
			OutputImageSlot->SetVerticalAlignment(VAlign_Center);
		}
		ConfigureTextBlockLeft(CraftOutputText, FText::GetEmpty(), FLinearColor::White, 18);
		UHorizontalBoxSlot* OutputTextSlot = CraftOutputRow->AddChildToHorizontalBox(CraftOutputText);
		if (OutputTextSlot)
		{
			OutputTextSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
			OutputTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			OutputTextSlot->SetHorizontalAlignment(HAlign_Fill);
			OutputTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		CraftRightStack->AddChildToVerticalBox(CraftOutputRow);
		ConfigureActionButton(CraftButton, CraftButtonText, FText::FromString(TEXT("\uC81C\uC870")));
		UVerticalBoxSlot* CraftButtonSlot = CraftRightStack->AddChildToVerticalBox(CraftButton);
		if (CraftButtonSlot)
		{
			CraftButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			CraftButtonSlot->SetHorizontalAlignment(HAlign_Right);
			CraftButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(DismantleSelectedItemTitleText, FText::FromString(TEXT("\uBD84\uD574\uD560 \uC544\uC774\uD15C")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		DismantleRightStack->AddChildToVerticalBox(DismantleSelectedItemTitleText);
		DismantleSelectedItemDropZone->SetPadding(FMargin(8.0f));
		DismantleSelectedItemDropZone->SetBrush(MakeRoundedBoxBrush(
			FVector2D(128.0f, 128.0f),
			FLinearColor(0.030f, 0.036f, 0.041f, 0.92f),
			FLinearColor(0.24f, 0.31f, 0.36f, 1.0f),
			1.0f));
		DismantleSelectedItemTileView->SetEntryWidth(WorkbenchTileWidth);
		DismantleSelectedItemTileView->SetEntryHeight(WorkbenchTileHeight);
		SetListViewEntryWidgetClass(DismantleSelectedItemTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(DismantleSelectedItemTileView);
		DismantleSelectedItemBox->SetWidthOverride(WorkbenchTileWidth);
		DismantleSelectedItemBox->SetHeightOverride(WorkbenchTileHeight);
		DismantleSelectedItemBox->SetContent(DismantleSelectedItemTileView);
		DismantleSelectedItemDropZone->SetContent(DismantleSelectedItemBox);
		UVerticalBoxSlot* DismantleSelectedItemSlot = DismantleRightStack->AddChildToVerticalBox(DismantleSelectedItemDropZone);
		if (DismantleSelectedItemSlot)
		{
			DismantleSelectedItemSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 18.0f));
			DismantleSelectedItemSlot->SetHorizontalAlignment(HAlign_Left);
			DismantleSelectedItemSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(DismantleResultTitleText, FText::FromString(TEXT("\uBD84\uD574 \uACB0\uACFC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		DismantleRightStack->AddChildToVerticalBox(DismantleResultTitleText);
		ConfigureTextBlockLeft(DismantleResultText, FText::GetEmpty(), FLinearColor::White, 17);
		UVerticalBoxSlot* DismantleResultSlot = DismantleRightStack->AddChildToVerticalBox(DismantleResultText);
		if (DismantleResultSlot)
		{
			DismantleResultSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			DismantleResultSlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleResultSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureActionButton(DismantleButton, DismantleButtonText, FText::FromString(TEXT("\uBD84\uD574")));
		UVerticalBoxSlot* DismantleButtonSlot = DismantleRightStack->AddChildToVerticalBox(DismantleButton);
		if (DismantleButtonSlot)
		{
			DismantleButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			DismantleButtonSlot->SetHorizontalAlignment(HAlign_Right);
			DismantleButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(BlueprintGuideText, FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uC544\uC774\uD15C")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		BlueprintRightStack->AddChildToVerticalBox(BlueprintGuideText);
		ConfigureTextBlockLeft(BlueprintSelectedItemTitleText, FText::FromString(TEXT("\uB4F1\uB85D\uD560 \uC124\uACC4\uB3C4")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		UVerticalBoxSlot* BlueprintSelectedItemTitleSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintSelectedItemTitleText);
		if (BlueprintSelectedItemTitleSlot)
		{
			BlueprintSelectedItemTitleSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
			BlueprintSelectedItemTitleSlot->SetHorizontalAlignment(HAlign_Fill);
			BlueprintSelectedItemTitleSlot->SetVerticalAlignment(VAlign_Top);
		}
		BlueprintSelectedItemDropZone->SetPadding(FMargin(8.0f));
		BlueprintSelectedItemDropZone->SetBrush(MakeRoundedBoxBrush(
			FVector2D(128.0f, 128.0f),
			FLinearColor(0.030f, 0.036f, 0.041f, 0.92f),
			FLinearColor(0.24f, 0.31f, 0.36f, 1.0f),
			1.0f));
		BlueprintSelectedItemTileView->SetEntryWidth(WorkbenchTileWidth);
		BlueprintSelectedItemTileView->SetEntryHeight(WorkbenchTileHeight);
		SetListViewEntryWidgetClass(BlueprintSelectedItemTileView, EntryWidgetClass);
		ApplyItemContainerScrollBarStyle(BlueprintSelectedItemTileView);
		BlueprintSelectedItemBox->SetWidthOverride(WorkbenchTileWidth);
		BlueprintSelectedItemBox->SetHeightOverride(WorkbenchTileHeight);
		BlueprintSelectedItemBox->SetContent(BlueprintSelectedItemTileView);
		BlueprintSelectedItemDropZone->SetContent(BlueprintSelectedItemBox);
		UVerticalBoxSlot* BlueprintSelectedItemSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintSelectedItemDropZone);
		if (BlueprintSelectedItemSlot)
		{
			BlueprintSelectedItemSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 18.0f));
			BlueprintSelectedItemSlot->SetHorizontalAlignment(HAlign_Left);
			BlueprintSelectedItemSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureTextBlockLeft(BlueprintRegisterText, FText::GetEmpty(), FLinearColor::White, 17);
		UVerticalBoxSlot* BlueprintTextSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintRegisterText);
		if (BlueprintTextSlot)
		{
			BlueprintTextSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			BlueprintTextSlot->SetHorizontalAlignment(HAlign_Fill);
			BlueprintTextSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureActionButton(BlueprintRegisterButton, BlueprintRegisterButtonText, FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D")));
		UVerticalBoxSlot* BlueprintButtonSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintRegisterButton);
		if (BlueprintButtonSlot)
		{
			BlueprintButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			BlueprintButtonSlot->SetHorizontalAlignment(HAlign_Right);
			BlueprintButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		AddModeStackToOverlay(RightModeOverlay, CraftRightStack);
		AddModeStackToOverlay(RightModeOverlay, DismantleRightStack);
		AddModeStackToOverlay(RightModeOverlay, BlueprintRightStack);

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudExternalPanelWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> LootContainerWidgetClass,
		TSubclassOf<UUserWidget> StorageContainerWidgetClass,
		TSubclassOf<UUserWidget> ShopContainerWidgetClass,
		TSubclassOf<UUserWidget> WorkbenchPanelWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree ||
			!LootContainerWidgetClass || !StorageContainerWidgetClass || !ShopContainerWidgetClass ||
			!WorkbenchPanelWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UOverlay* PanelOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PanelOverlay"));
		UOverlay* LootingBoxPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LootingBoxPanel"));
		UUserWidget* LootContainerWidget = WidgetTree->ConstructWidget<UUserWidget>(LootContainerWidgetClass, TEXT("LootContainerWidget"));
		UOverlay* StoragePanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("StoragePanel"));
		UUserWidget* StorageContainerWidget = WidgetTree->ConstructWidget<UUserWidget>(
			StorageContainerWidgetClass,
			TEXT("StorageContainerWidget"));
		UOverlay* ShopPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ShopPanel"));
		UUserWidget* ShopContainerWidget = WidgetTree->ConstructWidget<UUserWidget>(
			ShopContainerWidgetClass,
			TEXT("ShopContainerWidget"));
		UOverlay* WorkbenchPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WorkbenchPanel"));
		UUserWidget* WorkbenchPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(WorkbenchPanelWidgetClass, TEXT("WorkbenchPanelWidget"));

		if (!RootSizeBox || !PanelOverlay || !LootingBoxPanel || !LootContainerWidget ||
			!StoragePanel || !StorageContainerWidget || !ShopPanel || !ShopContainerWidget ||
			!WorkbenchPanel || !WorkbenchPanelWidget)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(WorkbenchPanelWidth);
		RootSizeBox->SetContent(PanelOverlay);

		UOverlaySlot* LootContainerSlot = LootingBoxPanel->AddChildToOverlay(LootContainerWidget);
		if (LootContainerSlot)
		{
			LootContainerSlot->SetHorizontalAlignment(HAlign_Right);
			LootContainerSlot->SetVerticalAlignment(VAlign_Top);
		}

		UOverlaySlot* StorageContainerSlot = StoragePanel->AddChildToOverlay(StorageContainerWidget);
		if (StorageContainerSlot)
		{
			StorageContainerSlot->SetHorizontalAlignment(HAlign_Right);
			StorageContainerSlot->SetVerticalAlignment(VAlign_Top);
		}

		UOverlaySlot* ShopContainerSlot = ShopPanel->AddChildToOverlay(ShopContainerWidget);
		if (ShopContainerSlot)
		{
			ShopContainerSlot->SetHorizontalAlignment(HAlign_Right);
			ShopContainerSlot->SetVerticalAlignment(VAlign_Top);
		}

		UOverlaySlot* WorkbenchSlot = WorkbenchPanel->AddChildToOverlay(WorkbenchPanelWidget);
		if (WorkbenchSlot)
		{
			WorkbenchSlot->SetHorizontalAlignment(HAlign_Fill);
			WorkbenchSlot->SetVerticalAlignment(VAlign_Top);
		}

		TArray<UWidget*> ExternalPanels = { LootingBoxPanel, WorkbenchPanel, ShopPanel, StoragePanel };
		for (UWidget* Panel : ExternalPanels)
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* PanelSlot = PanelOverlay->AddChildToOverlay(Panel);
			if (PanelSlot)
			{
				PanelSlot->SetHorizontalAlignment(HAlign_Fill);
				PanelSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		RegisterWidgetVariable(WidgetBlueprint, LootingBoxPanel);
		RegisterWidgetVariable(WidgetBlueprint, WorkbenchPanel);
		RegisterWidgetVariable(WidgetBlueprint, ShopPanel);
		RegisterWidgetVariable(WidgetBlueprint, StoragePanel);
		RegisterWidgetVariable(WidgetBlueprint, LootContainerWidget);
		RegisterWidgetVariable(WidgetBlueprint, StorageContainerWidget);
		RegisterWidgetVariable(WidgetBlueprint, ShopContainerWidget);
		RegisterWidgetVariable(WidgetBlueprint, WorkbenchPanelWidget);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildResearchNodeWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree) return false;
		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);
		UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* NodeBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NodeBorder"));
		UButton* NodeButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NodeButton"));
		UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NodeContent"));
		UTextBlock* NameText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		UTextBlock* RequirementText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RequirementText"));
		UTextBlock* RemainingTimeText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RemainingTimeText"));
		UProgressBar* ResearchProgressBar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ResearchProgressBar"));
		UTextBlock* ActionText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActionText"));
		if (!RootSizeBox || !NodeBorder || !NodeButton || !Content || !NameText || !RequirementText || !RemainingTimeText || !ResearchProgressBar || !ActionText) return false;
		Tree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(240.0f);
		RootSizeBox->SetHeightOverride(154.0f);
		RootSizeBox->SetContent(NodeBorder);
		NodeBorder->SetPadding(FMargin(2.0f));
		NodeBorder->SetBrush(MakeRoundedBoxBrush(FVector2D(240.0f, 154.0f), FLinearColor(0.02f, 0.035f, 0.04f, 0.96f), FLinearColor(0.28f, 0.62f, 0.52f, 0.9f), 2.0f, 8.0f));
		NodeBorder->SetContent(NodeButton);
		NodeButton->SetContent(Content);
		ConfigureTextBlock(NameText, FText::FromString(TEXT("연구 노드")), FLinearColor::White, 19);
		ConfigureTextBlock(RequirementText, FText::FromString(TEXT("개방 0")), FLinearColor(0.72f, 0.78f, 0.80f), 13);
		ConfigureTextBlock(RemainingTimeText, FText::GetEmpty(), FLinearColor(0.55f, 0.92f, 0.78f), 17);
		ConfigureTextBlock(ActionText, FText::FromString(TEXT("연구 시작")), FLinearColor(0.82f, 0.96f, 0.88f), 15);
		for (UWidget* Child : TArray<UWidget*>{ NameText, RequirementText, RemainingTimeText, ResearchProgressBar, ActionText })
		{
			if (UVerticalBoxSlot* Slot = Content->AddChildToVerticalBox(Child))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetPadding(FMargin(9.0f, 3.0f));
			}
		}
		ResearchProgressBar->SetPercent(0.0f);
		ResearchProgressBar->SetFillColorAndOpacity(FLinearColor(0.25f, 0.85f, 0.62f));
		for (UWidget* Widget : TArray<UWidget*>{ RootSizeBox, NodeBorder, NodeButton, Content, NameText, RequirementText, RemainingTimeText, ResearchProgressBar, ActionText }) RegisterWidgetVariable(WidgetBlueprint, Widget);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildResearchTreeWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> ResearchNodeWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !ResearchNodeWidgetClass) return false;
		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);
		UWidgetTree* Tree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* Background = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResearchBackground"));
		UVerticalBox* RootColumn = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootColumn"));
		UTextBlock* ResearchStatusText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResearchStatusText"));
		UScrollBox* ResearchScrollBox = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResearchScrollBox"));
		UVerticalBox* TreeRows = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TreeRows"));
		if (!RootSizeBox || !Background || !RootColumn || !ResearchStatusText || !ResearchScrollBox || !TreeRows) return false;
		Tree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(920.0f);
		RootSizeBox->SetHeightOverride(700.0f);
		RootSizeBox->SetContent(Background);
		Background->SetPadding(FMargin(22.0f));
		Background->SetBrush(MakeRoundedBoxBrush(FVector2D(920.0f, 700.0f), FLinearColor(0.008f, 0.014f, 0.017f, 0.96f), FLinearColor(0.20f, 0.36f, 0.34f, 0.9f), 2.0f, 10.0f));
		Background->SetContent(RootColumn);
		ConfigureTextBlockLeft(ResearchStatusText, FText::FromString(TEXT("적용 0 / 13")), FLinearColor(0.82f, 0.96f, 0.88f), 22);
		if (UVerticalBoxSlot* HeaderSlot = RootColumn->AddChildToVerticalBox(ResearchStatusText)) HeaderSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 12.0f));
		if (UVerticalBoxSlot* ScrollSlot = RootColumn->AddChildToVerticalBox(ResearchScrollBox)) ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResearchScrollBox->SetOrientation(Orient_Vertical);
		ResearchScrollBox->AddChild(TreeRows);
		const TArray<TArray<FName>> Rows = {
			{ NAME_None, TEXT("vitality_1"), NAME_None },
			{ TEXT("nutrition_1"), NAME_None, TEXT("hydration_1") },
			{ TEXT("vitality_2"), TEXT("stamina_1"), TEXT("carry_1") },
			{ TEXT("nutrition_2"), NAME_None, TEXT("hydration_2") },
			{ TEXT("vitality_3"), TEXT("stamina_2"), TEXT("carry_2") },
			{ NAME_None, TEXT("survival_mastery"), NAME_None },
			{ NAME_None, TEXT("ultimate_conditioning"), NAME_None }
		};
		for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
		{
			UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *FString::Printf(TEXT("ResearchRow_%d"), RowIndex));
			if (!Row) return false;
			for (int32 Column = 0; Column < 3; ++Column)
			{
				UWidget* Cell = nullptr;
				if (!Rows[RowIndex][Column].IsNone())
				{
					UTunaSweeperResearchNodeWidget* Node = Cast<UTunaSweeperResearchNodeWidget>(Tree->ConstructWidget<UUserWidget>(ResearchNodeWidgetClass, *FString::Printf(TEXT("ResearchNode_%s"), *Rows[RowIndex][Column].ToString())));
					if (!Node) return false;
					Node->NodeId = Rows[RowIndex][Column];
					Cell = Node;
				}
				else
				{
					Cell = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Empty_%d_%d"), RowIndex, Column));
				}
				if (UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell))
				{
					CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					CellSlot->SetHorizontalAlignment(HAlign_Center);
					CellSlot->SetPadding(FMargin(10.0f, 12.0f));
				}
			}
			if (UVerticalBoxSlot* RowSlot = TreeRows->AddChildToVerticalBox(Row)) RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildGameHudWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> TopReserveWidgetClass,
		TSubclassOf<UUserWidget> BottomStatusWidgetClass,
		TSubclassOf<UUserWidget> DebuffBarWidgetClass,
		TSubclassOf<UUserWidget> QuickSlotBarWidgetClass,
		TSubclassOf<UUserWidget> InventoryAreaWidgetClass,
		TSubclassOf<UUserWidget> ItemInfoPanelWidgetClass,
		TSubclassOf<UUserWidget> ExternalPanelWidgetClass,
		TSubclassOf<UUserWidget> ResearchTreeWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !TopReserveWidgetClass || !BottomStatusWidgetClass ||
			!DebuffBarWidgetClass ||
			!QuickSlotBarWidgetClass || !InventoryAreaWidgetClass || !ItemInfoPanelWidgetClass || !ExternalPanelWidgetClass || !ResearchTreeWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UUserWidget* TopStatusReserveWidget = WidgetTree->ConstructWidget<UUserWidget>(TopReserveWidgetClass, TEXT("TopStatusReserveWidget"));
		UCanvasPanel* CenterContentPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CenterContentPanel"));
		UUserWidget* InventoryAreaWidget = WidgetTree->ConstructWidget<UUserWidget>(InventoryAreaWidgetClass, TEXT("InventoryAreaWidget"));
		UUserWidget* ItemInfoPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ItemInfoPanelWidgetClass, TEXT("ItemInfoPanelWidget"));
		UUserWidget* ExternalPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ExternalPanelWidgetClass, TEXT("ExternalPanelWidget"));
		UUserWidget* ResearchPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ResearchTreeWidgetClass, TEXT("ResearchPanelWidget"));
		UBorder* UnsupportedModePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UnsupportedModePanel"));
		UTextBlock* UnsupportedModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnsupportedModeText"));
		UTextBlock* ModeTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeTitleText"));
		UCanvasPanel* BottomRow = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BottomRow"));
		UUserWidget* BottomStatusWidget = WidgetTree->ConstructWidget<UUserWidget>(BottomStatusWidgetClass, TEXT("BottomStatusWidget"));
		UUserWidget* DebuffBarWidget = WidgetTree->ConstructWidget<UUserWidget>(DebuffBarWidgetClass, TEXT("DebuffBarWidget"));
		UUserWidget* QuickSlotBarWidget = WidgetTree->ConstructWidget<UUserWidget>(QuickSlotBarWidgetClass, TEXT("QuickSlotBarWidget"));
		USizeBox* CenterCancelableActionGaugeRoot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CenterCancelableActionGaugeRoot"));
		UCanvasPanel* CenterCancelableActionGaugeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CenterCancelableActionGaugeCanvas"));
		UBorder* CenterCancelableActionGaugeBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CenterCancelableActionGaugeBackdrop"));
		UTunaSweeperReloadRingWidget* CenterCancelableActionRingWidget = WidgetTree->ConstructWidget<UTunaSweeperReloadRingWidget>(
			UTunaSweeperReloadRingWidget::StaticClass(),
			TEXT("CenterCancelableActionRingWidget"));
		UTextBlock* CenterCancelableActionPercentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterCancelableActionPercentText"));
		UHorizontalBox* CenterReloadPromptRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CenterReloadPromptRoot"));
		UTextBlock* CenterReloadPromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterReloadPromptText"));
		UBorder* CenterReloadPromptKeyBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CenterReloadPromptKeyBackground"));
		UTextBlock* CenterReloadPromptKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterReloadPromptKeyText"));

		if (!RootCanvas || !TopStatusReserveWidget || !CenterContentPanel || !InventoryAreaWidget || !ItemInfoPanelWidget ||
			!ExternalPanelWidget || !ResearchPanelWidget || !UnsupportedModePanel || !UnsupportedModeText || !ModeTitleText ||
			!BottomRow || !BottomStatusWidget || !DebuffBarWidget || !QuickSlotBarWidget ||
			!CenterCancelableActionGaugeRoot || !CenterCancelableActionGaugeCanvas || !CenterCancelableActionGaugeBackdrop || !CenterCancelableActionRingWidget || !CenterCancelableActionPercentText ||
			!CenterReloadPromptRoot || !CenterReloadPromptText || !CenterReloadPromptKeyBackground || !CenterReloadPromptKeyText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		TopStatusReserveWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(TopStatusReserveWidget);
		if (TopSlot)
		{
			TopSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			TopSlot->SetOffsets(FMargin(0.0f, 16.0f, HudTopModeTabPanelWidth, HudTopModeTabPanelHeight));
			TopSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		}

		ModeTitleText->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureTextBlockLeft(ModeTitleText, FText::GetEmpty(), FLinearColor(0.92f, 0.98f, 1.0f, 0.96f), 52);
		TunaSweeperUIFont::ApplyFont(ModeTitleText, 52.0f, ETunaSweeperUIFontWeight::Bold);
		ModeTitleText->SetShadowOffset(FVector2D(2.0f, 2.0f));
		ModeTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
		UCanvasPanelSlot* ModeTitleSlot = RootCanvas->AddChildToCanvas(ModeTitleText);
		if (ModeTitleSlot)
		{
			ModeTitleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ModeTitleSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ModeTitleSlot->SetPosition(FVector2D(42.0f, 92.0f));
			ModeTitleSlot->SetSize(FVector2D(420.0f, 84.0f));
			ModeTitleSlot->SetZOrder(20);
		}

		CenterContentPanel->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterContentPanel);
		if (CenterSlot)
		{
			CenterSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CenterSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			CenterSlot->SetOffsets(FMargin(
				HudUtilityPanelLeftInset,
				HudUtilityPanelTopOffset,
				HudUtilityPanelRightInset,
				HudUtilityPanelBottomInset));
		}

		UCanvasPanelSlot* InventorySlot = CenterContentPanel->AddChildToCanvas(InventoryAreaWidget);
		if (InventorySlot)
		{
			InventorySlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
			InventorySlot->SetAlignment(FVector2D(0.0f, 0.0f));
			InventorySlot->SetOffsets(FMargin(0.0f, 0.0f, InventoryAreaPanelWidth, 0.0f));
		}

		ItemInfoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* ItemInfoSlot = CenterContentPanel->AddChildToCanvas(ItemInfoPanelWidget);
		if (ItemInfoSlot)
		{
			ItemInfoSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ItemInfoSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ItemInfoSlot->SetAutoSize(true);
			ItemInfoSlot->SetOffsets(FMargin(0.0f, 0.0f, 429.0f, 620.0f));
		}

		UCanvasPanelSlot* ExternalSlot = CenterContentPanel->AddChildToCanvas(ExternalPanelWidget);
		if (ExternalSlot)
		{
			ExternalSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 1.0f));
			ExternalSlot->SetAlignment(FVector2D(1.0f, 0.0f));
			ExternalSlot->SetOffsets(FMargin(0.0f, 0.0f, WorkbenchPanelWidth, 0.0f));
		}

		ResearchPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* ResearchSlot = CenterContentPanel->AddChildToCanvas(ResearchPanelWidget);
		if (ResearchSlot)
		{
			ResearchSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			ResearchSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ResearchSlot->SetAutoSize(true);
		}

		UnsupportedModePanel->SetVisibility(ESlateVisibility::Collapsed);
		UnsupportedModePanel->SetPadding(FMargin(24.0f, 12.0f));
		UnsupportedModePanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(144.0f, 52.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.62f),
			FLinearColor(0.28f, 0.32f, 0.34f, 0.52f),
			1.0f,
			6.0f));
		ConfigureTextBlock(UnsupportedModeText, FText::FromString(TEXT("\uBBF8\uAD6C\uD604")), FLinearColor(0.88f, 0.92f, 0.94f, 0.96f), 18);
		UnsupportedModePanel->SetContent(UnsupportedModeText);
		UCanvasPanelSlot* UnsupportedSlot = CenterContentPanel->AddChildToCanvas(UnsupportedModePanel);
		if (UnsupportedSlot)
		{
			UnsupportedSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			UnsupportedSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			UnsupportedSlot->SetPosition(FVector2D(0.0f, 0.0f));
			UnsupportedSlot->SetSize(FVector2D(144.0f, 52.0f));
		}

		UCanvasPanelSlot* BottomSlot = RootCanvas->AddChildToCanvas(BottomRow);
		if (BottomSlot)
		{
			BottomSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			BottomSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			BottomSlot->SetPosition(FVector2D(0.0f, 0.0f));
			BottomSlot->SetSize(FVector2D(GameplayBottomPanelWidth, GameplayBottomPanelHeight));
		}

		DebuffBarWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* DebuffBarSlot = RootCanvas->AddChildToCanvas(DebuffBarWidget);
		if (DebuffBarSlot)
		{
			DebuffBarSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
			DebuffBarSlot->SetAlignment(FVector2D(0.0f, 1.0f));
			DebuffBarSlot->SetPosition(FVector2D(HudDebuffBarLeftOffset, -HudDebuffBarBottomOffset));
			DebuffBarSlot->SetAutoSize(true);
			DebuffBarSlot->SetZOrder(36);
		}

		CenterCancelableActionGaugeRoot->SetWidthOverride(96.0f);
		CenterCancelableActionGaugeRoot->SetHeightOverride(96.0f);
		CenterCancelableActionGaugeRoot->SetContent(CenterCancelableActionGaugeCanvas);
		CenterCancelableActionGaugeRoot->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterCancelableActionSlot = RootCanvas->AddChildToCanvas(CenterCancelableActionGaugeRoot);
		if (CenterCancelableActionSlot)
		{
			CenterCancelableActionSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterCancelableActionSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterCancelableActionSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterCancelableActionSlot->SetSize(FVector2D(96.0f, 96.0f));
		}

		CenterCancelableActionGaugeBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(58.0f, 58.0f),
			FLinearColor(0.012f, 0.016f, 0.018f, 0.68f),
			FLinearColor(0.48f, 0.66f, 0.46f, 0.3f),
			1.0f));
		UCanvasPanelSlot* CenterCancelableActionBackdropSlot = CenterCancelableActionGaugeCanvas->AddChildToCanvas(CenterCancelableActionGaugeBackdrop);
		if (CenterCancelableActionBackdropSlot)
		{
			CenterCancelableActionBackdropSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterCancelableActionBackdropSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterCancelableActionBackdropSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterCancelableActionBackdropSlot->SetSize(FVector2D(58.0f, 58.0f));
		}

		CenterCancelableActionRingWidget->SetCancelableActionProgress(0.0f, true);
		UCanvasPanelSlot* CenterCancelableActionRingSlot = CenterCancelableActionGaugeCanvas->AddChildToCanvas(CenterCancelableActionRingWidget);
		if (CenterCancelableActionRingSlot)
		{
			CenterCancelableActionRingSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterCancelableActionRingSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterCancelableActionRingSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterCancelableActionRingSlot->SetSize(FVector2D(90.0f, 90.0f));
		}

		ConfigureTextBlock(CenterCancelableActionPercentText, FText::GetEmpty(), FLinearColor(0.9f, 1.0f, 0.88f, 1.0f), 13);
		UCanvasPanelSlot* CenterCancelableActionTextSlot = CenterCancelableActionGaugeCanvas->AddChildToCanvas(CenterCancelableActionPercentText);
		if (CenterCancelableActionTextSlot)
		{
			CenterCancelableActionTextSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterCancelableActionTextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterCancelableActionTextSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterCancelableActionTextSlot->SetSize(FVector2D(54.0f, 24.0f));
		}

		CenterReloadPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterReloadPromptSlot = RootCanvas->AddChildToCanvas(CenterReloadPromptRoot);
		if (CenterReloadPromptSlot)
		{
			CenterReloadPromptSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadPromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadPromptSlot->SetPosition(FVector2D(0.0f, 92.0f));
			CenterReloadPromptSlot->SetAutoSize(true);
		}

		ConfigureTextBlock(CenterReloadPromptText, FText::FromString(TEXT("재장전")), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), 18);
		UHorizontalBoxSlot* CenterReloadPromptTextSlot = CenterReloadPromptRoot->AddChildToHorizontalBox(CenterReloadPromptText);
		if (CenterReloadPromptTextSlot)
		{
			CenterReloadPromptTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		CenterReloadPromptKeyBackground->SetPadding(FMargin(8.0f, 2.0f));
		CenterReloadPromptKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 24.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			0.0f));
		ConfigureTextBlock(CenterReloadPromptKeyText, FText::FromString(TEXT("R")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 13);
		CenterReloadPromptKeyBackground->SetContent(CenterReloadPromptKeyText);
		UHorizontalBoxSlot* CenterReloadPromptKeySlot = CenterReloadPromptRoot->AddChildToHorizontalBox(CenterReloadPromptKeyBackground);
		if (CenterReloadPromptKeySlot)
		{
			CenterReloadPromptKeySlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CenterReloadPromptKeySlot->SetVerticalAlignment(VAlign_Center);
		}

		UCanvasPanelSlot* BottomStatusSlot = BottomRow->AddChildToCanvas(BottomStatusWidget);
		if (BottomStatusSlot)
		{
			BottomStatusSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			BottomStatusSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			BottomStatusSlot->SetPosition(FVector2D(-(GameplayBottomQuickSlotWidth * 0.5f + GameplayBottomStatusGap), -20.0f));
			BottomStatusSlot->SetSize(FVector2D(GameplayBottomStatusWidth, GameplayBottomStatusHeight));
		}

		UCanvasPanelSlot* QuickSlotSlot = BottomRow->AddChildToCanvas(QuickSlotBarWidget);
		if (QuickSlotSlot)
		{
			QuickSlotSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			QuickSlotSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			QuickSlotSlot->SetPosition(FVector2D(0.0f, 0.0f));
			QuickSlotSlot->SetSize(FVector2D(GameplayBottomQuickSlotWidth, GameplayBottomQuickSlotHeight));
		}

		RegisterWidgetVariable(WidgetBlueprint, TopStatusReserveWidget);
		RegisterWidgetVariable(WidgetBlueprint, BottomStatusWidget);
		RegisterWidgetVariable(WidgetBlueprint, DebuffBarWidget);
		RegisterWidgetVariable(WidgetBlueprint, QuickSlotBarWidget);
		RegisterWidgetVariable(WidgetBlueprint, CenterCancelableActionGaugeRoot);
		RegisterWidgetVariable(WidgetBlueprint, CenterCancelableActionRingWidget);
		RegisterWidgetVariable(WidgetBlueprint, CenterReloadPromptRoot);
		RegisterWidgetVariable(WidgetBlueprint, CenterCancelableActionPercentText);
		RegisterWidgetVariable(WidgetBlueprint, CenterContentPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryAreaWidget);
		RegisterWidgetVariable(WidgetBlueprint, ItemInfoPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, ExternalPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, ResearchPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, UnsupportedModePanel);
		RegisterWidgetVariable(WidgetBlueprint, UnsupportedModeText);
		RegisterWidgetVariable(WidgetBlueprint, ModeTitleText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildPickupItemIconWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UImage* ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
		if (!RootSizeBox || !ItemIconImage)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(96.0f);
		RootSizeBox->SetHeightOverride(96.0f);
		RootSizeBox->SetContent(ItemIconImage);

		if (UTexture2D* DefaultIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol")))
		{
			ItemIconImage->SetBrushFromTexture(DefaultIconTexture, true);
		}
		ItemIconImage->SetColorAndOpacity(FLinearColor::White);
		ItemIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		ItemIconImage->SetOpacity(1.0f);

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ItemIconImage);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool EnsureCommonGameHudAssets()
	{
		if (!EnsureHudStatusIconTextures())
		{
			return false;
		}

		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* TopReserveWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudTopReserveWidgetAssetName,
			UTunaSweeperHudTopReserveWidget::StaticClass());
		UWidgetBlueprint* ResearchNodeWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ResearchNodeWidgetAssetName,
			UTunaSweeperResearchNodeWidget::StaticClass());
		UWidgetBlueprint* ResearchTreeWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ResearchTreeWidgetAssetName,
			UTunaSweeperResearchTreeWidget::StaticClass());
		UWidgetBlueprint* BottomStatusWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudBottomStatusWidgetAssetName,
			UTunaSweeperHudBottomStatusWidget::StaticClass());
		UWidgetBlueprint* DebuffBarWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudDebuffBarWidgetAssetName,
			UTunaSweeperHudDebuffBarWidget::StaticClass());
		UWidgetBlueprint* QuickSlotWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudQuickSlotBarWidgetAssetName,
			UTunaSweeperHudQuickSlotBarWidget::StaticClass());
		UWidgetBlueprint* InventoryAreaWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudInventoryAreaWidgetAssetName,
			UTunaSweeperHudInventoryAreaWidget::StaticClass());
		UWidgetBlueprint* ItemInfoPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudItemInfoPanelWidgetAssetName,
			UTunaSweeperHudItemInfoPanelWidget::StaticClass());
		UWidgetBlueprint* ExternalPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudExternalPanelWidgetAssetName,
			UTunaSweeperHudExternalPanelWidget::StaticClass());
		UWidgetBlueprint* LootContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			LootContainerWidgetAssetName,
			ULootContainerWidget::StaticClass());
		UWidgetBlueprint* StorageContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			StorageContainerWidgetAssetName,
			UStorageContainerWidget::StaticClass());
		UWidgetBlueprint* ShopContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ShopContainerWidgetAssetName,
			UShopContainerWidget::StaticClass());
		UWidgetBlueprint* WorkbenchPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			WorkbenchPanelWidgetAssetName,
			UTunaSweeperWorkbenchPanelWidget::StaticClass());
		UWidgetBlueprint* WorkbenchRecipeListEntryWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			WorkbenchRecipeListEntryWidgetAssetName,
			UTunaSweeperWorkbenchRecipeListEntryWidget::StaticClass());

		if (!ItemThumbnailWidgetBlueprint || !TopReserveWidgetBlueprint || !ResearchNodeWidgetBlueprint || !ResearchTreeWidgetBlueprint || !BottomStatusWidgetBlueprint ||
			!DebuffBarWidgetBlueprint || !QuickSlotWidgetBlueprint || !InventoryAreaWidgetBlueprint ||
			!ItemInfoPanelWidgetBlueprint || !ExternalPanelWidgetBlueprint ||
			!LootContainerWidgetBlueprint || !StorageContainerWidgetBlueprint || !ShopContainerWidgetBlueprint ||
			!WorkbenchPanelWidgetBlueprint || !WorkbenchRecipeListEntryWidgetBlueprint)
		{
			return false;
		}

		if (!BuildItemThumbnailSlotWidgetTree(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ItemThumbnailWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ItemThumbnailWidgetBlueprint);
		ItemThumbnailWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}

		const TSubclassOf<UUserWidget> ItemThumbnailWidgetClass = ItemThumbnailWidgetBlueprint->GeneratedClass.Get();
		if (!ItemThumbnailWidgetClass)
		{
			return false;
		}

		if (!BuildResearchNodeWidgetTree(ResearchNodeWidgetBlueprint)) return false;
		RegisterAllWidgetsInTree(ResearchNodeWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ResearchNodeWidgetBlueprint);
		ResearchNodeWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ResearchNodeWidgetBlueprint) || !ResearchNodeWidgetBlueprint->GeneratedClass) return false;
		if (!BuildResearchTreeWidgetTree(ResearchTreeWidgetBlueprint, ResearchNodeWidgetBlueprint->GeneratedClass.Get())) return false;
		RegisterAllWidgetsInTree(ResearchTreeWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ResearchTreeWidgetBlueprint);
		ResearchTreeWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ResearchTreeWidgetBlueprint) || !ResearchTreeWidgetBlueprint->GeneratedClass) return false;

		if (!BuildWorkbenchRecipeListEntryWidgetTree(WorkbenchRecipeListEntryWidgetBlueprint))
		{
			return false;
		}
		RegisterAllWidgetsInTree(WorkbenchRecipeListEntryWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(WorkbenchRecipeListEntryWidgetBlueprint);
		WorkbenchRecipeListEntryWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(WorkbenchRecipeListEntryWidgetBlueprint))
		{
			return false;
		}
		const TSubclassOf<UUserWidget> WorkbenchRecipeEntryWidgetClass =
			WorkbenchRecipeListEntryWidgetBlueprint->GeneratedClass.Get();
		if (!WorkbenchRecipeEntryWidgetClass)
		{
			return false;
		}

		const bool bChildWidgetsBuilt =
			BuildHudTopReserveWidgetTree(TopReserveWidgetBlueprint) &&
			BuildHudBottomStatusWidgetTree(BottomStatusWidgetBlueprint) &&
			BuildHudDebuffBarWidgetTree(DebuffBarWidgetBlueprint) &&
			BuildHudQuickSlotBarWidgetTree(QuickSlotWidgetBlueprint) &&
			BuildHudInventoryAreaWidgetTree(InventoryAreaWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildHudItemInfoPanelWidgetTree(ItemInfoPanelWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildLootContainerWidgetTree(LootContainerWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildLootContainerWidgetTree(StorageContainerWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildLootContainerWidgetTree(ShopContainerWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildWorkbenchPanelWidgetTree(
				WorkbenchPanelWidgetBlueprint,
				ItemThumbnailWidgetClass,
				WorkbenchRecipeEntryWidgetClass);

		if (!bChildWidgetsBuilt)
		{
			return false;
		}

		for (UWidgetBlueprint* ChildWidgetBlueprint : {
			TopReserveWidgetBlueprint,
			BottomStatusWidgetBlueprint,
			DebuffBarWidgetBlueprint,
			QuickSlotWidgetBlueprint,
			InventoryAreaWidgetBlueprint,
			ItemInfoPanelWidgetBlueprint,
			LootContainerWidgetBlueprint,
			StorageContainerWidgetBlueprint,
			ShopContainerWidgetBlueprint,
			WorkbenchRecipeListEntryWidgetBlueprint,
			WorkbenchPanelWidgetBlueprint
		})
		{
			RegisterAllWidgetsInTree(ChildWidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(ChildWidgetBlueprint);
			ChildWidgetBlueprint->MarkPackageDirty();
			if (!SaveAsset(ChildWidgetBlueprint))
			{
				return false;
			}
		}

		if (!BuildHudExternalPanelWidgetTree(
			ExternalPanelWidgetBlueprint,
			LootContainerWidgetBlueprint->GeneratedClass.Get(),
			StorageContainerWidgetBlueprint->GeneratedClass.Get(),
			ShopContainerWidgetBlueprint->GeneratedClass.Get(),
			WorkbenchPanelWidgetBlueprint->GeneratedClass.Get()))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ExternalPanelWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ExternalPanelWidgetBlueprint);
		ExternalPanelWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ExternalPanelWidgetBlueprint))
		{
			return false;
		}

		UWidgetBlueprint* GameHudWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			GameHudWidgetAssetName,
			UTunaSweeperGameHudWidget::StaticClass());
		if (!GameHudWidgetBlueprint)
		{
			return false;
		}

		if (!BuildGameHudWidgetTree(
			GameHudWidgetBlueprint,
			TopReserveWidgetBlueprint->GeneratedClass.Get(),
			BottomStatusWidgetBlueprint->GeneratedClass.Get(),
			DebuffBarWidgetBlueprint->GeneratedClass.Get(),
			QuickSlotWidgetBlueprint->GeneratedClass.Get(),
			InventoryAreaWidgetBlueprint->GeneratedClass.Get(),
			ItemInfoPanelWidgetBlueprint->GeneratedClass.Get(),
			ExternalPanelWidgetBlueprint->GeneratedClass.Get(),
			ResearchTreeWidgetBlueprint->GeneratedClass.Get()))
		{
			return false;
		}

		RegisterAllWidgetsInTree(GameHudWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(GameHudWidgetBlueprint);
		GameHudWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(GameHudWidgetBlueprint);
	}

	bool EnsureLootContainerOccupancyHeaderAssets()
	{
		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* LootContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			LootContainerWidgetAssetName,
			ULootContainerWidget::StaticClass());
		if (!ItemThumbnailWidgetBlueprint || !LootContainerWidgetBlueprint)
		{
			return false;
		}

		if (!ItemThumbnailWidgetBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(ItemThumbnailWidgetBlueprint);
		}

		const TSubclassOf<UUserWidget> ItemThumbnailWidgetClass = ItemThumbnailWidgetBlueprint->GeneratedClass.Get();
		if (!ItemThumbnailWidgetClass || !BuildLootContainerWidgetTree(LootContainerWidgetBlueprint, ItemThumbnailWidgetClass))
		{
			return false;
		}

		RegisterAllWidgetsInTree(LootContainerWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(LootContainerWidgetBlueprint);
		LootContainerWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(LootContainerWidgetBlueprint);
	}

	bool EnsureBackpackInventoryAssets()
	{
		if (!EnsureItemIconTextures() || !EnsureEquipmentIconTextures())
		{
			return false;
		}

		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* InventoryAreaWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudInventoryAreaWidgetAssetName,
			UTunaSweeperHudInventoryAreaWidget::StaticClass());
		if (!ItemThumbnailWidgetBlueprint || !InventoryAreaWidgetBlueprint)
		{
			return false;
		}

		if (!BuildItemThumbnailSlotWidgetTree(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ItemThumbnailWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ItemThumbnailWidgetBlueprint);
		ItemThumbnailWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}

		const TSubclassOf<UUserWidget> ItemThumbnailWidgetClass = ItemThumbnailWidgetBlueprint->GeneratedClass.Get();
		if (!ItemThumbnailWidgetClass)
		{
			return false;
		}

		if (!BuildHudInventoryAreaWidgetTree(InventoryAreaWidgetBlueprint, ItemThumbnailWidgetClass))
		{
			return false;
		}
		RegisterAllWidgetsInTree(InventoryAreaWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(InventoryAreaWidgetBlueprint);
		InventoryAreaWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(InventoryAreaWidgetBlueprint);
	}

	bool BuildInteractionMarkerWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();

		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UHorizontalBox* MarkerRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MarkerRoot"));
		USizeBox* MarkerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MarkerSizeBox"));
		UOverlay* MarkerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MarkerOverlay"));
		USizeBox* RingImage = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RingImage"));
		UImage* RingBrushImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RingBrushImage"));
		USizeBox* FilledImage = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FilledImage"));
		UImage* FilledBrushImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FilledBrushImage"));
		UBorder* LabelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LabelBackground"));
		UHorizontalBox* LabelContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LabelContentRow"));
		UTextBlock* DisplayNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplayNameText"));
		UBorder* RequirementBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RequirementBackground"));
		UHorizontalBox* RequirementRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RequirementRoot"));
		USizeBox* RequirementIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RequirementIconBox"));
		UImage* RequirementIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RequirementIconImage"));
		UTextBlock* RequirementQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RequirementQuantityText"));

		if (!RootCanvas || !MarkerRoot || !MarkerSizeBox || !MarkerOverlay || !RingImage || !RingBrushImage ||
			!FilledImage || !FilledBrushImage || !LabelBackground || !LabelContentRow || !DisplayNameText ||
			!RequirementBackground || !RequirementRoot || !RequirementIconBox || !RequirementIconImage ||
			!RequirementQuantityText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(MarkerRoot);
		if (RootSlot)
		{
			RootSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(400.0f, 56.0f));
		}

		MarkerRoot->SetRenderOpacity(0.0f);

		MarkerSizeBox->SetWidthOverride(56.0f);
		MarkerSizeBox->SetHeightOverride(56.0f);
		MarkerSizeBox->SetContent(MarkerOverlay);

		UHorizontalBoxSlot* MarkerSlot = MarkerRoot->AddChildToHorizontalBox(MarkerSizeBox);
		if (MarkerSlot)
		{
			MarkerSlot->SetHorizontalAlignment(HAlign_Center);
			MarkerSlot->SetVerticalAlignment(VAlign_Center);
		}

		RingImage->SetWidthOverride(34.0f);
		RingImage->SetHeightOverride(34.0f);
		RingImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		RingBrushImage->SetBrush(MakeCircularBrush(FVector2D(34.0f, 34.0f), FLinearColor::Transparent, FLinearColor::White, 3.0f));
		RingImage->SetContent(RingBrushImage);

		UOverlaySlot* RingSlot = MarkerOverlay->AddChildToOverlay(RingImage);
		if (RingSlot)
		{
			RingSlot->SetHorizontalAlignment(HAlign_Center);
			RingSlot->SetVerticalAlignment(VAlign_Center);
		}

		FilledImage->SetWidthOverride(12.0f);
		FilledImage->SetHeightOverride(12.0f);
		FilledBrushImage->SetBrush(MakeCircularBrush(FVector2D(12.0f, 12.0f), FLinearColor::White, FLinearColor::Transparent, 0.0f));
		FilledImage->SetContent(FilledBrushImage);

		UOverlaySlot* FilledSlot = MarkerOverlay->AddChildToOverlay(FilledImage);
		if (FilledSlot)
		{
			FilledSlot->SetHorizontalAlignment(HAlign_Center);
			FilledSlot->SetVerticalAlignment(VAlign_Center);
		}

		LabelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(128.0f, 28.0f),
			FLinearColor::White,
			FLinearColor::Transparent,
			0.0f,
			5.0f));
		LabelBackground->SetBrushColor(FLinearColor::White);
		LabelBackground->SetPadding(FMargin(12.0f, 4.0f, 10.0f, 4.0f));

		ConfigureTextBlock(DisplayNameText, FText::FromString(TEXT("Interact")), FLinearColor::Black, 18);
		LabelBackground->SetContent(LabelContentRow);

		if (UHorizontalBoxSlot* DisplayNameSlot = LabelContentRow->AddChildToHorizontalBox(DisplayNameText))
		{
			DisplayNameSlot->SetHorizontalAlignment(HAlign_Left);
			DisplayNameSlot->SetVerticalAlignment(VAlign_Center);
		}

		RequirementIconBox->SetWidthOverride(22.0f);
		RequirementIconBox->SetHeightOverride(22.0f);
		RequirementIconBox->SetContent(RequirementIconImage);
		RequirementIconImage->SetColorAndOpacity(FLinearColor::White);
		RequirementIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		RequirementIconImage->SetOpacity(0.0f);

		if (UHorizontalBoxSlot* RequirementIconSlot = RequirementRoot->AddChildToHorizontalBox(RequirementIconBox))
		{
			RequirementIconSlot->SetHorizontalAlignment(HAlign_Center);
			RequirementIconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(RequirementQuantityText, FText::FromString(TEXT("x0")), FLinearColor::White, 17);
		if (UHorizontalBoxSlot* RequirementQuantitySlot = RequirementRoot->AddChildToHorizontalBox(RequirementQuantityText))
		{
			RequirementQuantitySlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
			RequirementQuantitySlot->SetHorizontalAlignment(HAlign_Left);
			RequirementQuantitySlot->SetVerticalAlignment(VAlign_Center);
		}

		RequirementRoot->SetVisibility(ESlateVisibility::Collapsed);
		RequirementBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(58.0f, 30.0f),
			FLinearColor(0.03f, 0.50f, 0.68f, 0.96f),
			FLinearColor(0.72f, 0.95f, 1.0f, 0.30f),
			1.0f,
			5.0f));
		RequirementBackground->SetPadding(FMargin(8.0f, 3.0f, 9.0f, 3.0f));
		RequirementBackground->SetContent(RequirementRoot);
		RequirementBackground->SetVisibility(ESlateVisibility::Collapsed);

		UHorizontalBoxSlot* LabelSlot = MarkerRoot->AddChildToHorizontalBox(LabelBackground);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UHorizontalBoxSlot* RequirementSlot = MarkerRoot->AddChildToHorizontalBox(RequirementBackground))
		{
			RequirementSlot->SetPadding(FMargin(-6.0f, 0.0f, 0.0f, 0.0f));
			RequirementSlot->SetHorizontalAlignment(HAlign_Left);
			RequirementSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);

		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	UWidgetBlueprint* EnsureInteractionMarkerWidgetBlueprint();

	bool RebuildInteractionMarkerWidgetAlignment()
	{
		const FString ObjectPath = GetAssetObjectPath(UIAssetPath, InteractionMarkerAssetName);
		UWidgetBlueprint* MarkerWidgetBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath);
		if (!MarkerWidgetBlueprint)
		{
			MarkerWidgetBlueprint = EnsureInteractionMarkerWidgetBlueprint();
		}

		if (!MarkerWidgetBlueprint)
		{
			return false;
		}

		if (!BuildInteractionMarkerWidgetTree(MarkerWidgetBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to rebuild marker alignment for %s."), *ObjectPath);
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(MarkerWidgetBlueprint);
		MarkerWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(MarkerWidgetBlueprint);
	}

	UWidgetBlueprint* EnsureInteractionMarkerWidgetBlueprint()
	{
		const FString ObjectPath = GetAssetObjectPath(UIAssetPath, InteractionMarkerAssetName);
		if (UWidgetBlueprint* ExistingBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(UTunaSweeperInteractionMarkerWidget::StaticClass()))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on UTunaSweeperInteractionMarkerWidget."), *ObjectPath);
				return nullptr;
			}

			if (!ExistingBlueprint->WidgetTree || !ExistingBlueprint->WidgetTree->RootWidget)
			{
				if (!BuildInteractionMarkerWidgetTree(ExistingBlueprint))
				{
					UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build widget tree for %s."), *ObjectPath);
					return nullptr;
				}
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			SaveAsset(ExistingBlueprint);
			return ExistingBlueprint;
		}

		UWidgetBlueprintFactory* WidgetBlueprintFactory = NewObject<UWidgetBlueprintFactory>();
		WidgetBlueprintFactory->ParentClass = UTunaSweeperInteractionMarkerWidget::StaticClass();

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			InteractionMarkerAssetName,
			UIAssetPath,
			UWidgetBlueprint::StaticClass(),
			WidgetBlueprintFactory);

		UWidgetBlueprint* CreatedBlueprint = Cast<UWidgetBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		if (!BuildInteractionMarkerWidgetTree(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build widget tree for %s."), *ObjectPath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return CreatedBlueprint;
	}

	bool ConfigureIntroMenuWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildTitleIntroMenuWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureLevelTransitionVideoWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildLevelTransitionVideoWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureSpeechBubbleWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildSpeechBubbleWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureQuestWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildQuestWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

}
