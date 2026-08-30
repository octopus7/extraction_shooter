#include "SChainPhysicsPanel.h"

#include "Animation/AnimBlueprint.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/MessageDialog.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PropertyCustomizationHelpers.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SChainPhysicsPanel"

namespace
{
	FText PresetHelp(const ChainPhysicsSetup::EPreset Preset)
	{
		switch (Preset)
		{
		case ChainPhysicsSetup::EPreset::Accessory:
			return LOCTEXT("AccessoryHelp", "Accessory: firmer constraints and slightly larger bodies.");
		case ChainPhysicsSetup::EPreset::Cloth:
			return LOCTEXT("ClothHelp", "Cloth: smaller bodies with wider, softer angular motion.");
		default:
			return LOCTEXT("HairHelp", "Hair: balanced secondary-motion defaults.");
		}
	}
}

void SChainPhysicsPanel::Construct(const FArguments& InArgs)
{
	PresetItems = {
		MakeShared<FString>(TEXT("Hair")),
		MakeShared<FString>(TEXT("Accessory")),
		MakeShared<FString>(TEXT("Cloth"))
	};
	SelectedPresetItem = PresetItems[0];

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "Chain Physics Setup"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "Analyze a Skeletal Mesh, choose secondary-motion chains and compatible Anim Blueprints, then create or repair a dedicated Physics Asset and Rigid Body graph setup."))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(STextBlock).Text(LOCTEXT("MeshLabel", "Skeletal Mesh"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(USkeletalMesh::StaticClass())
						.ObjectPath(this, &SChainPhysicsPanel::GetSelectedMeshPath)
						.OnObjectChanged(this, &SChainPhysicsPanel::HandleMeshChanged)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("Analyze", "Analyze"))
						.OnClicked(this, &SChainPhysicsPanel::Analyze)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Chains", "Chain candidates"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(ChainRows, SVerticalBox)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SAssignNew(ManualRootTextBox, SEditableTextBox)
						.HintText(LOCTEXT("ManualRootHint", "Manual chain root bone"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("AddManual", "Add root"))
						.IsEnabled(this, &SChainPhysicsPanel::HasAnalysis)
						.OnClicked(this, &SChainPhysicsPanel::AddManualChain)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AnimBlueprints", "Compatible Anim Blueprints"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(AnimBlueprintRows, SVerticalBox)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 4)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("Preset", "Preset"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 20, 0)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&PresetItems)
						.InitiallySelectedItem(SelectedPresetItem)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
						{
							return SNew(STextBlock).Text(FText::FromString(*Item));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
						{
							SelectedPresetItem = Item;
							Preset = Item == PresetItems[1] ? ChainPhysicsSetup::EPreset::Accessory
								: Item == PresetItems[2] ? ChainPhysicsSetup::EPreset::Cloth
								: ChainPhysicsSetup::EPreset::Hair;
						})
						[
							SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(*SelectedPresetItem); })
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("RadiusScale", "Body radius scale"))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8, 0, 0, 0)
					[
						SNew(SNumericEntryBox<float>)
						.MinValue(0.5f).MaxValue(3.0f)
						.MinSliderValue(0.5f).MaxSliderValue(2.0f)
						.Value_Lambda([this]() -> TOptional<float> { return RadiusScale; })
						.OnValueChanged_Lambda([this](float Value) { RadiusScale = Value; })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 6)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return PresetHelp(Preset); })
					.ColorAndOpacity(FLinearColor(0.65f, 0.65f, 0.65f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
				[
					SNew(STextBlock).Text(LOCTEXT("OutputPath", "Physics Asset object path (optional)"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(OutputPathTextBox, SEditableTextBox)
					.HintText(LOCTEXT("OutputPathHint", "/Game/.../PA_Name.PA_Name"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
				[
					SNew(SUniformGridPanel).SlotPadding(FMargin(3))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton).Text(LOCTEXT("Repair", "Create / Repair"))
						.IsEnabled(this, &SChainPhysicsPanel::HasAnalysis)
						.OnClicked_Lambda([this]() { return RunSetup(ChainPhysicsSetup::ESetupMode::Repair); })
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton).Text(LOCTEXT("Regenerate", "Regenerate selected"))
						.IsEnabled(this, &SChainPhysicsPanel::HasAnalysis)
						.OnClicked_Lambda([this]() { return RunSetup(ChainPhysicsSetup::ESetupMode::Regenerate); })
					]
					+ SUniformGridPanel::Slot(0, 1)
					[
						SNew(SButton).Text(LOCTEXT("OpenPA", "Open detected PA"))
						.OnClicked(this, &SChainPhysicsPanel::OpenPhysicsAsset)
					]
					+ SUniformGridPanel::Slot(1, 1)
					[
						SNew(SButton).Text(LOCTEXT("OpenABP", "Open selected AnimBP"))
						.OnClicked(this, &SChainPhysicsPanel::OpenSelectedAnimBlueprint)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
				[
					SNew(STextBlock)
					.Text(this, &SChainPhysicsPanel::GetStatusText)
					.ColorAndOpacity(this, &SChainPhysicsPanel::GetStatusColor)
					.AutoWrapText(true)
				]
			]
		]
	];

	RebuildCandidateWidgets();
}

void SChainPhysicsPanel::HandleMeshChanged(const FAssetData& AssetData)
{
	SelectedMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
	Analysis = ChainPhysicsSetup::FAnalysisResult();
	if (OutputPathTextBox.IsValid())
	{
		OutputPathTextBox->SetText(FText::GetEmpty());
	}
	RebuildCandidateWidgets();
	Analyze();
}

FReply SChainPhysicsPanel::Analyze()
{
	if (!ChainPhysicsSetup::AnalyzeSkeletalMesh(SelectedMesh.Get(), Analysis))
	{
		SetStatus(Analysis.Summary, true);
	}
	else
	{
		SetStatus(Analysis.Summary);
	}
	RebuildCandidateWidgets();
	return FReply::Handled();
}

FReply SChainPhysicsPanel::AddManualChain()
{
	const FName Root(*ManualRootTextBox->GetText().ToString().TrimStartAndEnd());
	FString Error;
	if (Root.IsNone() || !ChainPhysicsSetup::AddManualChain(Analysis, Root, Error))
	{
		SetStatus(Root.IsNone() ? TEXT("Enter a root bone name.") : Error, true);
	}
	else
	{
		ManualRootTextBox->SetText(FText::GetEmpty());
		SetStatus(FString::Printf(TEXT("Added manual chain rooted at %s."), *Root.ToString()));
		RebuildCandidateWidgets();
	}
	return FReply::Handled();
}

FReply SChainPhysicsPanel::RunSetup(const ChainPhysicsSetup::ESetupMode Mode)
{
	if (Mode == ChainPhysicsSetup::ESetupMode::Regenerate
		&& FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("RegenerateConfirm", "Regenerate replaces bodies and constraints for the selected chains. Continue?")) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}
	ChainPhysicsSetup::FSetupOptions Options;
	Options.Preset = Preset;
	Options.Mode = Mode;
	Options.RadiusScale = RadiusScale;
	Options.OutputPhysicsAssetObjectPath = OutputPathTextBox->GetText().ToString().TrimStartAndEnd();
	const ChainPhysicsSetup::FSetupResult Result = ChainPhysicsSetup::SetupSelectedChains(Analysis, Options);
	SetStatus(Result.Message, !Result.bSucceeded);
	RebuildCandidateWidgets();
	return FReply::Handled();
}

FReply SChainPhysicsPanel::OpenPhysicsAsset()
{
	for (const ChainPhysicsSetup::FChainCandidate& Chain : Analysis.Chains)
	{
		if (Chain.bSelected && Chain.DetectedPhysicsAsset.IsValid())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Chain.DetectedPhysicsAsset.Get());
			return FReply::Handled();
		}
	}
	SetStatus(TEXT("No detected Physics Asset is associated with a selected chain."), true);
	return FReply::Handled();
}

FReply SChainPhysicsPanel::OpenSelectedAnimBlueprint()
{
	for (const ChainPhysicsSetup::FAnimBlueprintCandidate& Candidate : Analysis.AnimBlueprints)
	{
		if (Candidate.bSelected && Candidate.AnimBlueprint.IsValid())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Candidate.AnimBlueprint.Get());
			return FReply::Handled();
		}
	}
	SetStatus(TEXT("Select an Anim Blueprint first."), true);
	return FReply::Handled();
}

void SChainPhysicsPanel::RebuildCandidateWidgets()
{
	if (!ChainRows.IsValid() || !AnimBlueprintRows.IsValid())
	{
		return;
	}
	ChainRows->ClearChildren();
	AnimBlueprintRows->ClearChildren();
	if (Analysis.Chains.Num() == 0)
	{
		ChainRows->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoChains", "No candidates. Use a manual root for non-standard names."))];
	}
	for (int32 Index = 0; Index < Analysis.Chains.Num(); ++Index)
	{
		ChainPhysicsSetup::FChainCandidate& Chain = Analysis.Chains[Index];
		const FString Details = FString::Printf(TEXT("%s -> %s  |  %d bones  |  %.1f cm  |  %s"),
			*Chain.RootBone.ToString(), *Chain.EndBone.ToString(), Chain.BoneNames.Num(), Chain.TotalLength, *Chain.DetectionReason);
		ChainRows->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this, Index]() { return Analysis.Chains.IsValidIndex(Index) && Analysis.Chains[Index].bSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState State) { if (Analysis.Chains.IsValidIndex(Index)) Analysis.Chains[Index].bSelected = State == ECheckBoxState::Checked; })
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6, 0)
			[
				SNew(STextBlock).Text(FText::FromString(Details))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(ChainPhysicsSetup::GetSetupStateText(Chain.SetupState))
				.ColorAndOpacity(ChainPhysicsSetup::GetSetupStateColor(Chain.SetupState))
			]
		];
	}
	if (Analysis.AnimBlueprints.Num() == 0)
	{
		AnimBlueprintRows->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NoABP", "No compatible Anim Blueprint found for this Skeleton."))];
	}
	for (int32 Index = 0; Index < Analysis.AnimBlueprints.Num(); ++Index)
	{
		ChainPhysicsSetup::FAnimBlueprintCandidate& Candidate = Analysis.AnimBlueprints[Index];
		const FString Path = Candidate.AnimBlueprint.IsValid() ? Candidate.AnimBlueprint->GetPathName() : TEXT("Invalid asset");
		const FString Setup = Candidate.bHasConnectedRigidBody
			? FString::Printf(TEXT("Rigid Body -> %s"), Candidate.DetectedPhysicsAsset.IsValid() ? *Candidate.DetectedPhysicsAsset->GetName() : TEXT("mesh default"))
			: TEXT("No connected Rigid Body");
		AnimBlueprintRows->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this, Index]() { return Analysis.AnimBlueprints.IsValidIndex(Index) && Analysis.AnimBlueprints[Index].bSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState State) { if (Analysis.AnimBlueprints.IsValidIndex(Index)) Analysis.AnimBlueprints[Index].bSelected = State == ECheckBoxState::Checked; })
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6, 0)
			[
				SNew(STextBlock).Text(FText::FromString(Path))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Text(FText::FromString(Setup)).ColorAndOpacity(Candidate.bHasConnectedRigidBody ? FLinearColor(0.2f, 0.85f, 0.35f) : FLinearColor(0.65f, 0.65f, 0.65f))
			]
		];
	}
}

void SChainPhysicsPanel::SetStatus(const FString& Message, const bool bError)
{
	StatusMessage = Message;
	bStatusError = bError;
}

FString SChainPhysicsPanel::GetSelectedMeshPath() const
{
	return SelectedMesh.IsValid() ? SelectedMesh->GetPathName() : FString();
}

FText SChainPhysicsPanel::GetStatusText() const
{
	return FText::FromString(StatusMessage);
}

FSlateColor SChainPhysicsPanel::GetStatusColor() const
{
	return bStatusError ? FLinearColor(1.0f, 0.25f, 0.2f) : FLinearColor(0.35f, 0.8f, 1.0f);
}

bool SChainPhysicsPanel::HasAnalysis() const
{
	return SelectedMesh.IsValid() && Analysis.SkeletalMesh.IsValid();
}

#undef LOCTEXT_NAMESPACE
