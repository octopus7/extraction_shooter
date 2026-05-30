#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Debuff/TunaSweeperDebuffTypes.h"
#include "TunaSweeperDebuffComponent.generated.h"

class AActor;

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperDebuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperDebuffComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff")
	TArray<FTunaSweeperActiveDebuffState> GetActiveDebuffs() const { return ActiveDebuffs; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff")
	bool HasDebuff(FName DebuffId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debuff")
	bool TryApplyDebuff(FName DebuffId, int32 ApplyChanceBonus = 0, float DurationBonusSeconds = 0.0f, AActor* SourceActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debuff")
	bool RemoveDebuff(FName DebuffId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debuff")
	int32 RemoveDebuffs(const TArray<FName>& DebuffIds);

protected:
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_ActiveDebuffs, Category = "TunaSweeper|Debuff")
	TArray<FTunaSweeperActiveDebuffState> ActiveDebuffs;

	UFUNCTION()
	void OnRep_ActiveDebuffs();

	UFUNCTION(Server, Reliable)
	void ServerTryApplyDebuff(FName DebuffId, int32 ApplyChanceBonus, float DurationBonusSeconds, AActor* SourceActor);

	UFUNCTION(Server, Reliable)
	void ServerRemoveDebuff(FName DebuffId);

	UFUNCTION(Server, Reliable)
	void ServerRemoveDebuffs(const TArray<FName>& DebuffIds);

private:
	bool HasAuthority() const;
	bool ApplyDebuffInternal(FName DebuffId, int32 ApplyChanceBonus, float DurationBonusSeconds, AActor* SourceActor);
	bool RemoveDebuffInternal(FName DebuffId);
	int32 RemoveDebuffsInternal(const TArray<FName>& DebuffIds);
	void ApplyTickDamage(const FTunaSweeperActiveDebuffState& DebuffState);
	void NormalizeActiveDebuffs();
	void MarkDebuffsChanged();

	int32 NextAppliedOrder = 0;
};
