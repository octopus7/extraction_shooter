#include "Scenario/TrailerGameMode.h"

#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/TunaSweeperWheeledEnemyCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Component/TunaSweeperVerticalOcclusionRevealComponent.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Raid/TunaSweeperSupplyDropActor.h"
#include "Raid/TunaSweeperSupplyDropDataAsset.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "ContentStreaming.h"
#include "EngineUtils.h"
#include "FoldingCanopyGarageDoorActor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperBreakableAppleCrateActor.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "Kismet/GameplayStatics.h"
#include "LoopRailStation.h"
#include "LoopRailTrain.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	constexpr float FadeSeconds = .45f;

	AActor* FindPlaced(UWorld* World, const TCHAR* ClassPath, FVector Near)
	{
		UClass* Class = LoadClass<AActor>(nullptr, ClassPath);
		if (!Class) return nullptr;
		AActor* Best = nullptr;
		for (TActorIterator<AActor> It(World, Class); It; ++It)
			if (!Best || FVector::DistSquared(It->GetActorLocation(), Near) < FVector::DistSquared(Best->GetActorLocation(), Near))
				Best = *It;
		return Best;
	}
}

ATrailerGameMode::ATrailerGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<APawn> PawnBP(TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter"));
	if (PawnBP.Succeeded()) DefaultPawnClass = PawnBP.Class;
}

void ATrailerGameMode::InitGame(const FString& Map, const FString& Options, FString& Error)
{
	Super::InitGame(Map, Options, Error);
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
	{
		GI->SetTextLanguageOverride(ETunaSweeperItemTextLanguage::English);
		GI->BeginCombatTestSession();
	}
	if (Map.EndsWith(TEXT("BunkerMap"))) Shot = EShot::TeaRoom;
	else if (Map.EndsWith(TEXT("RaidVillage"))) Shot = EShot::Shop;
	else if (Map.EndsWith(TEXT("RaidPlains"))) Shot = EShot::ShipApproach;
}

void ATrailerGameMode::EndPlay(EEndPlayReason::Type Reason)
{
	if (FadePanel) FadePanel->RemoveFromParent();
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
	{
		GI->SetTextLanguageOverride({});
		GI->EndCombatTestSession();
	}
	Super::EndPlay(Reason);
}

void ATrailerGameMode::SetTrailerFadeOpacity(float Opacity)
{
	FadePanel->SetRenderOpacity(Opacity);
	// DumpMovie captures the scene before Slate draws the viewport panel.
	if (GIsDumpingMovie)
		if (auto* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
			Camera->SetManualCameraFade(Opacity, FLinearColor::Black, false);
}

void ATrailerGameMode::ReturnToTitle()
{
	bLeaving = true;
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/IntroMap"));
}

FVector ATrailerGameMode::GroundAt(FVector Point) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrailerFloor), false, Player.Get());
	if (GetWorld()->LineTraceSingleByChannel(Hit, Point + FVector(0,0,160), Point - FVector(0,0,600),
		ECC_Visibility, Params) && Hit.ImpactNormal.Z > .5f) return Hit.ImpactPoint;
	return Point;
}

void ATrailerGameMode::PlacePlayer(FVector Feet)
{
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->SetActorLocation(GroundAt(Feet) + FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2),
		false, nullptr, ETeleportType::TeleportPhysics);
}

void ATrailerGameMode::WalkTo(FVector Destination)
{
	FVector Offset = Destination - Player->GetActorLocation();
	if (Offset.SizeSquared2D() < FMath::Square(35.f))
	{
		Player->GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	Player->AddMovementInput(Offset.GetSafeNormal2D(), 1.f, true);
	Player->SetAimWorldPoint(Player->GetActorLocation() + Offset.GetSafeNormal2D() * 1500.f);
}

void ATrailerGameMode::Shoot(AActor* Actor)
{
	if (!IsValid(Actor) || Player->IsGameplayActionInputLocked()) return;
	FVector Aim = Actor->GetActorLocation();
	UPrimitiveComponent* HitComponent = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (Actor->IsA<ATunaSweeperExplosiveBarrelActor>() || Actor->IsA<ATunaSweeperBreakableAppleCrateActor>())
		HitComponent = Actor->FindComponentByClass<UBoxComponent>();
	if (auto* Boss = Cast<ATunaSweeperHopperEnemyCharacter>(Actor))
	{
		for (UActorComponent* Part : Boss->GetComponentsByTag(UPrimitiveComponent::StaticClass(), TEXT("HopperMechHitBox")))
			if (auto* HitBox = Cast<UPrimitiveComponent>(Part)) { HitComponent = HitBox; break; }
	}
	if (HitComponent) Aim = HitComponent->Bounds.Origin;
	if (!HitComponent || HitComponent->Bounds.SphereRadius <= 1.f)
	{
		FVector Extent;
		Actor->GetActorBounds(true, Aim, Extent);
	}
	const FVector Direction = (Aim - Player->GetActorLocation()).GetSafeNormal();
	Player->SetAimWorldHit(Aim, FHitResult(Actor, HitComponent, Aim, -Direction));
	if (Elapsed >= NextShot)
	{
		Player->FireWeapon();
		NextShot = Elapsed + .27f;
	}
}

bool ATrailerGameMode::SetUpTrikes()
{
	AActor* Shop = FindPlaced(GetWorld(), TEXT("/Game/Environment/Buildings/VillageGukbap/BP_GrandfatherGukbap.BP_GrandfatherGukbap_C"), FVector(3761,2062,152));
	UClass* Class = LoadClass<ATunaSweeperWheeledEnemyCharacter>(nullptr, TEXT("/Game/Characters/Region2Robots/BP_RabbitTrike.BP_RabbitTrike_C"));
	if (!Shop || !Class) return false;
	const FTransform Street(Shop->GetActorRotation(), Shop->GetActorLocation());
	PlacePlayer(Street.TransformPosition(FVector(0,730,0)));
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const float Sign = Index == 0 ? 1.f : -1.f;
		// Mirror the complete pass and the outward U-turn across the centre of the street.
		const auto Lane = [&](float Along, float Across)
		{
			return Street.TransformPosition(FVector(Sign * Along, 730.f - Sign * Across, 0));
		};
		FTrikeRoute& Route = Trikes[Index];
		Route.Waypoints = {Lane(530,155), Lane(625,195), Lane(635,255), Lane(525,285), Lane(225,245)};
		const FVector Start = GroundAt(Lane(-900,155)) + FVector(0,0,82);
		const FRotator Facing = (Route.Waypoints[0] - Start).GetSafeNormal2D().Rotation();
		auto* Bike = GetWorld()->SpawnActor<ATunaSweeperWheeledEnemyCharacter>(Class, Start, Facing);
		if (!Bike) return false;
		Bike->DetachFromControllerPendingDestroy();
		Bike->GetCharacterMovement()->bRunPhysicsWithNoController = true;
		Route.Bike = Bike;
	}
	Player->SetAimWorldPoint(Trikes[0].Bike->GetActorLocation());
	return true;
}

bool ATrailerGameMode::PlayTrikes(float Dt)
{
	if (Trikes[0].Bike.IsValid() && Trikes[1].Bike.IsValid())
	{
		const FVector First = Trikes[0].Bike->GetActorLocation();
		const FVector Second = Trikes[1].Bike->GetActorLocation();
		TrikeClosestPass = FMath::Min(TrikeClosestPass, float(FVector::Dist2D(First, Second)));
		if (HoldTime == 0.f)
			Player->SetAimWorldPoint(Trikes[0].Next == 0 ? First : Second);
	}
	bool bBothFired = true;
	for (FTrikeRoute& Route : Trikes)
	{
		auto* Bike = Route.Bike.Get();
		if (!Bike || Bike->IsDead()) continue;
		if (Route.Waypoints.IsValidIndex(Route.Next))
		{
			const FVector Delta = Route.Waypoints[Route.Next] - Bike->GetActorLocation();
			Bike->GetCharacterMovement()->MaxWalkSpeed = Route.Next == 0 ? 850.f : 390.f;
			if (Delta.SizeSquared2D() < FMath::Square(65.f))
				++Route.Next;
			else
				Bike->AddMovementInput(Delta.GetSafeNormal2D(), 1.f, true);
		}
		else
		{
			const FRotator Aim = (Player->GetActorLocation() - Bike->GetActorLocation()).GetSafeNormal2D().Rotation();
			Bike->SetActorRotation(FMath::RInterpConstantTo(Bike->GetActorRotation(), Aim, Dt, 240.f));
			if (Bike->TryFireProjectileAt(Player.Get()) == ETunaSweeperEnemyFireResult::Fired && !Route.bFired)
			{
				UE_LOG(LogTemp, Display, TEXT("Trailer: returning trike fired at %s"), *Bike->GetActorLocation().ToString());
				Route.bFired = true;
			}
		}
		bBothFired &= Route.bFired;
	}

	if (!bBothFired) return false;
	if (HoldTime == 0.f) UE_LOG(LogTemp, Display, TEXT("Trailer: closest trike pass %.1f cm"), TrikeClosestPass);
	HoldTime += Dt;
	if (HoldTime > .55f)
	{
		auto* TargetBike = Trikes[HoldTime < 1.6f ? 0 : 1].Bike.Get();
		if (TargetBike && !TargetBike->IsDead()) Shoot(TargetBike);
	}
	return HoldTime >= 3.f;
}

bool ATrailerGameMode::ReleaseSupplyDrop()
{
	auto* PC = Cast<APlayerController>(Player->GetController());
	const auto* Assets = LoadObject<UTunaSweeperSupplyDropDataAsset>(nullptr, TEXT("/Game/Interaction/SupplyDrop/DA_SupplyDrop.DA_SupplyDrop"));
	if (!PC || !Assets || !Assets->ContainerMesh || !Assets->ParachuteMesh) return false;

	// Start inside the frame, then drift in camera-right world space throughout the descent.
	int32 Width = 0, Height = 0;
	PC->GetViewportSize(Width, Height);
	FVector Origin, Ray;
	if (Width <= 0 || Height <= 0 || !PC->DeprojectScreenPositionToWorld(Width * .77f, Height * .55f, Origin, Ray))
	{
		// Offscreen benchmarks can run without a viewport. Use the same camera projection.
		const FMinimalViewInfo& View = PC->PlayerCameraManager->GetCameraCacheView();
		const float HalfWidth = FMath::Tan(FMath::DegreesToRadians(View.FOV * .5f));
		Origin = View.Location;
		Ray = View.Rotation.RotateVector(FVector(1.f, .54f * HalfWidth, -.1f * HalfWidth / FMath::Max(.1f, View.AspectRatio)));
	}
	if (Ray.Z >= -KINDA_SMALL_NUMBER) return false;
	const float FloorZ = Player->GetCharacterMovement()->GetActorFeetLocation().Z;
	const auto AtHeight = [&](float Z) { return Origin + Ray * ((Z - Origin.Z) / Ray.Z); };
	SupplyTravelDirection = PC->PlayerCameraManager->GetCameraRotation().RotateVector(FVector::RightVector).GetSafeNormal2D();
	const FTransform Release(FRotator::ZeroRotator, AtHeight(FloorZ + 230.f));
	SupplyLandingPoint = GroundAt(AtHeight(FloorZ) + SupplyTravelDirection * 1250.f);
	auto* Drop = GetWorld()->SpawnActorDeferred<ATunaSweeperSupplyDropActor>(
		ATunaSweeperSupplyDropActor::StaticClass(), Release, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Drop) return false;
	Drop->ConfigureDrop(*Assets, 7011, 8027, SupplyLandingPoint);
	Drop->FinishSpawning(Release);
	SupplyDrop = Drop;
	return true;
}

bool ATrailerGameMode::SetUpShot()
{
	UWorld* World = GetWorld();
	Target.Reset();
	Elapsed = NextShot = HoldTime = 0;
	Player->SetCanBeDamaged(false);
	Player->bExternalAimControl = true;
	Player->GetCharacterMovement()->MaxWalkSpeed = 240.f;
	const auto PlaceNear = [&](AActor* Subject, FVector LocalStart, FVector LocalEnd)
	{
		if (!Subject) return false;
		const FTransform Frame(Subject->GetActorRotation(), Subject->GetActorLocation());
		PlacePlayer(Frame.TransformPosition(LocalStart));
		Goal = Frame.TransformPosition(LocalEnd);
		Player->SetAimWorldPoint(Goal);
		Target = Subject;
		return true;
	};
	switch (Shot)
	{
	case EShot::TeaRoom:
	{
		const UStaticMesh* TeaTable = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Bunker/Agit/TeaProps/SM_TeaTable.SM_TeaTable"));
		if (!TeaTable) return false;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
			for (const auto* Mesh : Meshes)
			{
				if (Mesh->GetStaticMesh() != TeaTable) continue;
				const FBox Bounds = Mesh->Bounds.GetBox();
				const FVector Center(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
				const FVector Along = Mesh->GetForwardVector().GetSafeNormal2D();
				const FVector Across = Mesh->GetRightVector().GetSafeNormal2D();
				const auto* Capsule = Player->GetCapsuleComponent();
				const FCollisionShape Clearance = FCollisionShape::MakeCapsule(
					Capsule->GetScaledCapsuleRadius() + 3.f, Capsule->GetScaledCapsuleHalfHeight());
				const FVector Lift(0,0,Clearance.GetCapsuleHalfHeight() + 4.f);
				FCollisionQueryParams Query(SCENE_QUERY_STAT(TrailerTeaAisle), false, Player.Get());
				for (float HalfLength : {145.f, 115.f, 85.f})
				{
					for (float Offset : {-115.f, 115.f, -150.f, 150.f})
					{
						const FVector Middle = Center + Across * Offset;
						const FVector Start = GroundAt(Middle - Along * HalfLength);
						const FVector End = GroundAt(Middle + Along * HalfLength);
						if (FMath::Abs(Start.Z - Center.Z) > 20 || FMath::Abs(End.Z - Center.Z) > 20 ||
							World->SweepTestByChannel(Start + Lift, End + Lift, FQuat::Identity, ECC_Pawn, Clearance, Query))
							continue;
						PlacePlayer(Start);
						Goal = End;
						Player->SetAimWorldPoint(Goal);
						return true;
					}
				}
			}
		}
		return false;
	}
	case EShot::SupplyLanding:
		if (!SupplyDrop.IsValid()) return false;
		PlacePlayer(SupplyLandingPoint - SupplyTravelDirection * 1190.f);
		Goal = SupplyLandingPoint - SupplyTravelDirection * 240.f;
		Player->SetAimWorldPoint(SupplyLandingPoint);
		return true;
	case EShot::ExitGarage:
	case EShot::EnterGarage:
	{
		const bool bExit = Shot == EShot::ExitGarage;
		auto* Door = Cast<AFoldingCanopyGarageDoor>(UGameplayStatics::GetActorOfClass(this, AFoldingCanopyGarageDoor::StaticClass()));
		if (!Door) return false;
		Target = Door;
		const FVector Through = Door->GetActorRightVector().GetSafeNormal2D();
		Door->SetDoorEnabled(true);
		Door->SetOpenAlpha(bExit ? 1.f : 0.f);
		Player->SetActorLocation(Door->GetActorLocation() + Through*(bExit ? 165.f : -340.f) + FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2));
		// The raid entrance transitions beneath the landscape on its interior side.
		Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Player->SetActorEnableCollision(false);
		Goal = Door->GetActorLocation() + Through*(bExit ? -340 : 165);
		Player->SetAimWorldPoint(Goal);
		return true;
	}
	case EShot::GrayRobot:
	case EShot::BlueRobot:
	case EShot::Captain:
	{
		const bool bBoss = Shot == EShot::Captain;
		Player->SetActorEnableCollision(true);
		Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		const TCHAR* Path = bBoss ? TEXT("/Game/Characters/Hopper/BP_HopperEnemyMk2.BP_HopperEnemyMk2_C")
			: Shot == EShot::GrayRobot ? TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_DarkGray.BP_QuadrupedGunEnemy_DarkGray_C")
			: TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_Blue.BP_QuadrupedGunEnemy_Blue_C");
		UClass* Class = LoadClass<ACharacter>(nullptr, Path);
		if (!Class) return false;
		const FVector Feet = GroundAt(bBoss ? FVector(8680,-350,300) : Shot == EShot::GrayRobot ? FVector(-1650,-6350,0) : FVector(380,-6650,0));
		PlacePlayer(Feet + FVector(bBoss ? -960 : -300, bBoss ? 0 : -50, 0));
		Goal = Feet;
		Player->SetAimWorldPoint(Feet);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector Offsets[] = { FVector::ZeroVector, FVector(-85, -220, 0), FVector(65, 215, 0) };
		for (int32 Index = 0; Index < (bBoss ? 1 : 3); ++Index)
		{
			FVector Offset = Offsets[Index];
			if (Shot == EShot::BlueRobot) Offset.Y *= -1;
			const FVector Location = GroundAt(Feet + Offset) + FVector(0,0,105);
			const FRotator Facing = (Player->GetActorLocation() - Location).GetSafeNormal2D().Rotation();
			ACharacter* Enemy = World->SpawnActor<ACharacter>(Class, Location, Facing, Params);
			if (!Enemy) return false;
			Enemy->SpawnDefaultController();
			if (!bBoss) Robots.Add(Enemy);
			if (Index == 0) Target = Enemy;
		}
		break;
	}
	case EShot::Barrel:
	case EShot::Apples:
	{
		const bool bBarrel = Shot == EShot::Barrel;
		AActor* Prop = nullptr;
		if (bBarrel)
		{
			UClass* Class = LoadClass<ATunaSweeperExplosiveBarrelActor>(nullptr, TEXT("/Game/Interaction/BP_ExplosiveBarrel.BP_ExplosiveBarrel_C"));
			if (!Class) return false;
			const float Spacing = Class->GetDefaultObject<ATunaSweeperExplosiveBarrelActor>()->GetExplosionRadius() * .72f;
			const FVector Origin(-1650, -6350, 0);
			for (int32 Angle = 0; Angle < 360 && Barrels.IsEmpty(); Angle += 30)
			{
				const FVector Step = FVector::RightVector.RotateAngleAxis(Angle, FVector::UpVector) * Spacing;
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const FVector Feet = GroundAt(Origin + Step * (Index - 1));
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
					auto* Barrel = World->SpawnActor<ATunaSweeperExplosiveBarrelActor>(Class, Feet + FVector(0,0,2), FRotator::ZeroRotator, Params);
					if (!Barrel) break;
					Barrels.Add(Barrel);
				}
				if (Barrels.Num() != 3)
				{
					for (auto Barrel : Barrels) Barrel->Destroy();
					Barrels.Reset();
				}
			}
			if (Barrels.IsEmpty()) return false;
			Prop = Barrels[0].Get();
		}
		else
		{
			Prop = UGameplayStatics::GetActorOfClass(this, ATunaSweeperBreakableAppleCrateActor::StaticClass());
		}
		if (!Prop) return false;
		const auto* Collision = Prop->FindComponentByClass<UBoxComponent>();
		if (!Collision) return false;
		const FVector Center = Collision->Bounds.Origin;
		bool bFoundLane = false;
		for (int32 Angle = 0; Angle < 360; Angle += 30)
		{
			const FVector Offset = FVector(bBarrel ? -280.f : -360.f,0,0).RotateAngleAxis(Angle, FVector::UpVector);
			const FVector GroupCenter = bBarrel ? Barrels[1]->GetActorLocation() : Prop->GetActorLocation();
			const FVector Ground = GroundAt(GroupCenter + Offset);
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(TrailerFiringLane), false, Player.Get());
			if (!World->LineTraceSingleByChannel(Hit, Ground + FVector(0,0,95), Center, TunaSweeperCollisionChannels::Projectile, Params) || Hit.GetActor() != Prop) continue;
			PlacePlayer(Ground);
			Player->SetAimWorldPoint(Center);
			Target = Prop;
			bFoundLane = true;
			break;
		}
		if (!bFoundLane) return false;
		if (!bBarrel)
		{
			const FVector Forward = (Center - Player->GetActorLocation()).GetSafeNormal2D();
			// Place the two extra targets where the player has an unobstructed shot at muzzle height.
			const FVector Start = Player->GetActorLocation() + FVector(0, 0, 20);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(TrailerCrateLane), false, Player.Get());
			for (const float Sign : { -1.f, 1.f })
			{
				ATunaSweeperBreakableAppleCrateActor* Extra = nullptr;
				for (float Angle = 35.f; Angle <= 145.f && !Extra; Angle += 10.f)
				{
					const FVector Ray = Forward.RotateAngleAxis(Angle * Sign, FVector::UpVector);
					const FVector End = Start + Ray * 290.f;
					if (World->SweepTestByChannel(Start, End, FQuat::Identity, TunaSweeperCollisionChannels::Projectile, FCollisionShape::MakeSphere(30.f), Params)) continue;
					FVector Location(End.X, End.Y, Prop->GetActorLocation().Z);
					Extra = World->SpawnActor<ATunaSweeperBreakableAppleCrateActor>(Prop->GetClass(), Location, Prop->GetActorRotation());
				}
				if (!Extra) return false;
				Extra->SetActorScale3D(Prop->GetActorScale3D());
				ExtraCrates.Add(Extra);
			}
		}
		break;
	}
	case EShot::Shop:
		return PlaceNear(FindPlaced(World, TEXT("/Game/Environment/Buildings/VillageGukbap/BP_GrandfatherGukbap.BP_GrandfatherGukbap_C"), FVector(3761,2062,152)),
			FVector(-440,470,0), FVector(440,470,0));
	case EShot::Trikes:
		if (!SetUpTrikes()) return false;
		break;
	case EShot::Platform:
		for (TActorIterator<ALoopRailStation> It(World); It; ++It)
			if (It->StationId == TEXT("village_02")) Station = *It;
		Train = Cast<ALoopRailTrain>(UGameplayStatics::GetActorOfClass(this, ALoopRailTrain::StaticClass()));
		if (!Station.IsValid() || !Train.IsValid() || !Train->HasUsableTrack()) return false;
		Train->StartDistance = Station->GetStopDistance() - 1050;
		Train->Acceleration = 420.f;
		Train->BrakingDeceleration = 420.f;
		Train->ResetTrain();
		// Stand on the platform beside the first passenger car door.
		PlacePlayer(Station->GetActorTransform().TransformPosition(FVector(-624, (Station->bPlatformOnRight ? 1 : -1) * 185, 60)));
		Player->SetAimWorldPoint(Station->GetActorLocation());
		return true;
	case EShot::Carriage:
	{
		UBoxComponent* Floor = Train.IsValid() ? Train->GetVehicleFloor(2) : nullptr;
		if (!Floor) return false;
		Train->StartDistance = Station->GetStopDistance() + 600;
		Train->ResetTrain();
		Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		Player->SetActorEnableCollision(false);
		Player->SetActorLocation(Floor->GetComponentLocation() + FVector(0, 0, 10 + Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
		return true;
	}
	case EShot::ShipApproach:
	case EShot::ExhaustPass:
	{
		auto* Ship = FindPlaced(World, TEXT("/Game/MainRaid/SSTO/BP_SSTOLander.BP_SSTOLander_C"), FVector::ZeroVector);
		if (!Ship) return false;
		const bool bApproach = Shot == EShot::ShipApproach;
		if (bApproach)
		{
			TArray<UTunaSweeperVerticalOcclusionRevealComponent*> ShellReveal;
			Ship->GetComponents(ShellReveal, /*bIncludeFromChildActors=*/true);
			for (auto* Reveal : ShellReveal) Reveal->KeepExteriorVisible();
		}
		const FTransform Frame(FRotator(0, Ship->GetActorRotation().Yaw, 0), Ship->GetActorLocation(), Ship->GetActorScale3D());
		// The two aft nozzle lips sit at hull-local X=-1145 cm. Keep the runner outside the bells.
		const FVector Start = bApproach ? FVector(-3120,-1020,0) : FVector(-1510,-720,0);
		const FVector End = bApproach ? FVector(-1510,-720,0) : FVector(-1510,820,0);
		if (bApproach) PlacePlayer(Frame.TransformPosition(Start));
		Goal = Frame.TransformPosition(End);
		Player->CurrentCameraMode = ETunaSweeperPlayerCameraMode::Default;
		Player->DefaultCameraBoomRotation = FRotator(bApproach ? -42.f : -48.f, Ship->GetActorRotation().Yaw, 0);
		Player->DefaultCameraArmLength = bApproach ? 3400.f : 1850.f;
		Player->DefaultCameraTargetOffset = Frame.GetRotation().RotateVector(FVector(bApproach ? 850.f : 310.f,0,0));
		Player->bHideTrailerLaser = true;
		Player->SetAimWorldPoint(Goal);
		return true;
	}
	case EShot::Breakwater:
	{
		AActor* Beacon = FindPlaced(World, TEXT("/Game/Environment/FactionHarbor/Beacon/BP_HarborLighthouse.BP_HarborLighthouse_C"), FVector(-13322,-756,142));
		if (!Beacon) return false;
		const FVector Entrance(-11236,-1162,142);
		const FVector AlongPier = (Beacon->GetActorLocation() - Entrance).GetSafeNormal2D();
		Goal = Beacon->GetActorLocation() - AlongPier * 305.f;
		// Detach before enabling walking: the preceding shot ends on a moving coach.
		Player->SetBase(nullptr);
		PlacePlayer(Entrance);
		auto* Movement = Player->GetCharacterMovement();
		Movement->SetPlaneConstraintOrigin(Entrance);
		Movement->SetPlaneConstraintNormal(FVector::CrossProduct(AlongPier, FVector::UpVector));
		Movement->SetPlaneConstraintEnabled(true);
		Player->SetActorEnableCollision(true);
		Movement->SetMovementMode(MOVE_Walking);
		Player->SetAimWorldPoint(Goal);
		return true;
	}
	case EShot::Beach:
	{
		Player->GetCharacterMovement()->SetPlaneConstraintEnabled(false);
		// This straight stretch of the eastern strand follows the animated swash line.
		PlacePlayer(FVector(-10655, 9170, 95));
		Goal = FVector(-10655, 10430, 95);
		Player->SetAimWorldPoint(Goal);
		UClass* GullClass = LoadClass<AActor>(nullptr, TEXT("/Game/Environment/CoastalWildlife/BP_CoastalGull.BP_CoastalGull_C"));
		if (!GullClass) return false;
		Target = World->SpawnActor<AActor>(GullClass, GroundAt(FVector(-10595, 9950, 95)), FRotator(0,180,0));
		return Target.IsValid();
	}
	default: return false;
	}
	if (auto* GI = GetGameInstance<UTunaSweeperGameInstance>()) GI->ResetCombatTestLoadout();
	Player->SelectWeaponSlot(1);
	return true;
}

bool ATrailerGameMode::PlayShot(float Dt)
{
	switch (Shot)
	{
	case EShot::ShipApproach:
	case EShot::ExhaustPass:
	{
		const FVector Delta = (Goal - Player->GetActorLocation()).GetSafeNormal2D();
		const bool bRunning = FVector::Dist2D(Player->GetActorLocation(), Goal) > 36.f;
		Player->bSprintInputHeld = bRunning;
		Player->CurrentMoveInput = bRunning ? FVector2D(Delta.Y, Delta.X) : FVector2D::ZeroVector;
		WalkTo(Goal);
		if (!bRunning) HoldTime += Dt;
		return HoldTime > (Shot == EShot::ShipApproach ? 0.f : .35f);
	}
	case EShot::TeaRoom:
		if (Elapsed < .6f) return false;
		WalkTo(Goal);
		if (FVector::Dist2D(Player->GetActorLocation(), Goal) < 36.f) HoldTime += Dt;
		return HoldTime > .6f;
	case EShot::SupplyLanding:
	{
		if (!SupplyDrop.IsValid()) { ReturnToTitle(); return false; }
		const FVector ToGoal = Goal - Player->GetActorLocation();
		Player->bSprintInputHeld = ToGoal.SizeSquared2D() > FMath::Square(25.f);
		Player->CurrentMoveInput = Player->bSprintInputHeld ? FVector2D(1,0) : FVector2D::ZeroVector;
		if (Player->bSprintInputHeld)
		{
			const float MaxSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
			Player->AddMovementInput(ToGoal.GetSafeNormal2D(), FMath::Clamp(305.f / FMath::Max(1.f, MaxSpeed), 0.f, 1.f), true);
		}
		else
		{
			Player->GetCharacterMovement()->StopMovementImmediately();
			if (HoldTime == 0.f)
				UE_LOG(LogTemp, Display, TEXT("Trailer: supply approach stopped %.0f cm from crate, landed %d"),
					FVector::Dist2D(Player->GetActorLocation(), SupplyLandingPoint), !SupplyDrop->IsDescending());
			HoldTime += Dt;
		}
		return !SupplyDrop->IsDescending() && HoldTime > .35f;
	}
	case EShot::ExitGarage:
	case EShot::EnterGarage:
	{
		const FVector Through = Target->GetActorRightVector().GetSafeNormal2D();
		const bool bExit = Shot == EShot::ExitGarage;
		const float Alpha = FMath::Clamp((Elapsed - (bExit ? .45f : .9f)) / 3.1f, 0.f, 1.f);
		const float DoorMotion = FMath::SmoothStep(0.f, 1.f, (Elapsed - (bExit ? 1.95f : .55f)) / 2.25f);
		CastChecked<AFoldingCanopyGarageDoor>(Target.Get())->SetOpenAlpha(bExit ? 1.f - DoorMotion : DoorMotion);
		const float Distance = Shot == EShot::ExitGarage ? FMath::Lerp(165.f,-340.f,Alpha) : FMath::Lerp(-340.f,165.f,Alpha);
		Player->SetActorLocation(Target->GetActorLocation() + Through*Distance +
			FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2));
		if (Alpha > 0 && Alpha < 1) WalkTo(Goal);
		return Elapsed > 5.f;
	}
	case EShot::Shop:
		if (Elapsed > .8f) WalkTo(Goal);
		return Elapsed > 5.5f;
	case EShot::Breakwater:
	case EShot::Beach:
	{
		if (Elapsed <= .5f) return false;
		const bool bArrived = FVector::DistSquared2D(Player->GetActorLocation(), Goal) < FMath::Square(35.f);
		if (Shot == EShot::Breakwater)
		{
			Player->bSprintInputHeld = !bArrived;
			Player->CurrentMoveInput = bArrived ? FVector2D::ZeroVector : FVector2D(0,-1);
		}
		WalkTo(Goal);
		if (bArrived) HoldTime += Dt;
		return HoldTime > .8f;
	}
	case EShot::Trikes:
		return PlayTrikes(Dt);
	case EShot::Platform:
		if (Elapsed >= 1.f && Elapsed - Dt < 1.f)
			UE_LOG(LogTemp, Display, TEXT("Trailer: boarding wait base %s"), *GetNameSafe(Player->GetMovementBase()));
		if (Train->IsDwelling())
		{
			if (auto* Floor = Train->GetVehicleFloor(1)) WalkTo(Floor->GetComponentTransform().TransformPositionNoScale(FVector(356, 0, 0)));
			const auto* Base = Player->GetMovementBase();
			if (Base && Base->GetOwner() == Train.Get()) HoldTime += Dt;
			return HoldTime > .6f;
		}
		return false;
	case EShot::Carriage:
	{
		const auto* Rear = Train->GetVehicleFloor(2);
		const auto* Front = Train->GetVehicleFloor(1);
		if (!Rear || !Front) return false;
		// Follow the moving aisle, including the connector as the coaches turn independently.
		const FVector Points[] = { Rear->GetComponentLocation(), Rear->GetComponentLocation() + Rear->GetForwardVector()*420,
			Front->GetComponentLocation() - Front->GetForwardVector()*420, Front->GetComponentLocation() };
		float Distance = FMath::Max(0.f, Elapsed - .6f) * 180.f;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVector Segment = Points[Index+1] - Points[Index];
			const float Length = Segment.Size();
			if (Distance > Length && Index < 2) { Distance -= Length; continue; }
			const FVector Feet = Points[Index] + Segment.GetSafeNormal()*FMath::Min(Length, Distance);
			Player->SetActorLocation(Feet + FVector(0,0,10 + Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
			Player->SetAimWorldPoint(Feet + Segment.GetSafeNormal2D() * 1500.f);
			Player->AddMovementInput(Segment.GetSafeNormal2D(), .7f, true);
			break;
		}
		return Elapsed > 6.4f;
	}
	case EShot::GrayRobot:
	case EShot::BlueRobot:
		if (Shot == EShot::GrayRobot && Elapsed >= .5f && !SupplyDrop.IsValid() && !ReleaseSupplyDrop())
		{
			ReturnToTitle();
			return false;
		}
		if (Elapsed > .6f && !Robots.IsEmpty())
		{
			const int32 First = FMath::FloorToInt((Elapsed - .6f) / 1.05f) % Robots.Num();
			Target.Reset();
			for (int32 I = 0; I < Robots.Num(); ++I)
			{
				auto* Robot = Robots[(First + I) % Robots.Num()].Get();
				if (Robot && Robot->GetActorEnableCollision()) { Target = Robot; break; }
			}
		}
		if (Elapsed > .6f && Target.IsValid())
		{
			Player->AddMovementInput(Shot == EShot::BlueRobot ? FVector::RightVector : -FVector::RightVector, .35f, true);
			Shoot(Target.Get());
		}
		return Elapsed > 4.8f;
	case EShot::Barrel:
	case EShot::Apples:
	{
		auto* Barrel = Cast<ATunaSweeperExplosiveBarrelActor>(Target.Get());
		auto* Apples = Cast<ATunaSweeperBreakableAppleCrateActor>(Target.Get());
		const bool bDestroyed = !Target.IsValid() || (Barrel && Barrel->IsBarrelDestroyed()) || (Apples && Apples->IsCrateBroken());
		if (Barrel && bDestroyed)
		{
			for (auto Other : Barrels)
				if (Other.IsValid() && !Other->IsBarrelDestroyed()) return false;
		}
		if (bDestroyed)
		{
			if (Shot == EShot::Apples && HoldTime == 0)
				UE_LOG(LogTemp, Display, TEXT("Trailer: apple crate broken, %d remaining"), ExtraCrates.Num());
			HoldTime += Dt;
			if (Shot == EShot::Apples && !ExtraCrates.IsEmpty() && HoldTime >= .8f)
			{
				Target = ExtraCrates.Pop().Get();
				HoldTime = 0;
			}
		}
		else if (Elapsed > .75f) Shoot(Target.Get());
		return HoldTime > 2.2f;
	}
	case EShot::Captain:
	{
		auto* Boss = Cast<ATunaSweeperHopperEnemyCharacter>(Target.Get());
		if (!Boss || Boss->IsDead() || Boss->GetMechHealth() <= 0)
		{
			HoldTime += Dt;
			const auto* PC = Cast<ATunaSweeperPlayerController>(Player->GetController());
			return HoldTime > 5.f && PC && !PC->IsHopperShotActive();
		}
		const auto Phase = Boss->GetCombatPhase();
		if (Phase == EHopperCombatPhase::WaitingForSight)
		{
			Player->GetCharacterMovement()->MaxWalkSpeed = 500.f;
			const float Range = FVector::Dist2D(Player->GetActorLocation(), Boss->GetActorLocation());
			if (Range > 580.f) WalkTo(Boss->GetActorLocation());
			if (Elapsed >= 2.f && Range <= 600.f)
			{
				Boss->BeginEncounterBoarding(Cast<ATunaSweeperPlayerController>(Player->GetController()));
				Player->bHideTrailerLaser = Boss->GetCombatPhase() == EHopperCombatPhase::Boarding;
				Player->UpdateEquippedWeaponLaserSightBeam();
			}
		}
		if (Phase == EHopperCombatPhase::Mech && !CastChecked<ATunaSweeperPlayerController>(Player->GetController())->IsHopperShotActive())
		{
			Player->bHideTrailerLaser = false;
			MechCombatTime += Dt;
			const FVector ToBoss = (Boss->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, ToBoss);
			const float Strafe = FMath::Cos(MechCombatTime * 1.6f);
			Player->GetCharacterMovement()->MaxWalkSpeed = 240.f;
			Player->AddMovementInput(Side, Strafe * .7f, true);
			Boss->AddMovementInput(-Side, FMath::Cos(MechCombatTime * .55f), true);
			Boss->AttackTarget(Player.Get());
			if (MechCombatTime > 1.5f && FMath::Fmod(MechCombatTime, 1.8f) < 1.f)
				Shoot(Boss);
			else Player->SetAimWorldPoint(Boss->GetActorLocation());
		}
		return false;
	}
	default: return true;
	}
}

void ATrailerGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	auto* PC = Cast<ATunaSweeperPlayerController>(GetWorld()->GetFirstPlayerController());
	if (bLeaving || !PC || !PC->PlayerCameraManager) return;
	Player = Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn());
	if (!Player.IsValid()) return;
	if (auto* Hud = PC->GetGameHudWidget()) Hud->SetVisibility(ESlateVisibility::Collapsed);
	PC->bShowMouseCursor = false;
	if (!FadePanel)
	{
		FadePanel = CreateWidget<UTunaSweeperScreenFadeWidget>(PC, UTunaSweeperScreenFadeWidget::StaticClass());
		if (!FadePanel) { ReturnToTitle(); return; }
		FadePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		FadePanel->AddToViewport(20000); // Above screen-space interaction markers and gameplay transitions.
	}
	if (FadeRemaining > 0)
	{
		FadeRemaining = FMath::Max(0.f, FadeRemaining - Dt);
		SetTrailerFadeOpacity(1.f - FadeRemaining / FadeSeconds);
		bAtBlack = FadeRemaining <= 0.f;
		return; // Present the fully black frame before moving the player or changing maps.
	}
	if (bAtBlack)
	{
		bAtBlack = false;
		if (Shot == EShot::Trikes)
			for (auto& Route : Trikes)
				if (Route.Bike.IsValid()) Route.Bike->Destroy();
		for (auto Robot : Robots)
			if (Robot.IsValid()) Robot->Destroy();
		Robots.Reset();
		for (auto Barrel : Barrels)
			if (Barrel.IsValid()) Barrel->Destroy();
		Barrels.Reset();
		if (Shot == EShot::SupplyLanding)
		{
			Player->bSprintInputHeld = false;
			Player->CurrentMoveInput = FVector2D::ZeroVector;
			if (SupplyDrop.IsValid()) SupplyDrop->Destroy();
		}
		if (Shot == EShot::TeaRoom)
		{
			bLeaving = true;
			UGameplayStatics::OpenLevel(this, TEXT("/Game/MainRaid/RaidForest"), true, TravelOptions);
			return;
		}
		if (Shot == EShot::EnterGarage)
		{
			bLeaving = true;
			UGameplayStatics::OpenLevel(this, TEXT("/Game/MainRaid/RaidVillage"), true, TravelOptions);
			return;
		}
		if (Shot == EShot::Captain)
		{
			bLeaving = true;
			UGameplayStatics::OpenLevel(this, TEXT("/Game/MainRaid/RaidPlains"), true, TravelOptions);
			return;
		}
		if (Shot == EShot::ExhaustPass) { ReturnToTitle(); return; }
		Shot = static_cast<EShot>(static_cast<uint8>(Shot) + 1);
		bReady = false;
	}
	if (!bReady)
	{
		SetTrailerFadeOpacity(1.f);
		if (!SetUpShot())
		{
			UE_LOG(LogTemp, Error, TEXT("Trailer: missing subject for shot %d"), int32(Shot));
			ReturnToTitle();
			return;
		}
		PC->SetViewTarget(Player.Get());
		GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);
		IStreamingManager::Get().AddViewLocation(Player->GetActorLocation(), 1.f, false, 1.f);
		if (Shot == EShot::Platform || Shot == EShot::Carriage)
		{
			for (int32 Car : {1, 2})
			{
				const auto* Floor = Train->GetVehicleFloor(Car);
				if (!IsValid(Floor) || !Floor->IsRegistered()) { ReturnToTitle(); return; }
				IStreamingManager::Get().AddViewLocation(Floor->GetComponentLocation(), 1.f, false, 2.f);
			}
			Train->SetRunning(false);
			bWaitingForRail = true;
			bReady = true;
			return;
		}
		IStreamingManager::Get().StreamAllResources(1.f);
		bReady = true;
		UE_LOG(LogTemp, Display, TEXT("Trailer: begin shot %d"), int32(Shot));
	}
	if (bWaitingForRail)
	{
		// Let the viewport submit the new camera before checking the rail view requests.
		HoldTime += Dt;
		if (IStreamingManager::Get().StreamAllResources(.05f) != 0 || IsAsyncLoading())
		{
			if (HoldTime > 20.f) { UE_LOG(LogTemp, Error, TEXT("Trailer: rail streaming timed out")); ReturnToTitle(); }
			return;
		}
		Train->SetRunning(true);
		bWaitingForRail = false;
		HoldTime = 0;
		UE_LOG(LogTemp, Display, TEXT("Trailer: rail ready for shot %d"), int32(Shot));
	}
	SetTrailerFadeOpacity(FMath::Clamp(1.f - Elapsed / FadeSeconds, 0.f, 1.f));
	Elapsed += Dt;
	if (PlayShot(Dt))
	{
		UE_LOG(LogTemp, Display, TEXT("Trailer: completed shot %d, pawn %s, base %s"), int32(Shot), *Player->GetActorLocation().ToString(), *GetNameSafe(Player->GetMovementBase()));
		FadeRemaining = FadeSeconds;
	}
	else if (Elapsed > 40.f)
	{
		UE_LOG(LogTemp, Error, TEXT("Trailer: shot %d timed out at %s, goal %s"), int32(Shot), *Player->GetActorLocation().ToString(), *Goal.ToString());
		if (Shot == EShot::Trikes)
			for (const auto& Route : Trikes)
				if (Route.Bike.IsValid()) UE_LOG(LogTemp, Display, TEXT("Trailer: trike waypoint %d at %s"), Route.Next, *Route.Bike->GetActorLocation().ToString());
		if (auto* Boss = Cast<ATunaSweeperHopperEnemyCharacter>(Target.Get()))
			UE_LOG(LogTemp, Error, TEXT("Trailer: phase %d, mech HP %.0f, distance %.0f, rounds %d"), int32(Boss->GetCombatPhase()), Boss->GetMechHealth(), Boss->GetDistanceTo(Player.Get()), GetGameInstance<UTunaSweeperGameInstance>()->GetWeaponLoadedAmmoCount(1));
		ReturnToTitle();
	}
}
