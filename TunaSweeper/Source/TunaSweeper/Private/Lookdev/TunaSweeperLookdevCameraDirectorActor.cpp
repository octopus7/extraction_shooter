#include "Lookdev/TunaSweeperLookdevCameraDirectorActor.h"

#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"

ATunaSweeperLookdevCameraDirectorActor::ATunaSweeperLookdevCameraDirectorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);
}

void ATunaSweeperLookdevCameraDirectorActor::BeginPlay()
{
	Super::BeginPlay();

	ElapsedSeconds = 0.0f;
	bRestartedEffect = false;
	ForceCameraView();

	if (bRestartEffectOnBeginPlay && EffectRestartDelaySeconds <= 0.0f)
	{
		RestartNiagaraEffect();
	}
}

void ATunaSweeperLookdevCameraDirectorActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bLockViewTargetEveryTick)
	{
		ForceCameraView();
	}

	if (!bRestartEffectOnBeginPlay || bRestartedEffect)
	{
		return;
	}

	ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	if (ElapsedSeconds >= EffectRestartDelaySeconds)
	{
		RestartNiagaraEffect();
	}
}

void ATunaSweeperLookdevCameraDirectorActor::ConfigureLookdev(
	ACameraActor* InForcedCameraActor,
	ANiagaraActor* InNiagaraEffectActor)
{
	ForcedCameraActor = InForcedCameraActor;
	NiagaraEffectActor = InNiagaraEffectActor;
}

void ATunaSweeperLookdevCameraDirectorActor::ForceCameraView() const
{
	if (!ForcedCameraActor)
	{
		return;
	}

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->SetViewTarget(ForcedCameraActor);
	}
}

void ATunaSweeperLookdevCameraDirectorActor::RestartNiagaraEffect()
{
	bRestartedEffect = true;

	if (!NiagaraEffectActor)
	{
		return;
	}

	UNiagaraComponent* NiagaraComponent = NiagaraEffectActor->GetNiagaraComponent();
	if (!NiagaraComponent)
	{
		return;
	}

	NiagaraComponent->Deactivate();
	NiagaraComponent->Activate(true);
}
