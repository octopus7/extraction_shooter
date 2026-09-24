#if WITH_DEV_AUTOMATION_TESTS
#include "Online/TunaSweeperCoopStagingGameMode.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCoopPeerAdmissionTest, "TunaSweeper.OnlineCoop.Staging.AuthenticatedPeer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperCoopPeerAdmissionTest::RunTest(const FString& Parameters)
{
 using namespace TunaSweeperCoopStaging;
 const FString Product = TEXT("0123456789abcdef0123456789abcdef");
 const FString Id = TEXT("00000000000000000000000000000000|") + Product;
 TestTrue(TEXT("EOS Connect-only ID matches its authenticated transport peer"), MatchesAuthenticatedPeer(Id, TEXT("EOS:") + Product));
 TestFalse(TEXT("Claiming another lobby member ID cannot impersonate their P2P connection"), MatchesAuthenticatedPeer(Id, TEXT("EOS:11111111111111111111111111111111")));
 TestFalse(TEXT("IP fallback cannot authenticate a claimed EOS ID"), MatchesAuthenticatedPeer(Id, TEXT("127.0.0.1:7777")));
 TestFalse(TEXT("Invite code in URL is not authentication"), MatchesAuthenticatedPeer(Id, TEXT("EOS:") + Product + TEXT("?InviteCode=ABCDEFGH")));
 TestFalse(TEXT("Missing transport identity fails closed"), MatchesAuthenticatedPeer(Id, FString()));
 TestFalse(TEXT("Steam identity is not an EOS product user"), MatchesAuthenticatedPeer(TEXT("76561198000000000"), TEXT("EOS:") + Product));
 TestFalse(TEXT("Null product identity fails closed"), MatchesAuthenticatedPeer(TEXT("|00000000000000000000000000000000"), TEXT("EOS:00000000000000000000000000000000")));
 TestTrue(TEXT("Malformed product identity fails closed"), ProductUserIdFromUniqueId(TEXT("|0123456789abcdef0123456789abcdeg")).IsEmpty());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCoopStagingIsolationTest, "TunaSweeper.OnlineCoop.Staging.Isolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperCoopStagingIsolationTest::RunTest(const FString& Parameters)
{
 const ATunaSweeperCoopStagingGameMode* Mode = GetDefault<ATunaSweeperCoopStagingGameMode>();
 TestNull(TEXT("Staging never spawns a gameplay pawn"), Mode->DefaultPawnClass.Get());
 TestNull(TEXT("Staging has no gameplay HUD"), Mode->HUDClass.Get());
 TestEqual(TEXT("Staging uses its independent UI controller"), Mode->PlayerControllerClass.Get(), ATunaSweeperCoopStagingPlayerController::StaticClass());
 TestFalse(TEXT("Staging does not carry gameplay actors through seamless travel"), Mode->bUseSeamlessTravel);
 return true;
}
#endif
