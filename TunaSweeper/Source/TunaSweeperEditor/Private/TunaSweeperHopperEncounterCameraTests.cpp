#if WITH_DEV_AUTOMATION_TESTS
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Editor.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "TunaSweeperCollisionChannels.h"

namespace HopperEncounterTest
{
	enum class EStep { Far, FacingAway, Wall, Offscreen, Boarding, HeldAfterBoarding, FreshPress, Ejection, HeldAfterEjection, Interrupted, Takeover };

	class FPlay : public IAutomationLatentCommand
	{
	public:
		explicit FPlay(FAutomationTestBase* InTest) : Test(InTest) {}

		bool Update() override
		{
			if (FPlatformTime::Seconds() - Started > 65.)
			{
				Test->AddError(FString::Printf(TEXT("Hopper encounter timed out at step %d"), int32(Step)));
				return true;
			}
			UWorld* World = GEditor->PlayWorld;
			auto* PC = World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
			auto* Pawn = PC ? Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn()) : nullptr;
			if (!Pawn || !PC->PlayerCameraManager) return false;
			if (!Subject.IsValid())
			{
				for (TActorIterator<ATunaSweeperHopperEnemyCharacter> It(World); It; ++It) { Subject = *It; break; }
				if (!Subject.IsValid()) return false;
			}
			auto* Hopper = Subject.Get();
			const FVector Position = Hopper->GetActorLocation();
			const float Now = World->GetTimeSeconds();
			const float Elapsed = Now - Since;
			const auto Key = [PC](FKey K, EInputEvent Event, float Value = 1.f)
			{
				PC->InputKey(FInputKeyEventArgs::CreateSimulated(K, Event, Event == IE_Released ? 0.f : Value, 1));
			};
			auto* GI = World->GetGameInstance<UTunaSweeperGameInstance>();
			const auto Face = [Pawn](FVector Direction)
			{
				Pawn->SetActorRotation(Direction.Rotation());
				Pawn->SetAimWorldPoint(Pawn->GetActorLocation() + Direction * 600.f);
			};
			switch (Step)
			{
			case EStep::Far:
				if (Now < .25f) return false;
				Test->TestTrue(TEXT("Start beyond player sight"), Pawn->GetDistanceTo(Hopper) > 950.f);
				Test->TestEqual(TEXT("A distant Mk2 waits on the ground"), Hopper->GetCombatPhase(), EHopperCombatPhase::WaitingForSight);
				Test->TestTrue(TEXT("Pilot stands below the cockpit"), Hopper->GetPresentation()->GetPilotMesh()->GetComponentLocation().Z < Position.Z);
				OriginalView = PC->GetViewTarget();
				PC->SetIgnoreLookInput(true); // Independent owner; the encounter must not clear this lock.
				Pawn->SetActorLocation(Position + Hopper->GetActorForwardVector() * 300.f);
				Face(Hopper->GetActorForwardVector());
				break;
			case EStep::FacingAway:
				if (Elapsed < .25f) return false;
				Test->TestEqual(TEXT("Nearby but behind the player's field of view"), Hopper->GetCombatPhase(), EHopperCombatPhase::WaitingForSight);
				Wall = World->SpawnActor<AActor>();
				{
					auto* Box = NewObject<UBoxComponent>(Wall.Get());
					Wall->SetRootComponent(Box);
					Box->InitBoxExtent(FVector(30.f, 600.f, 250.f));
					Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					Box->SetCollisionResponseToAllChannels(ECR_Ignore);
					Box->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
					Box->RegisterComponent();
					Wall->SetActorLocation((Position + Pawn->GetActorLocation()) * .5f);
				}
				Face(-Hopper->GetActorForwardVector());
				Key(EKeys::W, IE_Pressed);
				Key(EKeys::RightMouseButton, IE_Pressed);
				Key(EKeys::Gamepad_LeftX, IE_Axis, .8f);
				break;
			case EStep::Wall:
				if (Elapsed < .3f) return false;
				Test->TestEqual(TEXT("The wall prevents boarding"), Hopper->GetCombatPhase(), EHopperCombatPhase::WaitingForSight);
				Test->TestTrue(TEXT("Movement input worked on approach"), Pawn->GetVelocity().Size2D() > 1.f);
				Test->TestTrue(TEXT("Aim held before the cut"), Pawn->IsAiming());
				OffscreenCamera = World->SpawnActor<ACameraActor>(Pawn->GetActorLocation(), Hopper->GetActorForwardVector().Rotation());
				PC->SetViewTarget(OffscreenCamera.Get());
				break;
			case EStep::Offscreen:
				if (Elapsed < .1f) return false;
				if (Wall.IsValid()) Wall->Destroy();
				if (Elapsed < .3f) return false;
				Test->TestEqual(TEXT("An unobstructed but offscreen cockpit still waits"), Hopper->GetCombatPhase(), EHopperCombatPhase::WaitingForSight);
				PC->SetViewTarget(OriginalView.Get());
				OffscreenCamera->Destroy();
				break;
			case EStep::Boarding:
				if (!PC->IsHopperShotActive()) return false;
				Test->TestEqual(TEXT("Seeing the mech starts boarding"), Hopper->GetCombatPhase(), EHopperCombatPhase::Boarding);
				Test->TestTrue(TEXT("Boarding cuts directly to a dedicated camera"),
					PC->GetViewTarget()->IsA<ACameraActor>() && !PC->PlayerCameraManager->PendingViewTarget.Target);
				Test->TestTrue(TEXT("Input and momentum clear immediately"), PC->IsMoveInputIgnored() && !Pawn->IsAiming() && Pawn->GetVelocity().Size2D() < 1.f);
				Still = Pawn->GetActorLocation();
				Magazine = GI->GetWeaponLoadedAmmoCount(1);
				Test->TestTrue(TEXT("Rifle has live rounds for the held-fire check"), Magazine > 0);
				Key(EKeys::W, IE_Released);
				Key(EKeys::W, IE_Pressed);
				Key(EKeys::LeftMouseButton, IE_Pressed);
				Key(EKeys::Tab, IE_Pressed);
				Key(EKeys::Tab, IE_Released);
				Key(EKeys::Gamepad_LeftX, IE_Axis, .8f);
				break;
			case EStep::HeldAfterBoarding:
				Key(EKeys::Gamepad_LeftX, IE_Axis, .8f);
				Key(EKeys::W, IE_Repeat);
				if (PC->IsHopperShotActive())
				{
					if (PC->GetViewTarget() == OriginalView.Get() && PC->IsMoveInputIgnored()) bSawLockedReturn = true;
					return false;
				}
				if (Elapsed < 4.65f) return false;
				Test->TestTrue(TEXT("Move lock lasts through the return blend"), bSawLockedReturn);
				Test->TestTrue(TEXT("Original view restored; independent look lock survives"),
					PC->GetViewTarget() == OriginalView.Get() && PC->IsLookInputIgnored() && !PC->IsMoveInputIgnored());
				Test->TestTrue(TEXT("Held and re-pressed controls stay neutral after boarding"),
					FVector::Dist2D(Still, Pawn->GetActorLocation()) < 2.f && !Pawn->IsAiming() && !PC->IsInventoryUiOpen()
					&& !PC->IsInputKeyDown(EKeys::W) && FMath::IsNearlyZero(PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX)));
				Test->TestEqual(TEXT("Holding fire consumed no ammunition"), GI->GetWeaponLoadedAmmoCount(1), Magazine);
				Key(EKeys::RightMouseButton, IE_Released);
				Key(EKeys::LeftMouseButton, IE_Released);
				Key(EKeys::W, IE_Released);
				Key(EKeys::Gamepad_LeftX, IE_Axis, 0.f);
				Key(EKeys::W, IE_Pressed);
				Key(EKeys::RightMouseButton, IE_Pressed);
				Key(EKeys::LeftMouseButton, IE_Pressed);
				break;
			case EStep::FreshPress:
				if (Elapsed < .28f) return false;
				Test->TestTrue(TEXT("A fresh press moves and aims normally"), Pawn->IsAiming() && FVector::Dist2D(Still, Pawn->GetActorLocation()) > 3.f);
				Test->TestTrue(TEXT("A fresh trigger press fires again"), GI->GetWeaponLoadedAmmoCount(1) < Magazine);
				Magazine = GI->GetWeaponLoadedAmmoCount(1);
				{
					const FVector PreviousPOV = PC->PlayerCameraManager->GetCameraLocation();
					Hopper->TakeDamage(10000.f, FDamageEvent(), PC, Pawn);
					Test->TestTrue(TEXT("Ejection blends from the actual gameplay camera"),
						PC->PlayerCameraManager->PendingViewTarget.Target && PC->PlayerCameraManager->ViewTarget.Target == OriginalView.Get()
						&& PC->PlayerCameraManager->GetCameraLocation().Equals(PreviousPOV, .1f));
				}
				Test->TestTrue(TEXT("Ejection immediately neutralizes held actions"), PC->IsMoveInputIgnored() && !Pawn->IsAiming());
				Still = Pawn->GetActorLocation();
				Key(EKeys::W, IE_Released);
				Key(EKeys::D, IE_Pressed);
				Key(EKeys::RightMouseButton, IE_Released);
				break;
			case EStep::Ejection:
				if (Elapsed < 1.f) return false;
				Test->TestTrue(TEXT("Ejection has blended into its own camera"),
					PC->IsViewingHopper(Hopper) && PC->GetViewTarget()->IsA<ACameraActor>() && !PC->PlayerCameraManager->PendingViewTarget.Target);
				break;
			case EStep::HeldAfterEjection:
				if (PC->IsHopperShotActive()) return false;
				Test->TestTrue(TEXT("Newly held eject key remains neutral at handback"), FVector::Dist2D(Still, Pawn->GetActorLocation()) < 2.f && !PC->IsInputKeyDown(EKeys::D));
				Test->TestEqual(TEXT("Ejection cancels the active repeating fire timer"), GI->GetWeaponLoadedAmmoCount(1), Magazine);
				Test->TestTrue(TEXT("Ejection returns to gameplay"), PC->GetViewTarget() == OriginalView.Get() && !PC->IsMoveInputIgnored());
				Key(EKeys::D, IE_Released);
				Key(EKeys::LeftMouseButton, IE_Released);
				{
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					Subject = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Hopper->GetClass(),
						Pawn->GetActorLocation() - Hopper->GetActorForwardVector() * 300.f, Hopper->GetActorRotation(), Params);
					Face(-Hopper->GetActorForwardVector());
				}
				break;
			case EStep::Interrupted:
				if (!PC->IsViewingHopper(Hopper)) return false;
				Hopper->TakeDamage(10000.f, FDamageEvent(), PC, Pawn);
				Test->TestTrue(TEXT("Ejection can interrupt boarding without releasing input"), PC->IsViewingHopper(Hopper) && PC->IsMoveInputIgnored() && Hopper->IsDead());
				Replacement = World->SpawnActor<ACameraActor>();
				PC->SetViewTarget(Replacement.Get());
				break;
			case EStep::Takeover:
				if (PC->IsHopperShotActive()) return false;
				Test->TestTrue(TEXT("Camera takeover releases only the encounter's locks"), PC->GetViewTarget() == Replacement.Get() && !PC->IsMoveInputIgnored() && PC->IsLookInputIgnored());
				PC->SetIgnoreLookInput(false);
				return true;
			}
			Step = EStep(int32(Step) + 1);
			Since = Now;
			return false;
		}
	private:
		FAutomationTestBase* Test;
		EStep Step = EStep::Far;
		double Started = FPlatformTime::Seconds();
		float Since = 0.f;
		int32 Magazine = 0;
		bool bSawLockedReturn = false;
		FVector Still = FVector::ZeroVector;
		TWeakObjectPtr<ATunaSweeperHopperEnemyCharacter> Subject;
		TWeakObjectPtr<AActor> OriginalView, Wall, OffscreenCamera, Replacement;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperEncounterCameraTest, "TunaSweeper.Hopper.Play.EncounterCameraAndInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperEncounterCameraTest::RunTest(const FString&)
{
	if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("Maps/HopperCombatTestMap.umap"), false, true)) return false;
	FRequestPlaySessionParams Play;
	Play.StartLocation = FVector(-610.f, 0.f, 110.f);
	Play.StartRotation = FRotator(0.f, 180.f, 0.f);
	Play.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>();
	GEditor->RequestPlaySession(Play);
	ADD_LATENT_AUTOMATION_COMMAND(HopperEncounterTest::FPlay(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
