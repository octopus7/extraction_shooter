#include "BossLab/TunaSweeperBossLabGameMode.h"

#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "BossLab/TunaSweeperModularBoss.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CanvasItem.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Combat/TunaSweeperBossEncounter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/InputComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/SlateRenderer.h"
#include "TimerManager.h"
#include "UI/TunaSweeperBossLabWidget.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaBossLabWorld
{
	const FVector PreviewPosition(14400.f, 10000.f, 0.f);
	const FVector BattlePosition(15100.f, 10000.f, 0.f);
	const FVector PlayerPosition(13000.f, 10000.f, 110.f);
	FText Text(const UObject* Context, FName Key)
	{
		const auto* GI = Cast<UTunaSweeperGameInstance>(UGameplayStatics::GetGameInstance(Context));
		return GI ? GI->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	}
}

ATunaSweeperBossLabGameMode::ATunaSweeperBossLabGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	HUDClass = ATunaSweeperBossLabHUD::StaticClass();
	PlayerControllerClass = ATunaSweeperBossLabPlayerController::StaticClass();
	static ConstructorHelpers::FClassFinder<APawn> PlayerBlueprint(TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter"));
	if (PlayerBlueprint.Succeeded()) DefaultPawnClass = PlayerBlueprint.Class;
}

void ATunaSweeperBossLabGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>()) GI->BeginCombatTestSession();
}

void ATunaSweeperBossLabGameMode::StartPlay()
{
	// AGameModeBase::StartPlay dispatches BeginPlay. Remove the authored test triggers first.
	TArray<AActor*> OldActors;
	for (TActorIterator<ATunaSweeperBossEncounter> It(GetWorld()); It; ++It) OldActors.Add(*It);
	for (TActorIterator<ATunaSweeperWarpPointActor> It(GetWorld()); It; ++It) OldActors.Add(*It);
	for (AActor* Actor : OldActors) Actor->Destroy();
	for (TActorIterator<ATextRenderActor> It(GetWorld()); It; ++It) It->SetActorHiddenInGame(true);
	Super::StartPlay();
	GetWorldTimerManager().SetTimerForNextTick(this, &ATunaSweeperBossLabGameMode::OpenWorkshop);
}

void ATunaSweeperBossLabGameMode::RestartPlayer(AController* NewPlayer)
{
	RestartPlayerAtTransform(NewPlayer, FTransform(FRotator::ZeroRotator, TunaBossLabWorld::PlayerPosition));
}

void ATunaSweeperBossLabGameMode::OpenWorkshop()
{
	if (bLeaving || bWorkshopReady) return;
	auto* PC = Cast<ATunaSweeperPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	auto* Lab = GetGameInstance()->GetSubsystem<UTunaSweeperBossLabSubsystem>();
	if (!PC || !Lab) return;
	PreviewCamera = GetWorld()->SpawnActor<ACameraActor>();
	WorkshopWidget = CreateWidget<UTunaSweeperBossLabWidget>(PC, UTunaSweeperBossLabWidget::StaticClass());
	if (!PreviewCamera || !WorkshopWidget) { StatusKey = TEXT("ui.boss_lab.battle.spawn_failed"); return; }
	PreviewCamera->GetCameraComponent()->SetFieldOfView(48.f);
	WorkshopWidget->AddToViewport(100);
	bWorkshopReady = true;
	RefreshPreview(Lab->GetDraft());
	WorkshopWidget->RefreshFromSession();
	SetWorkshopView(true);
}

void ATunaSweeperBossLabGameMode::RefreshPreview(const FTunaSweeperBossDefinition& Definition)
{
	if (bBattleActive || bLeaving) return;
	FName Error;
	if (!TunaSweeperBossDefinition::Validate(Definition, Error)) { StatusKey = Error; return; }
	if (!Boss)
	{
		FActorSpawnParameters Parameters; Parameters.Owner = this;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Boss = GetWorld()->SpawnActor<ATunaSweeperModularBoss>(ATunaSweeperModularBoss::StaticClass(),
			TunaBossLabWorld::PreviewPosition, FRotator::ZeroRotator, Parameters);
	}
	if (!Boss || !Boss->InitializeBoss(Definition, true, Error))
	{
		StatusKey = Error.IsNone() ? FName(TEXT("ui.boss_lab.battle.spawn_failed")) : Error; return;
	}
	Boss->SetActorLocation(TunaBossLabWorld::PreviewPosition + FVector(0, 0, Boss->GetGroundOffset()));
	Boss->SetActorRotation(FRotator(0, PreviewYaw, 0));
	if (PreviewCamera)
	{
		const FBox Bounds = Boss->GetAssemblyLocalBounds();
		const FVector Center = Boss->GetActorTransform().TransformPosition(Bounds.GetCenter());
		const float Distance = FMath::Max(1150.f, Bounds.GetExtent().Size() * 3.2f);
		const FVector CameraPosition = Center + FVector(-0.72f, -0.6f, 0.82f).GetSafeNormal() * Distance;
		PreviewCamera->SetActorLocationAndRotation(CameraPosition, (Center - CameraPosition).Rotation());
	}
}

void ATunaSweeperBossLabGameMode::SetPreviewSelectedPart(int32 InstanceId)
{
	if (Boss && !bBattleActive) Boss->SetSelectedPart(InstanceId);
}

void ATunaSweeperBossLabGameMode::RotatePreview(float Degrees)
{
	if (bBattleActive || !FMath::IsFinite(Degrees)) return;
	PreviewYaw = FRotator::NormalizeAxis(PreviewYaw + Degrees);
	if (Boss) Boss->SetActorRotation(FRotator(0, PreviewYaw, 0));
}

bool ATunaSweeperBossLabGameMode::PreparePlayer()
{
	auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
	auto* PC = Cast<ATunaSweeperPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!GI || !PC) return false;
	CleanupPlayer();
	GI->ResetCombatTestLoadout();
	PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
	RestartPlayer(PC);
	auto* Player = Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn());
	if (!Player) return false;
	Player->SetActorLocationAndRotation(TunaBossLabWorld::PlayerPosition, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	if (auto* Vitals = Player->GetVitalsComponent())
	{
		auto State = Vitals->GetVitalsState();
		State.Health = State.MaxHealth; State.Food = State.MaxFood; State.Hydration = State.MaxHydration;
		Vitals->SetVitalsState(State);
		FTunaSweeperVitalsDepletionRates Rates; Rates.FoodPerSecond = Rates.HydrationPerSecond = 0.f;
		Vitals->SetBaseDepletionRates(Rates);
	}
	Player->SelectWeaponSlot(1);
	return true;
}

void ATunaSweeperBossLabGameMode::StartBattle()
{
	if (bBattleActive || bLeaving) return;
	auto* Lab = GetGameInstance()->GetSubsystem<UTunaSweeperBossLabSubsystem>();
	if (!Lab) return;
	FName Error;
	// The runtime takes a copy; editor mutations can never change a live encounter.
	const FTunaSweeperBossDefinition Definition = Lab->GetDraft();
	if (!TunaSweeperBossDefinition::Validate(Definition, Error))
	{
		StatusKey = Error; if (WorkshopWidget) WorkshopWidget->RefreshFromSession(); return;
	}
	if (!Boss) RefreshPreview(Definition);
	auto RestoreWorkshopAfterFailure = [this, &Definition, &Error]()
	{
		StatusKey = Error.IsNone() ? FName(TEXT("ui.boss_lab.battle.spawn_failed")) : Error;
		RefreshPreview(Definition); SetWorkshopView(true);
		if (WorkshopWidget) WorkshopWidget->RefreshFromSession();
	};
	if (!Boss || !Boss->InitializeBoss(Definition, false, Error))
	{
		RestoreWorkshopAfterFailure(); return;
	}
	Boss->SetActorLocation(TunaBossLabWorld::BattlePosition + FVector(0, 0, Boss->GetGroundOffset()));
	Boss->SetActorRotation(FRotator(0, 180.f, 0));
	Boss->KeepInsideArena();
	if (!PreparePlayer()) { RestoreWorkshopAfterFailure(); return; }
	bBattleActive = true;
	AmmoCheckSeconds = 0.f;
	StatusKey = TEXT("ui.boss_lab.battle.active");
	SetWorkshopView(false);
	Boss->BeginCombat(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ATunaSweeperBossLabGameMode::SetWorkshopView(bool bWorkshop)
{
	auto* PC = Cast<ATunaSweeperPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PC) return;
	PC->SetPause(false);
	PC->StopMovement();
	PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput();
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->SetActorHiddenInGame(bWorkshop);
		Pawn->SetActorEnableCollision(!bWorkshop);
		if (auto* Character = Cast<ACharacter>(Pawn)) Character->GetCharacterMovement()->StopMovementImmediately();
	}
	if (auto* HUD = PC->GetGameHudWidget())
	{
		HUD->SetHudMode(ETunaSweeperHudMode::None);
		HUD->SetCenterPanelsVisible(false);
		HUD->SetVisibility(bWorkshop ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (WorkshopWidget) WorkshopWidget->SetVisibility(bWorkshop ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bWorkshop)
	{
		PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
		if (PreviewCamera) PC->SetViewTarget(PreviewCamera);
		FInputModeUIOnly InputMode;
		if (WorkshopWidget) InputMode.SetWidgetToFocus(WorkshopWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode); PC->bShowMouseCursor = true;
	}
	else
	{
		if (PC->GetPawn()) PC->SetViewTarget(PC->GetPawn());
		PC->ApplyDefaultGameInputMode();
	}
}

void ATunaSweeperBossLabGameMode::CleanupPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return;
	TArray<AActor*> OwnedActors;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (*It == Pawn || *It == PC) continue;
		TSet<const AActor*> Visited;
		for (const AActor* OwnershipCursor = It->GetOwner(); OwnershipCursor && !Visited.Contains(OwnershipCursor); OwnershipCursor = OwnershipCursor->GetOwner())
		{
			if (OwnershipCursor == Pawn) { OwnedActors.Add(*It); break; }
			Visited.Add(OwnershipCursor);
		}
	}
	for (AActor* Actor : OwnedActors) Actor->Destroy();
	PC->UnPossess(); Pawn->Destroy();
}

void ATunaSweeperBossLabGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBattleActive || bLeaving) return;
	AmmoCheckSeconds -= DeltaSeconds;
	if (AmmoCheckSeconds <= 0.f)
	{
		AmmoCheckSeconds = 1.f;
		if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TSet<int32> RefilledAmmo;
			const int32 EquipmentCount = GI->GetEquipmentSlots().Num();
			for (int32 WeaponSlot = 1; WeaponSlot <= EquipmentCount; ++WeaponSlot)
			{
				FTunaSweeperItemInstance Weapon;
				FTunaSweeperItemDefinition WeaponDefinition;
				if (!GI->TryGetEquipmentWeaponSlotItem(WeaponSlot, Weapon, WeaponDefinition)) continue;
				const int32 AmmoId = GI->GetWeaponSelectedAmmoItemId(WeaponSlot);
				if (AmmoId != INDEX_NONE && !RefilledAmmo.Contains(AmmoId) && GI->GetWeaponInventoryAmmoCount(WeaponSlot) < 60)
				{
					GI->AddItemToFirstAvailableInventorySlot(AmmoId, 60);
					RefilledAmmo.Add(AmmoId);
				}
			}
		}
	}
	const auto* Player = Cast<ATunaSweeperTopDownCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player || Player->IsDead()) FinishBattle(TEXT("ui.boss_lab.battle.defeat"));
	else if (!Boss || Boss->IsDefeated()) FinishBattle(TEXT("ui.boss_lab.battle.victory"));
	else
	{
		const FVector Offset = Player->GetActorLocation() - TunaBossLabWorld::PreviewPosition;
		if (FMath::Abs(Offset.X) > 2250.f || FMath::Abs(Offset.Y) > 2250.f || Offset.Z < -150.f)
			FinishBattle(TEXT("ui.boss_lab.battle.stopped"));
	}
}

void ATunaSweeperBossLabGameMode::FinishBattle(FName ResultKey)
{
	bBattleActive = false;
	StatusKey = ResultKey;
	if (Boss) Boss->StopCombat();
	CleanupPlayer();
	if (auto* Lab = GetGameInstance()->GetSubsystem<UTunaSweeperBossLabSubsystem>()) RefreshPreview(Lab->GetDraft());
	SetWorkshopView(true);
	if (WorkshopWidget) WorkshopWidget->RefreshFromSession();
}

void ATunaSweeperBossLabGameMode::StopBattle()
{
	if (bBattleActive) FinishBattle(TEXT("ui.boss_lab.battle.stopped"));
}

void ATunaSweeperBossLabGameMode::ReturnToTitle()
{
	if (bLeaving) return;
	bLeaving = true; bBattleActive = false;
	if (Boss) Boss->StopCombat();
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/IntroMap")), true);
}

FText ATunaSweeperBossLabGameMode::GetStatusText() const
{
	if (bBattleActive && Boss)
		return FText::Format(TunaBossLabWorld::Text(this, TEXT("ui.boss_lab.battle.active")),
			FText::AsNumber(FMath::CeilToInt(Boss->GetCoreHealth())), FText::AsNumber(FMath::CeilToInt(Boss->GetCoreMaxHealth())),
			FText::AsNumber(Boss->GetOperationalPartCount()));
	return TunaBossLabWorld::Text(this, StatusKey);
}

void ATunaSweeperBossLabGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	bLeaving = true; bBattleActive = false;
	if (Boss) Boss->StopCombat();
	if (WorkshopWidget) WorkshopWidget->RemoveFromParent();
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>()) GI->EndCombatTestSession();
	Super::EndPlay(Reason);
}

ATunaSweeperBossLabPlayerController::ATunaSweeperBossLabPlayerController()
{
	// This mode explicitly selects either the workshop camera or the current battle pawn.
	// Possession/spectator transitions must not replace that target after a return to the workshop.
	bAutoManageActiveCameraTarget = false;
}

void ATunaSweeperBossLabPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent) return;
	// Replace the ordinary pause action: a lab attempt always returns to its retained design.
	InputComponent->KeyBindings.RemoveAll([](const FInputKeyBinding& Binding)
	{
		return Binding.Chord.Key == EKeys::Escape || Binding.Chord.Key == EKeys::Home;
	});
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ATunaSweeperBossLabPlayerController::HandleLabBack).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ATunaSweeperBossLabPlayerController::HandleLabBack).bExecuteWhenPaused = true;
}

void ATunaSweeperBossLabPlayerController::HandleLabBack()
{
	if (auto* Mode = GetWorld()->GetAuthGameMode<ATunaSweeperBossLabGameMode>()) Mode->StopBattle();
}

void ATunaSweeperBossLabHUD::DrawHUD()
{
	Super::DrawHUD();
	const auto* Mode = GetWorld()->GetAuthGameMode<ATunaSweeperBossLabGameMode>();
	if (!Canvas || !Mode || !Mode->IsBattleActive()) return;
	FSlateFontInfo FontInfo = TunaSweeperUIFont::MakeFont(nullptr, 14);
	if (!CanvasFont)
	{
		// Canvas requires a UFont even when Slate supplies the project's composite font.
		CanvasFont = NewObject<UFont>(this);
		CanvasFont->FontCacheType = EFontCacheType::Runtime;
		if (FontInfo.CompositeFont.IsValid()) CanvasFont->GetMutableInternalCompositeFont() = *FontInfo.CompositeFont;
	}
	FontInfo.FontObject = CanvasFont;
	const float Width = FMath::Min(760.f, Canvas->ClipX - 32.f), Left = (Canvas->ClipX - Width) * 0.5f;
	DrawRect(FLinearColor(0.012f, 0.025f, 0.04f, 0.92f), Left, 14.f, Width, 91.f);
	auto Line = [this, Left, Width, &FontInfo](const FText& Text, float Y, FLinearColor Color)
	{
		FCanvasTextItem Item(FVector2D(Left + 14.f, Y), Text, FontInfo, Color);
		const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, FontInfo);
		const float Scale = FMath::Min(1.f, (Width - 28.f) / FMath::Max(1.f, static_cast<float>(TextSize.X)));
		Item.Scale = FVector2D(Scale);
		Canvas->DrawItem(Item);
	};
	Line(Mode->GetStatusText(), 22.f, FLinearColor::White);
	if (const auto* Boss = Mode->GetBoss())
	{
		DrawRect(FLinearColor(0.08f, 0.13f, 0.16f), Left + 14.f, 49.f, Width - 28.f, 8.f);
		DrawRect(FLinearColor(0.05f, 0.85f, 0.75f), Left + 14.f, 49.f,
			(Width - 28.f) * FMath::Clamp(Boss->GetCoreHealth() / Boss->GetCoreMaxHealth(), 0.f, 1.f), 8.f);
		if (Boss->IsWarningActive()) Line(TunaBossLabWorld::Text(this, TEXT("ui.boss_lab.battle.warning")), 108.f, FLinearColor(1.f, 0.45f, 0.12f));
	}
	Line(TunaBossLabWorld::Text(this, TEXT("ui.boss_lab.battle.controls")), 69.f, FLinearColor(0.7f, 0.82f, 0.88f));
}
