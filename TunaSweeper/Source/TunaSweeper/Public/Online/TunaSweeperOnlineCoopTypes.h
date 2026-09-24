#pragma once
#include "CoreMinimal.h"
#include "TunaSweeperOnlineCoopTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperOnlineCoopState : uint8 { Offline, Initializing, Authenticating, Ready, Creating, Hosting, Searching, Joining, Connected, Failed, Leaving };
UENUM(BlueprintType)
enum class ETunaSweeperOnlineCoopError : uint8 { None, SubsystemUnavailable, AuthenticationFailed, InvalidInviteCode, SessionCreateFailed, SessionSearchFailed, CodeNotFound, RoomFull, VersionMismatch, JoinFailed, ResolveConnectStringFailed, TravelFailed, OperationTimeout, CodeCollision, CleanupFailed };

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperOnlineCoopSettings
{
 GENERATED_BODY()
 static constexpr int32 InviteCodeLength = 8;
 static constexpr int32 MaxPlayers = 2;
 static constexpr int32 ProtocolVersion = 1;
 static FName SessionName();
 static FName InviteCodeKey();
 static FName ProtocolVersionKey();
};

namespace TunaSweeperOnlineCoop
{
TUNASWEEPER_API FString GenerateInviteCode(FRandomStream& RandomStream);
TUNASWEEPER_API ETunaSweeperOnlineCoopError ClassifySession(const FString& SessionCode, int32 Version, int32 OpenConnections, const FString& RequestedCode);
TUNASWEEPER_API bool IsValidInviteCode(const FString& InviteCode);
TUNASWEEPER_API FString NormalizeInviteCode(const FString& InviteCode);
TUNASWEEPER_API bool MatchesSessionMetadata(const FString& SessionCode, int32 SessionProtocolVersion, int32 OpenConnections, const FString& RequestedCode);
}
