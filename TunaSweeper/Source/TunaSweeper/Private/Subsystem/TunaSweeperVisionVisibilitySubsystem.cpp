#include "Subsystem/TunaSweeperVisionVisibilitySubsystem.h"

#include "Component/TunaSweeperVisionSubjectComponent.h"

void UTunaSweeperVisionVisibilitySubsystem::RegisterVisionSubject(
	UTunaSweeperVisionSubjectComponent* VisionSubject)
{
	if (!VisionSubject)
	{
		return;
	}

	CompactVisionSubjects();
	VisionSubjects.AddUnique(VisionSubject);
}

void UTunaSweeperVisionVisibilitySubsystem::UnregisterVisionSubject(
	UTunaSweeperVisionSubjectComponent* VisionSubject)
{
	if (!VisionSubject)
	{
		return;
	}

	VisionSubjects.RemoveAll(
		[VisionSubject](const TWeakObjectPtr<UTunaSweeperVisionSubjectComponent>& Candidate)
		{
			return !Candidate.IsValid() || Candidate.Get() == VisionSubject;
		});
}

void UTunaSweeperVisionVisibilitySubsystem::RestoreAllVisionSubjects()
{
	CompactVisionSubjects();
	for (const TWeakObjectPtr<UTunaSweeperVisionSubjectComponent>& VisionSubject : VisionSubjects)
	{
		if (UTunaSweeperVisionSubjectComponent* Subject = VisionSubject.Get())
		{
			Subject->ResetVisionVisibility();
		}
	}
}

void UTunaSweeperVisionVisibilitySubsystem::CompactVisionSubjects()
{
	VisionSubjects.RemoveAll(
		[](const TWeakObjectPtr<UTunaSweeperVisionSubjectComponent>& Candidate)
		{
			return !Candidate.IsValid();
		});
}
