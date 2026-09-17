#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Components/ButtonSlot.h"

namespace TunaSweeperIntroMenuLayout
{
	constexpr const TCHAR* DemoFishTexturePath =
		TEXT("/Game/UI/Title/tuna_sweeper_fish_transparent.tuna_sweeper_fish_transparent");
}

void UTunaSweeperIntroMenuWidget::ResetTitleViewportLayoutState()
{
	auto ResetWidgetTransform = [](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		Widget->SetRenderTransform(FWidgetTransform());
		Widget->SetRenderTransformPivot(FVector2D::ZeroVector);
		Widget->SetRenderScale(FVector2D::UnitVector);
	};

	ResetWidgetTransform(this);
	ResetWidgetTransform(GetRootWidget());
	ResetWidgetTransform(MainMenuPanel.Get());
	ResetWidgetTransform(SaveSlotPanel.Get());
	ResetWidgetTransform(SettingsPanel.Get());
	ResetWidgetTransform(CreditsPanel.Get());

	InvalidateLayoutAndVolatility();
}

void UTunaSweeperIntroMenuWidget::EnsureLanguageOptionRows()
{
	if (!WidgetTree)
	{
		return;
	}

	auto FindVerticalParent = [](UWidget* Widget, UWidget*& OutDirectChild) -> UVerticalBox*
	{
		OutDirectChild = Widget;
		for (UPanelWidget* Parent = Widget ? Widget->GetParent() : nullptr;
			Parent;
			OutDirectChild = Parent, Parent = Parent->GetParent())
		{
			if (UVerticalBox* VerticalParent = Cast<UVerticalBox>(Parent))
			{
				return VerticalParent;
			}
		}
		return nullptr;
	};

	auto CollapseLegacyControl = [&FindVerticalParent](UWidget* Widget, UVerticalBox* ExpectedParent)
	{
		UWidget* DirectChild = nullptr;
		if (FindVerticalParent(Widget, DirectChild) == ExpectedParent && DirectChild)
		{
			DirectChild->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	auto AddOptionRow = [this, &FindVerticalParent, &CollapseLegacyControl](
		TObjectPtr<UTunaSweeperOptionRowWidget>& OptionRow,
		const TCHAR* RowName,
		const FText& Label,
		const TArray<UWidget*>& LegacyControls,
		UWidget* FallbackPanel)
	{
		if (OptionRow)
		{
			OptionRow->Configure(Label);
			return;
		}

		UWidget* DirectChild = nullptr;
		UVerticalBox* Parent = LegacyControls.Num() > 0
			? FindVerticalParent(LegacyControls[0], DirectChild)
			: nullptr;
		if (!Parent)
		{
			Parent = Cast<UVerticalBox>(FallbackPanel);
		}
		if (!Parent)
		{
			return;
		}

		int32 InsertIndex = DirectChild ? Parent->GetChildIndex(DirectChild) : Parent->GetChildrenCount();
		if (InsertIndex == INDEX_NONE)
		{
			InsertIndex = Parent->GetChildrenCount();
		}
		for (UWidget* LegacyControl : LegacyControls)
		{
			CollapseLegacyControl(LegacyControl, Parent);
		}

		UWidgetTree* OwningTree = Parent->GetTypedOuter<UWidgetTree>();
		if (!OwningTree)
		{
			OwningTree = WidgetTree;
		}
		UTunaSweeperOptionRowWidget* NewRow = OwningTree->ConstructWidget<UTunaSweeperOptionRowWidget>(
			UTunaSweeperOptionRowWidget::StaticClass(),
			RowName);
		if (!NewRow)
		{
			return;
		}
		NewRow->Configure(Label);

		struct FChildLayout
		{
			UWidget* Widget = nullptr;
			FMargin Padding;
			FSlateChildSize Size;
			EHorizontalAlignment HorizontalAlignment = HAlign_Fill;
			EVerticalAlignment VerticalAlignment = VAlign_Fill;
		};
		TArray<FChildLayout> OrderedChildren;
		OrderedChildren.Reserve(Parent->GetChildrenCount() + 1);
		for (int32 ChildIndex = 0; ChildIndex < Parent->GetChildrenCount(); ++ChildIndex)
		{
			UWidget* Child = Parent->GetChildAt(ChildIndex);
			FChildLayout Layout;
			Layout.Widget = Child;
			if (const UVerticalBoxSlot* ExistingSlot = Cast<UVerticalBoxSlot>(Child ? Child->Slot : nullptr))
			{
				Layout.Padding = ExistingSlot->GetPadding();
				Layout.Size = ExistingSlot->GetSize();
				Layout.HorizontalAlignment = ExistingSlot->GetHorizontalAlignment();
				Layout.VerticalAlignment = ExistingSlot->GetVerticalAlignment();
			}
			OrderedChildren.Add(Layout);
		}

		FChildLayout RowLayout;
		RowLayout.Widget = NewRow;
		RowLayout.Padding = FMargin(0.0f, 5.0f);
		RowLayout.Size = FSlateChildSize(ESlateSizeRule::Automatic);
		RowLayout.HorizontalAlignment = HAlign_Fill;
		RowLayout.VerticalAlignment = VAlign_Center;
		OrderedChildren.Insert(RowLayout, FMath::Clamp(InsertIndex, 0, OrderedChildren.Num()));

		// Re-adding the ordered children keeps the live Slate box in sync with UMG's slot order.
		Parent->ClearChildren();
		for (const FChildLayout& ChildLayout : OrderedChildren)
		{
			if (UVerticalBoxSlot* AddedSlot = Parent->AddChildToVerticalBox(ChildLayout.Widget))
			{
				AddedSlot->SetPadding(ChildLayout.Padding);
				AddedSlot->SetSize(ChildLayout.Size);
				AddedSlot->SetHorizontalAlignment(ChildLayout.HorizontalAlignment);
				AddedSlot->SetVerticalAlignment(ChildLayout.VerticalAlignment);
			}
		}
		OptionRow = NewRow;
	};

	AddOptionRow(
		InterfaceLanguageOptionRow,
		TEXT("InterfaceLanguageOptionRow"),
		ResolveUiText(FName(TEXT("ui.settings.language")), FText::GetEmpty()),
		{LanguageEnglishButton.Get(), LanguageKoreanButton.Get(), LanguageJapaneseButton.Get(),
			FindIntroWidget(TEXT("LanguageLabelText"))},
		InterfaceSettingsPanel.Get());
	if (InterfaceLanguageOptionRow)
	{
		InterfaceLanguageOptionRow->OnStepRequested.RemoveAll(this);
		InterfaceLanguageOptionRow->OnStepRequested.AddUObject(
			this,
			&UTunaSweeperIntroMenuWidget::HandleInterfaceLanguageStepRequested);
	}
	if (UBorder* LegacyLanguageSection = Cast<UBorder>(FindIntroWidget(TEXT("SettingsLanguageSection"))))
	{
		FSlateBrush ClearBrush;
		ClearBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
		ClearBrush.SetResourceObject(nullptr);
		LegacyLanguageSection->SetBrush(ClearBrush);
		LegacyLanguageSection->SetBrushColor(FLinearColor::Transparent);
	}

	AddOptionRow(
		DebugDisplayLanguageOptionRow,
		TEXT("DebugDisplayLanguageOptionRow"),
		ResolveUiText(FName(TEXT("ui.settings.development.debug_display_language")), FText::GetEmpty()),
		{DebugDisplayLanguageKoreanButton.Get(), DebugDisplayLanguageEnglishButton.Get(),
			FindIntroWidget(TEXT("DebugDisplayLanguageLabelText"))},
		DevelopmentSettingsPanel.Get());
	if (DebugDisplayLanguageOptionRow)
	{
		DebugDisplayLanguageOptionRow->OnStepRequested.RemoveAll(this);
		DebugDisplayLanguageOptionRow->OnStepRequested.AddUObject(
			this,
			&UTunaSweeperIntroMenuWidget::HandleDebugDisplayLanguageStepRequested);
	}
}

void UTunaSweeperIntroMenuWidget::ApplyUnifiedControlStyles()
{
	using TunaSweeperUIStyle::EButtonRole;

	auto ApplyNestedLabels = [](UWidget* Root)
	{
		TArray<UWidget*> Pending;
		if (Root)
		{
			Pending.Add(Root);
		}
		while (Pending.Num() > 0)
		{
			UWidget* Widget = Pending.Pop(EAllowShrinking::No);
			if (UTextBlock* Label = Cast<UTextBlock>(Widget))
			{
				TunaSweeperUIStyle::ApplyLabel(Label);
			}
			if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
			{
				for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
				{
					Pending.Add(Panel->GetChildAt(ChildIndex));
				}
			}
		}
	};

	auto StyleButton = [&ApplyNestedLabels](UButton* Button, EButtonRole Role, bool bSelected = false)
	{
		if (!Button)
		{
			return;
		}
		TunaSweeperUIStyle::ApplyButton(Button, Role, bSelected);
		ApplyNestedLabels(Button->GetContent());
	};
	auto EnsureFittedTabLabel = [this](UButton* Button)
	{
		if (!Button || Cast<UScaleBox>(Button->GetContent()))
		{
			return;
		}

		UTextBlock* Label = Cast<UTextBlock>(Button->GetContent());
		UWidgetTree* OwningTree = Button->GetTypedOuter<UWidgetTree>();
		if (!Label || !OwningTree)
		{
			return;
		}

		Label->RemoveFromParent();
		Label->SetAutoWrapText(false);
		UScaleBox* LabelScaleBox = OwningTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(),
			FName(*(Button->GetName() + TEXT("_LabelScaleBox"))));
		LabelScaleBox->SetStretch(EStretch::ScaleToFit);
		LabelScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
		LabelScaleBox->SetContent(Label);
		if (UScaleBoxSlot* LabelSlot = Cast<UScaleBoxSlot>(Label->Slot))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		Button->SetContent(LabelScaleBox);
	};

	// Keep only the localized action label, removing the authored number and slash ornaments.
	for (UButton* Button : {StartButton.Get(), SlotSelectButton.Get(), SettingsButton.Get(), CreditsButton.Get(), QuitButton.Get()})
	{
		UTextBlock* Label = Button ? Cast<UTextBlock>(FindIntroWidget(FName(*(Button->GetName() + TEXT("Text"))))) : nullptr;
		if (!Label) continue;
		if (Button->GetContent() != Label)
		{
			Label->RemoveFromParent();
			Button->SetContent(Label);
		}
		Label->SetJustification(ETextJustify::Center);
		Label->SetMargin(FMargin(0.0f));
	}

	StyleButton(StartButton, EButtonRole::Primary);
	StyleButton(SlotSelectButton, EButtonRole::Secondary);
	StyleButton(SettingsButton, EButtonRole::Secondary);
	StyleButton(CreditsButton, EButtonRole::Secondary);
	StyleButton(QuitButton, EButtonRole::Secondary);
	StyleButton(SteamDemoWishlistButton, EButtonRole::Primary);
	if (SteamDemoWishlistButton)
	{
		FButtonStyle WishlistStyle = SteamDemoWishlistButton->GetStyle();
		WishlistStyle.Normal.TintColor = FLinearColor(0.55f, 0.24f, 0.10f);
		WishlistStyle.Hovered.TintColor = FLinearColor(0.70f, 0.34f, 0.16f);
		WishlistStyle.Pressed.TintColor = FLinearColor(0.38f, 0.15f, 0.06f);
		SteamDemoWishlistButton->SetStyle(WishlistStyle);
	}

	// Save-slot cards retain their comparison presentation; only their actions adopt the shared language.
	StyleButton(PrimarySaveSlotButton, EButtonRole::Primary);
	StyleButton(DeleteSaveSlotButton, EButtonRole::Danger);
	StyleButton(BackToMainMenuButton, EButtonRole::Secondary);
	StyleButton(ConfirmDeleteButton, EButtonRole::Danger);
	StyleButton(CancelDeleteButton, EButtonRole::Secondary);

	EnsureFittedTabLabel(SettingsGraphicsTabButton);
	EnsureFittedTabLabel(SettingsInterfaceTabButton);
	EnsureFittedTabLabel(SettingsDevelopmentTabButton);
	auto ExtendButtonToLeftEdge = [this](UButton* Button, const TCHAR* BoxName)
	{
		if (!Button || !Button->GetContent()) return;
		USizeBox* Box = Cast<USizeBox>(FindIntroWidget(BoxName));
		UCanvasPanelSlot* Slot = Box ? Cast<UCanvasPanelSlot>(Box->Slot) : nullptr;
		UVerticalBoxSlot* StackSlot = Box ? Cast<UVerticalBoxSlot>(Box->Slot) : nullptr;
		UCanvasPanelSlot* StackCanvasSlot = Box && Box->GetParent()
			? Cast<UCanvasPanelSlot>(Box->GetParent()->Slot) : nullptr;
		const float LeftInset = Slot ? Slot->GetPosition().X
			: (StackSlot && StackCanvasSlot ? StackCanvasSlot->GetPosition().X + StackSlot->GetPadding().Left : 0.0f);
		if (LeftInset <= 0.0f) return;
		if (Slot)
		{
			Slot->SetPosition(FVector2D(0.0f, Slot->GetPosition().Y));
			Slot->SetSize(Slot->GetSize() + FVector2D(LeftInset, 0.0f));
		}
		else
		{
			FMargin Padding = StackSlot->GetPadding();
			Padding.Left -= LeftInset;
			StackSlot->SetPadding(Padding);
		}
		if (Box->IsWidthOverride()) Box->SetWidthOverride(Box->GetWidthOverride() + LeftInset);
		// Expand the hit area and background while preserving the content's screen position.
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContent()->Slot))
		{
			FMargin Padding = ContentSlot->GetPadding();
			Padding.Left += LeftInset;
			ContentSlot->SetPadding(Padding);
			if (Cast<UScaleBox>(Button->GetContent())) ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	};
	ExtendButtonToLeftEdge(SettingsGraphicsTabButton, TEXT("GraphicsTabButtonBox"));
	ExtendButtonToLeftEdge(SettingsInterfaceTabButton, TEXT("InterfaceTabButtonBox"));
	ExtendButtonToLeftEdge(SettingsDevelopmentTabButton, TEXT("DevelopmentTabButtonBox"));
	ExtendButtonToLeftEdge(BackFromSettingsButton, TEXT("BackFromSettingsButtonBox"));
	auto StyleSettingsTab = [this, &ApplyNestedLabels](UButton* Button, bool bSelected)
	{
		ApplySettingsTabButtonStyle(Button, FVector2D(214.0f, 50.0f), bSelected);
		if (Button) ApplyNestedLabels(Button->GetContent());
	};
	StyleSettingsTab(SettingsGraphicsTabButton, !bShowingInterfaceSettingsTab && !bShowingDevelopmentSettingsTab);
	StyleSettingsTab(SettingsInterfaceTabButton, bShowingInterfaceSettingsTab);
	StyleSettingsTab(SettingsDevelopmentTabButton, bShowingDevelopmentSettingsTab);
	StyleButton(ConfirmInterfaceSettingsButton, EButtonRole::Primary);
	StyleButton(CancelInterfaceSettingsButton, EButtonRole::Secondary);
	StyleButton(BackFromSettingsButton, EButtonRole::Secondary);
	if (BackFromSettingsButton)
	{
		FButtonStyle BackStyle = BackFromSettingsButton->GetStyle();
		BackStyle.Normal.TintColor = FLinearColor::Transparent;
		BackStyle.Hovered.OutlineSettings.CornerRadii = FVector4(0.0f);
		BackStyle.Pressed.OutlineSettings.CornerRadii = FVector4(0.0f);
		BackFromSettingsButton->SetStyle(BackStyle);
	}
	StyleButton(DeleteCurrentSaveDataButton, EButtonRole::Danger);

	StyleButton(DifficultyStartButton, EButtonRole::Primary);
	StyleButton(DifficultyBackButton, EButtonRole::Secondary);
	StyleButton(DemoNoticeConfirmButton, EButtonRole::Primary);
	StyleButton(DemoNoticeBackButton, EButtonRole::Secondary);
	StyleButton(BackFromCreditsButton, EButtonRole::Secondary);
}

void UTunaSweeperIntroMenuWidget::EnsurePiggyBankToggleButton()
{
	if (PiggyBankToggleButton || !WidgetTree || !EnemyCombatDebugToggleButton)
	{
		return;
	}

	UVerticalBox* DevelopmentSettingsStack = nullptr;
	UCanvasPanel* DevelopmentSettingsCanvas = nullptr;
	for (UPanelWidget* Parent = EnemyCombatDebugToggleButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		DevelopmentSettingsStack = Cast<UVerticalBox>(Parent);
		if (DevelopmentSettingsStack)
		{
			break;
		}

		DevelopmentSettingsCanvas = Cast<UCanvasPanel>(Parent);
		if (DevelopmentSettingsCanvas)
		{
			break;
		}
	}

	if (!DevelopmentSettingsStack && !DevelopmentSettingsCanvas)
	{
		return;
	}

	UButton* NewPiggyBankToggleButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("PiggyBankToggleButton"));
	UTextBlock* PiggyBankToggleButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("PiggyBankToggleButtonText"));
	if (!NewPiggyBankToggleButton || !PiggyBankToggleButtonText)
	{
		return;
	}

	PiggyBankToggleButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.settings.development.piggy_bank")),
		FText::GetEmpty()));
	PiggyBankToggleButtonText->SetJustification(ETextJustify::Center);
	PiggyBankToggleButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(PiggyBankToggleButtonText, 17, ETunaSweeperUIFontWeight::Bold);
	NewPiggyBankToggleButton->SetContent(PiggyBankToggleButtonText);

	bool bAdded = false;
	if (DevelopmentSettingsStack)
	{
		if (UVerticalBoxSlot* AddedSlot = DevelopmentSettingsStack->AddChildToVerticalBox(NewPiggyBankToggleButton))
		{
			AddedSlot->SetHorizontalAlignment(HAlign_Fill);
			AddedSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			bAdded = true;
		}
	}
	else if (DevelopmentSettingsCanvas)
	{
		const UCanvasPanelSlot* ExistingSlot = Cast<UCanvasPanelSlot>(EnemyCombatDebugToggleButton->Slot);
		if (UCanvasPanelSlot* NewSlot = DevelopmentSettingsCanvas->AddChildToCanvas(NewPiggyBankToggleButton))
		{
			if (ExistingSlot)
			{
				NewSlot->SetAnchors(ExistingSlot->GetAnchors());
				NewSlot->SetAlignment(ExistingSlot->GetAlignment());
				NewSlot->SetSize(ExistingSlot->GetSize());
				NewSlot->SetPosition(ExistingSlot->GetPosition() + FVector2D(0.0f, ExistingSlot->GetSize().Y + 8.0f));
			}
			else
			{
				NewSlot->SetSize(FVector2D(660.0f, 46.0f));
			}
			bAdded = true;
		}
	}

	if (bAdded)
	{
		PiggyBankToggleButton = NewPiggyBankToggleButton;
	}
}

void UTunaSweeperIntroMenuWidget::EnsureAlwaysSlowPresentationToggleButton()
{
	UButton* AnchorButton = PiggyBankToggleButton ? PiggyBankToggleButton.Get() : EnemyCombatDebugToggleButton.Get();
	if (AlwaysSlowPresentationToggleButton || !WidgetTree || !AnchorButton)
	{
		return;
	}

	UVerticalBox* DevelopmentSettingsStack = nullptr;
	UCanvasPanel* DevelopmentSettingsCanvas = nullptr;
	for (UPanelWidget* Parent = AnchorButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		DevelopmentSettingsStack = Cast<UVerticalBox>(Parent);
		if (DevelopmentSettingsStack)
		{
			break;
		}

		DevelopmentSettingsCanvas = Cast<UCanvasPanel>(Parent);
		if (DevelopmentSettingsCanvas)
		{
			break;
		}
	}

	if (!DevelopmentSettingsStack && !DevelopmentSettingsCanvas)
	{
		return;
	}

	UButton* NewToggleButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("AlwaysSlowPresentationToggleButton"));
	UTextBlock* NewToggleButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("AlwaysSlowPresentationToggleButtonText"));
	if (!NewToggleButton || !NewToggleButtonText)
	{
		return;
	}

	NewToggleButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.settings.development.always_slow_presentation")),
		FText::GetEmpty()));
	NewToggleButtonText->SetJustification(ETextJustify::Center);
	NewToggleButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(NewToggleButtonText, 17, ETunaSweeperUIFontWeight::Bold);
	NewToggleButton->SetContent(NewToggleButtonText);

	bool bAdded = false;
	if (DevelopmentSettingsStack)
	{
		if (UVerticalBoxSlot* AddedSlot = DevelopmentSettingsStack->AddChildToVerticalBox(NewToggleButton))
		{
			AddedSlot->SetHorizontalAlignment(HAlign_Fill);
			AddedSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			bAdded = true;
		}
	}
	else if (DevelopmentSettingsCanvas)
	{
		const UCanvasPanelSlot* ExistingSlot = Cast<UCanvasPanelSlot>(AnchorButton->Slot);
		if (UCanvasPanelSlot* NewSlot = DevelopmentSettingsCanvas->AddChildToCanvas(NewToggleButton))
		{
			if (ExistingSlot)
			{
				NewSlot->SetAnchors(ExistingSlot->GetAnchors());
				NewSlot->SetAlignment(ExistingSlot->GetAlignment());
				NewSlot->SetSize(ExistingSlot->GetSize());
				NewSlot->SetPosition(ExistingSlot->GetPosition() + FVector2D(0.0f, ExistingSlot->GetSize().Y + 8.0f));
			}
			else
			{
				NewSlot->SetSize(FVector2D(660.0f, 46.0f));
			}
			bAdded = true;
		}
	}

	if (!bAdded)
	{
		return;
	}

	AlwaysSlowPresentationToggleButton = NewToggleButton;
	for (UPanelWidget* Parent = NewToggleButton->GetParent(); Parent; Parent = Parent->GetParent())
	{
		if (USizeBox* SectionBox = Cast<USizeBox>(Parent);
			SectionBox && SectionBox->GetFName() == FName(TEXT("EnemyCombatDebugSection")))
		{
			SectionBox->SetHeightOverride(212.0f);
			break;
		}
	}
}

void UTunaSweeperIntroMenuWidget::EnsureSaveDataManagementSection()
{
	if (SaveDataManagementSection || !WidgetTree)
	{
		return;
	}

	UVerticalBox* DevelopmentSettingsStack = Cast<UVerticalBox>(DevelopmentSettingsPanel.Get());
	if (!DevelopmentSettingsStack)
	{
		return;
	}

	UBorder* Section = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SaveDataManagementSection"));
	UVerticalBox* SectionStack = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("SaveDataManagementSectionStack"));
	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SaveDataManagementTitleText"));
	UTextBlock* StatusText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SaveDataManagementStatusText"));
	USizeBox* DeleteButtonBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("DeleteCurrentSaveDataButtonBox"));
	UButton* DeleteButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("DeleteCurrentSaveDataButton"));
	UTextBlock* DeleteButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DeleteCurrentSaveDataButtonText"));
	if (!Section || !SectionStack || !TitleText || !StatusText ||
		!DeleteButtonBox || !DeleteButton || !DeleteButtonText)
	{
		return;
	}

	TitleText->SetText(ResolveUiText(
		FName(TEXT("ui.settings.development.save_data_title")),
		FText::FromString(TEXT("세이브 데이터 관리"))));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.80f, 0.78f, 1.0f)));
	TitleText->SetJustification(ETextJustify::Left);
	TunaSweeperUIFont::ApplyFont(TitleText, 15, ETunaSweeperUIFontWeight::Bold);

	StatusText->SetAutoWrapText(true);
	StatusText->SetWrapTextAt(640.0f);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.74f, 0.80f, 0.79f, 1.0f)));
	StatusText->SetJustification(ETextJustify::Left);
	TunaSweeperUIFont::ApplyFont(StatusText, 14);

	DeleteButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.settings.development.delete_current_save")),
		FText::FromString(TEXT("현재 슬롯 세이브 데이터 즉시 삭제"))));
	DeleteButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DeleteButtonText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(DeleteButtonText, 17, ETunaSweeperUIFontWeight::Bold);
	DeleteButton->SetContent(DeleteButtonText);
	DeleteButtonBox->SetWidthOverride(660.0f);
	DeleteButtonBox->SetHeightOverride(46.0f);
	DeleteButtonBox->SetContent(DeleteButton);

	Section->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 16.0f));
	Section->SetBrush(TunaSweeperSettingsUi::MakeRoundedBoxBrush(
		FVector2D(760.0f, 166.0f),
		FLinearColor(0.045f, 0.020f, 0.020f, 0.72f),
		FLinearColor(0.64f, 0.24f, 0.22f, 0.62f),
		1.0f,
		8.0f));
	Section->SetContent(SectionStack);

	if (UVerticalBoxSlot* TitleSlot = SectionStack->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}
	if (UVerticalBoxSlot* StatusSlot = SectionStack->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}
	SectionStack->AddChildToVerticalBox(DeleteButtonBox);

	if (UVerticalBoxSlot* SectionSlot = DevelopmentSettingsStack->AddChildToVerticalBox(Section))
	{
		SectionSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		SectionSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	SaveDataManagementSection = Section;
	SaveDataManagementStatusText = StatusText;
	DeleteCurrentSaveDataButton = DeleteButton;
	DeleteCurrentSaveDataButtonText = DeleteButtonText;
}

void UTunaSweeperIntroMenuWidget::EnsureDevelopmentToggleButtonContent(
	UButton* ToggleButton,
	FName LabelWidgetName,
	FName IndicatorWidgetName)
{
	(void)IndicatorWidgetName;
	if (UTextBlock* LabelText = Cast<UTextBlock>(FindIntroWidget(LabelWidgetName)))
	{
		TunaSweeperUIStyle::SetCheckButton(WidgetTree, ToggleButton, LabelText, false);
	}
}

void UTunaSweeperIntroMenuWidget::EnsureDemoBuildImage()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!DemoBuildImage)
	{
		DemoBuildImage = Cast<UImage>(FindIntroWidget(TEXT("DemoBuildImage")));
	}

	if (!TunaSweeperBuildFlavor::IsDemo())
	{
		if (DemoBuildImage)
		{
			DemoBuildImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!DemoBuildImage)
	{
		UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		if (!RootCanvas)
		{
			return;
		}

		DemoBuildImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DemoBuildImage"));
		if (!DemoBuildImage)
		{
			return;
		}

		UTexture2D* DemoFishTexture = LoadObject<UTexture2D>(
			nullptr,
			TunaSweeperIntroMenuLayout::DemoFishTexturePath);
		if (!DemoFishTexture)
		{
			DemoBuildImage->RemoveFromParent();
			DemoBuildImage = nullptr;
			return;
		}

		DemoBuildImage->SetBrushFromTexture(DemoFishTexture, false);
		FSlateBrush FishBrush = DemoBuildImage->GetBrush();
		FishBrush.SetImageSize(FVector2D(63.0f, 36.0f));
		DemoBuildImage->SetBrush(FishBrush);
		if (UCanvasPanelSlot* DemoSlot = RootCanvas->AddChildToCanvas(DemoBuildImage))
		{
			DemoSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			DemoSlot->SetPosition(FVector2D(245.0f, 91.0f));
			DemoSlot->SetSize(FVector2D(63.0f, 36.0f));
			DemoSlot->SetZOrder(4);
		}
	}

	DemoBuildImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperIntroMenuWidget::EnsureTitleWindParticleOverlay()
{
	EnsureDemoBuildImage();
	if (TitleWindParticleOverlay || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto SetCanvasZOrder = [](UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	SetCanvasZOrder(FindIntroWidget(TEXT("LeftScrim")), 2);
	SetCanvasZOrder(FindIntroWidget(TEXT("LogoImage")), 3);
	SetCanvasZOrder(MainMenuPanel, 4);
	SetCanvasZOrder(FindIntroWidget(TEXT("VersionText")), 4);
	SetCanvasZOrder(SaveSlotPanel, 10);
	SetCanvasZOrder(SettingsPanel, 10);
	SetCanvasZOrder(CreditsPanel, 10);

	TitleWindParticleOverlay = WidgetTree->ConstructWidget<UTunaSweeperTitleWindParticleWidget>(
		UTunaSweeperTitleWindParticleWidget::StaticClass(),
		TEXT("TitleWindParticleOverlay"));
	if (!TitleWindParticleOverlay)
	{
		return;
	}

	TitleWindParticleOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* ParticleSlot = RootCanvas->AddChildToCanvas(TitleWindParticleOverlay);
	if (!ParticleSlot)
	{
		TitleWindParticleOverlay = nullptr;
		return;
	}

	ParticleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ParticleSlot->SetOffsets(FMargin(0.0f));
	ParticleSlot->SetAlignment(FVector2D::ZeroVector);
	ParticleSlot->SetZOrder(1);
}

