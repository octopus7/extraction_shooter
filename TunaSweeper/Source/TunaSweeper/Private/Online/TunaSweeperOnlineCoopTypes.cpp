#include "Online/TunaSweeperOnlineCoopTypes.h"
FName FTunaSweeperOnlineCoopSettings::SessionName(){ return FName(TEXT("TunaSweeperCoop")); }
FName FTunaSweeperOnlineCoopSettings::InviteCodeKey(){ return FName(TEXT("TunaSweeper.CoOp.InviteCode")); }
FName FTunaSweeperOnlineCoopSettings::ProtocolVersionKey(){ return FName(TEXT("TunaSweeper.CoOp.ProtocolVersion")); }
namespace TunaSweeperOnlineCoop {
FString GenerateInviteCode(FRandomStream& R){ return FString::Printf(TEXT("%08d"), R.RandRange(0, 99999999)); }
bool IsValidInviteCode(const FString& S){ if(S.Len()!=FTunaSweeperOnlineCoopSettings::InviteCodeLength)return false; for(TCHAR C:S) if(C<TEXT('0')||C>TEXT('9')) return false; return true; }
FString NormalizeInviteCode(const FString& S){ FString N=S.TrimStartAndEnd(); return IsValidInviteCode(N)?N:FString(); }
bool MatchesSessionMetadata(const FString& Code,int32 Version,int32 Open,const FString& Requested){ return IsValidInviteCode(Code)&&Code==NormalizeInviteCode(Requested)&&Version==FTunaSweeperOnlineCoopSettings::ProtocolVersion&&Open>0; }
}

ETunaSweeperOnlineCoopError TunaSweeperOnlineCoop::ClassifySession(const FString& Code, int32 Version, int32 Open, const FString& Requested)
{
    if (!IsValidInviteCode(Code) || Code != NormalizeInviteCode(Requested)) return ETunaSweeperOnlineCoopError::CodeNotFound;
    if (Version != FTunaSweeperOnlineCoopSettings::ProtocolVersion) return ETunaSweeperOnlineCoopError::VersionMismatch;
    if (Open <= 0) return ETunaSweeperOnlineCoopError::RoomFull;
    return ETunaSweeperOnlineCoopError::None;
}
