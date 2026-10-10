#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestPresenceTest,
	"TunaSweeper.Quest.NpcPresence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestPresenceTest::RunTest(const FString& Parameters)
{
	using Role = ETunaSweeperQuestNpcPresenceRole;
	using State = ETunaSweeperQuestState;
	const auto Present = ATunaSweeperQuestPresenceNpcActor::EvaluatePresence;
	TestTrue(TEXT("Field NPC offers an unlocked quest"), Present(Role::FieldOffer, true, true, State::Available));
	TestFalse(TEXT("Locked quest hides field NPC"), Present(Role::FieldOffer, true, false, State::Available));
	TestFalse(TEXT("Missing definition never creates a phantom offer"), Present(Role::FieldOffer, false, true, State::Available));
	for (const State Progress : {State::Accepted, State::RewardAvailable, State::RewardCompleted})
	{
		TestFalse(TEXT("Field NPC disappears after acceptance"), Present(Role::FieldOffer, true, false, Progress));
	}
	for (const State Progress : {State::Available, State::Accepted, State::RewardAvailable})
	{
		TestFalse(TEXT("Base NPC waits for claimed reward"), Present(Role::BaseResident, true, false, Progress));
	}
	TestTrue(TEXT("Claimed reward reveals base NPC including on load"), Present(Role::BaseResident, true, false, State::RewardCompleted));
	TestFalse(TEXT("Retired definition does not reveal base NPC"), Present(Role::BaseResident, false, false, State::RewardCompleted));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Npc = World->SpawnActor<ATunaSweeperQuestPresenceNpcActor>();
	TestNotNull(TEXT("Presence NPC can spawn"), Npc);
	if (Npc)
	{
		Npc->RefreshQuestPresence();
		TestTrue(TEXT("Without quest subsystem NPC hides safely"), Npc->IsHidden());
		TestFalse(TEXT("Hidden NPC cannot block player"), Npc->GetActorEnableCollision());
	}
	World->DestroyWorld(false);
	return true;
}
#endif
