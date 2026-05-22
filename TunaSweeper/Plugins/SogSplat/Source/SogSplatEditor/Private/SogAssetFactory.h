#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SogAssetFactory.generated.h"

UCLASS()
class USogAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	USogAssetFactory();

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
};
