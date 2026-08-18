#include "Camera/TunaSweeperLocationBlendCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ATunaSweeperLocationBlendCameraActor::ATunaSweeperLocationBlendCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = false;

	CameraRigRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRigRoot"));
	SetRootComponent(CameraRigRoot);

	LocationCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("LocationCamera"));
	LocationCamera->SetupAttachment(CameraRigRoot);
	LocationCamera->SetRelativeLocation(FVector(-600.0f, 0.0f, 900.0f));
	LocationCamera->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
	LocationCamera->FieldOfView = 70.0f;
	LocationCamera->bUsePawnControlRotation = false;

	BlendOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("BlendOrigin"));
	BlendOrigin->SetupAttachment(CameraRigRoot);

#if WITH_EDITORONLY_DATA
	BlendStartPreview = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("BlendStartPreview"));
	if (BlendStartPreview)
	{
		BlendStartPreview->SetupAttachment(BlendOrigin);
		BlendStartPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BlendStartPreview->SetGenerateOverlapEvents(false);
		BlendStartPreview->SetHiddenInGame(true);
		BlendStartPreview->ShapeColor = FColor(70, 150, 255);
		BlendStartPreview->bDrawOnlyIfSelected = true;
		BlendStartPreview->InitSphereRadius(BlendStartDistance);
	}

	BlendCompletePreview = CreateEditorOnlyDefaultSubobject<USphereComponent>(TEXT("BlendCompletePreview"));
	if (BlendCompletePreview)
	{
		BlendCompletePreview->SetupAttachment(BlendOrigin);
		BlendCompletePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BlendCompletePreview->SetGenerateOverlapEvents(false);
		BlendCompletePreview->SetHiddenInGame(true);
		BlendCompletePreview->ShapeColor = FColor(90, 230, 130);
		BlendCompletePreview->bDrawOnlyIfSelected = true;
		BlendCompletePreview->InitSphereRadius(BlendCompleteDistance);
	}
#endif
}

void ATunaSweeperLocationBlendCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!PlayerController || !ControlledPawn)
	{
		CurrentBlendWeight = 0.0f;
		return;
	}

	const FVector PlayerLocation = ControlledPawn->GetActorLocation();
	CurrentBlendWeight = bBlendEnabled ? GetBlendWeightAtLocation(PlayerLocation) : 0.0f;
	ATunaSweeperLocationBlendCameraActor* PreferredCamera = FindPreferredCamera(GetWorld(), PlayerLocation);
	AActor* CurrentViewTarget = PlayerController->GetViewTarget();

	if (PreferredCamera == this)
	{
		if (CurrentViewTarget != this && CanTakeViewTarget(*PlayerController))
		{
			PlayerController->SetViewTarget(this);
		}
		return;
	}

	if (CurrentViewTarget == this)
	{
		PlayerController->SetViewTarget(PreferredCamera ? static_cast<AActor*>(PreferredCamera) : ControlledPawn);
	}
}

void ATunaSweeperLocationBlendCameraActor::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!ControlledPawn || !LocationCamera)
	{
		Super::CalcCamera(DeltaTime, OutResult);
		return;
	}

	FMinimalViewInfo PlayerView;
	ControlledPawn->CalcCamera(DeltaTime, PlayerView);

	FMinimalViewInfo LocationView;
	LocationCamera->GetCameraView(DeltaTime, LocationView);

	CurrentBlendWeight = bBlendEnabled
		? GetBlendWeightAtLocation(ControlledPawn->GetActorLocation())
		: 0.0f;
	OutResult = PlayerView;
	OutResult.BlendViewInfo(LocationView, CurrentBlendWeight);
}

void ATunaSweeperLocationBlendCameraActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestorePawnViewTarget();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
bool ATunaSweeperLocationBlendCameraActor::IsDefaultPreviewEnabled() const
{
	return false;
}

void ATunaSweeperLocationBlendCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	BlendStartDistance = FMath::Max(1.0f, BlendStartDistance);
	BlendCompleteDistance = FMath::Clamp(BlendCompleteDistance, 0.0f, BlendStartDistance - 1.0f);
	RefreshEditorVisualization();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

float ATunaSweeperLocationBlendCameraActor::GetBlendWeightAtLocation(const FVector& WorldLocation) const
{
	if (!bBlendEnabled)
	{
		return 0.0f;
	}

	const float StartDistance = FMath::Max(1.0f, BlendStartDistance);
	const float CompleteDistance = FMath::Clamp(BlendCompleteDistance, 0.0f, StartDistance - 1.0f);
	const float Distance = GetDistanceToBlendOrigin(WorldLocation);
	float Weight = 1.0f - FMath::GetRangePct(CompleteDistance, StartDistance, Distance);
	Weight = FMath::Clamp(Weight, 0.0f, 1.0f);
	return bUseSmoothStep ? FMath::SmoothStep(0.0f, 1.0f, Weight) : Weight;
}

void ATunaSweeperLocationBlendCameraActor::SetBlendEnabled(bool bEnabled)
{
	bBlendEnabled = bEnabled;
	if (!bBlendEnabled)
	{
		CurrentBlendWeight = 0.0f;
		RestorePawnViewTarget();
	}
}

ATunaSweeperLocationBlendCameraActor* ATunaSweeperLocationBlendCameraActor::FindPreferredCamera(
	const UWorld* World,
	const FVector& PlayerLocation)
{
	if (!World)
	{
		return nullptr;
	}

	ATunaSweeperLocationBlendCameraActor* PreferredCamera = nullptr;
	float PreferredWeight = 0.0f;
	for (TActorIterator<ATunaSweeperLocationBlendCameraActor> It(World); It; ++It)
	{
		ATunaSweeperLocationBlendCameraActor* Candidate = *It;
		if (!IsValid(Candidate) || !Candidate->bBlendEnabled)
		{
			continue;
		}

		const float CandidateWeight = Candidate->GetBlendWeightAtLocation(PlayerLocation);
		if (CandidateWeight <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const bool bHigherPriority = !PreferredCamera || Candidate->Priority > PreferredCamera->Priority;
		const bool bSamePriorityHigherWeight = PreferredCamera
			&& Candidate->Priority == PreferredCamera->Priority
			&& CandidateWeight > PreferredWeight + KINDA_SMALL_NUMBER;
		const bool bStableTieBreak = PreferredCamera
			&& Candidate->Priority == PreferredCamera->Priority
			&& FMath::IsNearlyEqual(CandidateWeight, PreferredWeight)
			&& Candidate->GetUniqueID() < PreferredCamera->GetUniqueID();
		if (bHigherPriority || bSamePriorityHigherWeight || bStableTieBreak)
		{
			PreferredCamera = Candidate;
			PreferredWeight = CandidateWeight;
		}
	}

	return PreferredCamera;
}

bool ATunaSweeperLocationBlendCameraActor::CanTakeViewTarget(const APlayerController& PlayerController) const
{
	const AActor* CurrentViewTarget = PlayerController.GetViewTarget();
	return CurrentViewTarget == PlayerController.GetPawn()
		|| CurrentViewTarget == this
		|| Cast<ATunaSweeperLocationBlendCameraActor>(CurrentViewTarget) != nullptr;
}

float ATunaSweeperLocationBlendCameraActor::GetDistanceToBlendOrigin(const FVector& WorldLocation) const
{
	const FVector OriginLocation = BlendOrigin ? BlendOrigin->GetComponentLocation() : GetActorLocation();
	return bUse2DDistance
		? FVector::Dist2D(WorldLocation, OriginLocation)
		: FVector::Distance(WorldLocation, OriginLocation);
}

void ATunaSweeperLocationBlendCameraActor::RestorePawnViewTarget() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController && PlayerController->GetViewTarget() == this && PlayerController->GetPawn())
	{
		PlayerController->SetViewTarget(PlayerController->GetPawn());
	}
}

#if WITH_EDITOR
void ATunaSweeperLocationBlendCameraActor::RefreshEditorVisualization()
{
#if WITH_EDITORONLY_DATA
	if (BlendStartPreview)
	{
		BlendStartPreview->SetSphereRadius(FMath::Max(1.0f, BlendStartDistance));
	}
	if (BlendCompletePreview)
	{
		BlendCompletePreview->SetSphereRadius(FMath::Max(0.0f, BlendCompleteDistance));
	}
#endif
}
#endif
