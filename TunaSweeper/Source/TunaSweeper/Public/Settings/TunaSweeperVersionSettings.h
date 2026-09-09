#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TunaSweeperVersionSettings.generated.h"

UCLASS(Config=Game, defaultconfig, meta=(DisplayName="TunaSweeper Version"))
class TUNASWEEPER_API UTunaSweeperVersionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UTunaSweeperVersionSettings();

	/** 내부 화이트리스트 검사 및 핫픽스 식별을 위한 빌드 넘버 (예: 1000) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Version")
	int32 InternalBuildNumber;

	/** 유저에게 노출되는 표시용 버전 문자열 (예: "v1.0.0") */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Version")
	FString PublicVersionString;

	/** 버전 체크 JSON을 요청할 스태틱 웹페이지 URL */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Version")
	FString VersionCheckUrl;
};
