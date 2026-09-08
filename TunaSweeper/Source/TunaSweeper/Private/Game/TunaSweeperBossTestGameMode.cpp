#include "Game/TunaSweeperBossTestGameMode.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Combat/TunaSweeperBossEncounter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperBossTestGameMode::ATunaSweeperBossTestGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDClass = ATunaSweeperBossTestHUD::StaticClass();
	PlayerControllerClass = ATunaSweeperBossTestPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> PlayerBlueprint(TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter"));
	if (PlayerBlueprint.Succeeded()) DefaultPawnClass = PlayerBlueprint.Class;
}

void ATunaSweeperBossTestGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>()) Instance->BeginCombatTestSession();
}

void ATunaSweeperBossTestGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ATunaSweeperBossTestGameMode::ReturnToHub);
	}
	PreparePlayer();
}

void ATunaSweeperBossTestGameMode::PreparePlayer()
{
	if (auto* Player = Cast<ATunaSweeperTopDownCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		if (auto* Vitals = Player->GetVitalsComponent())
		{
			FTunaSweeperVitalsState State = Vitals->GetVitalsState();
			State.Health = State.MaxHealth;
			State.Food = State.MaxFood;
			State.Hydration = State.MaxHydration;
			Vitals->SetVitalsState(State);
			FTunaSweeperVitalsDepletionRates Rates;
			Rates.FoodPerSecond = Rates.HydrationPerSecond = 0.0f;
			Vitals->SetBaseDepletionRates(Rates);
		}
		Player->SelectWeaponSlot(1);
	}
}

void ATunaSweeperBossTestGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* Player = Cast<ATunaSweeperTopDownCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Player && Player->IsDead()) ReturnToHub();
}

void ATunaSweeperBossTestGameMode::ReturnToHub()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;
	PC->StopMovement();
	for (TActorIterator<ATunaSweeperBossEncounter> It(GetWorld()); It; ++It) It->ResetEncounter();
	if (APawn* Pawn = PC->GetPawn())
	{
		// Owner destruction does not destroy separate weapon/projectile actors in Unreal.
		TArray<TWeakObjectPtr<AActor>> OwnedActors;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			TSet<AActor*> Visited;
			for (AActor* Parent = It->GetOwner(); Parent && !Visited.Contains(Parent); Parent = Parent->GetOwner())
			{
				if (Parent == Pawn) { OwnedActors.Add(*It); break; }
				Visited.Add(Parent);
			}
		}
		for (const auto& Entry : OwnedActors) if (AActor* Actor = Entry.Get()) Actor->Destroy();
		PC->UnPossess();
		Pawn->Destroy();
	}
	if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>()) Instance->ResetCombatTestLoadout();
	PC->ResetIgnoreMoveInput();
	PC->ResetIgnoreLookInput();
	RestartPlayer(PC);
	PreparePlayer();
	UE_LOG(LogTemp, Display, TEXT("BossTest: returned to hub, encounters reset and supplies restored."));
}

void ATunaSweeperBossTestGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Instance = GetGameInstance<UTunaSweeperGameInstance>()) Instance->EndCombatTestSession();
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperBossTestHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas || !PlayerOwner || !PlayerOwner->GetPawn()) return;
	const FVector Location = PlayerOwner->GetPawn()->GetActorLocation();
	ATunaSweeperBossEncounter* Nearest = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ATunaSweeperBossEncounter> It(GetWorld()); It; ++It)
	{
		const float Distance = FVector::DistSquared2D(It->GetActorLocation(), Location);
		if (Distance < BestDistance) { BestDistance = Distance; Nearest = *It; }
	}
	const bool bAtHub = !Nearest || BestDistance > FMath::Square(7000.0f);
	FString Title = TEXT("BOSS COMBAT LAB  |  PORTAL HUB");
	FString Status = TEXT("Approach a portal and press F. Choose 01 Charge / 02 Robots / 03 Main boss.");
	FLinearColor Accent(0.12f, 0.9f, 0.82f);
	if (!bAtHub)
	{
		Title = Nearest->DisplayName.ToString();
		switch (Nearest->GetState())
		{
		case ETunaSweeperBossEncounterState::Active:
			Status = TEXT("COMBAT ACTIVE  |  Retreat past the orange line to reset.");
			Accent = FLinearColor(1.0f, 0.4f, 0.12f);
			break;
		case ETunaSweeperBossEncounterState::Cleared:
			Status = TEXT("CLEARED  |  Leave the arena, then re-enter for another attempt.");
			break;
		default:
			Status = TEXT("SAFE STAGING  |  Take your time. Cross the orange line to begin.");
			break;
		}
	}
	const float Width = FMath::Min(850.0f, Canvas->ClipX - 40.0f);
	const float Left = (Canvas->ClipX - Width) * 0.5f;
	DrawRect(FLinearColor(0.025f, 0.035f, 0.05f, 0.92f), Left, 18.0f, Width, 96.0f);
	DrawRect(Accent, Left, 18.0f, 4.0f, 96.0f);
	DrawText(Title, Accent, Left + 18, 27, nullptr, 1.25f);
	DrawText(Status, FLinearColor::White, Left + 18, 55, nullptr, 1.0f);
	DrawText(TEXT("WASD / MMB move | LMB fire | RMB aim | R reload | F portal | Home: hub + refill"),
		FLinearColor(0.7f, 0.77f, 0.84f), Left + 18, 82, nullptr, 0.95f);
}

void ATunaSweeperBossTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ATunaSweeperBossTestPlayerController::MoveToClickedFloor);
}

void ATunaSweeperBossTestPlayerController::MoveToClickedFloor()
{
	if (IsMoveInputIgnored() || IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeOpen() || !GetPawn()) return;
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && Hit.ImpactNormal.Z > 0.7f)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.ImpactPoint);
	}
}

void ATunaSweeperBossTestPlayerController::PlayerTick(float DeltaSeconds)
{
	// Cancel the optional path before warps and whenever the player takes manual control.
	if (IsMoveInputIgnored() || IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeOpen() ||
		IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::A) ||
		IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::D)) StopMovement();
	Super::PlayerTick(DeltaSeconds);
}
