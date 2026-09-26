#include "UI/TunaSweeperOnlineCoopWidget.h"
#include "Online/TunaSweeperOnlineCoopSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "UI/TunaSweeperUIFont.h"

void UTunaSweeperOnlineCoopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	if (DescriptionText)
	{
		// The authored 660px Card has 32px Surface padding per side. Wrap during prepass, before the first paint.
		DescriptionText->SetAutoWrapText(false);
		DescriptionText->SetWrapTextAt(596.f);
	}
	for (UTextBlock* Label : { ConnectButtonText.Get(), HostButtonText.Get(), JoinButtonText.Get(), LeaveButtonText.Get(), CloseButtonText.Get() })
	{
		if (Label)
		{
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.015f, 0.025f, 0.04f)));
			Label->SetAutoWrapText(false);
		}
	}
	if (ConnectButton) ConnectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ConnectOnline);
	if (InviteCodeInput)
	{
		InviteInputStyle = InviteCodeInput->GetWidgetStyle();
		InviteInputStyle.TextStyle.SetFont(TunaSweeperUIFont::MakeFont(nullptr, 24));
		InviteInputStyle.SetForegroundColor(FSlateColor(FLinearColor(0.015f, 0.025f, 0.04f)));
		InviteInputStyle.SetReadOnlyForegroundColor(FSlateColor(FLinearColor(0.015f, 0.025f, 0.04f)));
		InviteInputStyle.TextStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.015f, 0.025f, 0.04f)));
		InviteCodeInput->SetWidgetStyle(InviteInputStyle);
	}
	if (HostButton) HostButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HostSession);
	if (JoinButton) JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::JoinWithEnteredCode);
	if (LeaveButton) LeaveButton->OnClicked.AddUniqueDynamic(this, &ThisClass::LeaveSession);
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ClosePanel);
		const bool bStaging = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("CoopStaging"));
		CloseButton->SetVisibility(bStaging ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem())
	{
		Subsystem->OnStateChanged.AddUniqueDynamic(this, &ThisClass::HandleStateChanged);
		Subsystem->OnInviteCodeReady.AddUniqueDynamic(this, &ThisClass::HandleInviteCodeReady);
	}
	if (UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Instance->OnLanguageChanged.RemoveAll(this);
		Instance->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshLocalizedText);
	}
	RefreshLocalizedText();
}

void UTunaSweeperOnlineCoopWidget::NativeDestruct()
{
	if (ConnectButton) ConnectButton->OnClicked.RemoveDynamic(this, &ThisClass::ConnectOnline);
	if (HostButton) HostButton->OnClicked.RemoveDynamic(this, &ThisClass::HostSession);
	if (JoinButton) JoinButton->OnClicked.RemoveDynamic(this, &ThisClass::JoinWithEnteredCode);
	if (LeaveButton) LeaveButton->OnClicked.RemoveDynamic(this, &ThisClass::LeaveSession);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::ClosePanel);
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem())
	{
		Subsystem->OnStateChanged.RemoveDynamic(this, &ThisClass::HandleStateChanged);
		Subsystem->OnInviteCodeReady.RemoveDynamic(this, &ThisClass::HandleInviteCodeReady);
	}
	if (UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		Instance->OnLanguageChanged.RemoveAll(this);
	Super::NativeDestruct();
}

UTunaSweeperOnlineCoopSubsystem* UTunaSweeperOnlineCoopWidget::GetCoopSubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UTunaSweeperOnlineCoopSubsystem>() : nullptr;
}
void UTunaSweeperOnlineCoopWidget::ConnectOnline()
{
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem()) Subsystem->InitializeOnlineCoop();
}
void UTunaSweeperOnlineCoopWidget::HostSession()
{
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem()) Subsystem->CreateHostSession();
}
void UTunaSweeperOnlineCoopWidget::JoinWithEnteredCode()
{
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem(); Subsystem && InviteCodeInput)
		Subsystem->JoinSessionByInviteCode(InviteCodeInput->GetText().ToString());
}
void UTunaSweeperOnlineCoopWidget::LeaveSession()
{
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem()) Subsystem->LeaveSession();
}
void UTunaSweeperOnlineCoopWidget::ClosePanel()
{
	if (UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem())
	{
		const auto State = Subsystem->GetOnlineCoopState();
		if (Subsystem->IsOperationPending() || (State != ETunaSweeperOnlineCoopState::Offline && State != ETunaSweeperOnlineCoopState::Ready)) Subsystem->LeaveSession();
	}
	RemoveFromParent();
	if (ReturnFocusWidget.IsValid()) ReturnFocusWidget->SetKeyboardFocus();
}
void UTunaSweeperOnlineCoopWidget::HandleStateChanged(ETunaSweeperOnlineCoopState, ETunaSweeperOnlineCoopError) { RefreshState(); }
void UTunaSweeperOnlineCoopWidget::HandleInviteCodeReady(FString) { RefreshState(); }

void UTunaSweeperOnlineCoopWidget::RefreshLocalizedText()
{
	const UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Instance) return;
	auto SetText = [Instance](UTextBlock* Text, const TCHAR* Key)
	{
		if (Text) Text->SetText(Instance->ResolveLocalizedText(FName(Key), FText::GetEmpty()));
	};
	SetText(TitleText, TEXT("ui.coop.title"));
	SetText(DescriptionText, TEXT("ui.coop.description"));
	SetText(ConnectButtonText, TEXT("ui.coop.connect"));
	SetText(HostButtonText, TEXT("ui.coop.host"));
	SetText(JoinButtonText, TEXT("ui.coop.join"));
	SetText(LeaveButtonText, TEXT("ui.coop.leave"));
	SetText(CloseButtonText, TEXT("ui.coop.close"));
	SetText(InviteCodeLabel, TEXT("ui.coop.join_code"));
	if (InviteCodeInput) InviteCodeInput->SetHintText(Instance->ResolveLocalizedText(TEXT("ui.coop.code_hint"), FText::GetEmpty()));
	RefreshState();
}

void UTunaSweeperOnlineCoopWidget::RefreshState()
{
	const UTunaSweeperGameInstance* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const UTunaSweeperOnlineCoopSubsystem* Subsystem = GetCoopSubsystem();
	if (!Instance) return;
	const auto State = Subsystem ? Subsystem->GetOnlineCoopState() : ETunaSweeperOnlineCoopState::Offline;
	const auto Error = Subsystem ? Subsystem->GetLastError() : ETunaSweeperOnlineCoopError::SubsystemUnavailable;
	const bool bCanStart = Subsystem && !Subsystem->IsOperationPending() && (State == ETunaSweeperOnlineCoopState::Ready);
	if (ConnectButton) ConnectButton->SetIsEnabled(Subsystem && !Subsystem->IsOperationPending() && (State == ETunaSweeperOnlineCoopState::Offline || State == ETunaSweeperOnlineCoopState::Failed));
	if (HostButton) HostButton->SetIsEnabled(bCanStart);
	if (JoinButton) JoinButton->SetIsEnabled(bCanStart);
	if (InviteCodeInput) InviteCodeInput->SetIsEnabled(bCanStart);
	if (LeaveButton) LeaveButton->SetIsEnabled(Subsystem && State != ETunaSweeperOnlineCoopState::Offline && State != ETunaSweeperOnlineCoopState::Ready && State != ETunaSweeperOnlineCoopState::Leaving);
	if (CloseButton) CloseButton->SetIsEnabled(true);
	const FString StateKey = TEXT("ui.coop.status.") + StaticEnum<ETunaSweeperOnlineCoopState>()->GetNameStringByValue(static_cast<int64>(State));
	if (StatusText) StatusText->SetText(Instance->ResolveLocalizedText(FName(*StateKey), FText::GetEmpty()));
	if (ErrorText)
	{
		const FString ErrorKey = TEXT("ui.coop.error.") + StaticEnum<ETunaSweeperOnlineCoopError>()->GetNameStringByValue(static_cast<int64>(Error));
		ErrorText->SetText(Error == ETunaSweeperOnlineCoopError::None ? FText::GetEmpty() : Instance->ResolveLocalizedText(FName(*ErrorKey), FText::GetEmpty()));
	}
	if (InviteCodeText)
	{
		const FString Code = Subsystem ? Subsystem->GetCurrentInviteCode() : FString();
		InviteCodeText->SetText(Code.IsEmpty() ? Instance->ResolveLocalizedText(TEXT("ui.coop.no_code"), FText::GetEmpty()) : FText::Format(Instance->ResolveLocalizedText(TEXT("ui.coop.code_pattern"), FText::GetEmpty()), FText::FromString(Code)));
	}
}
