// One-off generator: remove after committing the generated WBP together with this file.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperGenerateVehicleHud,
	"TunaSweeper.Generate.VehicleHUD", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperGenerateVehicleHud::RunTest(const FString& Parameters)
{
	auto* Reference = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker"));
	if (!TestNotNull(TEXT("Reference interaction WBP"), Reference)) return false;
	auto* ReferenceText = Cast<UTextBlock>(Reference->WidgetTree->FindWidget(TEXT("DisplayNameText")));
	if (!TestNotNull(TEXT("Reference label font"), ReferenceText)) return false;
	// Resolve designer preview text through the same project string keys as runtime.
	auto* PreviewInstance = NewObject<UGameInstance>();
	auto* Strings = NewObject<UTunaSweeperTextSubsystem>(PreviewInstance);
	const FString PackageName(TEXT("/Game/UI/Vehicle/WBP_ATV_HUD"));
	if (FPackageName::DoesPackageExist(PackageName)) { AddError(TEXT("Refusing to overwrite an existing WBP")); return false; }
	auto* Package = CreatePackage(*PackageName);
	auto* BP = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
		UTunaSweeperVehicleDismountWidget::StaticClass(), Package, TEXT("WBP_ATV_HUD"), BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	auto* Tree = BP->WidgetTree.Get();
	auto* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VehicleHUDRoot"));
	Tree->RootWidget = Root;
	auto* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DurabilityFrame"));
	Frame->SetBrush(FSlateColorBrush(FLinearColor(0.008f, 0.008f, 0.008f, 1)));
	Frame->SetPadding(FMargin(1));
	auto* FrameSlot = Root->AddChildToCanvas(Frame);
	FrameSlot->SetPosition(FVector2D::ZeroVector);
	FrameSlot->SetSize(FVector2D(180, 14));
	auto* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("DurabilityBar"));
	Bar->bIsVariable = true;
	FProgressBarStyle Style;
	Style.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.035f,0.035f,0.035f,1)));
	Style.SetFillImage(FSlateColorBrush(FLinearColor(0.12f,0.82f,0.38f,1)));
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(FLinearColor::White);
	Bar->SetBorderPadding(FVector2D::ZeroVector);
	Bar->SetPercent(1);
	Frame->SetContent(Bar);
	auto* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DismountPanel"));
	Panel->bIsVariable = true;
	Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 5.0f));
	Panel->SetPadding(FMargin(7,1,6,1));
	auto* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetPosition(FVector2D(0,50));
	PanelSlot->SetAutoSize(true);
	auto* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DismountRow"));
	Panel->SetContent(Row);
	auto* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismountText"));
	Label->bIsVariable = true;
	Label->SetFont(ReferenceText->GetFont());
	Label->SetText(Strings->ResolveText(TEXT("ui.vehicle.dismount"), ETunaSweeperItemTextLanguage::Korean, FText::GetEmpty()));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	Label->SetShadowOffset(FVector2D::ZeroVector);
	Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
	auto* Key = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DismountKeycap"));
	Key->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 5.0f, FLinearColor::Black, 1.0f));
	Key->SetPadding(FMargin(5,0));
	auto* KeyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismountKeyText"));
	KeyText->bIsVariable = true;
	auto Font = ReferenceText->GetFont();
	Font.Size = FMath::Max(1.0f, Font.Size-2.0f);
	KeyText->SetFont(Font);
	KeyText->SetText(Strings->ResolveText(TEXT("ui.key.x"), ETunaSweeperItemTextLanguage::Korean, FText::GetEmpty()));
	KeyText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	KeyText->SetJustification(ETextJustify::Center);
	Key->SetContent(KeyText);
	auto* KeySlot = Row->AddChildToHorizontalBox(Key);
	KeySlot->SetPadding(FMargin(8,0,0,0));
	KeySlot->SetVerticalAlignment(VAlign_Center);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	if (!TestTrue(TEXT("WBP compiled"), BP->Status != BS_Error)) return false;
	FAssetRegistryModule::AssetCreated(BP);
	Package->MarkPackageDirty();
	FSavePackageArgs Save;
	Save.TopLevelFlags = RF_Public | RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	TestTrue(TEXT("Saved editable WBP"), UPackage::SavePackage(Package, BP, *Filename, Save));
	AddInfo(FString::Printf(TEXT("Interaction label font %.1f, key font %.1f"), ReferenceText->GetFont().Size, Font.Size));
	return true;
}
#endif
