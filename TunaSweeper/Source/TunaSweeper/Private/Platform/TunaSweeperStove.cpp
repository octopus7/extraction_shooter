#include "Platform/TunaSweeperStove.h"

#if WITH_TUNASWEEPER_STOVE

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Platform/TunaSweeperStoveCloudPolicy.h"
#include "Platform/TunaSweeperStovePolicy.h"
#include "TunaSweeperStoveCredentials.h"

THIRD_PARTY_INCLUDES_START
#include "Windows/AllowWindowsPlatformTypes.h"
#include "BaseSDK.h"
#include "OwnershipSDK.h"
#include "Windows/HideWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY_STATIC(LogTunaStove, Log, All);

namespace TunaSweeperStove
{
    namespace
    {
        using namespace Stove::PCSDK;
        enum class EStartupState { Pending, Ready, Relaunch, Failed };
        EStartupState State = EStartupState::Pending;
        bool bBaseStarted = false;
        bool bOwnershipStarted = false;
        FTSTicker::FDelegateHandle TickHandle;
        FDelegateHandle ExitHandle;
        FString FailureStage;
        uint32 FailureCode = 0;
        FString CloudSaveRoot;
        FString SaveAccountId;

        void CacheSaveLocation()
        {
            Base::StovePCUser User;
            const Result UserResult = Base::Base_GetUser(&User);
            if (UserResult.IsSuccessful() && User.GetMemberNumber() != 0)
            {
                SaveAccountId = LexToString(User.GetMemberNumber());
            }

            // Allow long configured paths. Length is in wchar_t elements, not bytes.
            TArray<wchar_t> PathBuffer;
            PathBuffer.SetNumZeroed(32768);
            const Result PathResult = Base::Base_GetCloudSavingPath(PathBuffer.GetData(), PathBuffer.Num());
            if (PathResult.IsSuccessful() && PathBuffer.Last() == 0
                && IsUsableCloudSaveRoot(PathBuffer.GetData()))
            {
                CloudSaveRoot = PathBuffer.GetData();
                FPaths::NormalizeDirectoryName(CloudSaveRoot);
                UE_LOG(LogTunaStove, Display, TEXT("STOVE launcher cloud save directory is available."));
            }
            else
            {
                // Never log the configured path or member number; both can identify the account.
                UE_LOG(LogTunaStove, Warning, TEXT("STOVE cloud save directory unavailable (code %u); using local saves."),
                    PathResult.GetResultCode());
            }
        }

        void Fail(const TCHAR* Stage, uint32 Code)
        {
            FailureStage = Stage;
            FailureCode = Code;
            State = EStartupState::Failed;
            // SDK messages may contain account data; log only our stage and numeric result.
            UE_LOG(LogTunaStove, Error, TEXT("STOVE %s failed (code %u)."), Stage, Code);
        }

        void __cdecl OnOwnership(CallbackResult Callback, Ownership::StovePCOwnership* Items, uint32_t Count)
        {
            const Result ResultValue = Callback.GetResult();
            if (!ResultValue.IsSuccessful())
            {
                Fail(TEXT("ownership query"), ResultValue.GetResultCode());
                return;
            }
            if (Count == 0 || !Items)
            {
                // A successful request can still return no entitlements. There is no
                // GameCode to compare in this case; distinguish it from a type mismatch.
                Fail(TEXT("empty ownership list"), 0);
                return;
            }
            FString OwnershipSummary;
            if (Items)
            {
                for (uint32_t Index = 0; Index < Count; ++Index)
                {
                    if (Index < 8)
                    {
                        const wchar_t* ItemGameId = Items[Index].GetGameId();
                        const bool bSameGame = ItemGameId && FCString::Strcmp(
                            ItemGameId, TunaSweeperStoveCredentials::GameId) == 0;
                        OwnershipSummary += FString::Printf(TEXT(" [id=%s type=%u owned=%u]"),
                            bSameGame ? TEXT("match") : TEXT("other"),
                            static_cast<uint32>(Items[Index].GetGameCode()),
                            static_cast<uint32>(Items[Index].GetOwnershipCode()));
                    }
                    if (IsOwnedGame(TunaSweeperStoveCredentials::GameId, Items[Index].GetGameId(),
                        static_cast<uint32>(Items[Index].GetGameCode()),
                        static_cast<uint32>(Items[Index].GetOwnershipCode())))
                    {
                        State = EStartupState::Ready;
                        UE_LOG(LogTunaStove, Display, TEXT("STOVE initialization and ownership verified."));
                        return;
                    }
                }
            }
            // Keep account identifiers out of diagnostics while making rejected entries actionable.
            Fail(*FString::Printf(TEXT("game ownership entries=%u%s"), Count, *OwnershipSummary), 0);
        }

        void __cdecl OnInitialized(CallbackResult Callback)
        {
            const Result ResultValue = Callback.GetResult();
            if (!ResultValue.IsSuccessful())
            {
                Fail(TEXT("initialization"), ResultValue.GetResultCode());
                return;
            }
            const Result OwnershipResult = Ownership::Ownership_Initialize();
            if (!OwnershipResult.IsSuccessful())
            {
                Fail(TEXT("ownership initialization"), OwnershipResult.GetResultCode());
                return;
            }
            bOwnershipStarted = true;
            Ownership::Ownership_OwnershipList(OnOwnership);
        }

        void __cdecl OnLauncherChecked(CallbackResult Callback, bool bRestartRequired)
        {
            const Result ResultValue = Callback.GetResult();
            if (!ResultValue.IsSuccessful())
            {
                Fail(TEXT("launcher check"), ResultValue.GetResultCode());
                return;
            }
            if (bRestartRequired)
            {
                State = EStartupState::Relaunch;
                return;
            }
            Base::Base_InitializeEx(OnInitialized);
        }
    }

    void Shutdown()
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
        TickHandle.Reset();
        FCoreDelegates::OnPreExit.Remove(ExitHandle);
        ExitHandle.Reset();
        if (bOwnershipStarted)
        {
            Ownership::Ownership_UnInitialize();
            bOwnershipStarted = false;
        }
        if (bBaseStarted)
        {
            Base::Base_UnInitialize();
            bBaseStarted = false;
        }
    }

    void Startup()
    {
        if (IsRunningCommandlet() || IsRunningDedicatedServer()) return;
        State = EStartupState::Pending;
        CloudSaveRoot.Reset();
        SaveAccountId.Reset();
        // Keep parameters alive throughout the asynchronous launcher and initialization flow.
        Base::StovePCInitializeParam Parameters;
        Parameters.SetEnvironment(L"LIVE");
        Parameters.SetGameID(TunaSweeperStoveCredentials::GameId);
        Parameters.SetApplicationKey(TunaSweeperStoveCredentials::ApplicationKey);
        bBaseStarted = true;
        Base::Base_RestartAppIfNecessaryAsync(&Parameters, 60000, OnLauncherChecked);

        // Module startup precedes world loading: no gameplay/save access before authorization.
        const double Deadline = FPlatformTime::Seconds() + 90.0;
        while (State == EStartupState::Pending && !IsEngineExitRequested())
        {
            Base::Base_RunCallback();
            if (State == EStartupState::Pending && FPlatformTime::Seconds() >= Deadline)
                Fail(TEXT("authentication timeout"), 0);
            FPlatformProcess::Sleep(0.01f);
        }
        if (State != EStartupState::Ready || IsEngineExitRequested())
        {
            Shutdown();
            if (State == EStartupState::Failed && !FApp::IsUnattended())
            {
                FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(
                    TEXT("STOVE 인증을 완료하지 못했습니다. STOVE 런처에 로그인한 뒤 라이브러리에서 게임을 실행해 주세요.\n\nSTOVE authentication failed. Please sign in and start the game from your STOVE library.\n(%s: %u)"),
                    *FailureStage, FailureCode)));
            }
            FPlatformMisc::RequestExitWithStatus(false, State == EStartupState::Relaunch ? 0 : 1);
            return;
        }
        CacheSaveLocation();
        TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
        {
            Base::Base_RunCallback();
            return true;
        }));
        // Base shutdown records play time; run before engine teardown, with module shutdown as fallback.
        ExitHandle = FCoreDelegates::OnPreExit.AddStatic(&Shutdown);
    }

    bool TryGetCloudSaveRoot(FString& OutDirectory)
    {
        OutDirectory = State == EStartupState::Ready ? CloudSaveRoot : FString();
        return !OutDirectory.IsEmpty();
    }

    bool TryGetSaveAccountId(FString& OutAccountId)
    {
        OutAccountId = State == EStartupState::Ready ? SaveAccountId : FString();
        return !OutAccountId.IsEmpty();
    }
}

#else

namespace TunaSweeperStove
{
    void Startup() {}
    void Shutdown() {}
    bool TryGetCloudSaveRoot(FString& OutDirectory) { OutDirectory.Reset(); return false; }
    bool TryGetSaveAccountId(FString& OutAccountId) { OutAccountId.Reset(); return false; }
}

#endif
