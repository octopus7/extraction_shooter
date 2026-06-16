#include "TunaSweeperMapCaptureActorDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "IDetailCustomization.h"
#include "Map/TunaSweeperMapCaptureActor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TunaSweeperMapCaptureActorDetails"

namespace
{
	const FName MapCaptureActorClassName(TEXT("TunaSweeperMapCaptureActor"));

	enum class EMapCaptureDetailAction : uint8
	{
		AutoDetectBounds,
		CaptureRgbPng,
		AutoDetectAndCapture
	};

	FText GetActionTransactionText(EMapCaptureDetailAction Action)
	{
		switch (Action)
		{
		case EMapCaptureDetailAction::AutoDetectBounds:
			return LOCTEXT("AutoDetectBoundsTransaction", "Auto Detect Map Capture Bounds");
		case EMapCaptureDetailAction::CaptureRgbPng:
			return LOCTEXT("CaptureRgbPngTransaction", "Capture Map RGB PNG");
		case EMapCaptureDetailAction::AutoDetectAndCapture:
			return LOCTEXT("AutoDetectAndCaptureTransaction", "Auto Detect and Capture Map RGB PNG");
		default:
			return LOCTEXT("MapCaptureActionTransaction", "Run Map Capture Action");
		}
	}

	void ExecuteAction(
		const TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>>& Actors,
		EMapCaptureDetailAction Action)
	{
		bool bHasValidActor = false;
		for (const TWeakObjectPtr<ATunaSweeperMapCaptureActor>& ActorPtr : Actors)
		{
			if (ActorPtr.IsValid())
			{
				bHasValidActor = true;
				break;
			}
		}

		if (!bHasValidActor)
		{
			return;
		}

		const FScopedTransaction Transaction(GetActionTransactionText(Action));
		for (const TWeakObjectPtr<ATunaSweeperMapCaptureActor>& ActorPtr : Actors)
		{
			ATunaSweeperMapCaptureActor* Actor = ActorPtr.Get();
			if (!Actor)
			{
				continue;
			}

			Actor->Modify();
			switch (Action)
			{
			case EMapCaptureDetailAction::AutoDetectBounds:
				Actor->AutoDetectCaptureBounds();
				break;
			case EMapCaptureDetailAction::CaptureRgbPng:
				Actor->CaptureOpaqueRgbPng();
				break;
			case EMapCaptureDetailAction::AutoDetectAndCapture:
				Actor->AutoDetectBoundsAndCaptureOpaqueRgbPng();
				break;
			default:
				break;
			}
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports(true);
		}
	}

	TSharedRef<SWidget> MakeActionButton(
		TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>> Actors,
		EMapCaptureDetailAction Action,
		const FText& ButtonText,
		const FText& TooltipText)
	{
		return SNew(SButton)
			.Text(ButtonText)
			.ToolTipText(TooltipText)
			.OnClicked_Lambda(
				[Actors = MoveTemp(Actors), Action]()
				{
					ExecuteAction(Actors, Action);
					return FReply::Handled();
				});
	}

	class FTunaSweeperMapCaptureActorDetails final : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FTunaSweeperMapCaptureActorDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>> Actors;
			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder.GetObjectsBeingCustomized(Objects);

			for (const TWeakObjectPtr<UObject>& ObjectPtr : Objects)
			{
				if (ATunaSweeperMapCaptureActor* Actor = Cast<ATunaSweeperMapCaptureActor>(ObjectPtr.Get()))
				{
					Actors.Add(Actor);
				}
			}

			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
				TEXT("Map Capture"),
				LOCTEXT("MapCaptureCategory", "Map Capture"),
				ECategoryPriority::Important);

			Category.AddCustomRow(LOCTEXT("MapCaptureActionsFilter", "Map Capture Actions Auto Detect Capture RGB PNG"))
				.WholeRowContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(LOCTEXT("MapCaptureActionsHelp", "Generate the editor-only map image for this level."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::AutoDetectBounds,
								LOCTEXT("AutoDetectBoundsButton", "Auto Detect Bounds"),
								LOCTEXT("AutoDetectBoundsTooltip", "Detect capture bounds from level geometry using downward traces."))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::CaptureRgbPng,
								LOCTEXT("CaptureRgbPngButton", "Capture RGB PNG"),
								LOCTEXT("CaptureRgbPngTooltip", "Capture the current bounds to an opaque RGB PNG."))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::AutoDetectAndCapture,
								LOCTEXT("AutoDetectAndCaptureButton", "Auto Detect + Capture"),
								LOCTEXT("AutoDetectAndCaptureTooltip", "Detect bounds, then immediately capture the opaque RGB PNG."))
						]
					]
				];
		}
	};
}

namespace TunaSweeperMapCaptureActorDetails
{
	void Register()
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomClassLayout(
			MapCaptureActorClassName,
			FOnGetDetailCustomizationInstance::CreateStatic(&FTunaSweeperMapCaptureActorDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	void Unregister()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.UnregisterCustomClassLayout(MapCaptureActorClassName);
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
