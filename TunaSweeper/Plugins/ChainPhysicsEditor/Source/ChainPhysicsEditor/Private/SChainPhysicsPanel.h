#pragma once

#include "ChainPhysicsSetup.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SVerticalBox;
class USkeletalMesh;

class SChainPhysicsPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChainPhysicsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void HandleMeshChanged(const FAssetData& AssetData);
	FReply Analyze();
	FReply AddManualChain();
	FReply RunSetup(ChainPhysicsSetup::ESetupMode Mode);
	FReply OpenPhysicsAsset();
	FReply OpenSelectedAnimBlueprint();
	void RebuildCandidateWidgets();
	void SetStatus(const FString& Message, bool bError = false);
	FString GetSelectedMeshPath() const;
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	bool HasAnalysis() const;

	TWeakObjectPtr<USkeletalMesh> SelectedMesh;
	ChainPhysicsSetup::FAnalysisResult Analysis;
	ChainPhysicsSetup::EPreset Preset = ChainPhysicsSetup::EPreset::Hair;
	float RadiusScale = 1.0f;
	FString StatusMessage = TEXT("Select a Skeletal Mesh, then analyze it.");
	bool bStatusError = false;
	TSharedPtr<SVerticalBox> ChainRows;
	TSharedPtr<SVerticalBox> AnimBlueprintRows;
	TSharedPtr<SEditableTextBox> ManualRootTextBox;
	TSharedPtr<SEditableTextBox> OutputPathTextBox;
	TArray<TSharedPtr<FString>> PresetItems;
	TSharedPtr<FString> SelectedPresetItem;
};
