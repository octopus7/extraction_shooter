#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperVisionVisibilitySubsystem.generated.h"

class UTunaSweeperVisionSubjectComponent;

UCLASS()
class TUNASWEEPER_API UTunaSweeperVisionVisibilitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterVisionSubject(UTunaSweeperVisionSubjectComponent* VisionSubject);
	void UnregisterVisionSubject(UTunaSweeperVisionSubjectComponent* VisionSubject);
	void RestoreAllVisionSubjects();

	template <typename PredicateType>
	void ForEachVisionSubject(PredicateType Predicate)
	{
		CompactVisionSubjects();
		for (const TWeakObjectPtr<UTunaSweeperVisionSubjectComponent>& VisionSubject : VisionSubjects)
		{
			if (UTunaSweeperVisionSubjectComponent* Subject = VisionSubject.Get())
			{
				Predicate(Subject);
			}
		}
	}

private:
	void CompactVisionSubjects();

	TArray<TWeakObjectPtr<UTunaSweeperVisionSubjectComponent>> VisionSubjects;
};
