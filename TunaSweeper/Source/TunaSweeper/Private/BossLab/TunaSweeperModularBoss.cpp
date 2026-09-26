#include "BossLab/TunaSweeperModularBoss.h"

#include "AI/TunaSweeperAttackTelegraph.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaBossRuntime
{
	constexpr float TurnDegreesPerSecond = 65.f;
	constexpr float FacingTolerance = 14.f;
	constexpr int32 SimultaneousAttackLimit = 4;
	constexpr int32 ProjectileLimit = 36;
	const FVector ArenaCenter(14400.f, 10000.f, 0.f);
	constexpr float ArenaHalfSize = 2100.f;
}

ATunaSweeperModularBoss::ATunaSweeperModularBoss()
{
	PrimaryActorTick.bCanEverTick = true;
	AssemblyRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Assembly"));
	SetRootComponent(AssemblyRoot);
	Faction = CreateDefaultSubobject<UTunaSweeperFactionComponent>(TEXT("Faction"));
	Faction->SetFactionId(TunaSweeperFactionIds::Enemy);
	SetCanBeDamaged(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Armor(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Armor.M_CP_Armor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Teal(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Teal.M_CP_Teal"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dark(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Dark.M_CP_Dark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Amber(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Amber.M_CP_Amber"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Highlight(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Glow.M_CP_Glow"));
	CubeMesh = Cube.Object; CylinderMesh = Cylinder.Object;
	ArmorMaterial = Armor.Object; TealMaterial = Teal.Object; DarkMaterial = Dark.Object;
	AmberMaterial = Amber.Object; SelectedMaterial = Highlight.Object;
}

bool ATunaSweeperModularBoss::InitializeBoss(const FTunaSweeperBossDefinition& Definition, bool bPreview, FName& OutError)
{
	// Validate before touching the existing assembly so failed previews leave it intact.
	if (!TunaSweeperBossDefinition::Validate(Definition, OutError)) return false;
	TMap<int32, FTransform> Transforms;
	if (!TunaSweeperBossDefinition::BuildTransforms(Definition, Transforms)) return false;
	StopCombat();
	ClearParts();
	Design = Definition;
	bPreviewMode = bPreview;
	AssemblyBounds = FBox(ForceInit);
	SelectedPart = INDEX_NONE;
	for (const FTunaSweeperBossPart& Part : Design.Parts)
	{
		const auto* Module = TunaSweeperBossDefinition::FindModule(Part.ModuleId);
		const FTransform* Transform = Transforms.Find(Part.InstanceId);
		if (!Module || !Transform) continue; // Both were verified by Validate.
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this,
			MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("BossPart")));
		AddInstanceComponent(Mesh);
		Mesh->SetupAttachment(AssemblyRoot);
		Mesh->SetStaticMesh(CubeMesh);
		Mesh->SetRelativeTransform(*Transform);
		Mesh->SetRelativeScale3D(Module->HalfExtent / 50.f);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetMaterial(0, GetPartMaterial(Part.ModuleId));
		Mesh->RegisterComponent();
		PartMeshes.Add(Mesh);
		FRuntimePart Runtime;
		Runtime.Definition = Part; Runtime.Mesh = Mesh; Runtime.Health = Module->Health;
		Parts.Add(Runtime);
		AssemblyBounds += FBox(-Module->HalfExtent, Module->HalfExtent).TransformBy(*Transform);
	}
	Faction->SetCanBeCombatTarget(!bPreview);
	SetCanBeDamaged(!bPreview);
	return Parts.Num() == Definition.Parts.Num();
}

UMaterialInterface* ATunaSweeperModularBoss::GetPartMaterial(FName ModuleId) const
{
	if (ModuleId == TEXT("core") || ModuleId == TEXT("laser")) return TealMaterial;
	if (ModuleId == TEXT("cannon") || ModuleId == TEXT("missile")) return AmberMaterial;
	if (ModuleId == TEXT("drive")) return DarkMaterial;
	return ArmorMaterial;
}

void ATunaSweeperModularBoss::ClearParts()
{
	for (UStaticMeshComponent* Mesh : PartMeshes) if (IsValid(Mesh)) Mesh->DestroyComponent();
	PartMeshes.Reset(); Parts.Reset();
}

void ATunaSweeperModularBoss::BeginCombat(APawn* Target)
{
	StopCombat();
	CombatTarget = Target;
	bCombatEnabled = !bPreviewMode && IsValid(Target) && !IsDefeated();
	AttackCountdown = 1.5f;
	NextWeapon = 0;
}

void ATunaSweeperModularBoss::StopCombat()
{
	bCombatEnabled = false;
	CombatTarget.Reset();
	for (FPendingAttack& Attack : Attacks) if (Attack.Warning.IsValid()) Attack.Warning->Destroy();
	Attacks.Reset();
	for (const auto& Actor : CombatActors) if (Actor.IsValid()) Actor->Destroy();
	CombatActors.Reset();
}

void ATunaSweeperModularBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bCombatEnabled || !CombatTarget.IsValid() || IsDefeated()) return;
	if (const auto* Player = Cast<ATunaSweeperTopDownCharacter>(CombatTarget.Get()); Player && Player->IsDead())
	{
		StopCombat(); return;
	}
	const float Step = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	UpdateMovement(Step);
	UpdateAttacks(Step);
}

void ATunaSweeperModularBoss::UpdateMovement(float DeltaSeconds)
{
	// Every warning locks both the advertised lane and the weapon origin until execution.
	if (!Attacks.IsEmpty()) return;
	const FVector ToTarget = CombatTarget->GetActorLocation() - GetActorLocation();
	const FVector Direction = ToTarget.GetSafeNormal2D();
	if (Direction.IsNearlyZero()) return;
	FRotator Rotation = GetActorRotation();
	Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, Direction.Rotation().Yaw, TunaBossRuntime::TurnDegreesPerSecond * DeltaSeconds);
	const float Radius = GetMovementRadius();
	const FBox RotatedBounds = GetOperationalBounds(Rotation);
	FVector RotationPosition;
	if (ToTarget.SizeSquared2D() >= FMath::Square(Radius + 80.f) &&
		TryClampArenaPosition(GetActorLocation(), RotatedBounds, RotationPosition) && RotationPosition.Equals(GetActorLocation(), 0.1f))
		SetActorRotation(Rotation);
	int32 Drives = 0;
	for (const FRuntimePart& Part : Parts) if (Part.bOperational && Part.Definition.ModuleId == TEXT("drive")) ++Drives;
	if (Drives == 0) return;
	const float Distance = ToTarget.Size2D();
	float Desired = Design.Tactic == ETunaSweeperBossTactic::Advance ? 450.f :
		Design.Tactic == ETunaSweeperBossTactic::KeepDistance ? 1200.f : 850.f;
	float Sign = Distance > Desired + 120.f ? 1.f : Distance < Desired - 120.f ? -1.f : 0.f;
	if (Sign == 0.f) return;
	const float Speed = FMath::Min(240.f, 110.f + Drives * 35.f);
	FVector Next = GetActorLocation() + Direction * (Sign * Speed * DeltaSeconds);
	const FBox Bounds = GetOperationalBounds(GetActorRotation());
	FVector ClampedNext;
	if (!TryClampArenaPosition(Next, Bounds, ClampedNext)) return;
	Next = ClampedNext;
	// Keep the whole assembly clear of the player; do not move child collision through a pawn.
	if (FVector::DistSquared2D(Next, CombatTarget->GetActorLocation()) < FMath::Square(Radius + 80.f)) return;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossLabMovement), false, this);
	FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FHitResult Hit;
	// Sweep the whole surviving assembly. A root-only sphere misses long side branches.
	if (!GetWorld()->SweepSingleByObjectType(Hit, GetActorLocation() + Bounds.GetCenter(), Next + Bounds.GetCenter(),
		FQuat::Identity, Objects, FCollisionShape::MakeBox(Bounds.GetExtent()), Query)) SetActorLocation(Next);
}

FBox ATunaSweeperModularBoss::GetOperationalBounds(const FRotator& Rotation) const
{
	FBox Bounds(ForceInit);
	for (const FRuntimePart& Part : Parts)
	{
		if (!Part.bOperational || !Part.Mesh.IsValid()) continue;
		const auto* Module = TunaSweeperBossDefinition::FindModule(Part.Definition.ModuleId);
		if (!Module) continue;
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector Vertex(Module->HalfExtent.X * (Corner & 1 ? 1.f : -1.f),
				Module->HalfExtent.Y * (Corner & 2 ? 1.f : -1.f), Module->HalfExtent.Z * (Corner & 4 ? 1.f : -1.f));
			Bounds += Rotation.RotateVector(Part.Mesh->GetRelativeTransform().TransformPositionNoScale(Vertex));
		}
	}
	return Bounds.IsValid ? Bounds : FBox(FVector(-1.f), FVector(1.f));
}

float ATunaSweeperModularBoss::GetMovementRadius() const
{
	float RadiusSquared = 0.f;
	for (const FRuntimePart& Part : Parts)
	{
		if (!Part.bOperational || !Part.Mesh.IsValid()) continue;
		const auto* Module = TunaSweeperBossDefinition::FindModule(Part.Definition.ModuleId);
		if (!Module) continue;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			const FVector Vertex(Module->HalfExtent.X * (Corner & 1 ? 1.f : -1.f),
				Module->HalfExtent.Y * (Corner & 2 ? 1.f : -1.f), 0.f);
			const FVector FromRoot = Part.Mesh->GetRelativeTransform().TransformPositionNoScale(Vertex);
			RadiusSquared = FMath::Max(RadiusSquared, static_cast<float>(FromRoot.SizeSquared2D()));
		}
	}
	return FMath::Sqrt(RadiusSquared) + 40.f;
}

bool ATunaSweeperModularBoss::TryClampArenaPosition(const FVector& Position, const FBox& Bounds, FVector& Result) const
{
	Result = Position;
	for (int32 Axis = 0; Axis < 2; ++Axis)
	{
		const double Minimum = TunaBossRuntime::ArenaCenter[Axis] - TunaBossRuntime::ArenaHalfSize - Bounds.Min[Axis];
		const double Maximum = TunaBossRuntime::ArenaCenter[Axis] + TunaBossRuntime::ArenaHalfSize - Bounds.Max[Axis];
		// A long diagonal may not fit at some yaw angles. Reject that rotation rather
		// than introducing a tighter definition limit or teleporting the assembly.
		if (Minimum > Maximum) return false;
		Result[Axis] = FMath::Clamp(Position[Axis], Minimum, Maximum);
	}
	return true;
}

void ATunaSweeperModularBoss::KeepInsideArena()
{
	FVector Position;
	if (TryClampArenaPosition(GetActorLocation(), GetOperationalBounds(GetActorRotation()), Position)) SetActorLocation(Position);
}

FVector ATunaSweeperModularBoss::GetMuzzle(const FRuntimePart& Part) const
{
	const auto* Module = TunaSweeperBossDefinition::FindModule(Part.Definition.ModuleId);
	if (!Part.Mesh.IsValid() || !Module) return GetActorLocation();
	// Keep fire in the player's fighting plane even for a weapon mounted above the core.
	const FVector Forward = GetActorForwardVector();
	FVector Result = Part.Mesh->GetComponentLocation() + Forward * (Module->HalfExtent.X + 28.f);
	Result.Z = CombatTarget.IsValid() ? CombatTarget->GetActorLocation().Z : Result.Z;
	return Result;
}

void ATunaSweeperModularBoss::UpdateAttacks(float DeltaSeconds)
{
	CombatActors.RemoveAll([](const TWeakObjectPtr<AActor>& Actor) { return !Actor.IsValid(); });
	for (int32 Index = Attacks.Num() - 1; Index >= 0; --Index)
	{
		FPendingAttack& Attack = Attacks[Index];
		if (!IsPartOperational(Attack.PartId))
		{
			if (Attack.Warning.IsValid()) Attack.Warning->Destroy();
			Attacks.RemoveAt(Index); continue;
		}
		Attack.Elapsed += DeltaSeconds;
		if (Attack.Warning.IsValid()) Attack.Warning->SetProgress(FMath::Clamp(Attack.Elapsed / Attack.Duration, 0.f, 1.f));
		if (Attack.Elapsed >= Attack.Duration)
		{
			const FPendingAttack Completed = Attack;
			Attacks.RemoveAt(Index);
			ExecuteAttack(Completed);
			if (Completed.Warning.IsValid())
			{
				Completed.Warning->SetLifeSpan(0.18f);
				CombatActors.Add(Completed.Warning.Get());
			}
		}
	}
	AttackCountdown -= DeltaSeconds;
	if (AttackCountdown > 0.f || !Attacks.IsEmpty()) return;
	const FVector TargetDirection = (CombatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetDirection.Rotation().Yaw)) > TunaBossRuntime::FacingTolerance) return;
	TArray<int32> Weapons;
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		const auto* Module = TunaSweeperBossDefinition::FindModule(Parts[Index].Definition.ModuleId);
		if (Parts[Index].bOperational && Module && Module->bWeapon) Weapons.Add(Index);
	}
	if (Weapons.IsEmpty()) { AttackCountdown = 1.f; return; }
	const int32 Count = Design.bAlternateWeapons ? 1 : FMath::Min(Weapons.Num(), TunaBossRuntime::SimultaneousAttackLimit);
	for (int32 Index = 0; Index < Count; ++Index) ScheduleAttack(Parts[Weapons[(NextWeapon + Index) % Weapons.Num()]]);
	NextWeapon = (NextWeapon + Count) % Weapons.Num();
	const bool bSecondPhase = GetCoreHealth() <= GetCoreMaxHealth() * Design.PhaseThreshold;
	AttackCountdown = Design.AttackInterval * (bSecondPhase ? 0.75f : 1.f);
}

void ATunaSweeperModularBoss::ScheduleAttack(FRuntimePart& Part)
{
	FPendingAttack Attack;
	Attack.PartId = Part.Definition.InstanceId; Attack.ModuleId = Part.Definition.ModuleId;
	Attack.Origin = GetMuzzle(Part);
	const FVector TargetPoint = CombatTarget->GetActorLocation();
	FVector Direction = (TargetPoint - Attack.Origin).GetSafeNormal2D();
	if (Direction.IsNearlyZero()) Direction = GetActorForwardVector();
	Attack.Duration = Attack.ModuleId == TEXT("missile") ? 1.3f : Attack.ModuleId == TEXT("laser") ? 1.1f : 0.65f;
	Attack.End = Attack.ModuleId == TEXT("missile") ? TargetPoint : Attack.Origin + Direction * 2300.f;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(BossLabAttackLane), false, this);
	Query.AddIgnoredActor(CombatTarget.Get());
	FHitResult Blocker;
	if (Attack.ModuleId != TEXT("missile") && GetWorld()->LineTraceSingleByChannel(Blocker,
		Attack.Origin, Attack.End, ECC_Visibility, Query)) Attack.End = Blocker.ImpactPoint;
	FActorSpawnParameters Parameters; Parameters.Owner = this;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Warning = GetWorld()->SpawnActor<ATunaSweeperAttackTelegraph>(
		ATunaSweeperAttackTelegraph::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
	if (!Warning) return;
	Attack.Warning = Warning;
	FVector GroundStart = Attack.Origin; GroundStart.Z = 0.f;
	FVector GroundEnd = Attack.End; GroundEnd.Z = 0.f;
	if (Attack.ModuleId == TEXT("missile")) Warning->InitCircle(GroundEnd, 180.f, Attack.Duration);
	else Warning->InitLane(GroundStart, GroundEnd, Attack.ModuleId == TEXT("laser") ? 50.f : 24.f, Attack.Duration);
	Attacks.Add(Attack);
}

void ATunaSweeperModularBoss::ExecuteAttack(const FPendingAttack& Attack)
{
	if (!bCombatEnabled || !IsPartOperational(Attack.PartId)) return;
	const auto* Module = TunaSweeperBossDefinition::FindModule(Attack.ModuleId);
	if (!Module) return;
	const FVector Direction = (Attack.End - Attack.Origin).GetSafeNormal();
	if (Attack.ModuleId == TEXT("cannon"))
	{
		if (CombatActors.Num() >= TunaBossRuntime::ProjectileLimit) return;
		const FTransform Transform(Direction.Rotation(), Attack.Origin);
		auto* Projectile = GetWorld()->SpawnActorDeferred<ATunaSweeperProjectile>(
			ATunaSweeperProjectile::StaticClass(), Transform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile)
		{
			Projectile->SetDamageAmount(Module->Damage);
			Projectile->SetSpeedMultiplier(0.65f);
			Projectile->FinishSpawning(Transform);
			CombatActors.Add(Projectile);
		}
		return;
	}
	const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	TArray<AActor*> Candidates;
	if (Factions) Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Hostile, Candidates);
	for (AActor* Victim : Candidates)
	{
		if (!IsValid(Victim) || !Factions->CanTargetActor(this, Victim)) continue;
		float Radius = 0.f, HalfHeight = 0.f; Victim->GetSimpleCollisionCylinder(Radius, HalfHeight);
		const FVector Location = Victim->GetActorLocation();
		FVector Source;
		if (Attack.ModuleId == TEXT("missile"))
		{
			if (FVector::Dist2D(Location, Attack.End) > 180.f + Radius || FMath::Abs(Location.Z - Attack.End.Z) > HalfHeight + 100.f) continue;
			Source = Attack.End;
		}
		else
		{
			Source = FMath::ClosestPointOnSegment(Location, Attack.Origin, Attack.End);
			if (FVector::Dist2D(Location, Source) > 50.f + Radius || FMath::Abs(Location.Z - Source.Z) > HalfHeight + 40.f) continue;
		}
		FCollisionQueryParams Query(SCENE_QUERY_STAT(BossLabAreaAttack), false, this);
		Query.AddIgnoredActor(Victim);
		FHitResult Blocker;
		const FVector SightStart = Attack.ModuleId == TEXT("laser") ? Attack.Origin : Source;
		if (!GetWorld()->LineTraceSingleByChannel(Blocker, SightStart, Location, ECC_Visibility, Query))
			UGameplayStatics::ApplyDamage(Victim, Module->Damage, nullptr, this, nullptr);
	}
	FVector EffectPoint = Attack.End; EffectPoint.Z = 4.f;
	auto* Effect = ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::Impact,
		EffectPoint, Attack.ModuleId == TEXT("missile") ? 180.f : 80.f, FVector::UpVector, this);
	if (Effect) CombatActors.Add(Effect);
}

float ATunaSweeperModularBoss::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bPreviewMode || IsDefeated() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f) return 0.f;
	const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	const AActor* Source = DamageCauser ? DamageCauser : EventInstigator;
	if (Factions && !Factions->CanApplyCombatEffect(Source, this)) return 0.f;
	FRuntimePart* Victim = nullptr;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const auto& Point = static_cast<const FPointDamageEvent&>(DamageEvent);
		for (FRuntimePart& Part : Parts) if (Part.Mesh.Get() == Point.HitInfo.GetComponent()) { Victim = &Part; break; }
		if (!Victim) return 0.f;
	}
	else
	{
		for (FRuntimePart& Part : Parts) if (Part.Definition.ModuleId == TEXT("core")) { Victim = &Part; break; }
	}
	if (!Victim || !Victim->bOperational) return 0.f;
	const float Adjusted = Victim->Definition.ModuleId == TEXT("armor") ? FMath::Max(1.f, DamageAmount - 3.f) : DamageAmount;
	const float Applied = FMath::Min(Victim->Health, Adjusted);
	Victim->Health -= Applied;
	if (Victim->Health <= 0.f)
	{
		const int32 DestroyedId = Victim->Definition.InstanceId;
		const FVector Location = Victim->Mesh.IsValid() ? Victim->Mesh->GetComponentLocation() : GetActorLocation();
		DisableBranch(DestroyedId);
		auto* Effect = ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::RobotDeath,
			Location, 100.f, FVector::UpVector, this);
		if (Effect) CombatActors.Add(Effect);
	}
	else UpdatePartAppearance(*Victim);
	if (IsDefeated()) { Faction->SetCanBeCombatTarget(false); StopCombat(); }
	return Applied;
}

void ATunaSweeperModularBoss::DisableBranch(int32 InstanceId)
{
	TSet<int32> Disabled{InstanceId};
	bool bAdded = true;
	while (bAdded)
	{
		bAdded = false;
		for (const FRuntimePart& Part : Parts)
			if (Disabled.Contains(Part.Definition.ParentId) && !Disabled.Contains(Part.Definition.InstanceId))
			{
				Disabled.Add(Part.Definition.InstanceId); bAdded = true;
			}
	}
	for (FRuntimePart& Part : Parts)
	{
		if (!Disabled.Contains(Part.Definition.InstanceId)) continue;
		Part.bOperational = false;
		Part.Health = 0.f;
		if (Part.Mesh.IsValid())
		{
			Part.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part.Mesh->SetVisibility(false);
		}
	}
	for (int32 Index = Attacks.Num() - 1; Index >= 0; --Index)
		if (Disabled.Contains(Attacks[Index].PartId))
		{
			if (Attacks[Index].Warning.IsValid()) Attacks[Index].Warning->Destroy();
			Attacks.RemoveAt(Index);
		}
	// Shots travel in a horizontal plane. As lower supports break, the remaining
	// assembly settles onto the floor, keeping the next support and finally the
	// core physically reachable. The editor definition and relative transforms stay intact.
	float SurvivingMinZ = TNumericLimits<float>::Max();
	for (const FRuntimePart& Part : Parts)
	{
		if (!Part.bOperational || !Part.Mesh.IsValid()) continue;
		const auto* Module = TunaSweeperBossDefinition::FindModule(Part.Definition.ModuleId);
		if (Module) SurvivingMinZ = FMath::Min(SurvivingMinZ,
			static_cast<float>(Part.Mesh->GetRelativeLocation().Z - Module->HalfExtent.Z));
	}
	if (SurvivingMinZ < TNumericLimits<float>::Max())
	{
		FVector Settled = GetActorLocation();
		Settled.Z = 4.f - SurvivingMinZ;
		SetActorLocation(Settled);
	}
}

void ATunaSweeperModularBoss::UpdatePartAppearance(FRuntimePart& Part)
{
	if (!Part.Mesh.IsValid()) return;
	const auto* Module = TunaSweeperBossDefinition::FindModule(Part.Definition.ModuleId);
	UMaterialInterface* Material = Part.Definition.InstanceId == SelectedPart ? SelectedMaterial.Get() :
		Module && Part.Health < Module->Health * 0.4f ? DarkMaterial.Get() : GetPartMaterial(Part.Definition.ModuleId);
	Part.Mesh->SetMaterial(0, Material);
}

void ATunaSweeperModularBoss::SetSelectedPart(int32 InstanceId)
{
	SelectedPart = bPreviewMode ? InstanceId : INDEX_NONE;
	for (FRuntimePart& Part : Parts) UpdatePartAppearance(Part);
}

ATunaSweeperModularBoss::FRuntimePart* ATunaSweeperModularBoss::FindPart(int32 InstanceId)
{
	return Parts.FindByPredicate([InstanceId](const FRuntimePart& Part) { return Part.Definition.InstanceId == InstanceId; });
}
const ATunaSweeperModularBoss::FRuntimePart* ATunaSweeperModularBoss::FindPart(int32 InstanceId) const
{
	return Parts.FindByPredicate([InstanceId](const FRuntimePart& Part) { return Part.Definition.InstanceId == InstanceId; });
}
float ATunaSweeperModularBoss::GetCoreHealth() const
{
	for (const FRuntimePart& Part : Parts) if (Part.Definition.ModuleId == TEXT("core")) return Part.Health;
	return 0.f;
}
float ATunaSweeperModularBoss::GetCoreMaxHealth() const
{
	const auto* Module = TunaSweeperBossDefinition::FindModule(TEXT("core"));
	return Module ? Module->Health : 1.f;
}
float ATunaSweeperModularBoss::GetPartHealth(int32 InstanceId) const
{
	const FRuntimePart* Part = FindPart(InstanceId); return Part ? Part->Health : 0.f;
}
bool ATunaSweeperModularBoss::IsPartOperational(int32 InstanceId) const
{
	const FRuntimePart* Part = FindPart(InstanceId); return Part && Part->bOperational;
}
int32 ATunaSweeperModularBoss::GetOperationalPartCount() const
{
	int32 Count = 0; for (const FRuntimePart& Part : Parts) if (Part.bOperational) ++Count; return Count;
}
UStaticMeshComponent* ATunaSweeperModularBoss::GetPartComponent(int32 InstanceId) const
{
	const FRuntimePart* Part = FindPart(InstanceId); return Part ? Part->Mesh.Get() : nullptr;
}
void ATunaSweeperModularBoss::EndPlay(const EEndPlayReason::Type Reason)
{
	StopCombat();
	Super::EndPlay(Reason);
}
