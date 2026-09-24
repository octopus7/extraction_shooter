#include "Online/TunaSweeperCoopStagingGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

namespace TunaSweeperCoopStaging
{
FString ProductUserIdFromUniqueId(const FString& UniqueId)
{
 FString Account, Product;
 if (!UniqueId.Split(TEXT("|"), &Account, &Product) || Product.Len() != 32) return FString();
 for (const TCHAR Character : Product) if (!FChar::IsHexDigit(Character)) return FString();
 if (Product == FString::ChrN(32, TEXT('0'))) return FString();
 return Product.ToLower();
}
bool MatchesAuthenticatedPeer(const FString& UniqueId, const FString& PeerAddress)
{
 const FString Product = ProductUserIdFromUniqueId(UniqueId);
 return !Product.IsEmpty() && PeerAddress.Equals(TEXT("EOS:") + Product, ESearchCase::IgnoreCase);
}
}
void ATunaSweeperCoopStagingPlayerController::BeginPlay()
{
 Super::BeginPlay();
 if (!IsLocalController()) return;
 const TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_OnlineCoop.WBP_OnlineCoop_C"));
 if (WidgetClass)
 {
  CoopWidget = CreateWidget<UUserWidget>(this, WidgetClass);
  if (CoopWidget)
  {
   CoopWidget->AddToViewport();
   FInputModeUIOnly InputMode;
   InputMode.SetWidgetToFocus(CoopWidget->TakeWidget());
   SetInputMode(InputMode);
  }
 }
 bShowMouseCursor = true;
}
ATunaSweeperCoopStagingGameMode::ATunaSweeperCoopStagingGameMode()
{
 PlayerControllerClass = ATunaSweeperCoopStagingPlayerController::StaticClass();
 DefaultPawnClass = nullptr;
 HUDClass = nullptr;
 bUseSeamlessTravel = false;
}
bool ATunaSweeperCoopStagingGameMode::IsLobbyMember(const FUniqueNetIdRepl& UniqueId) const
{
 if (!UniqueId.IsValid() || UniqueId.GetType() != FName(TEXT("EOS"))) return false;
 IOnlineSubsystem* EOS = Online::GetSubsystem(GetWorld(), FName(TEXT("EOS")));
 const IOnlineSessionPtr Sessions = EOS ? EOS->GetSessionInterface() : nullptr;
 const FNamedOnlineSession* Session = Sessions.IsValid() ? Sessions->GetNamedSession(FTunaSweeperOnlineCoopSettings::SessionName()) : nullptr;
 if (!Session || !Session->bHosting || !Session->SessionSettings.bUseLobbiesIfAvailable) return false;
 const FString Product = TunaSweeperCoopStaging::ProductUserIdFromUniqueId(UniqueId.ToString());
 if (Product.IsEmpty()) return false;
 // MemberSettings comes from EOS lobby callbacks; RegisteredPlayers is only local bookkeeping.
 for (const auto& Member : Session->SessionSettings.MemberSettings)
  if (Member.Key->GetType() == FName(TEXT("EOS")) && TunaSweeperCoopStaging::ProductUserIdFromUniqueId(Member.Key->ToString()) == Product) return true;
 return false;
}
bool ATunaSweeperCoopStagingGameMode::HasCapacityFor(const FUniqueNetIdRepl& UniqueId) const
{
 int32 Count = 0;
 for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
 {
  const APlayerController* Controller = It->Get();
  if (!Controller) continue;
  ++Count;
  if (Controller->PlayerState && Controller->PlayerState->GetUniqueId() == UniqueId) return false;
 }
 return Count < FTunaSweeperOnlineCoopSettings::MaxPlayers;
}
void ATunaSweeperCoopStagingGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
 // Bypass the base Steam-default ID compatibility check only in this EOS staging world.
 // Address is supplied by the transport, never travel URL options or invite codes.
 ErrorMessage.Reset();
 if (bAdmissionClosing || !IsLobbyMember(UniqueId) || !TunaSweeperCoopStaging::MatchesAuthenticatedPeer(UniqueId.ToString(), Address)) ErrorMessage = TEXT("coop_admission_denied");
 else if (!HasCapacityFor(UniqueId)) ErrorMessage = TEXT("coop_room_full");
 FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
}
APlayerController* ATunaSweeperCoopStagingGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
 ErrorMessage.Reset();
 bool bAuthenticatedPeer = false;
 if (UNetConnection* Connection = Cast<UNetConnection>(NewPlayer))
 {
  // Reject child/splitscreen connections and IP fallback even with a claimed lobby member ID.
  bAuthenticatedPeer = Connection->GetClass()->GetPathName() == TEXT("/Script/SocketSubsystemEOS.NetConnectionEOS")
   && TunaSweeperCoopStaging::MatchesAuthenticatedPeer(UniqueId.ToString(), Connection->LowLevelGetRemoteAddress());
 }
 else if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(NewPlayer))
 {
  IOnlineSubsystem* EOS = Online::GetSubsystem(GetWorld(), FName(TEXT("EOS")));
  const IOnlineIdentityPtr Identity = EOS ? EOS->GetIdentityInterface() : nullptr;
  const FUniqueNetIdPtr LocalId = Identity.IsValid() ? Identity->GetUniquePlayerId(LocalPlayer->GetControllerId()) : nullptr;
  bAuthenticatedPeer = LocalId.IsValid() && UniqueId.IsValid() && *LocalId == *UniqueId.GetUniqueNetId();
 }
 if (bAdmissionClosing || !bAuthenticatedPeer || !IsLobbyMember(UniqueId)) ErrorMessage = TEXT("coop_admission_denied");
 else if (!HasCapacityFor(UniqueId)) ErrorMessage = TEXT("coop_room_full");
 if (!ErrorMessage.IsEmpty()) return nullptr;
 return Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
}
FString ATunaSweeperCoopStagingGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
 if (!NewPlayerController || !NewPlayerController->PlayerState) return TEXT("coop_admission_denied");
 // Avoid base registration with Steam and gameplay PlayerStart lookup. EOS lobby owns membership.
 NewPlayerController->PlayerState->SetPlayerId(GameSession->GetNextPlayerID());
 NewPlayerController->PlayerState->SetUniqueId(UniqueId);
 return FString();
}
void ATunaSweeperCoopStagingGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
 // No RestartPlayer: staging intentionally spawns no gameplay pawn.
}

void ATunaSweeperCoopStagingGameMode::PreLoginAsync(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, const FOnPreLoginCompleteDelegate& OnComplete)
{
 // EOS authenticates the socket before Unreal login; never wait on an unverified claimed ID.
 if (bAdmissionClosing || !UniqueId.IsValid() || UniqueId.GetType() != FName(TEXT("EOS")) ||
  !TunaSweeperCoopStaging::MatchesAuthenticatedPeer(UniqueId.ToString(), Address) ||
  !HasCapacityFor(UniqueId) || IsLobbyMember(UniqueId))
 {
  FString Error;
  PreLogin(Options, Address, UniqueId, Error);
  OnComplete.ExecuteIfBound(Error);
  return;
 }
 // The client's join completion and the host's EOS member notification can arrive out of order.
 // Bound both the wait and queue; Login independently rechecks membership/capacity at allocation.
 if (PendingAdmissions.Num() >= 16)
 {
  OnComplete.ExecuteIfBound(TEXT("coop_admission_denied"));
  return;
 }
 FPendingAdmission& Pending = PendingAdmissions.AddDefaulted_GetRef();
 Pending.Options = Options;
 Pending.Address = Address;
 Pending.UniqueId = UniqueId;
 Pending.Completion = OnComplete;
 Pending.Deadline = FPlatformTime::Seconds() + 5.0;
 if (!GetWorldTimerManager().IsTimerActive(AdmissionTimer))
  GetWorldTimerManager().SetTimer(AdmissionTimer, this, &ThisClass::PollPendingAdmissions, 0.1f, true);
}

void ATunaSweeperCoopStagingGameMode::PollPendingAdmissions()
{
 TArray<FPendingAdmission> Completed;
 const double Now = FPlatformTime::Seconds();
 for (int32 Index = PendingAdmissions.Num() - 1; Index >= 0; --Index)
 {
  const FPendingAdmission& Pending = PendingAdmissions[Index];
  if (Now >= Pending.Deadline || IsLobbyMember(Pending.UniqueId) || !HasCapacityFor(Pending.UniqueId))
  {
   Completed.Add(MoveTemp(PendingAdmissions[Index]));
   PendingAdmissions.RemoveAtSwap(Index);
  }
 }
 if (PendingAdmissions.IsEmpty()) GetWorldTimerManager().ClearTimer(AdmissionTimer);
 // Detach requests before calling engine delegates, which may synchronously create controllers.
 for (const FPendingAdmission& Pending : Completed)
 {
  FString Error;
  PreLogin(Pending.Options, Pending.Address, Pending.UniqueId, Error);
  Pending.Completion.ExecuteIfBound(Error);
 }
}

void ATunaSweeperCoopStagingGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
 bAdmissionClosing = true;
 // The timer uses a UObject-bound method (no raw lambda capture) and is cancelled on teardown.
 GetWorldTimerManager().ClearTimer(AdmissionTimer);
 TArray<FPendingAdmission> Cancelled = MoveTemp(PendingAdmissions);
 PendingAdmissions.Reset();
 for (const FPendingAdmission& Pending : Cancelled)
  Pending.Completion.ExecuteIfBound(TEXT("coop_admission_denied"));
 Super::EndPlay(EndPlayReason);
}
