#pragma once
#include "Commandlets/Commandlet.h"
#include "TunaSweeperPipeGenerateCommandlet.generated.h"
UCLASS()
class UTunaSweeperPipeGenerateCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    virtual int32 Main(const FString& Params) override;
};
