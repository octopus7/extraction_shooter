// Authored by gpt-6-astra. Reasoning effort is not exposed in this session.
#include "UI/Astra/TeleportConfirmationGPT6Astra.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "UI/TunaSweeperUIFont.h"

bool UTeleportConfirmationGPT6Astra::HasAuthoredControls() const
{
	return TitleText && MessageText && YesText && NoText && YesButton && NoButton;
}

void UTeleportConfirmationGPT6Astra::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	const UTunaSweeperGameInstance* Instance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!HasAuthoredControls() || !Instance) return;
	TitleText->SetText(Instance->ResolveLocalizedText(TEXT("ui.movement_cheat.teleport_title"), FText::GetEmpty()));
	MessageText->SetText(Instance->ResolveLocalizedText(TEXT("ui.movement_cheat.teleport_confirm"), FText::GetEmpty()));
	YesText->SetText(Instance->ResolveLocalizedText(TEXT("ui.movement_cheat.yes"), FText::GetEmpty()));
	NoText->SetText(Instance->ResolveLocalizedText(TEXT("ui.movement_cheat.no"), FText::GetEmpty()));
	YesButton->OnClicked.AddUniqueDynamic(this, &UTeleportConfirmationGPT6Astra::Confirm);
	NoButton->OnClicked.AddUniqueDynamic(this, &UTeleportConfirmationGPT6Astra::Decline);
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	NoButton->SetUserFocus(GetOwningPlayer());
}

void UTeleportConfirmationGPT6Astra::NativeDestruct()
{
	if (!bResolved)
	{
		bResolved = true;
		OnResolved.ExecuteIfBound(false);
	}
	Super::NativeDestruct();
}

void UTeleportConfirmationGPT6Astra::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bResolved && (!IsStillValid.IsBound() || !IsStillValid.Execute())) Resolve(false);
}

void UTeleportConfirmationGPT6Astra::Confirm() { Resolve(true); }
void UTeleportConfirmationGPT6Astra::Decline() { Resolve(false); }
void UTeleportConfirmationGPT6Astra::Cancel() { Resolve(false); }
void UTeleportConfirmationGPT6Astra::FocusCancel() { if (NoButton) NoButton->SetUserFocus(GetOwningPlayer()); }

void UTeleportConfirmationGPT6Astra::Resolve(bool bConfirmed)
{
	if (bResolved) return;
	bResolved = true;
	RemoveFromParent();
	OnResolved.ExecuteIfBound(bConfirmed);
}

FReply UTeleportConfirmationGPT6Astra::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::M)
	{
		Resolve(false);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

FReply UTeleportConfirmationGPT6Astra::NativeOnKeyDown(const FGeometry&, const FKeyEvent&) { return FReply::Handled(); }
FReply UTeleportConfirmationGPT6Astra::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) { return FReply::Handled(); }
FReply UTeleportConfirmationGPT6Astra::NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) { return FReply::Handled(); }
