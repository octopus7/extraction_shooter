#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/TunaSweeperSlidingDoorActor.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSlidingDoorAudioTest,
	"TunaSweeper.Interaction.SlidingDoor.AudioAndMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSlidingDoorAudioTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("SlidingDoorAudioTest")),
		GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) { return false; }
	if (GEngine) { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		World->RemoveFromRoot();
	};

	ATunaSweeperSlidingDoorActor* Door = World->SpawnActor<ATunaSweeperSlidingDoorActor>();
	if (!TestNotNull(TEXT("Door"), Door)) { return false; }
	UAudioComponent* Audio = Door->FindComponentByClass<UAudioComponent>();
	if (!TestNotNull(TEXT("Owned audio component"), Audio)) { return false; }
	TestFalse(TEXT("No automatic playback on spawn"), Audio->bAutoActivate);
	TestTrue(TEXT("Spatial attenuation enabled"), Audio->bOverrideAttenuation && Audio->AttenuationOverrides.bAttenuate);
	TestEqual(TEXT("Quiet default volume"), Door->GetDoorSoundVolume(), 0.35f);

	Door->OpenDoor();
	USoundBase* OpeningSound = Audio->Sound;
	if (!TestNotNull(TEXT("Default opening SoundWave loads"), OpeningSound)) { return false; }
	TestEqual(TEXT("Opening asset"), OpeningSound->GetName(), FString(TEXT("SlidingDoor_Open")));
	TestTrue(TEXT("Opening duration matches motion"), FMath::IsNearlyEqual(OpeningSound->GetDuration(), 0.6f, 0.001f));
	Door->Tick(0.3f);
	TestTrue(TEXT("Halfway open"), FMath::IsNearlyEqual(Door->GetOpenAlpha(), 0.5f));
	Door->OpenDoor();
	TestTrue(TEXT("Repeated open preserves progress"), FMath::IsNearlyEqual(Door->GetOpenAlpha(), 0.5f));
	Door->SetDoorSoundVolume(0.12f);
	TestEqual(TEXT("Volume changes on component immediately"), Audio->VolumeMultiplier, 0.12f);
	Door->CloseDoor();
	if (!TestNotNull(TEXT("Default closing SoundWave loads"), Audio->Sound.Get())) { return false; }
	TestEqual(TEXT("Reversal selects closing sound"), Audio->Sound->GetName(), FString(TEXT("SlidingDoor_Close")));
	TestTrue(TEXT("Closing duration matches motion"), FMath::IsNearlyEqual(Audio->Sound->GetDuration(), 0.75f, 0.001f));
	Door->Tick(0.375f);
	TestEqual(TEXT("Reversed movement reaches closed"), Door->GetDoorState(), ETunaSweeperSlidingDoorState::Closed);
	TestFalse(TEXT("Completed door stops ticking"), Door->IsActorTickEnabled());
	Door->SetDoorSoundVolume(-1.0f);
	TestEqual(TEXT("Negative volume mutes safely"), Audio->VolumeMultiplier, 0.0f);
	Door->OpenDoor();
	Door->CloseDoor();
	TestFalse(TEXT("Reversal before first movement stops ticking"), Door->IsActorTickEnabled());
	Door->SetDoorOpen(true, true);
	TestEqual(TEXT("Instant open reaches target"), Door->GetOpenAlpha(), 1.0f);
	TestFalse(TEXT("Instant placement is silent"), Audio->IsPlaying());

	UClass* BlueprintClass = LoadClass<ATunaSweeperSlidingDoorActor>(nullptr,
		TEXT("/Game/Environment/Bunker/Agit/Door/BP_SlidingDoor.BP_SlidingDoor_C"));
	if (!TestNotNull(TEXT("Existing door Blueprint loads"), BlueprintClass)) { return false; }
	ATunaSweeperSlidingDoorActor* BlueprintDoor = World->SpawnActor<ATunaSweeperSlidingDoorActor>(BlueprintClass);
	if (!TestNotNull(TEXT("Existing Blueprint spawns"), BlueprintDoor)) { return false; }
	UAudioComponent* BlueprintAudio = BlueprintDoor->FindComponentByClass<UAudioComponent>();
	if (!TestNotNull(TEXT("Existing Blueprint inherits audio component"), BlueprintAudio)) { return false; }
	BlueprintDoor->SetDoorOpen(false, true);
	BlueprintDoor->OpenDoor();
	TestEqual(TEXT("Existing Blueprint inherits opening sound"), BlueprintAudio->Sound.Get(), OpeningSound);
	BlueprintDoor->SetDoorOpen(true, true);
	BlueprintDoor->CloseDoor();
	TestNotNull(TEXT("Existing Blueprint inherits closing sound"), BlueprintAudio->Sound.Get());
	return true;
}

#endif
