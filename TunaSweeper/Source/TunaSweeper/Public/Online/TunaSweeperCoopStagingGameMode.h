#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/OnlineReplStructs.h"
#include "TimerManager.h"
#include "TunaSweeperCoopStagingGameMode.generated.h"
class UUserWidget;
/** Isolated menu controller with no gameplay or save hooks. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperCoopStagingPlayerController : public APlayerController
{
 GENERATED_BODY()
protected:
 virtual void BeginPlay() override;
private:
 UPROPERTY(Transient) TObjectPtr<UUserWidget> CoopWidget;
};
UCLASS()
class TUNASWEEPER_API ATunaSweeperCoopStagingGameMode : public AGameModeBase
{
 GENERATED_BODY()
public:
 ATunaSweeperCoopStagingGameMode();
 virtual void PreLoginAsync(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, const FOnPreLoginCompleteDelegate& OnComplete) override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
 virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
 virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
 virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
private:
 struct FPendingAdmission
 {
  FString Options, Address;
  FUniqueNetIdRepl UniqueId;
  FOnPreLoginCompleteDelegate Completion;
  double Deadline = 0;
 };
 TArray<FPendingAdmission> PendingAdmissions;
 FTimerHandle AdmissionTimer;
 bool bAdmissionClosing = false;
 void PollPendingAdmissions();
 bool IsLobbyMember(const FUniqueNetIdRepl& UniqueId) const;
 bool HasCapacityFor(const FUniqueNetIdRepl& UniqueId) const;
};
namespace TunaSweeperCoopStaging
{
 TUNASWEEPER_API FString ProductUserIdFromUniqueId(const FString& UniqueId);
 TUNASWEEPER_API bool MatchesAuthenticatedPeer(const FString& UniqueId, const FString& PeerAddress);
}
