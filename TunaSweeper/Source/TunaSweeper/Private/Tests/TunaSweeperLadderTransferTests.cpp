#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVerticalOcclusionRevealComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/TunaSweeperLadderTransferActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"

namespace TunaSweeperLadderTests
{
	AActor* AddBox(UWorld* World, const FVector& Location, const FVector& Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLadderTransferValidationTest,
	"TunaSweeper.Ladder.TransferValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperLadderTransferValidationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Entry = World->SpawnActor<ATunaSweeperLadderTransferActor>(Spawn);
	auto* Exit = World->SpawnActor<ATunaSweeperLadderTransferActor>(FVector(700, 0, 300), FRotator(12, 70, 9), Spawn);
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(Spawn);
	auto* Controller = World->SpawnActor<APlayerController>(Spawn);
	if (!TestNotNull(TEXT("Entry exists"), Entry) || !TestNotNull(TEXT("Exit exists"), Exit) ||
		!TestNotNull(TEXT("Player exists"), Player) || !TestNotNull(TEXT("Controller exists"), Controller)) return false;
	// This unstarted fixture world does not call controller PostInitializeComponents.
	World->AddController(Controller);
	// The authored floor endpoints remain valid for a player with a non-default capsule and scale.
	Player->GetCapsuleComponent()->SetCapsuleSize(40, 110);
	Player->SetActorScale3D(FVector(1.25));
	Player->SetActorLocation(FVector(0, 0, 139.5));
	const FVector Start = Player->GetActorLocation();
	TestFalse(TEXT("Missing pawn is rejected"), Entry->TryTransferPlayer(nullptr));
	Controller->Possess(Player);
	TestFalse(TEXT("Missing endpoint is rejected"), Entry->TryTransferPlayer(Player));
	Entry->SetTargetEndpoint(Entry);
	TestFalse(TEXT("Self endpoint is rejected"), Entry->TryTransferPlayer(Player));
	Entry->SetTargetEndpoint(Exit);
	TestFalse(TEXT("One-way pairing is rejected"), Entry->TryTransferPlayer(Player));
	Exit->SetTargetEndpoint(Entry);
	TestFalse(TEXT("An endpoint without supporting ground is rejected"), Entry->TryTransferPlayer(Player));
	TunaSweeperLadderTests::AddBox(World, FVector(0, 0, -10), FVector(250, 250, 10));
	TunaSweeperLadderTests::AddBox(World, FVector(700, 0, 290), FVector(250, 250, 10));
	Controller->UnPossess();
	TestFalse(TEXT("An unpossessed character cannot use the player ladder"), Entry->TryTransferPlayer(Player));
	Controller->Possess(Player);
	Player->SetActorLocation(FVector(0, 0, 439.5));
	TestFalse(TEXT("Same XY on another floor cannot use this endpoint"), Entry->CanTransferPlayer(Player));
	Player->SetActorLocation(FVector(500, 0, 139.5));
	TestFalse(TEXT("A distant direct API call is rejected"), Entry->TryTransferPlayer(Player));
	Player->SetActorLocation(Start);
	Exit->SetActorHiddenInGame(true);
	TestFalse(TEXT("Hidden destination is rejected"), Entry->TryTransferPlayer(Player));
	Exit->SetActorHiddenInGame(false);
	auto* Blocker = TunaSweeperLadderTests::AddBox(World, FVector(700, 0, 440), FVector(60, 60, 140));
	Player->GetCharacterMovement()->Velocity = FVector(50, 0, 0);
	TestFalse(TEXT("Destination hull collision blocks transfer"), Entry->TryTransferPlayer(Player));
	TestTrue(TEXT("Blocked transfer leaves the player at the source"), Player->GetActorLocation().Equals(Start));
	TestTrue(TEXT("Blocked transfer preserves movement"), Player->GetCharacterMovement()->Velocity.Equals(FVector(50, 0, 0)));
	Blocker->Destroy();
	TestTrue(TEXT("The fixture player is available to the existing interaction subsystem"), UGameplayStatics::GetPlayerPawn(World, 0) == Player);
	TestTrue(TEXT("A clear supported endpoint accepts the actual player capsule"), Entry->CanTransferPlayer(Player));
	TestTrue(TEXT("The existing interaction subsystem offers the clear endpoint"), World->GetSubsystem<UTunaSweeperInteractionSubsystem>()->CanOfferInteraction(Entry->GetInteractableComponent()));
	if (!TestTrue(TEXT("The existing interaction dispatcher enters the upper deck"), Entry->RequestInteraction(Player))) return false;
	TestTrue(TEXT("Destination uses the actual scaled capsule half-height"), Player->GetActorLocation().Equals(FVector(700, 0, 439.5), 0.1));
	TestTrue(TEXT("Tilting the ship does not tilt the player capsule"), Player->GetActorRotation().Equals(FRotator(0, 70, 0), 0.1));
	TestTrue(TEXT("Successful transfer stops movement"), Player->GetCharacterMovement()->Velocity.IsNearlyZero());
	TestTrue(TEXT("Possession remains on the same character"), Controller->GetPawn() == Player);
	TestFalse(TEXT("Outer endpoint is unavailable from the upper deck"), Entry->CanTransferPlayer(Player));
	if (!TestTrue(TEXT("The paired interaction returns to the ground"), Exit->RequestInteraction(Player))) return false;
	TestTrue(TEXT("Round trip returns to the authored source floor"), Player->GetActorLocation().Equals(Start, 0.1));
	Exit->Destroy();
	TestFalse(TEXT("Destroyed destination is rejected"), Entry->TryTransferPlayer(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperVerticalRevealMaterialPreservationTest,
	"TunaSweeper.Ladder.VerticalRevealMaterialPreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperVerticalRevealMaterialPreservationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		World->RemoveFromRoot();
	};
	UMaterialInterface* RevealMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Effects/M_OcclusionVerticalRevealMasked.M_OcclusionVerticalRevealMasked"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Existing boundary dissolve material loads"), RevealMaterial) ||
		!TestNotNull(TEXT("Fixture mesh loads"), Cube)) return false;
	auto* Owner = World->SpawnActor<AActor>();
	auto* Mesh = NewObject<UStaticMeshComponent>(Owner);
	Owner->SetRootComponent(Mesh);
	Mesh->SetStaticMesh(Cube);
	Mesh->RegisterComponent();
	auto* Palette = UMaterialInstanceDynamic::Create(RevealMaterial, Owner);
	Palette->SetScalarParameterValue(TEXT("VerticalRevealFadeHeightCm"), 72.0f);
	Mesh->SetMaterial(0, Palette);
	auto* Reveal = NewObject<UTunaSweeperVerticalOcclusionRevealComponent>(Owner);
	Owner->AddInstanceComponent(Reveal);
	Reveal->bPreserveSourceMaterials = true;
	Reveal->RegisterComponent();
	auto* Preserved = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
	if (!TestNotNull(TEXT("Palette receives a runtime dissolve instance"), Preserved)) return false;
	float ActualFadeHeight = 0;
	TestTrue(TEXT("Source material values are retained by the runtime reveal instance"),
		Preserved->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VerticalRevealFadeHeightCm")), ActualFadeHeight) && FMath::IsNearlyEqual(ActualFadeHeight, 72.0f));
	TestTrue(TEXT("Runtime changes do not mutate an authored dynamic source"), Preserved != Palette);
	Preserved->SetScalarParameterValue(TEXT("VerticalRevealFadeHeightCm"), 30.0f);
	TestTrue(TEXT("The source retains its original material parameters"),
		Palette->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VerticalRevealFadeHeightCm")), ActualFadeHeight) && FMath::IsNearlyEqual(ActualFadeHeight, 72.0f));
	Reveal->ApplyVerticalRevealMaterial();
	TestTrue(TEXT("Repeated application keeps the same runtime instance"), Mesh->GetMaterial(0) == Preserved);
	Reveal->RestoreOriginalMaterials();
	TestTrue(TEXT("Restore returns the exact authored palette"), Mesh->GetMaterial(0) == Palette);
	Reveal->bPreserveSourceMaterials = false;
	Reveal->ApplyVerticalRevealMaterial();
	auto* DefaultInstance = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
	TestTrue(TEXT("Default behavior still uses the configured shared reveal material"), DefaultInstance && DefaultInstance->Parent == RevealMaterial);
	Reveal->RestoreOriginalMaterials();
	UMaterialInterface* Unsupported = Cube->GetMaterial(0);
	Mesh->SetMaterial(0, Unsupported);
	Reveal->bPreserveSourceMaterials = true;
	Reveal->ApplyVerticalRevealMaterial();
	auto* Fallback = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
	TestTrue(TEXT("Materials without the dissolve parameters use the existing fallback"), Fallback && Fallback->Parent == RevealMaterial);
	Reveal->RestoreOriginalMaterials();
	TestTrue(TEXT("Fallback restore retains unsupported original material"), Mesh->GetMaterial(0) == Unsupported);
	return true;
}

#endif
