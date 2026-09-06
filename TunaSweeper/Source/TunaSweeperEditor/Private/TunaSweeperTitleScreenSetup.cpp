#include "TunaSweeperEditorSetupShared.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "Blueprint/UserWidget.h"

namespace TunaSweeperEditorSetup
{
namespace TitleScreens
{
	bool SaveWidget(UWidgetBlueprint* BP);
	bool RefineSettingsAlignment()
	{
		UWidgetBlueprint* Settings = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleSettings.WBP_TitleSettings"));
		UWidgetBlueprint* Graphics = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleGraphics.WBP_TitleGraphics"));
		if (!Settings || !Graphics) return false;
		UWidgetTree* Tree = Settings->WidgetTree;
		if (Tree->FindWidget(TEXT("SettingsSidebarShade")))
		{
			if (UWidget* Root=Graphics->WidgetTree->FindWidget(TEXT("GraphicsSettingsRoot")))
				if (UCanvasPanelSlot* Slot=Cast<UCanvasPanelSlot>(Root->Slot)) Slot->SetOffsets(FMargin(480,80,40,40));
			for (const TCHAR* Name : { TEXT("ApplyGraphicsSettingsButton"), TEXT("CancelGraphicsSettingsButton") })
				if (UButton* Button=Cast<UButton>(Graphics->WidgetTree->FindWidget(Name))) { FButtonStyle Style=Button->GetStyle(); Style.NormalPadding=FMargin(0); Style.PressedPadding=FMargin(0); Button->SetStyle(Style); }
			return SaveWidget(Graphics);
		}
		auto LeftAlign = [](UButton* Button) {
			if (!Button) return;
			if (UWidget* Content = Button->GetContent()) {
				if (UButtonSlot* Slot = Cast<UButtonSlot>(Content->Slot)) { Slot->SetHorizontalAlignment(HAlign_Left); Slot->SetPadding(FMargin(0)); }
				if (UTextBlock* Text = Cast<UTextBlock>(Content)) Text->SetJustification(ETextJustify::Left);
			}
		};
		for (const TCHAR* Name : { TEXT("SettingsGraphicsTabButton"), TEXT("SettingsInterfaceTabButton"), TEXT("SettingsDevelopmentTabButton") })
			LeftAlign(Cast<UButton>(Tree->FindWidget(Name)));
		if (UWidget* Tabs = Tree->FindWidget(TEXT("SettingsNavigationStack")))
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Tabs->Slot)) Slot->SetOffsets(FMargin(110, 225, 280, 220));
		if (UImage* Leaves = Cast<UImage>(Tree->FindWidget(TEXT("SettingsBotanicalImage"))))
		{
			Leaves->SetRenderOpacity(.22f);
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Leaves->Slot)) { Slot->SetAnchors(FAnchors(1,1)); Slot->SetOffsets(FMargin(-560,-380,560,380)); }
		}
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(Tree->RootWidget);
		if (!Canvas) return false;
		UPackage* Package = CreatePackage(TEXT("/Game/UI/Title/T_SettingsSidebarFade"));
		UTexture2D* Fade = NewObject<UTexture2D>(Package, TEXT("T_SettingsSidebarFade"), RF_Public | RF_Standalone);
		TArray<FColor> Pixels; Pixels.SetNum(128);
		for (int32 X=0; X<128; ++X) { float T=FMath::Clamp((X/127.f-.65f)/.35f,0.f,1.f); Pixels[X]=FColor(0,0,0,FMath::RoundToInt(255*.18f*(1-T*T*(3-2*T)))); }
		Fade->Source.Init(128,1,1,1,TSF_BGRA8,reinterpret_cast<const uint8*>(Pixels.GetData()));
		Fade->CompressionSettings=TC_EditorIcon; Fade->MipGenSettings=TMGS_NoMipmaps; Fade->LODGroup=TEXTUREGROUP_UI;
		Fade->PostEditChange(); FAssetRegistryModule::AssetCreated(Fade); Fade->MarkPackageDirty(); if (!SaveAsset(Fade)) return false;
		UImage* Shade = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsSidebarShade"));
		Shade->SetBrushFromTexture(Fade); Shade->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* ShadeSlot=Canvas->AddChildToCanvas(Shade); ShadeSlot->SetAnchors(FAnchors(0,0,0,1)); ShadeSlot->SetOffsets(FMargin(0,0,440,0)); ShadeSlot->SetZOrder(1);
		UWidgetTree* GTree=Graphics->WidgetTree;
		UVerticalBox* Root=Cast<UVerticalBox>(GTree->FindWidget(TEXT("GraphicsSettingsRoot")));
		if (!Root) return false;
		if (UCanvasPanelSlot* Slot=Cast<UCanvasPanelSlot>(Root->Slot)) Slot->SetOffsets(FMargin(480,80,40,40));
		UTextBlock* Header=GTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("GraphicsSectionTitleText"));
		Header->SetText(FText::FromString(TEXT("그래픽"))); FSlateFontInfo HeaderFont=Header->GetFont(); HeaderFont.Size=30; Header->SetFont(HeaderFont);
		Root->InsertChildAt(0,Header);
		if (UVerticalBoxSlot* Slot=Cast<UVerticalBoxSlot>(Header->Slot)) Slot->SetPadding(FMargin(0,0,0,12));
		if (UWidget* Status=GTree->FindWidget(TEXT("GraphicsStatusText")))
			if (UVerticalBoxSlot* Slot=Cast<UVerticalBoxSlot>(Status->Slot)) Slot->SetPadding(FMargin(0,0,0,28));
		GTree->ForEachWidget([](UWidget* Widget) {
			const FString Name=Widget->GetName();
			if (UTextBlock* Text=Cast<UTextBlock>(Widget)) {
				FSlateFontInfo Font=Text->GetFont(); Font.Size=Name==TEXT("GraphicsSectionTitleText") ? 30 : (Name==TEXT("GraphicsStatusText") ? 16 : (Name.EndsWith(TEXT("Heading")) ? 22 : 18)); Text->SetFont(Font);
				if (Name.EndsWith(TEXT("Heading"))) if (UVerticalBoxSlot* Slot=Cast<UVerticalBoxSlot>(Text->Slot)) Slot->SetPadding(FMargin(0,24,0,10));
			}
			if (USizeBox* Box=Cast<USizeBox>(Widget)) {
				Box->SetHeightOverride(44);
				if (Box->GetWidthOverride()>0) Box->SetWidthOverride(Box->GetWidthOverride()*1.20f);
			}
		});
		for (const TCHAR* Name : { TEXT("ApplyGraphicsSettingsButton"), TEXT("CancelGraphicsSettingsButton") }) {
			UButton* Button=Cast<UButton>(GTree->FindWidget(Name)); if (!Button) return false; LeftAlign(Button);
			FSlateBrush Clear; Clear.DrawAs=ESlateBrushDrawType::NoDrawType;
			FButtonStyle Style=Button->GetStyle(); Style.SetNormal(Clear); Style.SetHovered(Clear); Style.SetPressed(Clear); Style.SetDisabled(Clear);
			Style.SetNormalForeground(FLinearColor(.8f,.88f,.85f,.9f)); Style.SetHoveredForeground(FLinearColor::White); Style.SetPressedForeground(FLinearColor(.65f,.85f,.75f));
			Style.NormalPadding=FMargin(0); Style.PressedPadding=FMargin(0);
			Button->SetStyle(Style); if (UTextBlock* Text=Cast<UTextBlock>(Button->GetContent())) { Text->SetColorAndOpacity(FSlateColor::UseForeground()); FSlateFontInfo Font=Text->GetFont(); Font.Size=24; Text->SetFont(Font); }
		}
		for (const TCHAR* Name : { TEXT("InterfaceSettingsPanel"), TEXT("DevelopmentSettingsPanel") })
			if (UWidget* Panel=Tree->FindWidget(Name)) if (UVerticalBoxSlot* Slot=Cast<UVerticalBoxSlot>(Panel->Slot)) Slot->SetPadding(FMargin(480,72,40,40));
		return SaveWidget(Graphics) && SaveWidget(Settings);
	}
	bool SetupSettingsBackHeader()
	{
		UWidgetBlueprint* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleSettings.WBP_TitleSettings"));
		if (!BP) return false;
		UWidgetTree* Tree = BP->WidgetTree;
		UButton* Button = Cast<UButton>(Tree->FindWidget(TEXT("BackFromSettingsButton")));
		USizeBox* Box = Cast<USizeBox>(Tree->FindWidget(TEXT("BackFromSettingsButtonBox")));
		UTextBlock* Title = Cast<UTextBlock>(Tree->FindWidget(TEXT("SettingsTitleText")));
		if (!Button || !Box || !Title) return false;
		UCanvasPanelSlot* Placement = Cast<UCanvasPanelSlot>(Box->Slot);
		if (!Placement) return false;
		UPackage* Package = CreatePackage(TEXT("/Game/UI/Title/M_SettingsBackArrow"));
		UMaterial* Material = FindObject<UMaterial>(Package, TEXT("M_SettingsBackArrow"));
		if (!Material)
		{
			Material = NewObject<UMaterial>(Package, TEXT("M_SettingsBackArrow"), RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Material);
		}
		Material->MaterialDomain = MD_UI;
		Material->BlendMode = BLEND_Translucent;
		Material->GetExpressionCollection().Empty();
		auto* UV = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		Material->GetExpressionCollection().AddExpression(UV);
		auto* Shape = NewObject<UMaterialExpressionCustom>(Material);
		Shape->OutputType = CMOT_Float1;
		Shape->Description = TEXT("Antialiased curved return arrow, resolution independent");
		Shape->Code = TEXT("float d=10; float2 a=float2(.78,.78); for(int i=1;i<=24;i++){float t=i/24.0; float2 b=(1-t)*(1-t)*float2(.78,.78)+2*(1-t)*t*float2(.88,.26)+t*t*float2(.23,.30); float2 e=b-a; d=min(d,length(UV-a-e*saturate(dot(UV-a,e)/dot(e,e)))); a=b;} float2 p=float2(.23,.30); float2 e=float2(.17,-.17); d=min(d,length(UV-p-e*saturate(dot(UV-p,e)/dot(e,e)))); e=float2(.17,.17); d=min(d,length(UV-p-e*saturate(dot(UV-p,e)/dot(e,e)))); float aa=max(fwidth(d),.001); return 1-smoothstep(.026-aa,.026+aa,d);");
		FCustomInput Input; Input.InputName = TEXT("UV"); Input.Input.Connect(0, UV); Shape->Inputs.Add(Input);
		Material->GetExpressionCollection().AddExpression(Shape);
		auto* Color = NewObject<UMaterialExpressionConstant3Vector>(Material);
		Color->Constant = FLinearColor::White;
		Material->GetExpressionCollection().AddExpression(Color);
		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Color);
		Material->GetEditorOnlyData()->Opacity.Connect(0, Shape);
		Material->PostEditChange(); Material->MarkPackageDirty();
		if (!SaveAsset(Material)) return false;
		// Store the simple vector shape as a UI texture so it also renders during shader warm-up.
		UPackage* IconPackage = CreatePackage(TEXT("/Game/UI/Title/T_SettingsBackArrow"));
		UTexture2D* Icon = NewObject<UTexture2D>(IconPackage, TEXT("T_SettingsBackArrow"), RF_Public | RF_Standalone);
		TArray<FColor> Pixels; Pixels.SetNum(128 * 128);
		for (int32 Y = 0; Y < 128; ++Y) for (int32 X = 0; X < 128; ++X)
		{
			const FVector2D P((X + 0.5f) / 128, (Y + 0.5f) / 128);
			float Distance = 10;
			auto Segment = [&](FVector2D A, FVector2D B) {
				FVector2D E = B - A;
				Distance = FMath::Min(Distance, static_cast<float>((P - A - E * FMath::Clamp(FVector2D::DotProduct(P - A, E) / E.SizeSquared(), 0.0, 1.0)).Size()));
			};
			FVector2D A(.78, .78);
			for (int32 I = 1; I <= 64; ++I) { double T = I / 64.0; FVector2D B = (1-T)*(1-T)*FVector2D(.78,.78) + 2*(1-T)*T*FVector2D(.88,.26) + T*T*FVector2D(.23,.30); Segment(A, B); A = B; }
			Segment(FVector2D(.23,.30), FVector2D(.40,.13)); Segment(FVector2D(.23,.30), FVector2D(.40,.47));
			Pixels[Y*128+X] = FColor(255,255,255, FMath::RoundToInt(255 * FMath::Clamp((.030f-Distance)/.008f, 0.0f, 1.0f)));
		}
		Icon->Source.Init(128,128,1,1,TSF_BGRA8,reinterpret_cast<const uint8*>(Pixels.GetData()));
		Icon->CompressionSettings = TC_EditorIcon; Icon->MipGenSettings = TMGS_NoMipmaps; Icon->LODGroup = TEXTUREGROUP_UI;
		Icon->PostEditChange(); FAssetRegistryModule::AssetCreated(Icon); Icon->MarkPackageDirty();
		if (!SaveAsset(Icon)) return false;
		Title->RemoveFromParent();
		UHorizontalBox* Row = Cast<UHorizontalBox>(Tree->FindWidget(TEXT("SettingsBackHeaderRow")));
		UImage* Arrow = Cast<UImage>(Tree->FindWidget(TEXT("SettingsBackArrowImage")));
		if (!Row) Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsBackHeaderRow"));
		if (!Arrow) Arrow = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsBackArrowImage"));
		Row->ClearChildren();
		FSlateBrush Brush; Brush.SetResourceObject(Icon); Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(48, 48); Brush.TintColor = FSlateColor::UseForeground(); Arrow->SetBrush(Brush);
		Arrow->SetVisibility(ESlateVisibility::HitTestInvisible);
		Row->AddChildToHorizontalBox(Arrow)->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Title);
		LabelSlot->SetPadding(FMargin(12, 0, 0, 0)); LabelSlot->SetVerticalAlignment(VAlign_Center);
		Title->SetColorAndOpacity(FSlateColor::UseForeground()); Title->SetRenderOpacity(1);
		Title->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->SetContent(Row);
		if (UButtonSlot* Slot = Cast<UButtonSlot>(Row->Slot)) { Slot->SetPadding(FMargin(0)); Slot->SetHorizontalAlignment(HAlign_Left); Slot->SetVerticalAlignment(VAlign_Center); }
		FSlateBrush Clear; Clear.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(Clear); Style.SetHovered(Clear); Style.SetPressed(Clear); Style.SetDisabled(Clear);
		Style.SetNormalForeground(FLinearColor(0.90f, 0.95f, 0.93f, 0.90f));
		Style.SetHoveredForeground(FLinearColor(1.0f, 1.0f, 0.94f, 1.0f));
		Style.SetPressedForeground(FLinearColor(0.70f, 0.88f, 0.79f, 1.0f));
		Style.NormalPadding = FMargin(0); Style.PressedPadding = FMargin(0);
		Button->SetStyle(Style); Button->SetBackgroundColor(FLinearColor::White);
		Box->SetWidthOverride(260); Box->SetHeightOverride(65);
		Placement->SetAnchors(FAnchors(0, 0)); Placement->SetAlignment(FVector2D::ZeroVector);
		Placement->SetOffsets(FMargin(50, 72, 260, 65));
		UWidgetBlueprint* Graphics = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/Title/Screens/WBP_TitleGraphics.WBP_TitleGraphics"));
		if (!Graphics) return false;
		UWidget* Page = Tree->FindWidget(TEXT("SettingsPageStack"));
		UCanvasPanelSlot* PageSlot = Page ? Cast<UCanvasPanelSlot>(Page->Slot) : nullptr;
		if (!PageSlot) return false;
		Page->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PageSlot->SetAnchors(FAnchors(0, 0, 1, 1)); PageSlot->SetOffsets(FMargin(0)); PageSlot->SetZOrder(1);
		for (const TCHAR* Name : { TEXT("InterfaceSettingsPanel"), TEXT("DevelopmentSettingsPanel") })
			if (UWidget* Panel = Tree->FindWidget(Name))
				if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Panel->Slot)) Slot->SetPadding(FMargin(440, 72, 32, 32));
		UWidgetTree* GTree = Graphics->WidgetTree;
		if (!GTree->FindWidget(TEXT("GraphicsFullscreenCanvas")))
		{
			UWidget* OldRoot = GTree->RootWidget;
			UWidget* Apply = GTree->FindWidget(TEXT("ApplyGraphicsSettingsButtonBox"));
			UWidget* Cancel = GTree->FindWidget(TEXT("CancelGraphicsSettingsButtonBox"));
			UWidget* OldActions = GTree->FindWidget(TEXT("GraphicsActionRow"));
			if (!Apply || !Cancel || !OldActions) return false;
			Apply->RemoveFromParent(); Cancel->RemoveFromParent(); OldActions->RemoveFromParent();
			UCanvasPanel* Canvas = GTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("GraphicsFullscreenCanvas"));
			Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			GTree->RootWidget = Canvas;
			UCanvasPanelSlot* ContentSlot = Canvas->AddChildToCanvas(OldRoot);
			ContentSlot->SetAnchors(FAnchors(0, 0, 1, 1)); ContentSlot->SetOffsets(FMargin(440, 72, 32, 32));
			UVerticalBox* Actions = GTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsActionColumn"));
			for (UWidget* Action : { Apply, Cancel })
			{
				if (USizeBox* Size = Cast<USizeBox>(Action)) { Size->SetWidthOverride(280); Size->SetHeightOverride(58); }
				Actions->AddChildToVerticalBox(Action)->SetPadding(FMargin(0, 0, 0, 12));
			}
			UCanvasPanelSlot* ActionsSlot = Canvas->AddChildToCanvas(Actions);
			ActionsSlot->SetAnchors(FAnchors(0, 1)); ActionsSlot->SetOffsets(FMargin(110, -190, 280, 140));
		}
		return SaveWidget(Graphics) && SaveWidget(BP);
	}
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
		// Compilation can repopulate detached source-object entries; keep only authored tree names.
		for (auto It = BP->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
			if (!ConnectedNames.Contains(It.Key())) It.RemoveCurrent();
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
		return RefineSettingsAlignment();
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
