#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Containers/Ticker.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#include "TunaSweeperOnlineCoopSubsystem.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTunaSweeperOnlineCoopStateChanged, ETunaSweeperOnlineCoopState, State, ETunaSweeperOnlineCoopError, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperOnlineCoopInviteCodeReady, FString, InviteCode);
UCLASS()
class TUNASWEEPER_API UTunaSweeperOnlineCoopSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop") bool InitializeOnlineCoop();
    UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop") bool CreateHostSession();
    UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop") bool JoinSessionByInviteCode(const FString& InviteCode);
    UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop") void LeaveSession();
    UFUNCTION(BlueprintPure, Category="TunaSweeper|Online Coop") ETunaSweeperOnlineCoopState GetOnlineCoopState() const { return State; }
    UFUNCTION(BlueprintPure, Category="TunaSweeper|Online Coop") ETunaSweeperOnlineCoopError GetLastError() const { return LastError; }
    UFUNCTION(BlueprintPure, Category="TunaSweeper|Online Coop") FString GetCurrentInviteCode() const { return CurrentInviteCode; }
    UFUNCTION(BlueprintPure, Category="TunaSweeper|Online Coop") bool IsOperationPending() const { return bPending || bTravelling; }
    UPROPERTY(BlueprintAssignable) FTunaSweeperOnlineCoopStateChanged OnStateChanged;
    UPROPERTY(BlueprintAssignable) FTunaSweeperOnlineCoopInviteCodeReady OnInviteCodeReady;
private:
    friend class FTunaCoopGenerationTest;
    void SetState(ETunaSweeperOnlineCoopState NewState, ETunaSweeperOnlineCoopError Error = ETunaSweeperOnlineCoopError::None);
    void Fail(ETunaSweeperOnlineCoopError Error);
    bool BeginSearch(bool bForHost);
    void BeginCreate();
    void Cleanup();
    void ReturnToMenu();
    void ClearOperationDelegates();
    void OnLoginComplete(int32 LocalUserNum, bool bSuccess, const FUniqueNetId& UserId, const FString& Error, uint64 Generation);
    void OnCreateComplete(FName SessionName, bool bSuccess, uint64 Generation);
    void OnFindComplete(bool bSuccess, uint64 Generation);
    void OnJoinComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result, uint64 Generation);
    void OnDestroyComplete(FName SessionName, bool bSuccess, uint64 Generation);
    void BeginOperation();
    bool AcceptCallback(uint64 Generation) const;
    bool Tick(float DeltaTime);
    void UseEOSIdentity();
    IOnlineSessionPtr SessionInterface;
    IOnlineIdentityPtr IdentityInterface;
    FDelegateHandle LoginHandle, CreateHandle, FindHandle, JoinHandle, DestroyHandle, NetworkFailureHandle, TravelFailureHandle;
    FTSTicker::FDelegateHandle TickHandle;
    TSharedPtr<FOnlineSessionSearch> Search;
    TSharedPtr<const FUniqueNetId> OriginalUserId;
    FString PendingInviteCode, CurrentInviteCode;
    ETunaSweeperOnlineCoopState State = ETunaSweeperOnlineCoopState::Offline;
    ETunaSweeperOnlineCoopError LastError = ETunaSweeperOnlineCoopError::None;
    uint64 OperationGeneration = 0;
    double Deadline = 0;
    int32 CodeAttempts = 0;
    bool bPending = false, bAbandon = false, bHostSearch = false, bTravelling = false, bIdentityOverridden = false;
};
