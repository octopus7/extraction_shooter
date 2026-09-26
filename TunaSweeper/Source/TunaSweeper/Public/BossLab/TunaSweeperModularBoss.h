#pragma once

#include "CoreMinimal.h"
#include "BossLab/TunaSweeperBossDefinition.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperModularBoss.generated.h"

class APawn;
class ATunaSweeperAttackTelegraph;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperFactionComponent;

/** Validated definitions are copied once; all health and attack state belongs to this runtime. */
UCLASS()
class TUNASWEEPER_API ATunaSweeperModularBoss : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperModularBoss();
	bool InitializeBoss(const FTunaSweeperBossDefinition& Definition, bool bPreview, FName& OutError);
	void BeginCombat(APawn* Target);
	void StopCombat();
	void SetSelectedPart(int32 InstanceId);
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	bool IsDefeated() const { return GetCoreHealth() <= 0.f; }
	bool IsWarningActive() const { return !Attacks.IsEmpty(); }
	float GetCoreHealth() const;
	float GetCoreMaxHealth() const;
	float GetPartHealth(int32 InstanceId) const;
	bool IsPartOperational(int32 InstanceId) const;
	int32 GetOperationalPartCount() const;
	UStaticMeshComponent* GetPartComponent(int32 InstanceId) const;
	FBox GetAssemblyLocalBounds() const { return AssemblyBounds; }
	float GetGroundOffset() const { return -AssemblyBounds.Min.Z + 4.f; }
	float GetMovementRadius() const;
	void KeepInsideArena();

private:
	struct FRuntimePart
	{
		FTunaSweeperBossPart Definition;
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		float Health = 0.f;
		bool bOperational = true;
	};
	struct FPendingAttack
	{
		int32 PartId = INDEX_NONE;
		FName ModuleId;
		FVector Origin = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		float Elapsed = 0.f;
		float Duration = 1.f;
		TWeakObjectPtr<ATunaSweeperAttackTelegraph> Warning;
	};
	void UpdateMovement(float DeltaSeconds);
	void UpdateAttacks(float DeltaSeconds);
	void ScheduleAttack(FRuntimePart& Part);
	void ExecuteAttack(const FPendingAttack& Attack);
	void DisableBranch(int32 InstanceId);
	void UpdatePartAppearance(FRuntimePart& Part);
	void ClearParts();
	FBox GetOperationalBounds(const FRotator& Rotation) const;
	bool TryClampArenaPosition(const FVector& Position, const FBox& Bounds, FVector& Result) const;
	FVector GetMuzzle(const FRuntimePart& Part) const;
	FRuntimePart* FindPart(int32 InstanceId);
	const FRuntimePart* FindPart(int32 InstanceId) const;
	UMaterialInterface* GetPartMaterial(FName ModuleId) const;

	UPROPERTY() TObjectPtr<USceneComponent> AssemblyRoot;
	UPROPERTY() TObjectPtr<UTunaSweeperFactionComponent> Faction;
	UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY() TObjectPtr<UMaterialInterface> ArmorMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> TealMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> DarkMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> AmberMaterial;
	UPROPERTY() TObjectPtr<UMaterialInterface> SelectedMaterial;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> PartMeshes;
	TArray<FRuntimePart> Parts;
	TArray<FPendingAttack> Attacks;
	TArray<TWeakObjectPtr<AActor>> CombatActors;
	TWeakObjectPtr<APawn> CombatTarget;
	FTunaSweeperBossDefinition Design;
	FBox AssemblyBounds = FBox(FVector(-50.f), FVector(50.f));
	float AttackCountdown = 1.5f;
	int32 NextWeapon = 0;
	int32 SelectedPart = INDEX_NONE;
	bool bPreviewMode = true;
	bool bCombatEnabled = false;
};
