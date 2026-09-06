#include "TunaSweeperEditorSetupShared.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "Blueprint/UserWidget.h"

namespace TunaSweeperEditorSetup
{
namespace TitleScreens
{
	FSlateBrush TextureBrush(UTexture2D* Texture, FLinearColor Tint = FLinearColor::White)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = Tint;
		Brush.ImageSize = FVector2D(360.0f, 64.0f);
		return Brush;
	}
	void StyleButtons(UWidgetTree* Tree, UTexture2D* Frame)
	{
		Tree->ForEachWidget([Frame](UWidget* Widget)
		{
			UButton* Button = Cast<UButton>(Widget);
			if (!Button) return;
			const FString Name = Button->GetName();
			const bool bChoice = Name.StartsWith(TEXT("Preset")) || Name.StartsWith(TEXT("Resolution")) || Name.StartsWith(TEXT("DLSS")) || Name.StartsWith(TEXT("FrameRate")) || Name.Contains(TEXT("ModeButton")) || Name.Contains(TEXT("Language"));
			FButtonStyle Style = Button->GetStyle();
			if (bChoice)
			{
				FSlateBrush Clear;
				Clear.DrawAs = ESlateBrushDrawType::NoDrawType;
				Style.SetNormal(Clear);
				Style.SetDisabled(Clear);
				FSlateBrush Hover = Clear;
				Hover.DrawAs = ESlateBrushDrawType::Box;
				Hover.TintColor = FLinearColor(0.13f, 0.22f, 0.20f, 0.4f);
				Style.SetHovered(Hover);
				Style.SetPressed(Hover);
			}
			else
			{
				Style.SetNormal(TextureBrush(Frame, FLinearColor(0.78f, 0.82f, 0.73f, 0.85f)));
				Style.SetHovered(TextureBrush(Frame));
				Style.SetPressed(TextureBrush(Frame, FLinearColor(0.6f, 0.75f, 0.65f, 1.0f)));
				Style.SetDisabled(TextureBrush(Frame, FLinearColor(0.5f, 0.6f, 0.55f, 0.5f)));
			}
			Button->SetStyle(Style);
			Button->SetBackgroundColor(FLinearColor::White);
		});
	}
	bool SaveWidget(UWidgetBlueprint* BP)
	{
		TSet<FName> ConnectedNames;
		BP->WidgetTree->ForEachWidget([&](UWidget* Widget) {
			ConnectedNames.Add(Widget->GetFName());
			RegisterWidgetVariable(BP, Widget);
		});
		for (UWidgetAnimation* Animation : BP->Animations)
			if (Animation) ConnectedNames.Add(Animation->GetFName());
		// Preserve every live widget/animation GUID; remove only stale detached entries.
		for (auto It = BP->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
			if (!ConnectedNames.Contains(It.Key())) It.RemoveCurrent();
		FKismetEditorUtilities::CompileBlueprint(BP);
		if (BP->Status == BS_Error) return false;
		BP->MarkPackageDirty();
		return SaveAsset(BP);
	}
	// Extract a copy of the subtree, preserving its existing parent slot in the shell.
	bool Extract(UWidgetBlueprint* Shell, const TCHAR* RootName, const TCHAR* AssetName)
	{
		UWidgetTree* Tree = Shell->WidgetTree;
		const FString ViewName = FString(RootName) + TEXT("View");
		if (Tree->FindWidget(FName(*ViewName))) return true;
		UWidget* Original = Tree->FindWidget(RootName);
		if (!Original || !Original->GetParent()) return false;
		UPanelWidget* Parent = Original->GetParent();
		const int32 Index = Parent->GetChildIndex(Original);
		const ESlateVisibility Visibility = Original->GetVisibility();
		UWidgetBlueprint* Child = EnsureWidgetBlueprint(TEXT("/Game/UI/Title/Screens"), AssetName, UUserWidget::StaticClass());
		if (!Child) return false;
		Child->Modify();
		// Duplicate the tree in one pass to remap all slot/content object references.
		Child->WidgetTree->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		Child->WidgetTree = DuplicateObject<UWidgetTree>(Tree, Child, TEXT("WidgetTree"));
		UWidget* Root = Child->WidgetTree->FindWidget(RootName);
		if (!Root) return false;
		Root->RemoveFromParent();
		Root->Slot = nullptr;
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Child->WidgetTree->RootWidget = Root;
		if (!SaveWidget(Child)) return false;
		UUserWidget* View = Tree->ConstructWidget<UUserWidget>(Child->GeneratedClass.Get(), FName(*ViewName));
		View->SetVisibility(Visibility);
		if (!Parent->ReplaceChildAt(Index, View)) return false;
		Original->Slot = nullptr;
		UE_LOG(LogTunaSweeperEditor, Display, TEXT("Title screen extracted: %s -> %s"), RootName, *Child->GetPathName());
		return true;
	}
}

bool EnsureTitleScreenAssetsSetup()
{
	using namespace TitleScreens;
	UWidgetBlueprint* Intro = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu"));
	UTexture2D* Botanical = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Title/T_SettingsLeafSimple.T_SettingsLeafSimple"));
	UTexture2D* Frame = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Title/T_ButtonLeafFrame.T_ButtonLeafFrame"));
	if (!Intro || !Botanical || !Frame) return false;
	UWidgetTree* Tree = Intro->WidgetTree;
	if (Tree->FindWidget(TEXT("SettingsPanelView")))
	{
		for (const TCHAR* Asset : { TEXT("WBP_TitleGraphics"), TEXT("WBP_TitleMain"), TEXT("WBP_TitleSaveSlots"), TEXT("WBP_TitleSettings"), TEXT("WBP_TitleCredits"), TEXT("WBP_TitleDemoNotice") })
		{
			UWidgetBlueprint* Child = LoadObject<UWidgetBlueprint>(nullptr, *FString::Printf(TEXT("/Game/UI/Title/Screens/%s.%s"), Asset, Asset));
			if (!Child || !SaveWidget(Child)) return false;
		}
		return SaveWidget(Intro);
	}
	Intro->Modify(); Tree->Modify();
	UCanvasPanel* Settings = Cast<UCanvasPanel>(Tree->FindWidget(TEXT("SettingsPanel")));
	if (!Settings) return false;
	UWidget* Title = Tree->FindWidget(TEXT("SettingsTitleText"));
	UWidget* Page = Tree->FindWidget(TEXT("SettingsPageStack"));
	UWidget* Tabs = Tree->FindWidget(TEXT("SettingsNavigationStack"));
	UWidget* Back = Tree->FindWidget(TEXT("BackFromSettingsButtonBox"));
	if (!Title || !Page || !Tabs || !Back) return false;
	// Keep references before disconnecting the previous card hierarchy.
	Title->RemoveFromParent(); Page->RemoveFromParent(); Tabs->RemoveFromParent(); Back->RemoveFromParent();
	Settings->ClearChildren();
	if (UWidget* OldLabel = Tree->FindWidget(TEXT("SettingsNavigationLabelText"))) OldLabel->RemoveFromParent();
	if (UPanelWidget* TabStack = Cast<UPanelWidget>(Tabs))
	{
		for (int32 Index = TabStack->GetChildrenCount() - 1; Index >= 0; --Index)
			if (TabStack->GetChildAt(Index)->GetFName() == TEXT("SettingsNavigationLabelText")) TabStack->RemoveChildAt(Index);
	}
	auto Place = [Settings](UWidget* Widget, FAnchors Anchors, FMargin Offsets, int32 Z)
	{
		UCanvasPanelSlot* Slot = Settings->AddChildToCanvas(Widget);
		Slot->SetAnchors(Anchors); Slot->SetOffsets(Offsets); Slot->SetZOrder(Z);
		return Slot;
	};
	UImage* Dim = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsDimImage"));
	Dim->SetColorAndOpacity(FLinearColor(0.005f, 0.020f, 0.023f, 0.97f));
	// This visible full-screen image intercepts clicks while the menu fades over the title.
	Place(Dim, FAnchors(0, 0, 1, 1), FMargin(0), 0);
	UImage* Leaves = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsBotanicalImage"));
	Leaves->SetBrushFromTexture(Botanical);
	Leaves->SetVisibility(ESlateVisibility::HitTestInvisible);
	Leaves->SetRenderOpacity(0.48f);
	Place(Leaves, FAnchors(0, 0, 1, 1), FMargin(0), 1);
	Place(Title, FAnchors(0, 0), FMargin(110, 72, 600, 65), 2);
	Place(Tabs, FAnchors(0, 0, 0, 1), FMargin(110, 225, 300, 155), 2);
	Place(Page, FAnchors(0, 0, 1, 1), FMargin(470, 150, 180, 105), 2);
	Place(Back, FAnchors(0, 1), FMargin(110, -115, 230, 58), 2);
	if (USizeBox* BackBox = Cast<USizeBox>(Back)) { BackBox->SetWidthOverride(230); BackBox->SetHeightOverride(58); }
	for (const TCHAR* Name : { TEXT("GraphicsTabButtonBox"), TEXT("InterfaceTabButtonBox"), TEXT("DevelopmentTabButtonBox") })
	{
		if (USizeBox* Box = Cast<USizeBox>(Tree->FindWidget(Name)))
		{
			Box->SetWidthOverride(280); Box->SetHeightOverride(64);
			if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Box->Slot)) Slot->SetPadding(FMargin(0, 0, 0, 24));
		}
	}
	StyleButtons(Tree, Frame);
	for (const TCHAR* Name : { TEXT("SettingsLanguageSection"), TEXT("EnemyCombatDebugSection"), TEXT("DebugDisplayLanguageSection") })
		if (UBorder* Border = Cast<UBorder>(Tree->FindWidget(Name))) { Border->SetBrushColor(FLinearColor::Transparent); Border->SetPadding(FMargin(0, 12)); }

	// Bake the existing graphics controls into an editable WBP instead of creating it on entry.
	UWidgetBlueprint* Graphics = EnsureWidgetBlueprint(TEXT("/Game/UI/Title/Screens"), TEXT("WBP_TitleGraphics"), UTunaSweeperGraphicsSettingsWidget::StaticClass());
	if (!Graphics) return false;
	UTunaSweeperGraphicsSettingsWidget* Template = NewObject<UTunaSweeperGraphicsSettingsWidget>(GetTransientPackage());
	Template->BuildEditorTemplate();
	Graphics->WidgetTree->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
	Graphics->WidgetTree = DuplicateObject<UWidgetTree>(Template->WidgetTree, Graphics, TEXT("WidgetTree"));
	StyleButtons(Graphics->WidgetTree, Frame);
	if (!SaveWidget(Graphics)) return false;
	UVerticalBox* GraphicsPanel = Cast<UVerticalBox>(Tree->FindWidget(TEXT("GraphicsSettingsPanel")));
	if (!GraphicsPanel) return false;
	GraphicsPanel->ClearChildren();
	UUserWidget* GraphicsView = Tree->ConstructWidget<UUserWidget>(Graphics->GeneratedClass.Get(), TEXT("TitleGraphicsSettingsWidget"));
	GraphicsPanel->AddChildToVerticalBox(GraphicsView)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	if (!Extract(Intro, TEXT("MainMenuPanel"), TEXT("WBP_TitleMain")) ||
		!Extract(Intro, TEXT("SaveSlotPanel"), TEXT("WBP_TitleSaveSlots")) ||
		!Extract(Intro, TEXT("SettingsPanel"), TEXT("WBP_TitleSettings")) ||
		!Extract(Intro, TEXT("CreditsPanel"), TEXT("WBP_TitleCredits")) ||
		!Extract(Intro, TEXT("DemoNoticePanel"), TEXT("WBP_TitleDemoNotice"))) return false;
	return SaveWidget(Intro);
}
}
