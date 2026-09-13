#include "Subsystem/TunaSweeperEffectCandidateSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaEffectCandidateCollectionTest,
	"TunaSweeper.Effects.CandidateCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaEffectCandidateCollectionTest::RunTest(const FString& Parameters)
{
	auto* GameInstance = NewObject<UGameInstance>();
	auto* Collector = NewObject<UTunaSweeperEffectCandidateSubsystem>(GameInstance);
	const FSoftObjectPath Path(TEXT("/Game/Tests/UnloadedCandidate.UnloadedCandidate"));
	TestNull(TEXT("Test candidate starts unloaded"), Path.ResolveObject());
	Collector->AddCandidate(Path, TEXT("Impact.Niagara"));
	Collector->AddCandidate(Path, TEXT("Explosion.Niagara"));
	Collector->AddCandidate(Path, TEXT("Impact.Niagara"));
	Collector->AddCandidate(FSoftObjectPath(), TEXT("Null"));
	Collector->AddCandidate(FSoftObjectPath(TEXT("/Engine/Transient.RuntimeMID")), TEXT("Transient"));
	Collector->AddCandidate(FSoftObjectPath(TEXT("/Game/Tests/Map.Map:PersistentLevel.Actor")), TEXT("Subobject"));
	TestEqual(TEXT("One entry per loadable asset path"), Collector->GetCandidates().Num(), 1);
	const auto* Candidate = Collector->GetCandidates().Find(Path);
	if (TestNotNull(TEXT("Candidate exists"), Candidate))
	{
		TestEqual(TEXT("Repeated requests counted"), Candidate->RequestCount, int64(3));
		TestEqual(TEXT("Call sites deduplicated"), Candidate->Sources.Num(), 2);
	}
	TestNull(TEXT("Collecting does not load candidate"), Path.ResolveObject());
	TSharedPtr<FJsonObject> Json;
	TestTrue(TEXT("Export is valid JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Collector->ToJson()), Json));
	if (Json)
	{
		TestEqual(TEXT("Export contains one candidate"), Json->GetArrayField(TEXT("candidates")).Num(), 1);
	}
	auto* OtherCollector = NewObject<UTunaSweeperEffectCandidateSubsystem>(NewObject<UGameInstance>());
	TestTrue(TEXT("Game instances have isolated collections"), OtherCollector->GetCandidates().IsEmpty());
	Collector->ResetCandidates();
	TestTrue(TEXT("Reset clears all candidates"), Collector->GetCandidates().IsEmpty());
	return true;
}
#endif
