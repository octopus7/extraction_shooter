# Unreal Engine 단일 플러그인에서 공개/비공개 구현 스위칭하기

> 목적: UE 게임 프로젝트와 플러그인의 기본 구조는 공개 저장소에서 관리하되, Steam 연동 코드와 출시용 퀘스트·대사·스포일러 에셋은 별도 비공개 저장소에서 관리한다.  
> 개발자는 하나의 UE 프로젝트와 하나의 `.uplugin`만 사용하며, 비공개 payload의 설치 여부에 따라 플러그인이 공개 구현 또는 비공개 구현으로 빌드된다.

---

## 1. 결론

이 요구사항에서는 다음 구조가 가장 현실적이다.

- UE에서 보이는 플러그인은 **하나**다.
- 플러그인의 `.uplugin`과 공개 API, 공개 fallback 구현은 공개 저장소가 소유한다.
- 플러그인 내부에 `PrivatePayload/`라는 **별도의 비공개 Git 저장소**를 clone한다.
- 비공개 저장소는 Steam 구현 코드와 출시용 퀘스트 에셋의 원본을 가진다.
- 동기화 스크립트가 비공개 파일을 UE가 인식하는 ignored 디렉터리에 복사한다.
- `Build.cs`는 동기화 완료 marker를 확인하여 `MYGAME_WITH_PRIVATE=0/1`을 정의한다.
- C++ factory/provider가 컴파일 시점에 공개 구현과 비공개 구현을 선택한다.
- 공개 빌드는 반드시 비공개 저장소가 없는 깨끗한 checkout에서 검증한다.
- Steam 출시 빌드는 공개 저장소와 비공개 저장소를 함께 checkout한 뒤 동기화하여 만든다.

최종 동작은 다음과 같다.

```text
PrivatePayload 없음
    ↓
MYGAME_WITH_PRIVATE=0
    ↓
공개 fallback 구현 + 공개 퀘스트만 사용

PrivatePayload 있음 + Sync 성공
    ↓
MYGAME_WITH_PRIVATE=1
    ↓
Steam 구현 + 출시용 비공개 퀘스트 사용
```

단, 이 구조는 “같은 C++ 클래스 파일을 덮어쓰기”가 아니다.  
**공통 인터페이스 뒤에서 구현체를 선택하는 구조**여야 한다.

---

## 2. 왜 비공개 저장소를 UE 소스 디렉터리에 그대로 넣지 않는가

UE 플러그인의 일반적인 위치는 다음과 같다.

```text
Plugins/MyGameRelease/
├─ MyGameRelease.uplugin
├─ Source/
│  └─ MyGameRelease/
│     ├─ MyGameRelease.Build.cs
│     ├─ Public/
│     └─ Private/
└─ Content/
```

C++ 파일은 모듈의 `Source/<ModuleName>/` 아래에 있어야 관리가 쉽고, 플러그인 에셋은 플러그인의 `Content/` 아래에 있어야 에디터·Asset Registry·Cook 과정에서 자연스럽게 처리된다.

하지만 하나의 nested Git 저장소 checkout은 하나의 연속된 루트 디렉터리를 가진다. 따라서 비공개 저장소 하나를 다음 두 위치에 동시에 직접 checkout할 수는 없다.

```text
Source/MyGameRelease/Private/PrivateImpl/
Content/Private/
```

심볼릭 링크나 junction으로 연결할 수도 있지만 Windows 권한, Git 설정, 에디터, Cook, CI 환경별 차이 때문에 장기 운영에는 추천하지 않는다.

따라서 이 문서는 다음 원칙을 사용한다.

```text
PrivatePayload/          ← 비공개 저장소의 원본
        ↓ Sync
Source/.../_PrivateImpl  ← UE 빌드용 복사본, 공개 Git에서 ignore
Content/_Private         ← UE 콘텐츠용 복사본, 공개 Git에서 ignore
```

복사본은 언제든 삭제하고 다시 만들 수 있어야 하며, 수정은 원칙적으로 `PrivatePayload/`에서만 한다.

---

## 3. 최종 디렉터리 구조

예시 프로젝트 이름은 `MyGame`, 플러그인 이름은 `MyGameRelease`로 가정한다.

```text
MyGame/
├─ .git/                                  # 공개 저장소
├─ .gitignore
├─ MyGame.uproject
├─ Source/
├─ Content/
└─ Plugins/
   └─ MyGameRelease/
      ├─ MyGameRelease.uplugin            # 공개
      ├─ README.md                        # 공개
      │
      ├─ Scripts/                         # 공개
      │  ├─ SyncPrivatePayload.ps1
      │  ├─ SyncPrivatePayload.sh
      │  └─ VerifyPublicSafety.ps1
      │
      ├─ PrivatePayload/                  # 비공개 nested Git 저장소
      │  ├─ .git/
      │  ├─ payload-manifest.json
      │  ├─ Source/
      │  │  ├─ SteamPrivateProvider.h
      │  │  ├─ SteamPrivateProvider.cpp
      │  │  └─ SteamQuestBridge.cpp
      │  ├─ Content/
      │  │  ├─ Quests/
      │  │  ├─ Dialogues/
      │  │  └─ AchievementMappings/
      │  └─ Config/
      │     └─ PrivateReleaseDefaults.ini
      │
      ├─ Source/
      │  └─ MyGameRelease/
      │     ├─ MyGameRelease.Build.cs      # 공개
      │     ├─ Public/
      │     │  ├─ IPlatformProvider.h
      │     │  ├─ QuestDefinition.h
      │     │  └─ MyGameReleaseSubsystem.h
      │     └─ Private/
      │        ├─ MyGameReleaseModule.cpp
      │        ├─ PlatformProviderFactory.cpp
      │        ├─ PublicFallback/
      │        │  ├─ PublicPlatformProvider.h
      │        │  └─ PublicPlatformProvider.cpp
      │        └─ _PrivateImpl/            # Sync 결과, 공개 Git ignore
      │           ├─ .private-payload-ready
      │           ├─ SteamPrivateProvider.h
      │           ├─ SteamPrivateProvider.cpp
      │           └─ SteamQuestBridge.cpp
      │
      ├─ Content/
      │  ├─ Public/
      │  │  └─ Quests/
      │  └─ _Private/                      # Sync 결과, 공개 Git ignore
      │     ├─ Quests/
      │     ├─ Dialogues/
      │     └─ AchievementMappings/
      │
      └─ Config/
         └─ _PrivateGenerated/              # Sync 결과, 공개 Git ignore
            └─ PrivateReleaseDefaults.ini
```

### 소유권

| 경로 | 공개 저장소 | 비공개 저장소 | 생성물 |
|---|---:|---:|---:|
| `.uplugin` | O | X | X |
| 공개 인터페이스 | O | X | X |
| 공개 fallback 구현 | O | X | X |
| `PrivatePayload/` | X | O | X |
| `_PrivateImpl/` | X | X | O |
| `Content/_Private/` | X | X | O |
| `Config/_PrivateGenerated/` | X | X | O |

`_PrivateImpl`, `Content/_Private`, `Config/_PrivateGenerated`는 비공개 저장소의 원본이 아니라 동기화 산출물이다.

---

## 4. 공개 저장소의 `.gitignore`

프로젝트 루트 `.gitignore`에 다음 항목을 넣는다.

```gitignore
# Unreal generated
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
*.sln

# One-plugin private payload repository
/Plugins/MyGameRelease/PrivatePayload/

# Materialized private code/content/config
/Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl/
/Plugins/MyGameRelease/Content/_Private/
/Plugins/MyGameRelease/Config/_PrivateGenerated/

# Local Steam development files
steam_appid.txt
```

검증:

```bash
git check-ignore -v Plugins/MyGameRelease/PrivatePayload/payload-manifest.json
git check-ignore -v Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl/example.cpp
git check-ignore -v Plugins/MyGameRelease/Content/_Private/example.uasset
```

주의할 점:

- `.gitignore`는 **아직 추적되지 않은 파일**에만 적용된다.
- 과거에 공개 저장소에 커밋한 파일은 `.gitignore`를 추가해도 Git 기록에서 사라지지 않는다.
- 이미 추적 중이라면 먼저 다음과 같이 index에서 제거해야 한다.

```bash
git rm -r --cached Plugins/MyGameRelease/PrivatePayload
git rm -r --cached Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl
git rm -r --cached Plugins/MyGameRelease/Content/_Private
git commit -m "Stop tracking private payload files"
```

공개 원격에 이미 push했다면 과거 기록 정리와 자격증명 교체 여부를 별도로 검토해야 한다.

---

## 5. 플러그인 descriptor

`Plugins/MyGameRelease/MyGameRelease.uplugin`:

```json
{
  "FileVersion": 3,
  "Version": 1,
  "VersionName": "1.0.0",
  "FriendlyName": "My Game Release Features",
  "Description": "Public feature API with an optional private release payload.",
  "Category": "Game",
  "CanContainContent": true,
  "EnabledByDefault": false,
  "Modules": [
    {
      "Name": "MyGameRelease",
      "Type": "Runtime",
      "LoadingPhase": "Default"
    }
  ]
}
```

플러그인은 하나이고 모듈도 하나다. 비공개 모듈을 `.uplugin`에 선택적으로 추가하지 않는다.

이렇게 하는 이유는 비공개 모듈이 존재하지 않는 공개 checkout에서도 `.uplugin`이 항상 유효해야 하기 때문이다.

---

## 6. 비공개 저장소 준비

공개 프로젝트를 먼저 clone한다.

```bash
git clone git@github.com:example/MyGame-Public.git MyGame
cd MyGame
```

그 뒤 비공개 저장소를 플러그인 내부에 clone한다.

```bash
git clone git@github.com:example/MyGame-PrivatePayload.git \
  Plugins/MyGameRelease/PrivatePayload
```

비공개 저장소 자체의 구조:

```text
PrivatePayload/
├─ payload-manifest.json
├─ Source/
├─ Content/
└─ Config/
```

예시 `payload-manifest.json`:

```json
{
  "schemaVersion": 1,
  "payloadName": "MyGame Steam Release",
  "pluginName": "MyGameRelease",
  "minimumPublicRevision": "REPLACE_WITH_PUBLIC_COMMIT",
  "unrealEngineVersion": "5.x",
  "privateFeatureVersion": "1.0.0"
}
```

이 파일은 다음 목적으로 사용한다.

- 올바른 비공개 저장소인지 확인
- 다른 플러그인의 payload를 잘못 복사하는 사고 방지
- 공개/비공개 호환 revision 기록
- 동기화 완료 marker 생성 기준

---

## 7. PowerShell 동기화 스크립트

`Plugins/MyGameRelease/Scripts/SyncPrivatePayload.ps1`:

```powershell
[CmdletBinding()]
param(
    [switch]$Public,
    [switch]$VerifyOnly
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PluginRoot = Split-Path -Parent $ScriptDir

$PayloadRoot = Join-Path $PluginRoot "PrivatePayload"
$Manifest = Join-Path $PayloadRoot "payload-manifest.json"

$SourceDestination = Join-Path `
    $PluginRoot "Source/MyGameRelease/Private/_PrivateImpl"

$ContentDestination = Join-Path `
    $PluginRoot "Content/_Private"

$ConfigDestination = Join-Path `
    $PluginRoot "Config/_PrivateGenerated"

$Marker = Join-Path `
    $SourceDestination ".private-payload-ready"

function Assert-ChildPath {
    param(
        [string]$Parent,
        [string]$Child
    )

    $ParentFull = [System.IO.Path]::GetFullPath($Parent)
    $ChildFull = [System.IO.Path]::GetFullPath($Child)

    if (-not $ChildFull.StartsWith(
        $ParentFull,
        [System.StringComparison]::OrdinalIgnoreCase
    )) {
        throw "Unsafe destination path: $ChildFull"
    }
}

function Remove-GeneratedPrivateData {
    foreach ($Path in @(
        $SourceDestination,
        $ContentDestination,
        $ConfigDestination
    )) {
        Assert-ChildPath -Parent $PluginRoot -Child $Path

        if (Test-Path $Path) {
            Remove-Item -Recurse -Force $Path
        }
    }
}

function Mirror-Directory {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        New-Item -ItemType Directory -Force `
            -Path $Destination | Out-Null
        return
    }

    New-Item -ItemType Directory -Force `
        -Path $Destination | Out-Null

    & robocopy `
        $Source `
        $Destination `
        /MIR `
        /XD ".git" `
        /XF ".git" "*.pdb" "*.obj" "*.dll" `
        /NFL /NDL /NJH /NJS /NP

    $RobocopyExitCode = $LASTEXITCODE

    # Robocopy의 0~7은 성공 또는 경미한 차이를 의미한다.
    if ($RobocopyExitCode -gt 7) {
        throw "Robocopy failed with exit code $RobocopyExitCode"
    }
}

if ($Public) {
    Remove-GeneratedPrivateData
    Write-Host "Public mode enabled. Generated private files removed."
    exit 0
}

if (-not (Test-Path $Manifest)) {
    Remove-GeneratedPrivateData

    Write-Host `
        "Private payload not installed. Public mode will be used."

    exit 0
}

$ManifestData = Get-Content $Manifest -Raw | ConvertFrom-Json

if ($ManifestData.pluginName -ne "MyGameRelease") {
    throw "Payload pluginName mismatch."
}

if ($VerifyOnly) {
    Write-Host "Private payload is valid."
    exit 0
}

Remove-GeneratedPrivateData

Mirror-Directory `
    -Source (Join-Path $PayloadRoot "Source") `
    -Destination $SourceDestination

Mirror-Directory `
    -Source (Join-Path $PayloadRoot "Content") `
    -Destination $ContentDestination

Mirror-Directory `
    -Source (Join-Path $PayloadRoot "Config") `
    -Destination $ConfigDestination

$PrivateRevision = "unknown"

try {
    $PrivateRevision = (
        git -C $PayloadRoot rev-parse HEAD
    ).Trim()
} catch {
    Write-Warning "Could not read private repository revision."
}

$MarkerContent = @"
payloadName=$($ManifestData.payloadName)
privateRevision=$PrivateRevision
syncedAtUtc=$([DateTime]::UtcNow.ToString("o"))
"@

Set-Content `
    -Path $Marker `
    -Value $MarkerContent `
    -Encoding UTF8

Write-Host "Private payload synchronized."
Write-Host "Private revision: $PrivateRevision"
Write-Host ""
Write-Host "Regenerate project files and perform a clean rebuild."
```

실행:

```powershell
# 비공개 모드
.\Plugins\MyGameRelease\Scripts\SyncPrivatePayload.ps1

# 공개 모드로 복귀
.\Plugins\MyGameRelease\Scripts\SyncPrivatePayload.ps1 -Public
```

### 왜 빌드 단계에서 자동 복사하지 않는가

`.uplugin`과 TargetRules에는 pre-build step 기능이 있지만, 새 `.cpp` 파일을 빌드 직전에 생성하거나 복사하면 UBT의 makefile·source discovery·IDE project generation 시점과 어긋날 수 있다.

따라서 다음 순서가 더 안전하다.

```text
1. payload clone/pull
2. SyncPrivatePayload 실행
3. 프로젝트 파일 재생성
4. Binaries/Intermediate 정리
5. Build 또는 Package
```

PreBuildSteps는 검증이나 리소스 생성에는 유용하지만, 이번 구조에서 C++ 소스 checkout 자체를 대신하는 핵심 수단으로 사용하지 않는 편이 좋다.

---

## 8. macOS/Linux 동기화 스크립트

`Plugins/MyGameRelease/Scripts/SyncPrivatePayload.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PAYLOAD_ROOT="${PLUGIN_ROOT}/PrivatePayload"
MANIFEST="${PAYLOAD_ROOT}/payload-manifest.json"

SOURCE_DEST="${PLUGIN_ROOT}/Source/MyGameRelease/Private/_PrivateImpl"
CONTENT_DEST="${PLUGIN_ROOT}/Content/_Private"
CONFIG_DEST="${PLUGIN_ROOT}/Config/_PrivateGenerated"
MARKER="${SOURCE_DEST}/.private-payload-ready"

remove_generated() {
  rm -rf "${SOURCE_DEST}"
  rm -rf "${CONTENT_DEST}"
  rm -rf "${CONFIG_DEST}"
}

if [[ "${1:-}" == "--public" ]]; then
  remove_generated
  echo "Public mode enabled."
  exit 0
fi

if [[ ! -f "${MANIFEST}" ]]; then
  remove_generated
  echo "Private payload not installed. Public mode will be used."
  exit 0
fi

remove_generated

mkdir -p "${SOURCE_DEST}"
mkdir -p "${CONTENT_DEST}"
mkdir -p "${CONFIG_DEST}"

rsync -a --delete \
  --exclude=".git/" \
  "${PAYLOAD_ROOT}/Source/" \
  "${SOURCE_DEST}/"

rsync -a --delete \
  --exclude=".git/" \
  "${PAYLOAD_ROOT}/Content/" \
  "${CONTENT_DEST}/"

rsync -a --delete \
  --exclude=".git/" \
  "${PAYLOAD_ROOT}/Config/" \
  "${CONFIG_DEST}/"

PRIVATE_REVISION="$(
  git -C "${PAYLOAD_ROOT}" rev-parse HEAD 2>/dev/null \
  || echo unknown
)"

cat > "${MARKER}" <<EOF
privateRevision=${PRIVATE_REVISION}
syncedAtUtc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
EOF

echo "Private payload synchronized."
echo "Regenerate project files and perform a clean rebuild."
```

권한 부여:

```bash
chmod +x Plugins/MyGameRelease/Scripts/SyncPrivatePayload.sh
```

---

## 9. `Build.cs`에서 공개/비공개 모드 결정

`Plugins/MyGameRelease/Source/MyGameRelease/MyGameRelease.Build.cs`:

```csharp
using UnrealBuildTool;
using System.IO;

public class MyGameRelease : ModuleRules
{
    public MyGameRelease(ReadOnlyTargetRules Target)
        : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "AssetRegistry"
            }
        );

        string PrivateImplDirectory = Path.Combine(
            ModuleDirectory,
            "Private",
            "_PrivateImpl"
        );

        string PrivateMarker = Path.Combine(
            PrivateImplDirectory,
            ".private-payload-ready"
        );

        bool bWithPrivatePayload =
            Directory.Exists(PrivateImplDirectory)
            && File.Exists(PrivateMarker);

        PrivateDefinitions.Add(
            "MYGAME_WITH_PRIVATE="
            + (bWithPrivatePayload ? "1" : "0")
        );

        if (bWithPrivatePayload)
        {
            PrivateIncludePaths.Add(PrivateImplDirectory);

            // Marker가 변경되면 UBT makefile이 무효화되도록 한다.
            ExternalDependencies.Add(PrivateMarker);

            // 실제 비공개 구현이 필요로 하는 모듈만 여기에 추가한다.
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "OnlineSubsystem",
                    "OnlineSubsystemUtils",
                    "OnlineSubsystemSteam"
                }
            );
        }
    }
}
```

### 중요한 규칙

`MYGAME_WITH_PRIVATE`는 `PrivateDefinitions`에 둔다.

비공개 구현의 존재 여부를 플러그인 외부 모듈에 공개할 필요가 없다면 `PublicDefinitions`로 노출하지 않는다.

C++에서는 항상 값 비교로 사용한다.

```cpp
#if MYGAME_WITH_PRIVATE
// ...
#endif
```

다음처럼 정의 여부만 확인하지 않는다.

```cpp
#ifdef MYGAME_WITH_PRIVATE
// MYGAME_WITH_PRIVATE=0이어도 true가 되므로 잘못된 사용
#endif
```

---

## 10. 공통 인터페이스

공개 저장소에 실제 게임 코드가 의존할 인터페이스를 둔다.

`Public/IPlatformProvider.h`:

```cpp
#pragma once

#include "CoreMinimal.h"

class IPlatformProvider
{
public:
    virtual ~IPlatformProvider() = default;

    virtual FName GetProviderName() const = 0;
    virtual bool IsAvailable() const = 0;

    virtual void UnlockAchievement(
        const FString& AchievementId
    ) = 0;

    virtual void SetStat(
        const FString& StatId,
        int32 Value
    ) = 0;
};
```

공개 인터페이스에는 다음을 넣지 않는다.

- Steamworks SDK 전용 타입
- Steam 전용 header
- 비공개 퀘스트 ID
- 실제 업적 이름
- 출시 일정이나 내부 endpoint
- 비공개 구현 클래스의 전방 선언

공개 인터페이스는 플랫폼 중립적이어야 한다.

---

## 11. 공개 fallback 구현

`Private/PublicFallback/PublicPlatformProvider.h`:

```cpp
#pragma once

#include "IPlatformProvider.h"

class FPublicPlatformProvider final
    : public IPlatformProvider
{
public:
    virtual FName GetProviderName() const override
    {
        return TEXT("PublicFallback");
    }

    virtual bool IsAvailable() const override
    {
        return true;
    }

    virtual void UnlockAchievement(
        const FString& AchievementId
    ) override
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("Achievement ignored in public mode: %s"),
            *AchievementId
        );
    }

    virtual void SetStat(
        const FString& StatId,
        int32 Value
    ) override
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("Stat ignored in public mode: %s=%d"),
            *StatId,
            Value
        );
    }
};
```

공개 checkout만으로도 항상 컴파일되고 실행되어야 한다.

공개 구현은 단순 no-op이어도 되고, 로컬 테스트용 mock 데이터를 반환해도 된다.

---

## 12. 비공개 Steam 구현

비공개 저장소의 `PrivatePayload/Source/SteamPrivateProvider.h`:

```cpp
#pragma once

#include "IPlatformProvider.h"

class FSteamPrivateProvider final
    : public IPlatformProvider
{
public:
    virtual FName GetProviderName() const override;
    virtual bool IsAvailable() const override;

    virtual void UnlockAchievement(
        const FString& AchievementId
    ) override;

    virtual void SetStat(
        const FString& StatId,
        int32 Value
    ) override;
};
```

동기화 후 다음 위치에 복사된다.

```text
Source/MyGameRelease/Private/_PrivateImpl/
```

UE 모듈 소스 트리 안의 `.cpp`이므로 정상적인 모듈 소스로 빌드된다.

비공개 구현 내부에서도 Steam 초기화 실패를 고려해야 한다.

```cpp
bool FSteamPrivateProvider::IsAvailable() const
{
    // 실제 프로젝트의 subsystem 초기화 확인으로 교체한다.
    return IsSteamSubsystemReady();
}
```

비공개 payload가 설치되어 있다는 이유만으로 Steam 초기화가 반드시 성공한다고 가정하면 안 된다.

---

## 13. Factory에서 구현 스위칭

`Private/PlatformProviderFactory.cpp`:

```cpp
#include "IPlatformProvider.h"
#include "PublicFallback/PublicPlatformProvider.h"

#if MYGAME_WITH_PRIVATE
#include "SteamPrivateProvider.h"
#endif

TUniquePtr<IPlatformProvider> CreatePlatformProvider()
{
#if MYGAME_WITH_PRIVATE
    TUniquePtr<IPlatformProvider> SteamProvider =
        MakeUnique<FSteamPrivateProvider>();

    if (SteamProvider->IsAvailable())
    {
        return SteamProvider;
    }
#endif

    return MakeUnique<FPublicPlatformProvider>();
}
```

선택 순서:

```text
비공개 구현이 컴파일됨
    ↓
Steam runtime 사용 가능 여부 검사
    ├─ 가능 → Steam provider
    └─ 불가능 → public fallback

비공개 구현이 컴파일되지 않음
    ↓
public fallback
```

이 방식은 에디터가 Steam 없이 실행되거나 subsystem 초기화가 실패해도 게임이 최소 기능으로 시작될 수 있게 한다.

---

## 14. Subsystem에서 사용

예시 `UGameInstanceSubsystem`:

```cpp
UCLASS()
class MYGAMERELEASE_API UMyPlatformSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(
        FSubsystemCollectionBase& Collection
    ) override;

    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void UnlockAchievement(
        const FString& AchievementId
    );

private:
    TUniquePtr<IPlatformProvider> Provider;
};
```

```cpp
void UMyPlatformSubsystem::Initialize(
    FSubsystemCollectionBase& Collection
)
{
    Super::Initialize(Collection);

    Provider = CreatePlatformProvider();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("Platform provider: %s"),
        *Provider->GetProviderName().ToString()
    );
}

void UMyPlatformSubsystem::UnlockAchievement(
    const FString& AchievementId
)
{
    if (Provider)
    {
        Provider->UnlockAchievement(AchievementId);
    }
}
```

게임 본체는 Steam 구현 클래스를 직접 include하지 않는다.

```text
게임 코드
    ↓
UMyPlatformSubsystem
    ↓
IPlatformProvider
    ↓
Public fallback 또는 Steam private provider
```

---

## 15. 퀘스트 콘텐츠 분리

공개 퀘스트:

```text
Plugins/MyGameRelease/Content/Public/Quests/
```

비공개 출시 퀘스트:

```text
PrivatePayload/Content/Quests/
```

동기화 후:

```text
Plugins/MyGameRelease/Content/_Private/Quests/
```

에디터상의 가상 경로 예시:

```text
/MyGameRelease/Public/Quests
/MyGameRelease/_Private/Quests
```

### 의존성 방향

권장:

```text
비공개 퀘스트
    → 공개 캐릭터
    → 공개 아이템
    → 공개 퀘스트 타입
```

금지에 가까운 구조:

```text
공개 맵
    → 비공개 퀘스트 직접 hard reference

공개 GameMode
    → 비공개 Data Asset 직접 hard reference
```

공개 콘텐츠가 비공개 콘텐츠를 직접 참조하면 공개 checkout에서 broken reference가 생긴다.

### 권장 로딩 방식

공개 코드에는 퀘스트 타입과 registry만 둔다.

```cpp
UCLASS(BlueprintType)
class MYGAMERELEASE_API UQuestDefinition
    : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId()
        const override
    {
        return FPrimaryAssetId(
            TEXT("Quest"),
            GetFName()
        );
    }
};
```

Asset Manager 또는 Asset Registry가 현재 존재하는 퀘스트를 검색하도록 한다.

개념 예시:

```cpp
TArray<FString> QuestPaths;

QuestPaths.Add(
    TEXT("/MyGameRelease/Public/Quests")
);

#if MYGAME_WITH_PRIVATE
QuestPaths.Add(
    TEXT("/MyGameRelease/_Private/Quests")
);
#endif

// 프로젝트의 Asset Manager 설정 또는
// ScanPathsForPrimaryAssets 호출에 QuestPaths를 사용한다.
```

실제 API signature와 설정 위치는 사용 중인 UE 버전에 맞춰 확인한다.

핵심은 게임 코드가 특정 비공개 `.uasset` 경로를 하드코딩하지 않고, 현재 설치된 quest provider 또는 Asset Manager가 목록을 발견하게 하는 것이다.

---

## 16. Steam 플러그인 활성화

비공개 코드가 `OnlineSubsystemSteam` 모듈에 의존한다면 엔진의 Steam 관련 플러그인도 해당 타깃에서 활성화되어야 한다.

선택지는 두 가지다.

### 선택 A: 프로젝트에 Optional로 공개

Steam 연동을 사용한다는 사실 자체가 민감하지 않다면 `.uproject`에 optional plugin reference를 둔다.

```json
{
  "Name": "OnlineSubsystemSteam",
  "Enabled": true,
  "Optional": true
}
```

비공개 payload가 없으면 `Build.cs`가 Steam module dependency를 추가하지 않는다.

### 선택 B: Steam 전용 Target에서 활성화

플러그인 이름조차 공개 프로젝트 설정에 남기고 싶지 않다면 Steam 출시용 Target에서만 활성화한다.

```csharp
EnablePlugins.Add("OnlineSubsystemSteam");
```

공개 Target에서는 활성화하지 않는다.

단, `Build.cs`의 private mode와 Target의 Steam plugin 활성화가 서로 일치해야 한다. CI에서 둘 중 하나만 활성화되는 조합을 허용하면 안 된다.

---

## 17. 공개/비공개 모드 전환 절차

### 공개 → 비공개

```powershell
git clone git@github.com:example/MyGame-PrivatePayload.git `
  Plugins/MyGameRelease/PrivatePayload

.\Plugins\MyGameRelease\Scripts\SyncPrivatePayload.ps1
```

그 뒤:

1. Unreal Editor 종료
2. `Binaries/` 삭제
3. `Intermediate/` 삭제
4. 프로젝트 파일 재생성
5. Editor 또는 target clean build
6. 로그에서 `Platform provider: Steam...` 확인
7. Content Browser에서 private quest 경로 확인

### 비공개 → 공개

```powershell
.\Plugins\MyGameRelease\Scripts\SyncPrivatePayload.ps1 -Public
```

필요하면 nested checkout도 제거한다.

```powershell
Remove-Item `
  -Recurse `
  -Force `
  Plugins/MyGameRelease/PrivatePayload
```

그 뒤:

1. Unreal Editor 종료
2. `Binaries/` 삭제
3. `Intermediate/` 삭제
4. Cook/Staging 출력 삭제
5. 프로젝트 파일 재생성
6. 공개 빌드
7. private asset path가 없는지 검사

### Live Coding 주의

컴파일 매크로와 소스 파일 집합이 바뀌므로 Live Coding이나 Hot Reload만으로 모드를 전환하지 않는다.

모드 변경 후에는 에디터를 재시작하고 clean build하는 편이 안전하다.

---

## 18. 주간 호환 revision 관리

공개 저장소와 비공개 저장소의 커밋 시점은 달라도 된다.

비공개 저장소의 `payload-manifest.json`에 테스트한 공개 revision을 기록한다.

```json
{
  "schemaVersion": 1,
  "pluginName": "MyGameRelease",
  "testedPublicRevision": "72ac1d884c7b40e735...",
  "privateFeatureVersion": "1.4.0",
  "unrealEngineVersion": "5.x"
}
```

주 1회 동기화 절차:

```text
1. 공개 main pull
2. 비공개 main pull
3. SyncPrivatePayload 실행
4. clean editor build
5. public-only build
6. Steam release build
7. 자동화 테스트
8. testedPublicRevision 갱신
9. 비공개 저장소 commit
```

릴리스 태그 예시:

```text
공개 저장소: release/1.4.0-public
비공개 저장소: release/1.4.0-steam
```

두 태그의 생성 시각과 commit 시각이 같을 필요는 없다.  
“이 두 revision 조합을 테스트했다”는 기록이 중요하다.

---

## 19. CI 구성

### 공개 CI

공개 CI는 비공개 저장소 접근 권한을 절대로 받지 않는다.

```text
1. 공개 저장소만 clone
2. PrivatePayload가 없는지 확인
3. generated private 경로가 없는지 확인
4. clean build
5. public package 생성
6. package 내부 private path 검사
```

검사 예시:

```bash
test ! -d Plugins/MyGameRelease/PrivatePayload
test ! -d Plugins/MyGameRelease/Content/_Private
test ! -d Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl
```

### Steam 출시 CI

```text
1. 공개 저장소 clone
2. 비공개 payload 저장소 clone
3. SyncPrivatePayload 실행
4. revision compatibility 검사
5. clean build
6. cook/package
7. Steam staging/upload
```

### 같은 workspace 재사용 금지

가장 위험한 사례:

```text
어제 Steam private build
    ↓
오늘 같은 workspace에서 public build
    ↓
이전 Cook/Staged private asset이 남음
    ↓
공개 패키지 또는 artifact에 포함
```

공개 빌드는 가능하면 매번 새 workspace/container/runner에서 만든다.

불가피하게 workspace를 재사용한다면 최소한 다음을 삭제한다.

```text
Binaries/
Intermediate/
Saved/Cooked/
Saved/StagedBuilds/
Plugins/MyGameRelease/Content/_Private/
Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl/
Plugins/MyGameRelease/Config/_PrivateGenerated/
```

---

## 20. 공개 저장소 누출 방지 검사

`Plugins/MyGameRelease/Scripts/VerifyPublicSafety.ps1`:

```powershell
$ErrorActionPreference = "Stop"

$ForbiddenPrefixes = @(
    "Plugins/MyGameRelease/PrivatePayload/",
    "Plugins/MyGameRelease/Source/MyGameRelease/Private/_PrivateImpl/",
    "Plugins/MyGameRelease/Content/_Private/",
    "Plugins/MyGameRelease/Config/_PrivateGenerated/"
)

$TrackedFiles = git ls-files

foreach ($File in $TrackedFiles) {
    foreach ($Prefix in $ForbiddenPrefixes) {
        if ($File.StartsWith(
            $Prefix,
            [System.StringComparison]::OrdinalIgnoreCase
        )) {
            throw "Private path is tracked by public Git: $File"
        }
    }
}

$StagedFiles = git diff --cached --name-only

foreach ($File in $StagedFiles) {
    foreach ($Prefix in $ForbiddenPrefixes) {
        if ($File.StartsWith(
            $Prefix,
            [System.StringComparison]::OrdinalIgnoreCase
        )) {
            throw "Private path is staged: $File"
        }
    }
}

Write-Host "Public repository safety check passed."
```

공개 push 전에 실행:

```powershell
.\Plugins\MyGameRelease\Scripts\VerifyPublicSafety.ps1
git push origin main
```

CI에서도 동일 검사를 실행한다.

### `git add -f` 주의

ignored 파일도 다음 명령으로 강제 추가할 수 있다.

```bash
git add -f <ignored-path>
```

`.gitignore`는 보안 장벽이 아니라 실수 방지 장치다.  
최종 방어선은 CI 검사와 코드 리뷰다.

---

## 21. `git clean` 주의

Git은 일반적인 `git clean -fd`에서는 nested Git 저장소를 쉽게 지우지 않도록 보호하지만, 강한 force 옵션을 사용하면 비공개 checkout이 삭제될 수 있다.

특히 다음 명령은 프로젝트 루트에서 무심코 실행하지 않는다.

```bash
git clean -ffdx
```

이 명령은 ignored 파일과 nested repository까지 제거할 수 있다.

대신 public mode 전환 스크립트가 정확한 generated private 경로만 삭제하게 한다.

CI에서는 workspace 전체를 폐기하는 방식이 더 안전하다.

---

## 22. 패키징 시 비공개 콘텐츠 확인

Steam 패키지를 만들기 전:

- `Content/_Private`가 존재하는지 확인
- marker가 존재하는지 확인
- private provider가 선택되었는지 로그 또는 automated test로 확인
- 출시 퀘스트의 Primary Asset ID가 발견되는지 확인
- 업적 mapping asset 수가 예상값과 일치하는지 확인

공개 패키지를 만들기 전:

- private generated 디렉터리가 모두 없는지 확인
- `MYGAME_WITH_PRIVATE=0` 빌드인지 확인
- Cooked Asset Registry에 private 경로가 없는지 검사
- staging 디렉터리를 재사용하지 않았는지 확인

간단한 자동화 테스트 개념:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPublicBuildHasNoPrivateQuestTest,
    "MyGame.Release.PublicBuildHasNoPrivateQuest",
    EAutomationTestFlags::ApplicationContextMask
        | EAutomationTestFlags::ProductFilter
)

bool FPublicBuildHasNoPrivateQuestTest::RunTest(
    const FString& Parameters
)
{
#if MYGAME_WITH_PRIVATE
    AddError(
        TEXT("Public build was compiled with private payload.")
    );
    return false;
#else
    return true;
#endif
}
```

Steam 전용 target에서는 반대 조건의 테스트를 둔다.

---

## 23. 비공개 저장소에 넣어도 안 되는 것

비공개 Git 저장소는 공개 노출 방지에는 도움이 되지만 secret vault가 아니다.

다음 항목은 비공개 Git에도 넣지 않는다.

- Steamworks Web API publisher key
- Steam 계정 ID와 비밀번호
- CI access token
- 클라우드 서비스 secret
- 코드 서명 private key
- 백엔드 관리자 credential
- DB password
- 개인 인증서

이 값은 다음 중 하나에서 주입한다.

- GitHub Actions Secrets
- GitLab CI variables
- 사내 secret manager
- 환경 변수
- CI runner의 보호된 파일
- 플랫폼별 인증서 저장소

Steam App ID나 클라이언트에 포함되는 achievement ID는 성격상 완전한 secret으로 보기 어렵다. Shipping client에 포함되는 정보는 결국 추출될 수 있다는 전제로 설계한다.

---

## 24. 이 구조가 보호하는 것과 보호하지 않는 것

### 보호하는 것

- 공개 GitHub 저장소에서 출시 퀘스트 원본 감춤
- 공개 코드 리뷰에서 Steam 상세 구현 감춤
- 개발 중 스토리 스포일러 노출 감소
- 공개 fork가 Steam 전용 코드를 그대로 가져가는 것 방지
- 공개/비공개 revision을 독립적으로 관리

### 보호하지 않는 것

- 출시된 게임 executable에서 코드가 분석되는 것
- packaged `.pak` 또는 IoStore container에서 에셋이 추출되는 것
- 클라이언트에 포함된 문자열과 ID가 발견되는 것
- 진짜 비밀키 보호
- 내부 개발자가 비공개 저장소 내용을 복사하는 것

출시 후 데이터 추출 방지가 목표라면 저장소 분리 외에 packaging, encryption, server-authoritative design 등을 별도로 검토해야 한다.

---

## 25. 실패하기 쉬운 설계

### 25.1 같은 파일을 공개/비공개 저장소가 동시에 소유

```text
공개 Git: Plugins/MyGameRelease/Source/.../SteamProvider.cpp
비공개 Git: 같은 경로의 SteamProvider.cpp
```

한 working tree의 같은 파일을 두 저장소 index가 동시에 관리하면 실수와 충돌이 발생하기 쉽다.

### 25.2 private branch만 별도 remote에 push

하나의 Git object database를 공유하기 때문에 잘못된 merge/push로 과거 private commit이 공개 remote에 전달될 위험이 있다.

### 25.3 공개 콘텐츠가 private asset을 hard reference

private payload가 없는 checkout에서 missing reference가 발생한다.

### 25.4 Build.cs가 비공개 파일을 자동 생성

Build.cs 평가 중 작업 트리를 수정하거나 네트워크 clone을 수행하지 않는다. 프로젝트 파일 생성, incremental build, UBT cache와 충돌하기 쉽다.

### 25.5 같은 workspace에서 public/private package 반복

이전 cook 결과가 다음 package에 섞일 수 있다.

---

## 26. 운영 체크리스트

### 최초 구성

- [ ] 공개 플러그인 생성
- [ ] 공개 fallback만으로 프로젝트 빌드
- [ ] `PrivatePayload/` ignore
- [ ] generated private 경로 ignore
- [ ] private repository 생성
- [ ] payload manifest 추가
- [ ] sync script 추가
- [ ] `Build.cs` marker 분기 추가
- [ ] provider factory 추가
- [ ] public-only CI 추가
- [ ] private release CI 추가

### 매일 개발

- [ ] 비공개 수정은 `PrivatePayload/`에서 수행
- [ ] 수정 후 sync script 실행
- [ ] generated 복사본은 직접 수정하지 않음
- [ ] public Git commit 전 safety script 실행
- [ ] private Git 상태를 별도로 확인

```bash
git status
git -C Plugins/MyGameRelease/PrivatePayload status
```

### 공개 push 전

- [ ] forbidden path가 public Git에 추적되지 않음
- [ ] staged file 검사 통과
- [ ] public-only checkout에서 빌드 성공
- [ ] private 문자열·퀘스트 ID grep 검사
- [ ] 공개 package에 private asset 경로가 없음

### Steam 출시 전

- [ ] private payload revision 고정
- [ ] public revision 고정
- [ ] sync marker revision 확인
- [ ] clean cook
- [ ] Steam provider 사용 확인
- [ ] 퀘스트 수와 업적 mapping 검증
- [ ] secret이 Git 또는 client package에 없음

---

## 27. 추천 최종 형태

```text
MyGameRelease.uplugin                  ← 하나
MyGameRelease 모듈                    ← 하나

공개 저장소
├─ 공통 인터페이스
├─ 퀘스트 타입
├─ 플랫폼 중립 subsystem
├─ public fallback
├─ Build.cs switch
├─ sync/검증 스크립트
└─ 공개 퀘스트

비공개 nested 저장소
├─ Steam provider
├─ 실제 업적 mapping
├─ 출시 퀘스트
├─ 대사/스포일러
└─ 호환 revision manifest

Build.cs
├─ marker 없음 → MYGAME_WITH_PRIVATE=0
└─ marker 있음 → MYGAME_WITH_PRIVATE=1

Runtime factory
├─ private provider 사용 가능 → private
└─ 그 외 → public fallback
```

이 구조에서는 UE 관점에서 플러그인은 하나다.  
Git 관점에서는 공개 플러그인 skeleton과 비공개 payload가 서로 다른 이력을 가진다.

가장 중요한 운영 원칙은 다음 세 가지다.

1. **같은 파일을 두 저장소가 동시에 추적하지 않는다.**
2. **공개 구현만으로도 항상 빌드 가능해야 한다.**
3. **공개 패키지는 비공개 파일이 한 번도 존재하지 않았던 깨끗한 workspace에서 만든다.**

---

## 참고 자료

- Unreal Engine Plugins  
  https://dev.epicgames.com/documentation/unreal-engine/plugins-in-unreal-engine

- Unreal Engine Modules  
  https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-modules

- Unreal Build Tool Module Properties  
  https://dev.epicgames.com/documentation/unreal-engine/module-properties-in-unreal-engine

- Unreal Build Tool Target Reference  
  https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-build-tool-target-reference

- Unreal Engine Asset Management  
  https://dev.epicgames.com/documentation/unreal-engine/asset-management-in-unreal-engine

- Git `.gitignore` documentation  
  https://git-scm.com/docs/gitignore

- Git `check-ignore` documentation  
  https://git-scm.com/docs/git-check-ignore

- Git `clean` documentation  
  https://git-scm.com/docs/git-clean
