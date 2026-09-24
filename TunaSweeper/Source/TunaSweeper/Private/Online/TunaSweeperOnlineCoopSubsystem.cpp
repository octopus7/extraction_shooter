#include "Online/TunaSweeperOnlineCoopSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameMapsSettings.h"
#include "NetDriverEOS.h"
#include "Online/TunaSweeperPlatformAuthAdapter.h"
#include "Online/TunaSweeperCoopStagingGameMode.h"
#include "Misc/Guid.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace
{
const FName CoopName = FTunaSweeperOnlineCoopSettings::SessionName();
constexpr double OperationSeconds = 45.0;
const TCHAR* StagingMap = TEXT("/Game/Maps/CoopStaging");
}

void UTunaSweeperOnlineCoopSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick));
    if (GEngine)
    {
        NetworkFailureHandle = GEngine->OnNetworkFailure().AddWeakLambda(this,
            [this](UWorld* World, UNetDriver*, ENetworkFailure::Type, const FString&)
            {
                if (World == GetWorld() && (bTravelling || State == ETunaSweeperOnlineCoopState::Hosting || State == ETunaSweeperOnlineCoopState::Connected))
                    Fail(ETunaSweeperOnlineCoopError::TravelFailed);
            });
        TravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
            [this](UWorld* World, ETravelFailure::Type, const FString&)
            {
                if (World == GetWorld() && bTravelling) Fail(ETunaSweeperOnlineCoopError::TravelFailed);
            });
    }
}

void UTunaSweeperOnlineCoopSubsystem::Deinitialize()
{
    ++OperationGeneration;
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    // A create/join may finish after this game instance is gone. Detach cleanup
    // from the UObject so that its eventual success cannot leave an orphan lobby.
    const bool bCreating = bPending && CreateHandle.IsValid();
    const bool bJoining = bPending && JoinHandle.IsValid();
    const bool bDestroying = bPending && DestroyHandle.IsValid();
    ClearOperationDelegates();
    if (SessionInterface && (bCreating || bJoining))
    {
        const TWeakPtr<IOnlineSession, ESPMode::ThreadSafe> WeakSessions = SessionInterface;
        const auto Handle = MakeShared<FDelegateHandle>();
        if (bCreating)
        {
            *Handle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateLambda(
                [WeakSessions, Handle](FName Name, bool)
                {
                    if (Name != CoopName) return;
                    if (const auto Sessions = WeakSessions.Pin())
                    {
                        Sessions->ClearOnCreateSessionCompleteDelegate_Handle(*Handle);
                        if (Sessions->GetNamedSession(Name)) Sessions->DestroySession(Name);
                    }
                }));
        }
        else
        {
            *Handle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateLambda(
                [WeakSessions, Handle](FName Name, EOnJoinSessionCompleteResult::Type)
                {
                    if (Name != CoopName) return;
                    if (const auto Sessions = WeakSessions.Pin())
                    {
                        Sessions->ClearOnJoinSessionCompleteDelegate_Handle(*Handle);
                        if (Sessions->GetNamedSession(Name)) Sessions->DestroySession(Name);
                    }
                }));
        }
    }
    else if (SessionInterface && !bDestroying && SessionInterface->GetNamedSession(CoopName))
    {
        SessionInterface->DestroySession(CoopName);
    }
    if (bIdentityOverridden)
    {
        if (ULocalPlayer* Player = GetGameInstance()->GetFirstGamePlayer()) Player->SetCachedUniqueNetId(FUniqueNetIdRepl(OriginalUserId));
    }
    SessionInterface.Reset();
    IdentityInterface.Reset();
    Super::Deinitialize();
}

void UTunaSweeperOnlineCoopSubsystem::ClearOperationDelegates()
{
    if (IdentityInterface) IdentityInterface->ClearOnLoginCompleteDelegate_Handle(0, LoginHandle);
    if (SessionInterface)
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
    }
    LoginHandle.Reset(); CreateHandle.Reset(); FindHandle.Reset(); JoinHandle.Reset(); DestroyHandle.Reset();
}

void UTunaSweeperOnlineCoopSubsystem::SetState(ETunaSweeperOnlineCoopState NewState, ETunaSweeperOnlineCoopError Error)
{
    if (State == NewState && LastError == Error) return;
    State = NewState;
    LastError = Error;
    OnStateChanged.Broadcast(State, LastError);
}

void UTunaSweeperOnlineCoopSubsystem::BeginOperation()
{
    ClearOperationDelegates();
    ++OperationGeneration;
    bPending = true;
    Deadline = FPlatformTime::Seconds() + OperationSeconds;
}

bool UTunaSweeperOnlineCoopSubsystem::AcceptCallback(uint64 Generation) const
{
    return bPending && Generation == OperationGeneration;
}

bool UTunaSweeperOnlineCoopSubsystem::InitializeOnlineCoop()
{
    if (bPending || bTravelling || State == ETunaSweeperOnlineCoopState::Leaving) return false;
    if (SessionInterface && SessionInterface->GetNamedSession(CoopName)) return false;
    if (IdentityInterface && IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        bAbandon = false;
        SetState(ETunaSweeperOnlineCoopState::Ready);
        return true;
    }
    // EOS AutoLogin obtains the native Steam WebApi ticket itself. Never copy or log that ticket.
    IOnlineSubsystem* Native = Online::GetSubsystem(GetWorld());
    IOnlineSubsystem* EOS = Online::GetSubsystem(GetWorld(), EOS_SUBSYSTEM);
    if (!Native || Native->GetSubsystemName() != STEAM_SUBSYSTEM || !EOS ||
        !Native->GetIdentityInterface() || Native->GetIdentityInterface()->GetLoginStatus(0) != ELoginStatus::LoggedIn)
    {
        SetState(ETunaSweeperOnlineCoopState::Failed, ETunaSweeperOnlineCoopError::SubsystemUnavailable);
        return false;
    }
    SessionInterface = EOS->GetSessionInterface();
    IdentityInterface = EOS->GetIdentityInterface();
    if (!SessionInterface || !IdentityInterface)
    {
        SetState(ETunaSweeperOnlineCoopState::Failed, ETunaSweeperOnlineCoopError::SubsystemUnavailable);
        return false;
    }
    bAbandon = false;
    BeginOperation();
    SetState(ETunaSweeperOnlineCoopState::Authenticating);
    LoginHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(0, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnLoginComplete, OperationGeneration));
    const uint64 Generation = OperationGeneration;
    if (!FTunaSweeperPlatformAuthAdapter::AutoLogin(IdentityInterface) && AcceptCallback(Generation))
    {
        bPending = false; ClearOperationDelegates(); Fail(ETunaSweeperOnlineCoopError::AuthenticationFailed);
    }
    return State != ETunaSweeperOnlineCoopState::Failed;
}

void UTunaSweeperOnlineCoopSubsystem::OnLoginComplete(int32, bool bSuccess, const FUniqueNetId&, const FString&, uint64 Generation)
{
    if (!AcceptCallback(Generation)) return;
    bPending = false;
    ClearOperationDelegates();
    if (bAbandon) { Cleanup(); return; }
    SetState(bSuccess ? ETunaSweeperOnlineCoopState::Ready : ETunaSweeperOnlineCoopState::Failed,
        bSuccess ? ETunaSweeperOnlineCoopError::None : ETunaSweeperOnlineCoopError::AuthenticationFailed);
}

bool UTunaSweeperOnlineCoopSubsystem::CreateHostSession()
{
    if (!InitializeOnlineCoop() || State != ETunaSweeperOnlineCoopState::Ready) return false;
    CodeAttempts = 0;
    return BeginSearch(true);
}

bool UTunaSweeperOnlineCoopSubsystem::JoinSessionByInviteCode(const FString& InviteCode)
{
    if (bPending || bTravelling || (SessionInterface && SessionInterface->GetNamedSession(CoopName))) return false;
    PendingInviteCode = TunaSweeperOnlineCoop::NormalizeInviteCode(InviteCode);
    if (PendingInviteCode.IsEmpty())
    {
        SetState(ETunaSweeperOnlineCoopState::Failed, ETunaSweeperOnlineCoopError::InvalidInviteCode);
        return false;
    }
    if (!InitializeOnlineCoop() || State != ETunaSweeperOnlineCoopState::Ready) return false;
    return BeginSearch(false);
}

bool UTunaSweeperOnlineCoopSubsystem::BeginSearch(bool bForHost)
{
    bHostSearch = bForHost;
    if (bForHost)
    {
        if (++CodeAttempts > 5) { Fail(ETunaSweeperOnlineCoopError::CodeCollision); return false; }
        // Windows GUID generation is supplied by the OS, unlike a clock-seeded PRNG.
        const FGuid Random = FGuid::NewGuid();
        PendingInviteCode = FString::Printf(TEXT("%08u"), Random.A % 100000000u);
    }
    Search = MakeShared<FOnlineSessionSearch>();
    Search->MaxSearchResults = 100;
    Search->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
    Search->QuerySettings.Set(FTunaSweeperOnlineCoopSettings::InviteCodeKey(), PendingInviteCode, EOnlineComparisonOp::Equals);
    BeginOperation();
    SetState(bForHost ? ETunaSweeperOnlineCoopState::Creating : ETunaSweeperOnlineCoopState::Searching);
    FindHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindComplete, OperationGeneration));
    const uint64 Generation = OperationGeneration;
    if (!SessionInterface->FindSessions(0, Search.ToSharedRef()) && AcceptCallback(Generation)) OnFindComplete(false, Generation);
    return State != ETunaSweeperOnlineCoopState::Failed;
}

void UTunaSweeperOnlineCoopSubsystem::OnFindComplete(bool bSuccess, uint64 Generation)
{
    if (!AcceptCallback(Generation)) return;
    bPending = false;
    ClearOperationDelegates();
    if (bAbandon) { Cleanup(); return; }
    if (!bSuccess || !Search) { Fail(ETunaSweeperOnlineCoopError::SessionSearchFailed); return; }
    int32 Matches = 0;
    const FOnlineSessionSearchResult* Match = nullptr;
    for (const FOnlineSessionSearchResult& Result : Search->SearchResults)
    {
        FString Code;
        Result.Session.SessionSettings.Get(FTunaSweeperOnlineCoopSettings::InviteCodeKey(), Code);
        if (Code == PendingInviteCode) { ++Matches; Match = &Result; }
    }
    if (bHostSearch)
    {
        if (Matches > 0) BeginSearch(true); else BeginCreate();
        return;
    }
    if (Matches != 1) { Fail(Matches > 1 ? ETunaSweeperOnlineCoopError::CodeCollision : ETunaSweeperOnlineCoopError::CodeNotFound); return; }
    FString Flavor;
    Match->Session.SessionSettings.Get(FName(TEXT("TunaSweeper.CoOp.BuildFlavor")), Flavor);
    if (Flavor != TunaSweeperBuildFlavor::GetName().ToString()) { Fail(ETunaSweeperOnlineCoopError::VersionMismatch); return; }
    int32 Version = 0;
    Match->Session.SessionSettings.Get(FTunaSweeperOnlineCoopSettings::ProtocolVersionKey(), Version);
    const auto Error = TunaSweeperOnlineCoop::ClassifySession(PendingInviteCode, Version, Match->Session.NumOpenPublicConnections, PendingInviteCode);
    if (Error != ETunaSweeperOnlineCoopError::None) { Fail(Error); return; }
    BeginOperation();
    SetState(ETunaSweeperOnlineCoopState::Joining);
    JoinHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinComplete, OperationGeneration));
    const uint64 JoinGeneration = OperationGeneration;
    if (!SessionInterface->JoinSession(0, CoopName, *Match) && AcceptCallback(JoinGeneration)) OnJoinComplete(CoopName, EOnJoinSessionCompleteResult::UnknownError, JoinGeneration);
}

void UTunaSweeperOnlineCoopSubsystem::BeginCreate()
{
    FOnlineSessionSettings Settings;
    Settings.NumPublicConnections = FTunaSweeperOnlineCoopSettings::MaxPlayers;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = false;
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bUsesPresence = true;
    Settings.bAllowInvites = false;
    Settings.bAllowJoinViaPresence = false;
    Settings.bUseLobbiesVoiceChatIfAvailable = false;
    Settings.Set(FName(TEXT("TunaSweeper.CoOp.BuildFlavor")), TunaSweeperBuildFlavor::GetName().ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
    Settings.BuildUniqueId = FTunaSweeperOnlineCoopSettings::ProtocolVersion;
    Settings.Set(FTunaSweeperOnlineCoopSettings::InviteCodeKey(), PendingInviteCode, EOnlineDataAdvertisementType::ViaOnlineService);
    Settings.Set(FTunaSweeperOnlineCoopSettings::ProtocolVersionKey(), FTunaSweeperOnlineCoopSettings::ProtocolVersion, EOnlineDataAdvertisementType::ViaOnlineService);
    BeginOperation();
    CreateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateComplete, OperationGeneration));
    const uint64 Generation = OperationGeneration;
    if (!SessionInterface->CreateSession(0, CoopName, Settings) && AcceptCallback(Generation)) OnCreateComplete(CoopName, false, Generation);
}

void UTunaSweeperOnlineCoopSubsystem::UseEOSIdentity()
{
    if (ULocalPlayer* Player = GetGameInstance()->GetFirstGamePlayer())
    {
        if (!bIdentityOverridden) OriginalUserId = Player->GetCachedUniqueNetId().GetUniqueNetId();
        Player->SetCachedUniqueNetId(FUniqueNetIdRepl(IdentityInterface->GetUniquePlayerId(0)));
        bIdentityOverridden = true;
    }
}

void UTunaSweeperOnlineCoopSubsystem::OnCreateComplete(FName Name, bool bSuccess, uint64 Generation)
{
    if (Name != CoopName || !AcceptCallback(Generation)) return;
    bPending = false;
    ClearOperationDelegates();
    if (bAbandon) { Cleanup(); return; }
    if (!bSuccess) { Fail(ETunaSweeperOnlineCoopError::SessionCreateFailed); return; }
    UseEOSIdentity();
    bTravelling = true;
    Deadline = FPlatformTime::Seconds() + OperationSeconds;
    UGameplayStatics::OpenLevel(this, FName(StagingMap), true, TEXT("listen"));
}

void UTunaSweeperOnlineCoopSubsystem::OnJoinComplete(FName Name, EOnJoinSessionCompleteResult::Type Result, uint64 Generation)
{
    if (Name != CoopName || !AcceptCallback(Generation)) return;
    bPending = false;
    ClearOperationDelegates();
    if (bAbandon) { Cleanup(); return; }
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        Fail(Result == EOnJoinSessionCompleteResult::SessionIsFull ? ETunaSweeperOnlineCoopError::RoomFull : ETunaSweeperOnlineCoopError::JoinFailed);
        return;
    }
    FString Address;
    if (!SessionInterface->GetResolvedConnectString(Name, Address) || !Address.StartsWith(TEXT("EOS:"), ESearchCase::IgnoreCase)) { Fail(ETunaSweeperOnlineCoopError::ResolveConnectStringFailed); return; }
    APlayerController* Controller = GetGameInstance()->GetFirstLocalPlayerController();
    if (!Controller) { Fail(ETunaSweeperOnlineCoopError::TravelFailed); return; }
    UseEOSIdentity();
    bTravelling = true;
    Deadline = FPlatformTime::Seconds() + OperationSeconds;
    Controller->ClientTravel(Address, TRAVEL_Absolute);
}

void UTunaSweeperOnlineCoopSubsystem::Fail(ETunaSweeperOnlineCoopError Error)
{
    bAbandon = true;
    CurrentInviteCode.Reset();
    ReturnToMenu();
    SetState(ETunaSweeperOnlineCoopState::Failed, Error);
    if (!bPending) Cleanup();
}

void UTunaSweeperOnlineCoopSubsystem::LeaveSession()
{
    bAbandon = true;
    CurrentInviteCode.Reset();
    SetState(ETunaSweeperOnlineCoopState::Leaving);
    ReturnToMenu();
    // A create/join cannot be cancelled safely. Keep its delegate until completion,
    // then destroy any resulting session before allowing another operation.
    if (!bPending) Cleanup();
}

void UTunaSweeperOnlineCoopSubsystem::ReturnToMenu()
{
    const bool bWasOnline = bIdentityOverridden || bTravelling;
    bTravelling = false;
    if (bIdentityOverridden)
    {
        if (ULocalPlayer* Player = GetGameInstance()->GetFirstGamePlayer()) Player->SetCachedUniqueNetId(FUniqueNetIdRepl(OriginalUserId));
        OriginalUserId.Reset();
        bIdentityOverridden = false;
    }
    if (bWasOnline) UGameplayStatics::OpenLevel(this, FName(*UGameMapsSettings::GetGameDefaultMap()));
}

void UTunaSweeperOnlineCoopSubsystem::Cleanup()
{
    if (bPending) return;
    if (SessionInterface && SessionInterface->GetNamedSession(CoopName))
    {
        BeginOperation();
        DestroyHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyComplete, OperationGeneration));
        const uint64 Generation = OperationGeneration;
        if (!SessionInterface->DestroySession(CoopName) && AcceptCallback(Generation)) OnDestroyComplete(CoopName, false, Generation);
        return;
    }
    Search.Reset();
    PendingInviteCode.Reset();
    if (State != ETunaSweeperOnlineCoopState::Failed)
        SetState(IdentityInterface && IdentityInterface->GetLoginStatus(0) == ELoginStatus::LoggedIn ? ETunaSweeperOnlineCoopState::Ready : ETunaSweeperOnlineCoopState::Offline);
    else
        OnStateChanged.Broadcast(State, LastError); // Cleanup drained; retry controls can now enable.
}

void UTunaSweeperOnlineCoopSubsystem::OnDestroyComplete(FName Name, bool bSuccess, uint64 Generation)
{
    if (Name != CoopName || !AcceptCallback(Generation)) return;
    bPending = false;
    ClearOperationDelegates();
    if (!bSuccess) { SetState(ETunaSweeperOnlineCoopState::Failed, ETunaSweeperOnlineCoopError::CleanupFailed); return; }
    Cleanup();
}

bool UTunaSweeperOnlineCoopSubsystem::Tick(float)
{
    if ((bPending || bTravelling) && FPlatformTime::Seconds() > Deadline)
    {
        Deadline = DBL_MAX;
        Fail(ETunaSweeperOnlineCoopError::OperationTimeout);
    }
    if ((State == ETunaSweeperOnlineCoopState::Hosting || State == ETunaSweeperOnlineCoopState::Connected || bTravelling) &&
        IdentityInterface && IdentityInterface->GetLoginStatus(0) != ELoginStatus::LoggedIn)
        Fail(ETunaSweeperOnlineCoopError::AuthenticationFailed);
    UWorld* World = GetWorld();
    if (!bTravelling || !World || !World->GetPackage()->GetName().Contains(TEXT("CoopStaging"))) return true;
    UNetDriver* Driver = World->GetNetDriver();
    const UNetDriverEOS* EOSDriver = Cast<UNetDriverEOS>(Driver);
    if (!EOSDriver || EOSDriver->bIsPassthrough) return true;
    if (State == ETunaSweeperOnlineCoopState::Creating && World->GetNetMode() == NM_ListenServer)
    {
        bTravelling = false;
        CurrentInviteCode = PendingInviteCode;
        SetState(ETunaSweeperOnlineCoopState::Hosting);
        OnInviteCodeReady.Broadcast(CurrentInviteCode);
    }
    else if (State == ETunaSweeperOnlineCoopState::Joining && World->GetNetMode() == NM_Client &&
        Driver->ServerConnection && Driver->ServerConnection->GetConnectionState() == USOCK_Open &&
        Cast<ATunaSweeperCoopStagingPlayerController>(GetGameInstance()->GetFirstLocalPlayerController()))
    {
        bTravelling = false;
        SetState(ETunaSweeperOnlineCoopState::Connected);
    }
    return true;
}
