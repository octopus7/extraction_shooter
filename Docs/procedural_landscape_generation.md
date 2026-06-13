# Procedural Landscape Generation

이 문서는 imagegen 기반 프로토타입 환경에서 UE Landscape를 코드로 만들고, 여러 지형 텍스처 레이어를 절차적 weightmap으로 자동 페인팅하는 과정을 나중에 다시 구현할 수 있도록 정리한 것이다.

## 목표

- 별도 확인용 맵 `/Game/PrototypeImagegenEnvironmentMap`에 수동 작업 없이 바닥 Landscape를 생성한다.
- imagegen으로 만든 지형 텍스처를 Landscape 레이어 머티리얼에 연결한다.
- 콘크리트, 흙자갈, 풀이끼, 어두운 균열 레이어를 코드에서 만든 weightmap으로 자동 페인팅한다.
- 소품 배치 높이도 같은 height 평가식으로 맞춰 Landscape 위에 자연스럽게 얹는다.

## 레이아웃 기준

Landscape와 소품 배치는 랜덤 노이즈를 먼저 만들고 그 위에 소품을 흩뿌리는 방식으로 만들면 안 된다. `Docs/SSOT/area_unlocking.md`의 단일 레이드 맵 기준을 먼저 반영해야 한다.

현재 프로토타입은 다음 구역 마스크를 사용한다.

- 남쪽 시작/초기 외부 clearing
- 중앙 저위험 파밍권
- 시작점에서 중앙 파밍권을 지나 북쪽으로 이어지는 main route
- 초반 숲길
- 서쪽 우회로와 초기 구역으로 돌아오는 shortcut 감각
- 콘크리트 더미로 막힌 반복 이동 편의성 장애물
- 동쪽 선택형 보급/파밍 pocket
- 북쪽 고급보안구역
- 외곽 숲 밀도

이 구역 마스크가 heightmap, weightmap, 소품 placement에 모두 같은 기준으로 들어가야 한다. 그래야 기본 큐브 같은 기준 오브젝트를 올려 봤을 때도 “게임 설계상의 배치”가 보인다.

## 현재 구현 위치

- 에디터 생성 코드: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperExperimentalVegetation.cpp`
- 자동 실행 등록: `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperEditor.cpp`
- 모듈 의존성: `TunaSweeper/Source/TunaSweeperEditor/TunaSweeperEditor.Build.cs`
- source art: `TunaSweeper/Content/SourceArt/EnvironmentPrototype/`
- 생성 에셋: `/Game/Prototype/Environment`
- 생성 맵: `/Game/PrototypeImagegenEnvironmentMap`

## 입력 이미지

현재 프로토타입은 지형 레이어를 한 장의 2x2 아틀라스로 관리한다.

- 파일: `T_PrototypeLandscapeLayerAtlas_Source.png`
- 위치: `TunaSweeper/Content/SourceArt/EnvironmentPrototype/`
- 2x2 구성:
  - 좌상단: `Concrete`
  - 우상단: `DirtGravel`
  - 좌하단: `MossGrass`
  - 우하단: `DarkCracks`

이 파일은 `EnsureImagegenEnvironmentTextureAsset()`을 통해 `/Game/Prototype/Environment/T_ImagegenLandscapeLayerAtlas`로 저장된다.

## 모듈과 include

`TunaSweeperEditor.Build.cs`에 필요하다.

```csharp
"Landscape",
"Foliage",
```

주요 include:

```cpp
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionLandscapeLayerCoords.h"
```

`LandscapeEdit.h`가 foliage 헤더를 포함하므로 `Foliage` 모듈이 빠지면 `InstancedFoliageActor.h` include 오류가 난다.

## 레이어 정의

레이어 이름, LayerInfo 에셋 이름, 디버그 색상, 아틀라스 사분면 UV offset을 하나의 구조체로 묶는다.

```cpp
struct FImagegenLandscapeLayerDefinition
{
    FName LayerName;
    FString LayerInfoAssetName;
    FLinearColor DebugColor;
    FVector2f AtlasOffset;
    float PreviewWeight;
};
```

현재 레이어:

```cpp
Concrete   -> LI_ImagegenLandscape_Concrete   -> offset (0.0, 0.0)
DirtGravel -> LI_ImagegenLandscape_DirtGravel -> offset (0.5, 0.0)
MossGrass  -> LI_ImagegenLandscape_MossGrass  -> offset (0.0, 0.5)
DarkCracks -> LI_ImagegenLandscape_DarkCracks -> offset (0.5, 0.5)
```

## LayerInfo 생성

각 레이어마다 `ULandscapeLayerInfoObject`를 만든다.

핵심 설정:

```cpp
LayerInfo->SetLayerName(LayerName, false);
LayerInfo->SetLayerUsageDebugColor(DebugColor, false, EPropertyChangeType::ValueSet);
LayerInfo->SetHardness(0.35f, false, EPropertyChangeType::ValueSet);
LayerInfo->SetMinimumCollisionRelevanceWeight(0.12f, false, EPropertyChangeType::ValueSet);
LayerInfo->SetBlendMethod(ELandscapeTargetLayerBlendMethod::FinalWeightBlending, false);
```

`FinalWeightBlending`을 쓰면 레거시 weight blend 방식으로 레이어 합이 조정된다. 코드 쪽에서도 각 샘플의 네 레이어 합이 255가 되도록 정규화해 넣는다.

## Landscape 머티리얼 생성

생성 에셋:

- `/Game/Prototype/Environment/M_ImagegenLandscape_AutoPainted`

머티리얼 구조:

1. `UMaterialExpressionLandscapeLayerCoords`로 Landscape XY 좌표를 만든다.
2. 각 레이어마다 좌표를 `* 0.5 + AtlasOffset` 해서 2x2 아틀라스의 사분면을 샘플링한다.
3. `UMaterialExpressionLandscapeLayerBlend`에 네 레이어를 `LB_WeightBlend`로 연결한다.
4. 결과를 `BaseColor`와 `EmissiveColor`에 연결한다.

현재 프로토타입 맵은 조명을 별도로 보장하지 않으므로 Landscape 머티리얼은 `MSM_Unlit`으로 만든다. `DefaultLit`으로 만들면 레이어와 텍스처가 정상이어도 Lit 뷰포트에서 검게 보일 수 있다.

현재 좌표 스케일:

```cpp
LayerCoordinates->MappingType = TCMT_XY;
LayerCoordinates->MappingScale = 380.0f;
```

## Landscape 크기

현재 프로토타입 크기:

```cpp
ImagegenLandscapeComponentCount = 2;
ImagegenLandscapeNumSubsections = 1;
ImagegenLandscapeSubsectionSizeQuads = 63;
ImagegenLandscapeQuads = 126;
ImagegenLandscapeVerts = 127;
ImagegenLandscapeScaleXY = 40.0f;
ImagegenLandscapeScaleZ = 24.0f;
ImagegenLandscapeWorldSize = 5040.0f;
```

즉 약 50.4m 정사각형 Landscape다. 기존 소품 배치 범위가 대략 48m x 36m라 확인용 바닥 여유가 있다.

## Heightmap 생성

heightmap은 `TArray<uint16>`로 만든다. 샘플 개수는 `ImagegenLandscapeVerts * ImagegenLandscapeVerts`다.

현재 height는 다음 요소를 섞는다.

- 큰 Perlin noise
- 작은 Perlin noise
- 중앙 이동 경로를 약간 낮추는 path mask
- 나무 주변 완만한 상승
- 바위/잔해 주변 완만한 상승

월드 높이 cm를 Landscape uint16 height로 인코딩한다.

```cpp
uint16 EncodeImagegenLandscapeHeight(float HeightCm)
{
    const int32 Encoded = 32768 + FMath::RoundToInt(HeightCm * 128.0f / ImagegenLandscapeScaleZ);
    return static_cast<uint16>(FMath::Clamp(Encoded, 0, 65535));
}
```

## Weightmap 생성

레이어별 weightmap은 `TArray<TArray<uint8>>`로 만든다. 각 레이어 배열 크기도 `ImagegenLandscapeVerts * ImagegenLandscapeVerts`다.

현재 평가 요소:

- `StartClearing`: 시작/초기 외부는 평탄하고 낮은 콘크리트/흙 혼합
- `CentralFarm`: 중앙 저위험 파밍권은 흙/풀 비중 증가
- `MainRoute`: 주 동선은 콘크리트 비중 증가
- `ForestTrail`, `WestBypass`: 숲길과 서쪽 우회로는 풀/흙 비중 증가
- `ConcreteRubble`: 콘크리트 더미는 콘크리트/어두운 균열 비중 증가
- `SupplyPocket`: 선택형 파밍 pocket은 흙자갈/잔해 비중 증가
- `HighSecurity`: 북쪽 고급보안구역은 콘크리트/어두운 균열 비중 증가
- `BorderForest`: 외곽 숲은 풀/나무 밀도 증가
- `PatchNoise`: 구역 안의 자연스러운 불규칙성
- `FineNoise`: 미세 균열과 표면 변동
- tree placement: 주변 `MossGrass`, `DirtGravel` 증가
- grass placement: 주변 `MossGrass` 증가
- rock placement: 주변 `DirtGravel`, `DarkCracks` 증가
- concrete slab placement: 주변 `Concrete`, `DarkCracks` 증가

마지막에는 네 레이어의 합이 255가 되도록 정규화한다.

## Landscape 생성과 import

`SpawnImagegenLandscapeActor()`에서 `ALandscape`를 스폰하고 `ALandscape::Import()`를 호출한다.

핵심 순서:

```cpp
ALandscape* Landscape = World->SpawnActor<ALandscape>(Location, FRotator::ZeroRotator, SpawnParameters);
Landscape->LandscapeMaterial = LandscapeMaterial;
Landscape->SetActorRelativeScale3D(FVector(ImagegenLandscapeScaleXY, ImagegenLandscapeScaleXY, ImagegenLandscapeScaleZ));

TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
HeightDataPerLayers.Add(FGuid(), MoveTemp(HeightData));

TArray<FLandscapeImportLayerInfo> ImportLayerInfos;
// 각 LayerInfo와 LayerData를 채운다.

TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
MaterialLayerDataPerLayers.Add(FGuid(), MoveTemp(ImportLayerInfos));

Landscape->Import(
    FGuid::NewGuid(),
    0,
    0,
    ImagegenLandscapeQuads,
    ImagegenLandscapeQuads,
    ImagegenLandscapeNumSubsections,
    ImagegenLandscapeSubsectionSizeQuads,
    HeightDataPerLayers,
    TEXT(""),
    MaterialLayerDataPerLayers,
    ELandscapeImportAlphamapType::Additive,
    TArrayView<const FLandscapeLayer>());
```

Import 후 처리:

```cpp
LandscapeInfo->UpdateLayerInfoMap(Landscape);
Landscape->UpdateAllComponentMaterialInstances(true);
Landscape->MarkPackageDirty();
```

## 소품 높이 보정

소품 배치 전에 같은 height 평가식을 다시 호출한다.

```cpp
PropLocation.Z = EvaluateImagegenLandscapeHeightCm(WorldX, WorldY, Placements, PropDefinitions) + 6.0f;
```

Landscape collision을 샘플링하지 않아도 재생성 결과가 결정적이고 빠르다. height 평가식이 바뀌면 소품 높이도 같이 바뀐다.

## 재생성 명령

에디터 자동 재생성 플래그:

```text
-TunaSweeperRebuildImagegenEnvironmentPrototype
-TunaSweeperImagegenEnvironmentPrototypeQuit
```

빌드:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' TunaSweeperEditor Win64 Development -Project='D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' -WaitMutex
```

에셋/맵 재생성:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject' /Game/PrototypeImagegenEnvironmentMap -TunaSweeperRebuildImagegenEnvironmentPrototype -TunaSweeperImagegenEnvironmentPrototypeQuit -unattended -nop4 -nosplash -log
```

확인용 에디터 실행:

```powershell
Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList @('D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject','/Game/PrototypeImagegenEnvironmentMap','-nop4')
```

## 주의점

- `BuildAndRunTunaSweeper.bat`는 빌드 성공 후 에디터를 자동 실행하므로, Live Coding과 충돌할 수 있다. 컴파일만 필요하면 UE `Build.bat`를 직접 호출한다.
- 실행 중인 에디터나 `LiveCodingConsole`이 있으면 `Unable to build while Live Coding is active` 오류가 난다.
- UBT와 Unreal Editor는 AppData 로그/캐시와 에셋 저장을 사용하므로 샌드박스 밖 실행 승인이 필요할 수 있다.
- LayerInfo와 머티리얼의 레이어 이름은 정확히 일치해야 한다.
- `FLandscapeImportLayerInfo::LayerData`는 heightmap과 같은 inclusive vertex grid 기준으로 `Verts * Verts` 크기를 넣는다.
- 구역 마스크 없이 noise와 placement influence만으로 만들면 그냥 랜덤 지형처럼 보인다. 항상 시작지, 중앙 파밍권, 숲길, 서쪽 우회로, 콘크리트 더미, 고급보안구역 같은 설계 구역을 먼저 만든다.
- weightmap이 너무 한 레이어로 몰리면 시각적으로 평평해진다. 프로토타입에서는 path, noise, placement influence를 명확히 섞어 레이어 경계가 보이게 하는 편이 좋다.
- 조명 없는 확인용 맵에서는 Landscape 머티리얼을 `MSM_Unlit`으로 두고 레이어 블렌드 결과를 `EmissiveColor`에도 연결한다. `DefaultLit`만 쓰면 지형이 검게 보일 수 있다.
- Landscape는 heightfield이므로 오버행, 절벽 옆면, 부서진 slab 같은 구조는 StaticMesh 보조 지형으로 처리한다.

## 다시 구현할 때 체크리스트

- imagegen 지형 레이어 아틀라스를 `SourceArt/EnvironmentPrototype`에 저장한다.
- `TunaSweeperEditor.Build.cs`에 `Landscape`, `Foliage`가 있는지 확인한다.
- SSOT 맵 구역을 먼저 정의하고, 각 구역을 distance field/ellipse/segment mask로 만든다.
- 레이어 정의 배열을 만든다.
- 각 레이어의 `ULandscapeLayerInfoObject`를 만든다.
- 아틀라스 사분면을 샘플하는 Landscape 머티리얼을 만든다.
- `HeightData`와 레이어별 `LayerData`를 `Verts * Verts` 크기로 만든다.
- 소품 배치는 수동 10여 개가 아니라 구역별 밀도 규칙으로 만든다.
- `ALandscape::Import()`로 heightmap과 weightmap을 한 번에 넣는다.
- `UpdateLayerInfoMap()`과 `UpdateAllComponentMaterialInstances(true)`를 호출한다.
- 같은 height 평가식으로 소품 Z를 보정한다.
- 재생성 플래그로 맵을 저장하고 MapCheck가 0 error인지 확인한다.
