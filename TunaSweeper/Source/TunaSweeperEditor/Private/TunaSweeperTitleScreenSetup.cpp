#include "TunaSweeperEditorSetupShared.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "Blueprint/UserWidget.h"

namespace TunaSweeperEditorSetup
{
namespace TitleScreens
{
	bool SimplifySettingsTabs()
	{
		UWidgetBlueprint* Settings = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleSettings.WBP_TitleSettings"));
		UWidgetBlueprint* Graphics = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleGraphics.WBP_TitleGraphics"));
		if (!Settings || !Graphics) return false;
		UPackage* Package = CreatePackage(TEXT("/Game/UI/Title/M_SettingsTabMist"));
		UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Game/UI/Title/M_SettingsTabMist.M_SettingsTabMist"));
		if (!Material)
		{
			Material = NewObject<UMaterial>(Package, TEXT("M_SettingsTabMist"), RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Material);
		}
		if (!Material) return false;
		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_UI;
		Material->BlendMode = BLEND_Translucent;
		auto* UV = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Material->GetExpressionCollection().AddExpression(UV);
		auto* Mask = NewObject<UMaterialExpressionCustom>(Material);
		Mask->OutputType = CMOT_Float1;
		Mask->Description = TEXT("Soft horizontal fade with broad rising wisps; no hard frame");
		Mask->Code = TEXT("float x=saturate(UV.x), y=saturate(UV.y); float edge=pow(saturate(sin(x*3.14159265)),1.6); float vertical=pow(saturate(sin(y*3.14159265)),1.1); float wisp=0.78+0.12*sin(x*19+y*7+sin(y*8)*1.3)+0.10*sin(x*31-y*5); return saturate(edge*vertical*wisp)*0.26;");
		FCustomInput Input; Input.InputName = TEXT("UV"); Input.Input.Connect(0, UV); Mask->Inputs.Add(Input);
		Material->GetExpressionCollection().AddExpression(Mask);
		auto* Color = NewObject<UMaterialExpressionConstant3Vector>(Material);
		Color->Constant = FLinearColor(0.40f, 0.62f, 0.53f);
		Material->GetExpressionCollection().AddExpression(Color);
		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Color);
		Material->GetEditorOnlyData()->Opacity.Connect(0, Mask);
		Material->PostEditChange(); Material->MarkPackageDirty();
		if (!SaveAsset(Material)) return false;
		FSlateBrush Clear; Clear.DrawAs = ESlateBrushDrawType::NoDrawType;
		FSlateBrush Mist; Mist.SetResourceObject(Material); Mist.DrawAs = ESlateBrushDrawType::Image;
		Mist.ImageSize = FVector2D(300, 64);
		for (const TCHAR* Name : { TEXT("SettingsGraphicsTabButton"), TEXT("SettingsInterfaceTabButton"), TEXT("SettingsDevelopmentTabButton") })
		{
			UButton* Button = Cast<UButton>(Settings->WidgetTree->FindWidget(Name));
			if (!Button) return false;
			FButtonStyle Style = Button->GetStyle();
			Style.SetNormal(Clear); Style.SetHovered(Mist); Style.SetPressed(Mist); Style.SetDisabled(Mist);
			Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White);
			if (UTextBlock* Text = Cast<UTextBlock>(Button->GetContent()))
			{
				FSlateFontInfo Font = Text->GetFont(); Font.Size = 30; Text->SetFont(Font);
				Text->SetColorAndOpacity(FLinearColor(0.88f, 0.92f, 0.88f));
			}
		}
		for (const TCHAR* Name : { TEXT("VSyncToggleButton"), TEXT("MotionBlurToggleButton"), TEXT("DynamicResolutionToggleButton"), TEXT("HardwareRayTracingToggleButton") })
		{
			UButton* Button = Cast<UButton>(Graphics->WidgetTree->FindWidget(Name));
			if (!Button) return false;
			FButtonStyle Style = Button->GetStyle();
			Style.SetNormal(Clear); Style.SetHovered(Clear); Style.SetPressed(Clear); Style.SetDisabled(Clear);
			Button->SetStyle(Style);
		}
		for (UWidgetBlueprint* BP : { Settings, Graphics })
		{
			FKismetEditorUtilities::CompileBlueprint(BP);
			if (BP->Status == BS_Error) return false;
			BP->MarkPackageDirty(); if (!SaveAsset(BP)) return false;
		}
		return true;
	}
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
		return SimplifySettingsTabs();
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
