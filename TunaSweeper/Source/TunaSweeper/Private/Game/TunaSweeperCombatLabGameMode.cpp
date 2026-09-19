#include "Game/TunaSweeperCombatLabGameMode.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperCombatLabAutopilotComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "UI/TunaSweeperUIFont.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatLab, Log, All);
namespace
{
FText LabText(const UObject* Context, FName Key)
{
	const auto* GI = Cast<UTunaSweeperGameInstance>(UGameplayStatics::GetGameInstance(Context));
	return GI ? GI->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
}
}
ATunaSweeperCombatLabGameMode::ATunaSweeperCombatLabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDClass = ATunaSweeperCombatLabHUD::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> PawnBP(TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter"));
	if (PawnBP.Succeeded()) DefaultPawnClass = PawnBP.Class;
	static ConstructorHelpers::FClassFinder<ATunaSweeperEnemyCharacter> EnemyBP(TEXT("/Game/Blueprints/BP_QuadrupedGunEnemy"));
	if (EnemyBP.Succeeded()) EnemyClass = EnemyBP.Class;
}
void ATunaSweeperCombatLabGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>()) GI->BeginCombatTestSession();
}
void ATunaSweeperCombatLabGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		EnableInput(PC);
		InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ATunaSweeperCombatLabGameMode::ResetRound);
		InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ATunaSweeperCombatLabGameMode::ToggleAutopilot);
	}
	ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CombatLab"), FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT("_rounds.csv"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath), true);
	FFileHelper::SaveStringToFile(TEXT("round,result,seconds,player_projectiles,enemy_projectiles,damage_samples,damage_dealt,damage_taken,distance_cm,idle_seconds,rolls,ai\n"), *ReportPath);
	SpawnHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ATunaSweeperCombatLabGameMode::HandleSpawned));
	NextRoundAt = GetWorld()->GetTimeSeconds() + 1.0f;
}
TArray<ATunaSweeperEnemyCharacter*> ATunaSweeperCombatLabGameMode::GetAliveEnemies() const
{
	TArray<ATunaSweeperEnemyCharacter*> Result;
	for (const auto& Enemy : Enemies) if (Enemy.IsValid() && !Enemy->IsDead()) Result.Add(Enemy.Get());
	return Result;
}
bool ATunaSweeperCombatLabGameMode::BelongsToRound(AActor* Actor) const
{
	TSet<const AActor*> Seen;
	TArray<AActor*> Pending{ Actor };
	while (!Pending.IsEmpty())
	{
		AActor* Current = Pending.Pop();
		if (!Current || Seen.Contains(Current)) continue;
		Seen.Add(Current);
		// Player controller owns the persistent camera/HUD; the mode owns GameState.
		// Neither is a disposable combat root.
		if (Current == this || Cast<APlayerController>(Current) || Cast<APlayerState>(Current)) continue;
		if (Current == Player.Get()) return true;
		for (const auto& Enemy : Enemies) if (Current == Enemy.Get()) return true;
		Pending.Add(Current->GetOwner()); Pending.Add(Current->GetInstigator());
		if (const auto* Controller = Cast<AController>(Current)) Pending.Add(Controller->GetPawn());
	}
	return false;
}
void ATunaSweeperCombatLabGameMode::HandleSpawned(AActor* Actor)
{
	if (bCleaning || !Actor || !BelongsToRound(Actor)) return;
	RoundActors.Add(Actor);
	if (bRoundActive && Cast<ATunaSweeperProjectile>(Actor))
	{
		if (Actor->GetInstigator() == Player.Get()) ++PlayerProjectiles;
		else if (Cast<ATunaSweeperEnemyCharacter>(Actor->GetInstigator())) ++EnemyProjectiles;
	}
}
void ATunaSweeperCombatLabGameMode::CleanupRound()
{
	bCleaning = true;
	if (Pilot) Pilot->SetAutopilotEnabled(false);
	Pilot = nullptr;
	if (!Player.IsValid()) Player = Cast<ATunaSweeperTopDownCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		if (BelongsToRound(*It)) RoundActors.Add(*It);
	if (auto* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->StopMovement(); PC->UnPossess(); }
	for (auto Entry : RoundActors)
	{
		AActor* Actor = Entry.Get();
		if (!Actor || Actor == this || Cast<APlayerController>(Actor) || Cast<APlayerState>(Actor)) continue;
		Actor->SetActorEnableCollision(false); Actor->SetActorTickEnabled(false); Actor->Destroy();
	}
	RoundActors.Reset(); Player.Reset(); Enemies.Reset(); EnemyHealth.Reset();
	bCleaning = false;
}
void ATunaSweeperCombatLabGameMode::StartRound()
{
	CleanupRound();
	auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
	auto* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!GI || !PC || !EnemyClass) { NextRoundAt = TNumericLimits<float>::Max(); ResultKey = TEXT("ui.combat_lab.error"); return; }
	GI->ResetCombatTestLoadout();
	PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
	RestartPlayer(PC);
	Player = Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn());
	if (!Player.IsValid()) { NextRoundAt = TNumericLimits<float>::Max(); ResultKey = TEXT("ui.combat_lab.error"); return; }
	RoundActors.Add(Player.Get());
	if (auto* Vitals = Player->GetVitalsComponent())
	{
		auto State = Vitals->GetVitalsState();
		State.Health = State.MaxHealth; State.Food = State.MaxFood; State.Hydration = State.MaxHydration;
		Vitals->SetVitalsState(State); LastHealth = State.Health;
		FTunaSweeperVitalsDepletionRates Rates; Rates.FoodPerSecond = Rates.HydrationPerSecond = 0; Vitals->SetBaseDepletionRates(Rates);
	}
	Player->SelectWeaponSlot(1);
	TArray<ATargetPoint*> Markers;
	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It) if (It->ActorHasTag(TEXT("CombatLabEnemy"))) Markers.Add(*It);
	Markers.Sort([](const ATargetPoint& A, const ATargetPoint& B) { return A.GetName() < B.GetName(); });
	for (int32 Index = 0; Index < FMath::Min(3, Markers.Num()); ++Index)
	{
		const FTransform Transform = Markers[Index]->GetActorTransform();
		auto* Enemy = GetWorld()->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(EnemyClass, Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (!Enemy) continue;
		Enemies.Add(Enemy); RoundActors.Add(Enemy);
		FTunaSweeperEnemyCombatProfile Profile;
		if (auto* Spawns = GI->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>())
			if (Spawns->TryGetEnemyCombatProfile(TEXT("enemy.rifle_anchor"), Profile))
				Enemy->ConfigureCombatProfile(Profile, 10, FName(*FString::Printf(TEXT("lab.%d"), Index)), 0);
		Enemy->ConfigureSpawnData({}, TEXT("enemy.combat_lab"), INDEX_NONE, INDEX_NONE, 60.f, 0, 0, 0, 1002, 2002, 120);
		Enemy->FinishSpawning(Transform);
		EnemyHealth.Add(Enemy, Enemy->GetHealth());
	}
	if (Enemies.Num() != 3)
	{
		UE_LOG(LogCombatLab, Error, TEXT("Expected 3 enemy markers/spawns; obtained %d."), Enemies.Num());
		CleanupRound(); ResultKey = TEXT("ui.combat_lab.error"); NextRoundAt = TNumericLimits<float>::Max(); return;
	}
	++Round; Elapsed = Distance = IdleSeconds = DamageTaken = DamageDealt = 0;
	PlayerProjectiles = EnemyProjectiles = Hits = 0;
	PreviousLocation = Player->GetActorLocation(); NextSampleAt = 0;
	bRoundActive = true;
	Pilot = NewObject<UTunaSweeperCombatLabAutopilotComponent>(Player.Get());
	Pilot->RegisterComponent(); Pilot->AddTickPrerequisiteActor(PC); Player->AddTickPrerequisiteComponent(Pilot);
	Pilot->SetAutopilotEnabled(bAutopilot);
	UE_LOG(LogCombatLab, Display, TEXT("ROUND_START round=%d enemies=%d ai=%d"), Round, Enemies.Num(), bAutopilot);
}
void ATunaSweeperCombatLabGameMode::FinishRound(FName Result)
{
	if (!bRoundActive) return;
	bRoundActive = false; ResultKey = Result;
	const int32 Rolls = Pilot ? Pilot->GetRollCount() : 0;
	if (Pilot) Pilot->SetAutopilotEnabled(false);
	const FString Row = FString::Printf(TEXT("%d,%s,%.2f,%d,%d,%d,%.1f,%.1f,%.1f,%.2f,%d,%d\n"), Round, *Result.ToString(), Elapsed, PlayerProjectiles, EnemyProjectiles, Hits, DamageDealt, DamageTaken, Distance, IdleSeconds, Rolls, bAutopilot);
	FFileHelper::SaveStringToFile(Row, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
	UE_LOG(LogCombatLab, Display, TEXT("ROUND_END %s"), *Row.TrimEnd());
	CleanupRound();
	NextRoundAt = GetWorld()->GetTimeSeconds() + 2.f;
}
void ATunaSweeperCombatLabGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bRoundActive) { if (GetWorld()->GetTimeSeconds() >= NextRoundAt) StartRound(); return; }
	Elapsed += Dt;
	for (auto& Entry : EnemyHealth)
	{
		const float Health = Entry.Key.IsValid() ? Entry.Key->GetHealth() : 0.f;
		const float Loss = FMath::Max(0.f, Entry.Value - Health);
		if (Loss > 0) { DamageDealt += Loss; ++Hits; }
		Entry.Value = Health;
	}
	if (!Player.IsValid()) { FinishRound(TEXT("ui.combat_lab.loss")); return; }
	const FVector Location = Player->GetActorLocation();
	const float Step = FVector::Dist2D(Location, PreviousLocation);
	Distance += Step; if (Step < 30.f * Dt) IdleSeconds += Dt; PreviousLocation = Location;
	if (auto* Vitals = Player->GetVitalsComponent())
	{
		const float Health = Vitals->GetVitalsState().Health;
		DamageTaken += FMath::Max(0.f, LastHealth - Health); LastHealth = Health;
	}
	if (Elapsed >= NextSampleAt)
	{
		NextSampleAt = Elapsed + 1.f;
		UE_LOG(LogCombatLab, Display, TEXT("SAMPLE round=%d t=%.1f hp=%.1f alive=%d player_bullets=%d enemy_bullets=%d pos=(%.0f,%.0f) state=%s"), Round, Elapsed, LastHealth, GetAliveEnemies().Num(), PlayerProjectiles, EnemyProjectiles, Location.X, Location.Y, Pilot ? *Pilot->GetStateKey().ToString() : TEXT("none"));
	}
	if (Player->IsDead()) FinishRound(TEXT("ui.combat_lab.loss"));
	else if (GetAliveEnemies().IsEmpty()) FinishRound(TEXT("ui.combat_lab.win"));
	else if (Elapsed >= 90.f) FinishRound(TEXT("ui.combat_lab.timeout"));
}
void ATunaSweeperCombatLabGameMode::ResetRound()
{
	if (bRoundActive) FinishRound(TEXT("ui.combat_lab.reset"));
	NextRoundAt = GetWorld()->GetTimeSeconds() + 0.1f;
}
void ATunaSweeperCombatLabGameMode::ToggleAutopilot()
{
	bAutopilot = !bAutopilot;
	if (Pilot) Pilot->SetAutopilotEnabled(bAutopilot);
	UE_LOG(LogCombatLab, Display, TEXT("AUTOPILOT enabled=%d"), bAutopilot);
}
FText ATunaSweeperCombatLabGameMode::GetStatusText() const
{
	return FText::Format(LabText(this, TEXT("ui.combat_lab.status")), FText::AsNumber(Round),
		LabText(this, bRoundActive && Pilot ? Pilot->GetStateKey() : ResultKey), FText::AsNumber(GetAliveEnemies().Num()));
}
FText ATunaSweeperCombatLabGameMode::GetMetricsText() const
{
	return FText::Format(LabText(this, TEXT("ui.combat_lab.metrics")), FText::AsNumber(FMath::RoundToInt(Elapsed)), FText::AsNumber(PlayerProjectiles), FText::AsNumber(EnemyProjectiles), FText::AsNumber(FMath::RoundToInt(DamageTaken)));
}
void ATunaSweeperCombatLabGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (bRoundActive) FinishRound(TEXT("ui.combat_lab.stopped"));
	GetWorld()->RemoveOnActorSpawnedHandler(SpawnHandle);
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>()) GI->EndCombatTestSession();
	Super::EndPlay(Reason);
}
void ATunaSweeperCombatLabHUD::DrawHUD()
{
	Super::DrawHUD();
	const auto* Mode = GetWorld()->GetAuthGameMode<ATunaSweeperCombatLabGameMode>();
	if (!Canvas || !Mode) return;
	const float Width = FMath::Min(720.f, Canvas->ClipX - 32.f), Left = (Canvas->ClipX - Width) / 2;
	DrawRect(FLinearColor(0.015f,0.025f,0.035f,0.88f), Left, 14, Width, 78);
	DrawRect(FLinearColor(0.1f,0.7f,0.85f), Left, 14, 3, 78);
	auto Line = [this, Left](const FText& Text, float Y, FLinearColor Color)
	{
		FCanvasTextItem Item(FVector2D(Left+12,Y), Text, TunaSweeperUIFont::MakeFont(nullptr, 13), Color);
		Canvas->DrawItem(Item);
	};
	Line(Mode->GetStatusText(), 19, FLinearColor::White);
	Line(Mode->GetMetricsText(), 42, FLinearColor(0.7f,0.85f,0.9f));
	Line(LabText(this,TEXT("ui.combat_lab.controls")), 65, FLinearColor(0.65f,0.7f,0.75f));
}
