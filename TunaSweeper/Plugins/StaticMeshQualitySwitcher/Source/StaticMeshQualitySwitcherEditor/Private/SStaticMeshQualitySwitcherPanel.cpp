#include "SStaticMeshQualitySwitcherPanel.h"

#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "StaticMeshQualityProfile.h"
#include "StaticMeshQualitySwitcherSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "StaticMeshQualitySwitcherPanel"

namespace
{
	FText ScopeToText(const EStaticMeshQualityScope Scope)
	{
		return Scope == EStaticMeshQualityScope::SelectedActors
			? LOCTEXT("SelectedActorsScope", "Selected Actors")
			: LOCTEXT("LoadedLevelScope", "All Loaded Actors in Current Level");
	}
}

void SStaticMeshQualitySwitcherPanel::Construct(const FArguments& InArgs)
{
	ScopeOptions.Add(MakeShared<EStaticMeshQualityScope>(EStaticMeshQualityScope::SelectedActors));
	ScopeOptions.Add(MakeShared<EStaticMeshQualityScope>(EStaticMeshQualityScope::LoadedLevel));
	CurrentScope = ScopeOptions[0];

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
				.Text(LOCTEXT("Title", "Static Mesh Quality Switcher"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 6.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT(
					"Description",
					"Switches placed Static Mesh Components between strictly one-to-one Original and Low assets. The entire operation is rejected when the profile is invalid."))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 3.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ProfileLabel", "Profile"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UStaticMeshQualityProfile::StaticClass())
					.ObjectPath(this, &SStaticMeshQualitySwitcherPanel::GetProfileObjectPath)
					.OnObjectChanged(this, &SStaticMeshQualitySwitcherPanel::HandleProfileChanged)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 3.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ScopeLabel", "Scope"))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SComboBox<TSharedPtr<EStaticMeshQualityScope>>)
					.OptionsSource(&ScopeOptions)
					.InitiallySelectedItem(CurrentScope)
					.OnGenerateWidget(this, &SStaticMeshQualitySwitcherPanel::GenerateScopeWidget)
					.OnSelectionChanged(this, &SStaticMeshQualitySwitcherPanel::HandleScopeChanged)
					[
						SNew(STextBlock)
						.Text(this, &SStaticMeshQualitySwitcherPanel::GetCurrentScopeText)
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f)
			[
				SNew(SSeparator)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ValidateButton", "Validate Profile"))
					.OnClicked(this, &SStaticMeshQualitySwitcherPanel::HandleValidateClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OriginalButton", "Apply Original"))
					.OnClicked(this, &SStaticMeshQualitySwitcherPanel::HandleApplyClicked, EStaticMeshQualityTarget::Original)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("LowButton", "Apply Low"))
					.OnClicked(this, &SStaticMeshQualitySwitcherPanel::HandleApplyClicked, EStaticMeshQualityTarget::Low)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SAssignNew(StatusText, STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT("InitialStatus", "Select or create a Static Mesh Quality Profile."))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
		]
	];
}

FString SStaticMeshQualitySwitcherPanel::GetProfileObjectPath() const
{
	return GetDefault<UStaticMeshQualitySwitcherSettings>()->ActiveProfile.ToSoftObjectPath().ToString();
}

void SStaticMeshQualitySwitcherPanel::HandleProfileChanged(const FAssetData& AssetData)
{
	UStaticMeshQualitySwitcherSettings* Settings = GetMutableDefault<UStaticMeshQualitySwitcherSettings>();
	Settings->ActiveProfile = TSoftObjectPtr<UStaticMeshQualityProfile>(AssetData.ToSoftObjectPath());
	Settings->SaveConfig();
	SetStatus(LOCTEXT("ProfileChanged", "Profile selected. Validate it before applying a quality level."), false);
}

TSharedRef<SWidget> SStaticMeshQualitySwitcherPanel::GenerateScopeWidget(
	const TSharedPtr<EStaticMeshQualityScope> Scope) const
{
	return SNew(STextBlock).Text(Scope.IsValid() ? ScopeToText(*Scope) : FText::GetEmpty());
}

void SStaticMeshQualitySwitcherPanel::HandleScopeChanged(
	const TSharedPtr<EStaticMeshQualityScope> Scope,
	ESelectInfo::Type SelectInfo)
{
	if (Scope.IsValid())
	{
		CurrentScope = Scope;
	}
}

FText SStaticMeshQualitySwitcherPanel::GetCurrentScopeText() const
{
	return CurrentScope.IsValid() ? ScopeToText(*CurrentScope) : FText::GetEmpty();
}

FReply SStaticMeshQualitySwitcherPanel::HandleValidateClicked()
{
	UStaticMeshQualityProfile* Profile = LoadProfile();
	if (!Profile)
	{
		SetStatus(LOCTEXT("ProfileLoadFailed", "No profile is selected, or the selected profile could not be loaded."), true);
		return FReply::Handled();
	}

	TArray<FText> Errors;
	if (!Profile->ValidateProfile(Errors))
	{
		SetStatusFromErrors(Errors);
		return FReply::Handled();
	}

	SetStatus(FText::Format(
		LOCTEXT("ProfileValid", "Profile is valid: {0} strictly unique mesh pairs."),
		FText::AsNumber(Profile->MeshPairs.Num())), false);
	return FReply::Handled();
}

FReply SStaticMeshQualitySwitcherPanel::HandleApplyClicked(const EStaticMeshQualityTarget Target)
{
	if (!CurrentScope.IsValid())
	{
		SetStatus(LOCTEXT("MissingScope", "Select an application scope."), true);
		return FReply::Handled();
	}

	const FStaticMeshQualityApplyResult Result = FStaticMeshQualitySwitcherService::Apply(
		LoadProfile(), Target, *CurrentScope);
	if (!Result.bSucceeded)
	{
		SetStatusFromErrors(Result.Errors);
		return FReply::Handled();
	}

	SetStatus(FText::Format(
		LOCTEXT(
			"ApplySucceeded",
			"Completed. Actors: {0}, components inspected: {1}, components changed: {2}, unmapped components: {3}. Save the level when the result is ready."),
		FText::AsNumber(Result.InspectedActorCount),
		FText::AsNumber(Result.InspectedComponentCount),
		FText::AsNumber(Result.ChangedComponentCount),
		FText::AsNumber(Result.UnmappedComponentCount)), false);
	return FReply::Handled();
}

UStaticMeshQualityProfile* SStaticMeshQualitySwitcherPanel::LoadProfile() const
{
	return GetDefault<UStaticMeshQualitySwitcherSettings>()->ActiveProfile.LoadSynchronous();
}

void SStaticMeshQualitySwitcherPanel::SetStatus(const FText& Text, const bool bIsError)
{
	if (!StatusText.IsValid())
	{
		return;
	}

	StatusText->SetText(Text);
	StatusText->SetColorAndOpacity(bIsError
		? FSlateColor(FLinearColor(0.9f, 0.15f, 0.1f))
		: FSlateColor::UseForeground());
}

void SStaticMeshQualitySwitcherPanel::SetStatusFromErrors(const TArray<FText>& Errors)
{
	if (Errors.IsEmpty())
	{
		SetStatus(LOCTEXT("UnknownError", "The operation failed without an error message."), true);
		return;
	}

	FString CombinedErrors;
	for (const FText& Error : Errors)
	{
		if (!CombinedErrors.IsEmpty())
		{
			CombinedErrors += LINE_TERMINATOR;
		}
		CombinedErrors += FString::Printf(TEXT("- %s"), *Error.ToString());
	}
	SetStatus(FText::FromString(CombinedErrors), true);
}

#undef LOCTEXT_NAMESPACE
