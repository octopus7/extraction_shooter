#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperNoiseSubsystem.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperNoiseEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FVector SourceLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	float Loudness = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	float MaxRange = 1800.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FName NoiseTag = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperHeardNoiseEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FVector SourceLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FVector ListenerLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FVector DirectionFromListener = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	float Distance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	float Strength = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	FName NoiseTag = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Noise")
	TObjectPtr<AActor> InstigatorActor = nullptr;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FTunaSweeperNoiseReportedSignature, const FTunaSweeperNoiseEvent&);

UCLASS()
class TUNASWEEPER_API UTunaSweeperNoiseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void ReportNoiseAtLocation(
		const FVector& SourceLocation,
		float Loudness = 1.0f,
		float MaxRange = 1800.0f,
		FName NoiseTag = NAME_None,
		AActor* SourceActor = nullptr,
		AActor* InstigatorActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	bool CalculateHeardNoiseAtLocation(
		const FTunaSweeperNoiseEvent& NoiseEvent,
		const FVector& ListenerLocation,
		float ListenerHearingRange,
		float ListenerSensitivity,
		float ListenerMinStrength,
		FTunaSweeperHeardNoiseEvent& OutHeardNoise,
		AActor* ListenerActor = nullptr) const;

	FTunaSweeperNoiseReportedSignature OnNoiseReported;
};
