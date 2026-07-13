#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TunaSweeperPetCompanionCharacter.generated.h"

class UTunaSweeperFactionComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPetCompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATunaSweeperPetCompanionCharacter();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Pet Companion")
	void SetFollowTarget(AActor* NewTarget);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pet Companion")
	AActor* GetFollowTarget() const { return FollowTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pet Companion")
	float GetDistanceToFollowTarget() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Pet Companion")
	bool HasValidFollowTarget() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Faction")
	UTunaSweeperFactionComponent* GetFactionComponent() const { return FactionComponent; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperFactionComponent> FactionComponent;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "TunaSweeper|Pet Companion")
	TObjectPtr<AActor> FollowTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Pet Companion|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FollowDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Pet Companion|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFollowDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Pet Companion|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopDistance = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Pet Companion|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PetWalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Pet Companion|Movement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PetRunSpeed = 600.0f;

protected:
	virtual void BeginPlay() override;
};
