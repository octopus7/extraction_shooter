#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#include "TunaSweeperOnlineCoopSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTunaSweeperOnlineCoopStateChanged, ETunaSweeperOnlineCoopState, State, ETunaSweeperOnlineCoopError, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperOnlineCoopInviteCodeReady, FString, InviteCode);

UCLASS()
class TUNASWEEPER_API UTunaSweeperOnlineCoopSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop") bool InitializeOnlineCoop();
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop") bool CreateHostSession();
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop") bool JoinSessionByInviteCode(const FString& InviteCode);
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop") void LeaveSession();
 UFUNCTION(BlueprintPure,Category="TunaSweeper|Online Coop") ETunaSweeperOnlineCoopState GetOnlineCoopState() const { return State; }
 UFUNCTION(BlueprintPure,Category="TunaSweeper|Online Coop") FString GetCurrentInviteCode() const { return CurrentInviteCode; }
 UPROPERTY(BlueprintAssignable) FTunaSweeperOnlineCoopStateChanged OnStateChanged;
 UPROPERTY(BlueprintAssignable) FTunaSweeperOnlineCoopInviteCodeReady OnInviteCodeReady;
private:
 void SetState(ETunaSweeperOnlineCoopState NewState, ETunaSweeperOnlineCoopError Error=ETunaSweeperOnlineCoopError::None);
 void OnLoginComplete(int32 LocalUserNum,bool bSuccess,const FUniqueNetId& UserId,const FString& Error);
 void OnCreateComplete(FName SessionName,bool bSuccess);
 void OnFindComplete(bool bSuccess);
 void OnJoinComplete(FName SessionName,EOnJoinSessionCompleteResult::Type Result);
 void OnDestroyComplete(FName SessionName,bool bSuccess);
 TSharedPtr<const FUniqueNetId> GetUserId() const;
 IOnlineSessionPtr SessionInterface;
 IOnlineIdentityPtr IdentityInterface;
 FDelegateHandle LoginHandle,CreateHandle,FindHandle,JoinHandle,DestroyHandle;
 TSharedPtr<FOnlineSessionSearch> Search;
 FString PendingInviteCode;
 FString CurrentInviteCode;
 ETunaSweeperOnlineCoopState State=ETunaSweeperOnlineCoopState::Offline;
 uint32 OperationGeneration=0;
 bool bInitialized=false;
};
