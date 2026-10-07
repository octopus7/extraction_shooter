#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCameraRelativeMovementTest,
	"TunaSweeper.Player.Movement.CameraRelative", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperCameraRelativeMovementTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		World->RemoveFromRoot();
	};
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>(
		ATunaSweeperTopDownCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Character);
	Controller->PlayerCameraManager = World->SpawnActor<APlayerCameraManager>();
	if (!TestNotNull(TEXT("Camera manager"), Controller->PlayerCameraManager.Get())) return false;
	// Deliberately different from the rendered POV and character facing.
	Controller->SetControlRotation(FRotator(0, -120, 0));
	Character->SetActorRotation(FRotator(0, 170, 0));
	auto SetView = [&](float Yaw, float Pitch)
	{
		FMinimalViewInfo View;
		View.Rotation = FRotator(Pitch, Yaw, 0);
		Controller->PlayerCameraManager->FillCameraCache(View);
	};
	for (const float Yaw : {0.f, 22.5f, 45.f, 90.f, -90.f, 180.f})
	{
		SetView(Yaw, -48.f);
		const float Angle = FMath::DegreesToRadians(Yaw);
		const FVector Forward(FMath::Cos(Angle), FMath::Sin(Angle), 0);
		const FVector Right(-FMath::Sin(Angle), FMath::Cos(Angle), 0);
		for (const FVector2D Input : {FVector2D(0, 1), FVector2D(0, -1), FVector2D(1, 0), FVector2D(-1, 0), FVector2D(1, 1), FVector2D(.2, .3)})
		{
			Character->ConsumeMovementInputVector();
			Character->HandleMove(FInputActionValue(Input));
			const FVector Expected = (Forward * Input.Y + Right * Input.X).GetClampedToMaxSize(1.f);
			TestTrue(FString::Printf(TEXT("POV yaw %.1f maps input %s to horizontal movement"), Yaw, *Input.ToString()),
				Character->GetPendingMovementInputVector().Equals(Expected, .0001f));
			TestTrue(TEXT("Roll follows the same camera-relative input"), Character->ResolveRollDirection().Equals(Expected.GetSafeNormal(), .0001f));
		}
	}
	// A camera blend can advance while a direction remains held, before another move callback.
	SetView(0, -48);
	Character->HandleMove(FInputActionValue(FVector2D(0, 1)));
	SetView(90, -90);
	TestTrue(TEXT("Roll resolves held input using current POV even at vertical pitch"), Character->ResolveRollDirection().Equals(FVector::RightVector, .0001f));
	Character->ConsumeMovementInputVector();
	Character->HandleMove(FInputActionValue(FVector2D(0, 1)));
	TestTrue(TEXT("Vertical camera keeps full horizontal speed"), Character->ConsumeMovementInputVector().Equals(FVector::RightVector, .0001f));
	Character->HandleMoveStopped(FInputActionValue(FVector2D::ZeroVector));
	Character->AimDirection = FVector(-1, 0, 0);
	TestTrue(TEXT("Idle roll retains aim fallback"), Character->ResolveRollDirection().Equals(FVector(-1, 0, 0)));
	Character->HandleMove(FInputActionValue(FVector2D::ZeroVector));
	TestTrue(TEXT("Released input has no movement"), Character->ConsumeMovementInputVector().IsNearlyZero());
	return true;
}

#endif
