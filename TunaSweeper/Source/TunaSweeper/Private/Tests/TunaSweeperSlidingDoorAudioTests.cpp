#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/TunaSweeperSlidingDoorActor.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

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
	TestEqual(TEXT("35 percent default volume"), Door->GetDoorSoundVolume(), 0.35f);
	TestTrue(TEXT("Top-down camera height does not suppress nearby door"),
		Audio->AttenuationOverrides.Evaluate(FTransform::Identity, FVector(0.0f, 0.0f, 1500.0f)) > 0.99f);
	TestTrue(TEXT("Distant doors still fade out horizontally"),
		Audio->AttenuationOverrides.Evaluate(FTransform::Identity, FVector(3000.0f, 0.0f, 1500.0f)) < 0.01f);

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
	const FVector BlueprintLocation(1000.0, 0.0, 0.0);
	ATunaSweeperSlidingDoorActor* BlueprintDoor = World->SpawnActor<ATunaSweeperSlidingDoorActor>(
		BlueprintClass, FTransform(BlueprintLocation));
	if (!TestNotNull(TEXT("Existing Blueprint spawns"), BlueprintDoor)) { return false; }
	UAudioComponent* BlueprintAudio = BlueprintDoor->FindComponentByClass<UAudioComponent>();
	if (!TestNotNull(TEXT("Existing Blueprint inherits audio component"), BlueprintAudio)) { return false; }
	TestEqual(TEXT("Existing Blueprint inherits 35 percent volume"), BlueprintDoor->GetDoorSoundVolume(), 0.35f);
	TestTrue(TEXT("Existing Blueprint preserves top-down audibility"),
		BlueprintAudio->AttenuationOverrides.Evaluate(FTransform::Identity, FVector(0.0f, 0.0f, 1500.0f)) > 0.99f);

	auto CheckSignal = [this](ATunaSweeperSlidingDoorActor* Instance, const TCHAR* Phase, float Expected)
	{
		for (const TCHAR* Name : { TEXT("DoorFrameMesh"), TEXT("LeftDoorMesh"), TEXT("RightDoorMesh") })
		{
			const FString Label = FString::Printf(TEXT("%s / %s"), Phase, Name);
			const auto* Mesh = Cast<UStaticMeshComponent>(Instance->GetDefaultSubobjectByName(Name));
			if (!TestNotNull(Label, Mesh)) { continue; }
			const auto& Data = Mesh->GetCustomPrimitiveData().Data;
			if (TestTrue(Label + TEXT(" provides its material signal"), Data.IsValidIndex(0)))
			{
				TestEqual(Label + TEXT(" open signal"), Data[0], Expected);
			}
		}
	};

	const auto* Frame = Cast<UStaticMeshComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("DoorFrameMesh")));
	const auto* Left = Cast<UStaticMeshComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("LeftDoorMesh")));
	const auto* Right = Cast<UStaticMeshComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("RightDoorMesh")));
	const auto* LeftBox = Cast<UBoxComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("LeftPanelCollision")));
	const auto* RightBox = Cast<UBoxComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("RightPanelCollision")));
	const auto* Trigger = Cast<UBoxComponent>(BlueprintDoor->GetDefaultSubobjectByName(TEXT("ProximityTrigger")));
	if (!TestTrue(TEXT("Blueprint has a frame, panels, panel blockers and proximity trigger"),
		Frame && Left && Right && LeftBox && RightBox && Trigger)) { return false; }
	for (const auto* Part : { Frame, Left, Right })
	{
		TestEqual(Part->GetName() + TEXT(" material reads DoorSignalOpen at index 0"),
			Part->GetCustomPrimitiveDataIndexForScalarParameter(TEXT("DoorSignalOpen")), 0);
	}
	TestTrue(TEXT("Player proximity is sensed from both sides"), Trigger->GetGenerateOverlapEvents()
		&& Trigger->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
		&& Trigger->GetUnscaledBoxExtent().Equals(FVector(116.0, 180.0, 137.5)));
	const FTransform FrameAtRest = Frame->GetComponentTransform();
	auto CheckPose = [&](const FString& Phase, double PanelOffset, bool bPassageBlocked)
	{
		const FVector Center = BlueprintLocation + FVector(0.0, 0.0, 97.5);
		const FVector Side(PanelOffset, 0.0, 0.0);
		TestTrue(Phase + TEXT(" left panel and blocker"), Left->Bounds.Origin.Equals(Center - Side, 0.1)
			&& LeftBox->GetComponentLocation().Equals(Center - Side, 0.1));
		TestTrue(Phase + TEXT(" right panel and blocker"), Right->Bounds.Origin.Equals(Center + Side, 0.1)
			&& RightBox->GetComponentLocation().Equals(Center + Side, 0.1));
		TestTrue(Phase + TEXT(" stationary frame"), Frame->GetComponentTransform().Equals(FrameAtRest));
		const FVector Approach(0.0, 100.0, 0.0);
		TestEqual(Phase + TEXT(" blocks a player capsule through the center"), World->SweepTestByChannel(
			Center - Approach, Center + Approach, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(34.f, 88.f)),
			bPassageBlocked);
	};

	CheckSignal(BlueprintDoor, TEXT("Initially closed"), 0.f);
	BlueprintDoor->DispatchBeginPlay();
	CheckPose(TEXT("Closed"), 33.0, true);
	BlueprintDoor->OpenDoor();
	TestEqual(TEXT("Existing Blueprint inherits opening sound"), BlueprintAudio->Sound.Get(), OpeningSound);
	CheckSignal(BlueprintDoor, TEXT("Opening before first tick"), 1.f);
	BlueprintDoor->Tick(0.3f);
	CheckPose(TEXT("Halfway open"), 69.0, false);
	CheckSignal(BlueprintDoor, TEXT("Halfway open"), 1.f);
	BlueprintDoor->CloseDoor();
	TestNotNull(TEXT("Existing Blueprint inherits closing sound"), BlueprintAudio->Sound.Get());
	CheckSignal(BlueprintDoor, TEXT("Reversed to closing"), 0.f);
	BlueprintDoor->Tick(0.1875f);
	CheckPose(TEXT("Partly closed after reversal"), 44.25, true);
	CheckSignal(BlueprintDoor, TEXT("Partly closed after reversal"), 0.f);
	BlueprintDoor->ToggleDoor();
	CheckSignal(BlueprintDoor, TEXT("Reversed to opening"), 1.f);
	BlueprintDoor->Tick(0.6f);
	CheckPose(TEXT("Fully open"), 105.0, false);
	CheckSignal(BlueprintDoor, TEXT("Open at rest"), 1.f);
	BlueprintDoor->CloseDoor();
	BlueprintDoor->OpenDoor();
	CheckSignal(BlueprintDoor, TEXT("Reopened at the open endpoint"), 1.f);
	BlueprintDoor->CloseDoor();
	BlueprintDoor->Tick(0.75f);
	CheckPose(TEXT("Returned to closed"), 33.0, true);
	CheckSignal(BlueprintDoor, TEXT("Closed at rest"), 0.f);
	BlueprintDoor->OpenDoor();
	BlueprintDoor->CloseDoor();
	CheckSignal(BlueprintDoor, TEXT("Reclosed at the closed endpoint"), 0.f);
	BlueprintDoor->SetDoorOpen(true, true);
	CheckSignal(BlueprintDoor, TEXT("Instantly opened"), 1.f);
	BlueprintDoor->SetDoorOpen(false, true);
	CheckSignal(BlueprintDoor, TEXT("Instantly closed"), 0.f);

	const FTransform OpenPlacement(FVector(2000.0, 0.0, 0.0));
	auto* StartsOpenDoor = World->SpawnActorDeferred<ATunaSweeperSlidingDoorActor>(BlueprintClass, OpenPlacement);
	auto* StartsOpen = FindFProperty<FBoolProperty>(BlueprintClass, TEXT("bStartsOpen"));
	if (!TestTrue(TEXT("Start-open placement available"), StartsOpenDoor && StartsOpen)) { return false; }
	StartsOpen->SetPropertyValue_InContainer(StartsOpenDoor, true);
	StartsOpenDoor->FinishSpawning(OpenPlacement);
	CheckSignal(StartsOpenDoor, TEXT("Placed starting open"), 1.f);
	StartsOpenDoor->SetDoorOpen(false, true);
	StartsOpenDoor->DispatchBeginPlay();
	CheckSignal(StartsOpenDoor, TEXT("Starts open in play"), 1.f);
	CheckSignal(BlueprintDoor, TEXT("Other instance stays closed"), 0.f);

	auto* AlternatePanel = Cast<UStaticMeshComponent>(StartsOpenDoor->GetDefaultSubobjectByName(TEXT("LeftDoorMesh")));
	if (!TestNotNull(TEXT("Replaceable panel"), AlternatePanel)) { return false; }
	for (int32 Slot = 0; Slot < AlternatePanel->GetNumMaterials(); ++Slot)
	{
		AlternatePanel->SetMaterial(Slot, UMaterial::GetDefaultMaterial(MD_Surface));
	}
	AlternatePanel->SetCustomPrimitiveDataFloat(0, 0.375f);
	StartsOpenDoor->CloseDoor();
	TestEqual(TEXT("An alternate panel material retains its own primitive data"),
		AlternatePanel->GetCustomPrimitiveData().Data[0], 0.375f);
	return true;
}

#endif
