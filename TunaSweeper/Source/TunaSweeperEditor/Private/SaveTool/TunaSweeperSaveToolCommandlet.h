#pragma once
#include "Commandlets/Commandlet.h"
#include "TunaSweeperSaveToolCommandlet.generated.h"

UCLASS()
class UTunaSweeperSaveToolCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UTunaSweeperSaveToolCommandlet();
	virtual int32 Main(const FString& Params) override;
};
