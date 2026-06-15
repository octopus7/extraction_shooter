#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/StaticMeshComponent.h"
#include "ContentBrowserModule.h"
#include "Engine/StaticMesh.h"
#include "EditorViewportClient.h"
#include "Framework/Docking/TabManager.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "Styling/CoreStyle.h"
#include "ToolMenus.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "SEditorViewport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "BushMeshBuilderEditor"

DEFINE_LOG_CATEGORY_STATIC(LogBushMeshBuilderEditor, Log, All);

namespace BushMeshBuilder
{
	const FName TabName(TEXT("BushMeshBuilder"));
	const FString DefaultOutputPath(TEXT("/Game/Environment/Bushes/Generated"));
	const FString DefaultAssetName(TEXT("SM_Bush_Generated"));

	struct FBushPart
	{
		TWeakObjectPtr<UStaticMesh> Mesh;
	};

	struct FOutputMaterialSlot
	{
		UMaterialInterface* Material = nullptr;
		FName SlotName = NAME_None;
		FPolygonGroupID PolygonGroupID;
	};

	FString SanitizeAssetName(FString Name)
	{
		Name.TrimStartAndEndInline();
		if (Name.IsEmpty())
		{
			Name = DefaultAssetName;
		}

		for (TCHAR& Character : Name)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}

		if (!FChar::IsAlpha(Name[0]) && Name[0] != TEXT('_'))
		{
			Name = TEXT("SM_") + Name;
		}

		return Name;
	}

	FString SanitizeLongPackagePath(FString Path)
	{
		Path.TrimStartAndEndInline();
		if (Path.IsEmpty())
		{
			Path = DefaultOutputPath;
		}

		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Path.Contains(TEXT("//")))
		{
			Path.ReplaceInline(TEXT("//"), TEXT("/"));
		}

		if (!Path.StartsWith(TEXT("/")))
		{
			Path = TEXT("/Game/") + Path;
		}

		if (Path.EndsWith(TEXT("/")))
		{
			Path.LeftChopInline(1);
		}

		if (!FPackageName::IsValidLongPackageName(Path, false))
		{
			Path = DefaultOutputPath;
		}

		return Path;
	}

	FVector2D ReadUV(
		const TVertexInstanceAttributesConstRef<FVector2f>& SourceUVs,
		const FVertexInstanceID VertexInstanceID,
		int32 ChannelIndex)
	{
		if (!SourceUVs.IsValid() || ChannelIndex >= SourceUVs.GetNumChannels())
		{
			return FVector2D::ZeroVector;
		}

		const FVector2f SourceUV = SourceUVs.Get(VertexInstanceID, ChannelIndex);
		return FVector2D(SourceUV);
	}

	FVector ReadVectorAttribute(
		const TVertexInstanceAttributesConstRef<FVector3f>& Attribute,
		const FVertexInstanceID VertexInstanceID,
		const FVector& Fallback)
	{
		return Attribute.IsValid() ? FVector(Attribute[VertexInstanceID]) : Fallback;
	}

	FVector4f ReadColorAttribute(
		const TVertexInstanceAttributesConstRef<FVector4f>& Attribute,
		const FVertexInstanceID VertexInstanceID)
	{
		return Attribute.IsValid() ? Attribute[VertexInstanceID] : FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	float ReadBinormalSign(
		const TVertexInstanceAttributesConstRef<float>& Attribute,
		const FVertexInstanceID VertexInstanceID)
	{
		return Attribute.IsValid() ? Attribute[VertexInstanceID] : 1.0f;
	}

	class SBushMeshPreviewViewport final : public SEditorViewport
	{
	public:
		SLATE_BEGIN_ARGS(SBushMeshPreviewViewport) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
			PreviewScene->SetFloorVisibility(true, true);
			PreviewScene->SetEnvironmentVisibility(true, true);

			PreviewMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			PreviewMeshComponent->SetMobility(EComponentMobility::Movable);
			PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PreviewScene->AddComponent(PreviewMeshComponent, FTransform::Identity);

			SEditorViewport::Construct(SEditorViewport::FArguments());
		}

		virtual ~SBushMeshPreviewViewport() override
		{
			if (ViewportClient.IsValid())
			{
				ViewportClient->Viewport = nullptr;
			}

			if (PreviewScene.IsValid() && PreviewMeshComponent)
			{
				PreviewScene->RemoveComponent(PreviewMeshComponent);
			}
			PreviewMeshComponent = nullptr;
		}

		void SetPreviewMesh(UStaticMesh* Mesh)
		{
			if (!PreviewMeshComponent)
			{
				return;
			}

			PreviewMeshComponent->SetStaticMesh(Mesh);
			PreviewMeshComponent->SetRelativeTransform(FTransform::Identity);
			PreviewMeshComponent->UpdateBounds();
			FocusPreviewMesh();
			Invalidate();
		}

		void ClearPreviewMesh()
		{
			SetPreviewMesh(nullptr);
		}

		void FocusPreviewMesh()
		{
			if (!ViewportClient.IsValid() || !PreviewMeshComponent || !PreviewMeshComponent->GetStaticMesh())
			{
				return;
			}

			PreviewMeshComponent->UpdateBounds();
			const FBox Bounds = PreviewMeshComponent->Bounds.GetBox();
			if (Bounds.IsValid)
			{
				ViewportClient->FocusViewportOnBox(Bounds);
			}
		}

		virtual bool IsVisible() const override
		{
			return ViewportWidget.IsValid() && SEditorViewport::IsVisible();
		}

	private:
		virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
		{
			ViewportClient = MakeShared<FEditorViewportClient>(nullptr, PreviewScene.Get(), SharedThis(this));
			ViewportClient->SetViewLocation(FVector(-260.0, -360.0, 220.0));
			ViewportClient->SetViewRotation(FRotator(-24.0, 42.0, 0.0));
			ViewportClient->SetViewLocationForOrbiting(FVector::ZeroVector);
			ViewportClient->SetRealtime(true);
			ViewportClient->bSetListenerPosition = false;
			ViewportClient->EngineShowFlags.EnableAdvancedFeatures();
			ViewportClient->EngineShowFlags.SetLighting(true);
			ViewportClient->EngineShowFlags.SetPostProcessing(true);
			ViewportClient->VisibilityDelegate.BindSP(this, &SBushMeshPreviewViewport::IsVisible);
			return ViewportClient.ToSharedRef();
		}

		TSharedPtr<FAdvancedPreviewScene> PreviewScene;
		TSharedPtr<FEditorViewportClient> ViewportClient;
		UStaticMeshComponent* PreviewMeshComponent = nullptr;
	};

	class SBushMeshBuilder final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SBushMeshBuilder) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SBorder)
				.Padding(12.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "Bush Mesh Builder"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Subtitle", "Compose one generated Static Mesh asset from selected Static Mesh part assets."))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SSplitter)
						+ SSplitter::Slot()
						.Value(0.34f)
						[
							MakeControlPanel()
						]
						+ SSplitter::Slot()
						.Value(0.66f)
						[
							MakePreviewPanel()
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SAssignNew(StatusText, STextBlock)
						.Text(Status)
					]
				]
			];

			RefreshPartList();
		}

	private:
		TArray<TSharedPtr<FBushPart>> Parts;
		TSharedPtr<SVerticalBox> PartListBox;
		TSharedPtr<STextBlock> StatusText;
		TSharedPtr<SBushMeshPreviewViewport> PreviewViewport;
		TStrongObjectPtr<UStaticMesh> PreviewMesh;
		FText Status = LOCTEXT("Ready", "Ready.");
		bool bPreviewDirty = true;

		FString OutputPath = DefaultOutputPath;
		FString AssetName = DefaultAssetName;
		int32 InstancesPerPart = 4;
		int32 Seed = 12345;
		float RadiusCm = 120.0f;
		float ZVarianceCm = 12.0f;
		float MinScale = 0.85f;
		float MaxScale = 1.25f;
		float PitchRollDegrees = 8.0f;

		TSharedRef<SWidget> MakeControlPanel()
		{
			return SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakePartActionRow()
					]
					+ SVerticalBox::Slot()
					.FillHeight(0.44f)
					.Padding(0.0f, 8.0f, 0.0f, 8.0f)
					[
						MakePartList()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot()
					.FillHeight(0.56f)
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							MakeSettingsPanel()
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						MakeGenerateRow()
					]
				];
		}

		TSharedRef<SWidget> MakePreviewPanel()
		{
			return SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SBushMeshBuilder::GetPreviewTitleText)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("RebuildPreview", "Rebuild Preview"))
							.IsEnabled(this, &SBushMeshBuilder::CanGenerate)
							.OnClicked(this, &SBushMeshBuilder::HandleRebuildPreview)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("FramePreview", "Frame"))
							.IsEnabled(this, &SBushMeshBuilder::HasPreviewMesh)
							.OnClicked(this, &SBushMeshBuilder::HandleFramePreview)
						]
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SAssignNew(PreviewViewport, SBushMeshPreviewViewport)
					]
				];
		}

		TSharedRef<SWidget> MakePartActionRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddSelected", "Add Selected Static Meshes"))
					.ToolTipText(LOCTEXT("AddSelectedTooltip", "Add Static Mesh assets currently selected in the Content Browser."))
					.OnClicked(this, &SBushMeshBuilder::HandleAddSelectedMeshes)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearParts", "Clear"))
					.OnClicked(this, &SBushMeshBuilder::HandleClearParts)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SBushMeshBuilder::GetPartSummaryText)
				];
		}

		TSharedRef<SWidget> MakePartList()
		{
			return SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(PartListBox, SVerticalBox)
					]
				];
		}

		TSharedRef<SWidget> MakeSettingsPanel()
		{
			return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[MakeTextSettingRow(LOCTEXT("OutputPath", "Output Path"), OutputPath)]
				+ SVerticalBox::Slot().AutoHeight()[MakeTextSettingRow(LOCTEXT("AssetName", "Asset Name"), AssetName)]
				+ SVerticalBox::Slot().AutoHeight()[MakeIntSettingRow(LOCTEXT("InstancesPerPart", "Instances Per Part"), InstancesPerPart, 1, 128)]
				+ SVerticalBox::Slot().AutoHeight()[MakeIntSettingRow(LOCTEXT("Seed", "Seed"), Seed, 1, INT32_MAX)]
				+ SVerticalBox::Slot().AutoHeight()[MakeFloatSettingRow(LOCTEXT("RadiusCm", "Radius cm"), RadiusCm, 0.0f, 1000.0f, 1.0f)]
				+ SVerticalBox::Slot().AutoHeight()[MakeFloatSettingRow(LOCTEXT("ZVariance", "Z Variance cm"), ZVarianceCm, 0.0f, 300.0f, 1.0f)]
				+ SVerticalBox::Slot().AutoHeight()[MakeFloatSettingRow(LOCTEXT("MinScale", "Min Scale"), MinScale, 0.01f, 10.0f, 0.01f)]
				+ SVerticalBox::Slot().AutoHeight()[MakeFloatSettingRow(LOCTEXT("MaxScale", "Max Scale"), MaxScale, 0.01f, 10.0f, 0.01f)]
				+ SVerticalBox::Slot().AutoHeight()[MakeFloatSettingRow(LOCTEXT("PitchRoll", "Pitch/Roll Degrees"), PitchRollDegrees, 0.0f, 45.0f, 0.25f)];
		}

		TSharedRef<SWidget> MakeTextSettingRow(const FText& Label, FString& Value)
		{
			FString* ValuePtr = &Value;
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock).Text(Label)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([ValuePtr]()
					{
						return FText::FromString(*ValuePtr);
					})
					.OnTextCommitted_Lambda([ValuePtr](const FText& NewText, ETextCommit::Type)
					{
						*ValuePtr = NewText.ToString();
					})
				];
		}

		TSharedRef<SWidget> MakeIntSettingRow(const FText& Label, int32& Value, int32 Min, int32 Max)
		{
			int32* ValuePtr = &Value;
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock).Text(Label)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SSpinBox<int32>)
					.MinValue(Min)
					.MaxValue(Max)
					.Value_Lambda([ValuePtr]() { return *ValuePtr; })
					.OnValueChanged_Lambda([this, ValuePtr, Min, Max](int32 NewValue)
					{
						*ValuePtr = FMath::Clamp(NewValue, Min, Max);
						MarkPreviewDirty();
					})
				];
		}

		TSharedRef<SWidget> MakeFloatSettingRow(const FText& Label, float& Value, float Min, float Max, float Delta)
		{
			float* ValuePtr = &Value;
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock).Text(Label)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SSpinBox<float>)
					.MinValue(Min)
					.MaxValue(Max)
					.MinSliderValue(Min)
					.MaxSliderValue(Max)
					.Delta(Delta)
					.Value_Lambda([ValuePtr]() { return *ValuePtr; })
					.OnValueChanged_Lambda([this, ValuePtr, Min, Max](float NewValue)
					{
						*ValuePtr = FMath::Clamp(NewValue, Min, Max);
						MarkPreviewDirty();
					})
				];
		}

		TSharedRef<SWidget> MakeGenerateRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Generate", "Generate Static Mesh"))
					.IsEnabled(this, &SBushMeshBuilder::CanGenerate)
					.OnClicked(this, &SBushMeshBuilder::HandleGenerate)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("NextSeed", "Next Seed"))
					.OnClicked(this, &SBushMeshBuilder::HandleNextSeed)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearPreview", "Clear Preview"))
					.IsEnabled(this, &SBushMeshBuilder::HasPreviewMesh)
					.OnClicked(this, &SBushMeshBuilder::HandleClearPreview)
				];
		}

		FText GetPartSummaryText() const
		{
			return FText::Format(LOCTEXT("PartSummary", "{0} part mesh(es)"), Parts.Num());
		}

		FText GetPreviewTitleText() const
		{
			if (!CanGenerate())
			{
				return LOCTEXT("PreviewEmptyTitle", "Preview Viewport - add Static Mesh parts");
			}

			return bPreviewDirty
				? LOCTEXT("PreviewDirtyTitle", "Preview Viewport - stale")
				: LOCTEXT("PreviewReadyTitle", "Preview Viewport");
		}

		FReply HandleAddSelectedMeshes()
		{
			TArray<FAssetData> SelectedAssets;
			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
			ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

			int32 AddedCount = 0;
			for (const FAssetData& AssetData : SelectedAssets)
			{
				UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
				if (!StaticMesh || ContainsPart(StaticMesh))
				{
					continue;
				}

				TSharedPtr<FBushPart> NewPart = MakeShared<FBushPart>();
				NewPart->Mesh = StaticMesh;
				Parts.Add(NewPart);
				++AddedCount;
			}

			RefreshPartList();
			MarkPreviewDirty();
			if (AddedCount > 0)
			{
				RebuildPreviewMesh();
			}
			SetStatus(FText::Format(LOCTEXT("PartsAdded", "Added {0} Static Mesh part(s)."), AddedCount));
			return FReply::Handled();
		}

		FReply HandleClearParts()
		{
			Parts.Reset();
			PreviewMesh.Reset();
			if (PreviewViewport.IsValid())
			{
				PreviewViewport->ClearPreviewMesh();
			}
			bPreviewDirty = true;
			RefreshPartList();
			SetStatus(LOCTEXT("PartsCleared", "Part list cleared."));
			return FReply::Handled();
		}

		FReply HandleGenerate()
		{
			FString ObjectPath;
			if (GenerateBushMesh(ObjectPath))
			{
				SetStatus(FText::Format(LOCTEXT("Generated", "Generated: {0}"), FText::FromString(ObjectPath)));
			}
			return FReply::Handled();
		}

		FReply HandleNextSeed()
		{
			Seed = FMath::Max(1, Seed + 101);
			MarkPreviewDirty();
			RebuildPreviewMesh();
			SetStatus(FText::Format(LOCTEXT("SeedChanged", "Seed changed to {0}."), Seed));
			return FReply::Handled();
		}

		FReply HandleRebuildPreview()
		{
			RebuildPreviewMesh();
			return FReply::Handled();
		}

		FReply HandleFramePreview()
		{
			if (PreviewViewport.IsValid())
			{
				PreviewViewport->FocusPreviewMesh();
			}
			return FReply::Handled();
		}

		FReply HandleClearPreview()
		{
			PreviewMesh.Reset();
			if (PreviewViewport.IsValid())
			{
				PreviewViewport->ClearPreviewMesh();
			}
			bPreviewDirty = true;
			SetStatus(LOCTEXT("PreviewCleared", "Preview cleared."));
			return FReply::Handled();
		}

		bool CanGenerate() const
		{
			for (const TSharedPtr<FBushPart>& Part : Parts)
			{
				if (Part.IsValid() && Part->Mesh.IsValid())
				{
					return true;
				}
			}
			return false;
		}

		bool HasPreviewMesh() const
		{
			return PreviewMesh.IsValid();
		}

		bool ContainsPart(UStaticMesh* Mesh) const
		{
			for (const TSharedPtr<FBushPart>& Part : Parts)
			{
				if (Part.IsValid() && Part->Mesh.Get() == Mesh)
				{
					return true;
				}
			}
			return false;
		}

		void RefreshPartList()
		{
			if (!PartListBox.IsValid())
			{
				return;
			}

			PartListBox->ClearChildren();
			if (Parts.Num() == 0)
			{
				PartListBox->AddSlot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoParts", "No part meshes. Select Static Mesh assets in the Content Browser and click Add Selected Static Meshes."))
				];
				return;
			}

			for (const TSharedPtr<FBushPart>& Part : Parts)
			{
				UStaticMesh* Mesh = Part.IsValid() ? Part->Mesh.Get() : nullptr;
				const FText MeshText = Mesh
					? FText::FromString(Mesh->GetPathName())
					: LOCTEXT("MissingMesh", "Missing mesh");

				PartListBox->AddSlot()
				.AutoHeight()
				.Padding(0.0f, 2.0f)
				[
					SNew(STextBlock).Text(MeshText)
				];
			}
		}

		void SetStatus(const FText& NewStatus)
		{
			Status = NewStatus;
			if (StatusText.IsValid())
			{
				StatusText->SetText(Status);
			}
		}

		void MarkPreviewDirty()
		{
			bPreviewDirty = true;
		}

		FName MakeOutputMaterialSlotName(UStaticMesh* SourceMesh, FPolygonGroupID SourceGroupID, const TPolygonGroupAttributesConstRef<FName>& SourceSlotNames) const
		{
			FString SlotName = SourceSlotNames.IsValid() ? SourceSlotNames[SourceGroupID].ToString() : FString();
			if (SlotName.IsEmpty() && SourceMesh && SourceMesh->GetStaticMaterials().IsValidIndex(SourceGroupID.GetValue()))
			{
				SlotName = SourceMesh->GetStaticMaterials()[SourceGroupID.GetValue()].MaterialSlotName.ToString();
			}
			if (SlotName.IsEmpty())
			{
				SlotName = TEXT("Material");
			}

			const FString MeshPrefix = SourceMesh ? SourceMesh->GetName() : TEXT("Part");
			return FName(SanitizeAssetName(MeshPrefix + TEXT("_") + SlotName));
		}

		FPolygonGroupID FindOrAddOutputMaterialSlot(
			FMeshDescriptionBuilder& Builder,
			TArray<FOutputMaterialSlot>& OutputSlots,
			UStaticMesh* SourceMesh,
			FPolygonGroupID SourceGroupID,
			const TPolygonGroupAttributesConstRef<FName>& SourceSlotNames) const
		{
			const FName OutputSlotName = MakeOutputMaterialSlotName(SourceMesh, SourceGroupID, SourceSlotNames);
			for (const FOutputMaterialSlot& ExistingSlot : OutputSlots)
			{
				if (ExistingSlot.SlotName == OutputSlotName)
				{
					return ExistingSlot.PolygonGroupID;
				}
			}

			UMaterialInterface* Material = nullptr;
			if (SourceMesh && SourceMesh->GetStaticMaterials().IsValidIndex(SourceGroupID.GetValue()))
			{
				Material = SourceMesh->GetStaticMaterials()[SourceGroupID.GetValue()].MaterialInterface;
			}
			if (!Material)
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}

			FOutputMaterialSlot& NewSlot = OutputSlots.AddDefaulted_GetRef();
			NewSlot.Material = Material;
			NewSlot.SlotName = OutputSlotName;
			NewSlot.PolygonGroupID = Builder.AppendPolygonGroup(OutputSlotName);
			return NewSlot.PolygonGroupID;
		}

		FTransform MakePartTransform(FRandomStream& RandomStream) const
		{
			const float AngleRadians = RandomStream.FRandRange(0.0f, 2.0f * UE_PI);
			const float Distance = FMath::Sqrt(RandomStream.FRand()) * FMath::Max(0.0f, RadiusCm);
			const FVector Location(
				FMath::Cos(AngleRadians) * Distance,
				FMath::Sin(AngleRadians) * Distance,
				RandomStream.FRandRange(-ZVarianceCm, ZVarianceCm));

			const FRotator Rotation(
				RandomStream.FRandRange(-PitchRollDegrees, PitchRollDegrees),
				RandomStream.FRandRange(0.0f, 360.0f),
				RandomStream.FRandRange(-PitchRollDegrees, PitchRollDegrees));

			const float LowScale = FMath::Min(MinScale, MaxScale);
			const float HighScale = FMath::Max(MinScale, MaxScale);
			const float Scale = RandomStream.FRandRange(FMath::Max(0.01f, LowScale), FMath::Max(0.01f, HighScale));

			return FTransform(Rotation, Location, FVector(Scale));
		}

		bool AppendStaticMesh(
			UStaticMesh* SourceMesh,
			const FTransform& Transform,
			FMeshDescriptionBuilder& Builder,
			TArray<FOutputMaterialSlot>& OutputSlots)
		{
			if (!SourceMesh)
			{
				return false;
			}

			const FMeshDescription* SourceDescription = SourceMesh->GetMeshDescription(0);
			if (!SourceDescription)
			{
				UE_LOG(LogBushMeshBuilderEditor, Warning, TEXT("Static Mesh has no LOD0 MeshDescription: %s"), *SourceMesh->GetPathName());
				return false;
			}

			FStaticMeshConstAttributes SourceAttributes(*SourceDescription);
			const TVertexAttributesConstRef<FVector3f> SourcePositions = SourceAttributes.GetVertexPositions();
			const TVertexInstanceAttributesConstRef<FVector2f> SourceUVs = SourceAttributes.GetVertexInstanceUVs();
			const TVertexInstanceAttributesConstRef<FVector3f> SourceNormals = SourceAttributes.GetVertexInstanceNormals();
			const TVertexInstanceAttributesConstRef<FVector3f> SourceTangents = SourceAttributes.GetVertexInstanceTangents();
			const TVertexInstanceAttributesConstRef<float> SourceBinormalSigns = SourceAttributes.GetVertexInstanceBinormalSigns();
			const TVertexInstanceAttributesConstRef<FVector4f> SourceColors = SourceAttributes.GetVertexInstanceColors();
			const TPolygonGroupAttributesConstRef<FName> SourceSlotNames = SourceAttributes.GetPolygonGroupMaterialSlotNames();

			const int32 NumUVLayers = FMath::Clamp(SourceUVs.IsValid() ? SourceUVs.GetNumChannels() : 1, 1, 8);

			for (const FPolygonID PolygonID : SourceDescription->Polygons().GetElementIDs())
			{
				const FPolygonGroupID SourceGroupID = SourceDescription->GetPolygonPolygonGroup(PolygonID);
				const FPolygonGroupID OutputGroupID = FindOrAddOutputMaterialSlot(Builder, OutputSlots, SourceMesh, SourceGroupID, SourceSlotNames);
				const TArrayView<const FTriangleID> TriangleIDs = SourceDescription->GetPolygonTriangles(PolygonID);

				for (const FTriangleID TriangleID : TriangleIDs)
				{
					const TArrayView<const FVertexInstanceID> SourceInstances = SourceDescription->GetTriangleVertexInstances(TriangleID);
					if (SourceInstances.Num() != 3)
					{
						continue;
					}

					FVertexInstanceID OutputInstances[3];
					for (int32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
					{
						const FVertexInstanceID SourceInstanceID = SourceInstances[CornerIndex];
						const FVertexID SourceVertexID = SourceDescription->GetVertexInstanceVertex(SourceInstanceID);
						const FVector LocalPosition(SourcePositions[SourceVertexID]);
						const FVertexID OutputVertexID = Builder.AppendVertex(Transform.TransformPosition(LocalPosition));
						const FVertexInstanceID OutputInstanceID = Builder.AppendInstance(OutputVertexID);

						const FVector Normal = Transform.TransformVectorNoScale(
							ReadVectorAttribute(SourceNormals, SourceInstanceID, FVector::UpVector)).GetSafeNormal();
						const FVector Tangent = Transform.TransformVectorNoScale(
							ReadVectorAttribute(SourceTangents, SourceInstanceID, FVector::ForwardVector)).GetSafeNormal();
						Builder.SetInstanceTangentSpace(
							OutputInstanceID,
							Normal,
							Tangent,
							ReadBinormalSign(SourceBinormalSigns, SourceInstanceID));
						Builder.SetInstanceColor(OutputInstanceID, ReadColorAttribute(SourceColors, SourceInstanceID));

						for (int32 UVLayerIndex = 0; UVLayerIndex < NumUVLayers; ++UVLayerIndex)
						{
							Builder.SetInstanceUV(OutputInstanceID, ReadUV(SourceUVs, SourceInstanceID, UVLayerIndex), UVLayerIndex);
						}

						OutputInstances[CornerIndex] = OutputInstanceID;
					}

					Builder.AppendTriangle(OutputInstances[0], OutputInstances[1], OutputInstances[2], OutputGroupID);
				}
			}

			return true;
		}

		bool BuildCombinedMeshDescription(
			FMeshDescription& MeshDescription,
			TArray<FOutputMaterialSlot>& OutputSlots,
			int32& OutAppendedMeshCount)
		{
			OutputSlots.Reset();
			OutAppendedMeshCount = 0;

			FStaticMeshAttributes OutputAttributes(MeshDescription);
			OutputAttributes.Register();

			FMeshDescriptionBuilder Builder;
			Builder.SetMeshDescription(&MeshDescription);
			Builder.SetNumUVLayers(8);

			FRandomStream RandomStream(Seed);
			for (const TSharedPtr<FBushPart>& Part : Parts)
			{
				UStaticMesh* SourceMesh = Part.IsValid() ? Part->Mesh.Get() : nullptr;
				if (!SourceMesh)
				{
					continue;
				}

				for (int32 InstanceIndex = 0; InstanceIndex < InstancesPerPart; ++InstanceIndex)
				{
					if (AppendStaticMesh(SourceMesh, MakePartTransform(RandomStream), Builder, OutputSlots))
					{
						++OutAppendedMeshCount;
					}
				}
			}

			return OutAppendedMeshCount > 0 && MeshDescription.Triangles().Num() > 0;
		}

		bool ApplyMeshDescriptionToStaticMesh(
			UStaticMesh* StaticMesh,
			FMeshDescription& MeshDescription,
			const TArray<FOutputMaterialSlot>& OutputSlots,
			bool bBuildSimpleCollision,
			bool bMarkPackageDirty)
		{
			if (!StaticMesh)
			{
				return false;
			}

			StaticMesh->GetStaticMaterials().Reset();
			for (const FOutputMaterialSlot& Slot : OutputSlots)
			{
				StaticMesh->GetStaticMaterials().Add(FStaticMaterial(Slot.Material, Slot.SlotName));
			}

			UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
			BuildParams.bBuildSimpleCollision = bBuildSimpleCollision;
			BuildParams.bCommitMeshDescription = true;
			BuildParams.bMarkPackageDirty = bMarkPackageDirty;
			BuildParams.bAllowCpuAccess = true;

			const TArray<const FMeshDescription*> MeshDescriptions = { &MeshDescription };
			if (!StaticMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams))
			{
				return false;
			}

			StaticMesh->PostEditChange();
			return true;
		}

		bool RebuildPreviewMesh()
		{
			FMeshDescription MeshDescription;
			TArray<FOutputMaterialSlot> OutputSlots;
			int32 AppendedMeshCount = 0;
			if (!BuildCombinedMeshDescription(MeshDescription, OutputSlots, AppendedMeshCount))
			{
				PreviewMesh.Reset();
				if (PreviewViewport.IsValid())
				{
					PreviewViewport->ClearPreviewMesh();
				}
				SetStatus(LOCTEXT("NoPreviewGeometry", "No preview geometry was generated. Check that the part meshes have LOD0 geometry."));
				return false;
			}

			const FName PreviewName = MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), TEXT("BushMeshPreview"));
			UStaticMesh* NewPreviewMesh = NewObject<UStaticMesh>(GetTransientPackage(), PreviewName, RF_Transient);
			if (!ApplyMeshDescriptionToStaticMesh(NewPreviewMesh, MeshDescription, OutputSlots, false, false))
			{
				SetStatus(LOCTEXT("PreviewBuildFailed", "Preview Static Mesh build failed."));
				return false;
			}

			PreviewMesh.Reset(NewPreviewMesh);
			if (PreviewViewport.IsValid())
			{
				PreviewViewport->SetPreviewMesh(PreviewMesh.Get());
			}

			bPreviewDirty = false;
			SetStatus(FText::Format(
				LOCTEXT("PreviewBuilt", "Preview rebuilt from {0} placed part mesh instance(s)."),
				AppendedMeshCount));
			return true;
		}

		bool GenerateBushMesh(FString& OutObjectPath)
		{
			FMeshDescription MeshDescription;
			TArray<FOutputMaterialSlot> OutputSlots;
			int32 AppendedMeshCount = 0;
			if (!BuildCombinedMeshDescription(MeshDescription, OutputSlots, AppendedMeshCount))
			{
				SetStatus(LOCTEXT("NoGeometry", "No geometry was generated. Check that the part meshes have LOD0 geometry."));
				return false;
			}

			const FString CleanOutputPath = SanitizeLongPackagePath(OutputPath);
			const FString CleanAssetName = SanitizeAssetName(AssetName);
			const FString RequestedPackageName = CleanOutputPath / CleanAssetName;

			FString UniquePackageName;
			FString UniqueAssetName;
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			AssetToolsModule.Get().CreateUniqueAssetName(RequestedPackageName, TEXT(""), UniquePackageName, UniqueAssetName);

			UPackage* Package = CreatePackage(*UniquePackageName);
			if (!Package)
			{
				SetStatus(LOCTEXT("PackageFailed", "Failed to create output package."));
				return false;
			}

			UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, *UniqueAssetName, RF_Public | RF_Standalone);
			if (!StaticMesh)
			{
				SetStatus(LOCTEXT("MeshCreateFailed", "Failed to create Static Mesh object."));
				return false;
			}

			if (!ApplyMeshDescriptionToStaticMesh(StaticMesh, MeshDescription, OutputSlots, true, true))
			{
				SetStatus(LOCTEXT("BuildFailed", "Static Mesh build failed."));
				return false;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
			Package->MarkPackageDirty();

			const FString PackageFileName = FPackageName::LongPackageNameToFilename(UniquePackageName, FPackageName::GetAssetPackageExtension());
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;
			if (!UPackage::SavePackage(Package, StaticMesh, *PackageFileName, SaveArgs))
			{
				SetStatus(FText::Format(LOCTEXT("SaveFailed", "Generated mesh but failed to save: {0}"), FText::FromString(PackageFileName)));
				return false;
			}

			OutObjectPath = UniquePackageName + TEXT(".") + UniqueAssetName;
			OutputPath = CleanOutputPath;
			AssetName = CleanAssetName;
			PreviewMesh.Reset(StaticMesh);
			if (PreviewViewport.IsValid())
			{
				PreviewViewport->SetPreviewMesh(PreviewMesh.Get());
			}
			bPreviewDirty = false;
			return true;
		}
	};
}

class FBushMeshBuilderEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			BushMeshBuilder::TabName,
			FOnSpawnTab::CreateRaw(this, &FBushMeshBuilderEditorModule::SpawnToolTab))
			.SetDisplayName(LOCTEXT("TabTitle", "Bush Mesh Builder"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Compose generated bush Static Mesh assets from selected part meshes."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBushMeshBuilderEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BushMeshBuilder::TabName);
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"),
			NAME_None,
			EMultiBoxType::Menu,
			false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
			TEXT("AssetTools"),
			LOCTEXT("TunaSweeperAssetToolsMenuSection", "Asset Tools"));
		Section.AddMenuEntry(
			TEXT("OpenBushMeshBuilder"),
			LOCTEXT("MenuEntry", "Bush Mesh Builder"),
			LOCTEXT("MenuEntryTooltip", "Open the dockable Bush Mesh Builder tool."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FBushMeshBuilderEditorModule::OpenToolWindow)));
	}

	void OpenToolWindow()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(BushMeshBuilder::TabName);
	}

	TSharedRef<SDockTab> SpawnToolTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(BushMeshBuilder::SBushMeshBuilder)
			];
	}
};

IMPLEMENT_MODULE(FBushMeshBuilderEditorModule, BushMeshBuilderEditor)

#undef LOCTEXT_NAMESPACE
