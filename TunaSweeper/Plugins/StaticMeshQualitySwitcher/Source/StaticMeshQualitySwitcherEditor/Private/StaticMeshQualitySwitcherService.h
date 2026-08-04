#pragma once

#include "CoreMinimal.h"

class AActor;
class UStaticMesh;
class UStaticMeshComponent;
class UStaticMeshQualityProfile;

enum class EStaticMeshQualityTarget : uint8
{
	Original,
	Low
};

enum class EStaticMeshQualityScope : uint8
{
	SelectedActors,
	LoadedLevel
};

struct FStaticMeshQualityApplyResult
{
	bool bSucceeded = false;
	int32 InspectedActorCount = 0;
	int32 InspectedComponentCount = 0;
	int32 ChangedComponentCount = 0;
	int32 UnmappedComponentCount = 0;
	TArray<FText> Errors;
};

class FStaticMeshQualitySwitcherService
{
public:
	static FStaticMeshQualityApplyResult Apply(
		const UStaticMeshQualityProfile* Profile,
		EStaticMeshQualityTarget Target,
		EStaticMeshQualityScope Scope);

private:
	static void GatherActors(EStaticMeshQualityScope Scope, TArray<AActor*>& OutActors);
};
