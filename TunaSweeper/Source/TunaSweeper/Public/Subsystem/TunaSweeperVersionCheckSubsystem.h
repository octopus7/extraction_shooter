#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "TunaSweeperVersionCheckSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVersionCheckCompleted, bool, bIsAllowed, const FString&, Message, const FString&, UpdateUrl);

UCLASS()
class TUNASWEEPER_API UTunaSweeperVersionCheckSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 세팅에 정의된 스태틱 웹페이지 URL로 버전 체크 JSON을 요청합니다. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Version")
	void RequestVersionCheck();

	/** 버전 체크가 완료되면 발생합니다. (허용 여부, 출력 메시지, 브라우저 이동 URL) */
	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Version")
	FOnVersionCheckCompleted OnVersionCheckCompleted;

private:
	void OnVersionCheckResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
