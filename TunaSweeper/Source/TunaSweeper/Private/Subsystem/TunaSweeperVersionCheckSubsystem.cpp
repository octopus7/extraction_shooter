#include "Subsystem/TunaSweeperVersionCheckSubsystem.h"
#include "Settings/TunaSweeperVersionSettings.h"
#include "Json.h"

void UTunaSweeperVersionCheckSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTunaSweeperVersionCheckSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTunaSweeperVersionCheckSubsystem::RequestVersionCheck()
{
	const UTunaSweeperVersionSettings* VersionSettings = GetDefault<UTunaSweeperVersionSettings>();
	FString TargetUrl = VersionSettings->VersionCheckUrl;

	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

	Request->OnProcessRequestComplete().BindUObject(this, &UTunaSweeperVersionCheckSubsystem::OnVersionCheckResponse);
	Request->SetURL(TargetUrl);
	Request->SetVerb("GET");
	
	// 캐시 방지를 위해 헤더 추가
	Request->SetHeader(TEXT("Cache-Control"), TEXT("no-cache, no-store, must-revalidate"));
	Request->SetHeader(TEXT("Pragma"), TEXT("no-cache"));
	Request->SetHeader(TEXT("Expires"), TEXT("0"));
	
	Request->ProcessRequest();
}

void UTunaSweeperVersionCheckSubsystem::OnVersionCheckResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bool bIsAllowed = false;
	FString OutMessage = TEXT("네트워크 오류가 발생했습니다. 잠시 후 다시 시도해 주세요.");
	FString OutUrl = TEXT("");

	if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
	{
		const UTunaSweeperVersionSettings* VersionSettings = GetDefault<UTunaSweeperVersionSettings>();
		int32 CurrentBuildNumber = VersionSettings->InternalBuildNumber;

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			// 화이트리스트 체크 (allowed_builds)
			const TArray<TSharedPtr<FJsonValue>>* AllowedBuildsArray;
			if (JsonObject->TryGetArrayField(TEXT("allowed_builds"), AllowedBuildsArray))
			{
				for (const TSharedPtr<FJsonValue>& BuildVal : *AllowedBuildsArray)
				{
					int32 AllowedBuild = 0;
					if (BuildVal->TryGetNumber(AllowedBuild))
					{
						if (AllowedBuild == CurrentBuildNumber)
						{
							bIsAllowed = true;
							break;
						}
					}
				}
			}

			// 허용되지 않은 버전인 경우 메시지와 URL 파싱
			if (!bIsAllowed)
			{
				bool bSpecificFound = false;
				
				// 1. 개별 대응 메시지 (specific_rejects) 확인
				const TSharedPtr<FJsonObject>* SpecificRejectsObj;
				if (JsonObject->TryGetObjectField(TEXT("specific_rejects"), SpecificRejectsObj))
				{
					FString BuildKey = FString::FromInt(CurrentBuildNumber);
					const TSharedPtr<FJsonObject>* SpecificInfo;
					if ((*SpecificRejectsObj)->TryGetObjectField(BuildKey, SpecificInfo))
					{
						(*SpecificInfo)->TryGetStringField(TEXT("message"), OutMessage);
						(*SpecificInfo)->TryGetStringField(TEXT("update_url"), OutUrl);
						bSpecificFound = true;
					}
				}

				// 2. 개별 대응이 없으면 기본 실패 메시지 (default_reject) 적용
				if (!bSpecificFound)
				{
					const TSharedPtr<FJsonObject>* DefaultRejectObj;
					if (JsonObject->TryGetObjectField(TEXT("default_reject"), DefaultRejectObj))
					{
						(*DefaultRejectObj)->TryGetStringField(TEXT("message"), OutMessage);
						(*DefaultRejectObj)->TryGetStringField(TEXT("update_url"), OutUrl);
					}
					else
					{
						OutMessage = TEXT("현재 버전은 더 이상 지원하지 않습니다. 최신 버전으로 업데이트 해주세요.");
					}
				}
			}
		}
		else
		{
			OutMessage = TEXT("버전 정보를 확인할 수 없습니다. (데이터 파싱 오류)");
		}
	}

	// 정상 접속인 경우 메시지와 URL을 비움
	if (bIsAllowed)
	{
		OutMessage = TEXT("");
		OutUrl = TEXT("");
	}

	OnVersionCheckCompleted.Broadcast(bIsAllowed, OutMessage, OutUrl);
}
