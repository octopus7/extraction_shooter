#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "TunaSweeperEffectCandidateSubsystem.generated.h"

/** Observed asset requests, not proof of a rendered frame or a measured hitch. */
struct FTunaSweeperEffectCandidate
{
	int64 RequestCount = 0;
	TSet<FString> Sources;
};

/** Diagnostic collection only: never loads assets, retains objects, or warms effects. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperEffectCandidateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static void Record(const UObject* Context, const FSoftObjectPath& AssetPath, const TCHAR* Source);
	static void RecordObject(const UObject* Context, const UObject* Asset, const TCHAR* Source);
	void AddCandidate(const FSoftObjectPath& AssetPath, const FString& Source);
	void ResetCandidates() { Candidates.Reset(); }
	const TMap<FSoftObjectPath, FTunaSweeperEffectCandidate>& GetCandidates() const { return Candidates; }
	FString ToJson() const;
	bool ExportCandidates() const;
	virtual void Deinitialize() override;

private:
	TMap<FSoftObjectPath, FTunaSweeperEffectCandidate> Candidates;
};
