#include "UI/TunaSweeperOnlineCoopWidget.h"
#include "Online/TunaSweeperOnlineCoopSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
void UTunaSweeperOnlineCoopWidget::NativeConstruct(){Super::NativeConstruct();if(HostButton)HostButton->OnClicked.AddDynamic(this,&ThisClass::HostSession);if(JoinButton)JoinButton->OnClicked.AddDynamic(this,&ThisClass::JoinWithEnteredCode);if(LeaveButton)LeaveButton->OnClicked.AddDynamic(this,&ThisClass::LeaveSession);if(UTunaSweeperOnlineCoopSubsystem* S=GetCoopSubsystem()){S->OnStateChanged.AddDynamic(this,&ThisClass::HandleStateChanged);S->OnInviteCodeReady.AddDynamic(this,&ThisClass::HandleInviteCodeReady);}RefreshLocalizedText();}
UTunaSweeperOnlineCoopSubsystem* UTunaSweeperOnlineCoopWidget::GetCoopSubsystem() const{if(const UGameInstance* GI=GetGameInstance())return GI->GetSubsystem<UTunaSweeperOnlineCoopSubsystem>();return nullptr;}
void UTunaSweeperOnlineCoopWidget::HostSession(){if(UTunaSweeperOnlineCoopSubsystem* S=GetCoopSubsystem())S->CreateHostSession();}
void UTunaSweeperOnlineCoopWidget::JoinWithEnteredCode(){if(UTunaSweeperOnlineCoopSubsystem* S=GetCoopSubsystem())if(InviteCodeInput)S->JoinSessionByInviteCode(InviteCodeInput->GetText().ToString());}
void UTunaSweeperOnlineCoopWidget::LeaveSession(){if(UTunaSweeperOnlineCoopSubsystem* S=GetCoopSubsystem())S->LeaveSession();}
void UTunaSweeperOnlineCoopWidget::RefreshLocalizedText(){const UTunaSweeperGameInstance* GI=GetGameInstance()?Cast<UTunaSweeperGameInstance>(GetGameInstance()):nullptr;if(!GI)return;if(HostButton)HostButton->SetToolTipText(GI->ResolveLocalizedText(TEXT("ui.coop.host"),FText::GetEmpty()));if(JoinButton)JoinButton->SetToolTipText(GI->ResolveLocalizedText(TEXT("ui.coop.join"),FText::GetEmpty()));if(InviteCodeText)InviteCodeText->SetText(GI->ResolveLocalizedText(TEXT("ui.coop.join_code"),FText::GetEmpty()));}
void UTunaSweeperOnlineCoopWidget::HandleStateChanged(ETunaSweeperOnlineCoopState State,ETunaSweeperOnlineCoopError){if(StatusText){const UTunaSweeperGameInstance* GI=GetGameInstance()?Cast<UTunaSweeperGameInstance>(GetGameInstance()):nullptr;if(GI)StatusText->SetText(GI->ResolveLocalizedText(FName(*FString::Printf(TEXT("ui.coop.status.%d"),static_cast<int32>(State))),FText::GetEmpty()));}}
void UTunaSweeperOnlineCoopWidget::HandleInviteCodeReady(FString Code){if(InviteCodeText){const UTunaSweeperGameInstance* GI=GetGameInstance()?Cast<UTunaSweeperGameInstance>(GetGameInstance()):nullptr;if(GI)InviteCodeText->SetText(FText::Format(GI->ResolveLocalizedText(TEXT("ui.coop.code_pattern"),FText::GetEmpty()),FText::FromString(Code)));}}

