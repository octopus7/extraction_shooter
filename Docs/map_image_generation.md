# 지도 이미지 생성 사용법

이 문서는 레벨 지형과 맞는 지도 이미지를 에디터에서 생성하는 절차를 정리한다.

현재 방식은 런타임 실시간 캡처가 아니라, 에디터에서 지도용 RGB PNG를 미리 굽는 방식이다. 지도 캡처 액터와 BP는 제작 도구이므로 cooked/package 빌드에서 직접 사용하지 않는다.

## 관련 자산

- 캡처 액터 C++ 클래스: `ATunaSweeperMapCaptureActor`
- 에디터 전용 BP: `/Game/EditorOnly/MapCapture/BP_Editor_MapCaptureActor`
- `RaidMap` 배치 액터 라벨: `TS_Editor_MapCapture_Raid`
- 기본 RGB 출력 경로: `Saved/MapCaptures/{level}_Map_RGB.png`
- 기본 마스크 경로 메모: `Saved/MapCaptures/{level}_Map_Mask.png`
- 런타임 지도 메타데이터: `/Game/UI/Map/DA_UIMap_{level}`
- 런타임 지도 레지스트리: `/Game/UI/Map/DA_UIMapRegistry`

`/Game/EditorOnly`는 `DefaultGame.ini`의 `DirectoriesToNeverCook`에 등록되어 있다. 따라서 이 폴더의 BP나 액터를 런타임 코드가 직접 참조하면 안 된다.

## 빠른 생성 절차

1. Unreal Editor에서 `RaidMap`을 연다.
2. World Outliner에서 `TS_Editor_MapCapture_Raid`를 선택한다.
3. Details 패널의 `Map Capture` 카테고리 상단 액션 버튼 중 하나를 누른다.
   - `Auto Detect Bounds`: 캡처 영역만 자동 검출한다.
   - `Capture RGB PNG`: 현재 캡처 영역 기준으로 RGB PNG를 저장한다.
   - `Auto Detect + Capture`: 자동 검출 후 바로 RGB PNG를 저장한다.
   - `Capture + Import`: 현재 캡처 영역 기준으로 RGB PNG를 저장한 뒤 `/Game/UI/Map/T_UIMap_{level}_RGB` 에셋으로 임포트한다.
   - `Auto Detect + Capture + Import`: 자동 검출, RGB PNG 저장, UI 텍스처 임포트를 한 번에 실행한다.
4. 기본 설정이면 다음 파일이 생성된다.
   - `TunaSweeper/Saved/MapCaptures/RaidMap_Map_RGB.png`
5. 임포트 버튼을 사용하면 같은 이름의 기존 텍스처 에셋을 덮어쓰고 지도 메타데이터와 레지스트리도 갱신한다.
6. 이 PNG를 확인하고, 필요하면 바운더리를 조정한 뒤 다시 캡처한다. 출력 크기는 항상 `2048x2048`이다.

## 주요 설정

`Map Capture|Bounds`

- `CaptureWorldSize`: 캡처할 월드 영역 크기다. 단위는 Unreal cm다.
- `BoundsPaddingCm`: 자동 검출된 바운더리에 추가로 붙일 여백이다.

`Map Capture|Auto Detect`

- `AutoDetectGridStepCm`: 자동 검출 레이 간격이다. 기본값은 `100`, 즉 1m다.
- `AutoDetectSearchExtent`: 액터 위치 기준으로 자동 검출을 시도할 탐색 범위다.
- `AutoDetectTraceStartHeight`: 위에서 아래로 쏠 레이 시작 높이다.
- `AutoDetectTraceDepth`: 레이가 아래로 내려가는 거리다.
- `AutoDetectTraceChannel`: 지형 검출에 사용할 충돌 채널이다. 기본값은 `Visibility`다.
- `bIgnoreMovableComponentsForBounds`: 켜져 있으면 Movable 컴포넌트는 바운더리 계산에서 제외한다.
- `bRequireIncludedActorTag`: 켜면 `IncludedActorTags`에 들어 있는 태그를 가진 액터만 바운더리 검출 대상으로 본다.
- `IncludedActorTags`: 기본값은 `MapCapture`다. 태그 필터를 사용할 때만 의미가 있다.

`Map Capture|Output`

- `LongSideResolution`: 호환성을 위해 노출되는 읽기 전용 값이다. 출력은 항상 `2048x2048`이다.
- `bAutoDetectBoundsBeforeCapture`: 켜져 있으면 `CaptureOpaqueRgbPng`를 누를 때도 먼저 자동 검출을 실행한다.
- `RgbPngOutputPath`: RGB PNG 저장 경로다. `{level}`은 현재 레벨 이름으로 치환된다.
- `MaskPngPath`: 수동 제작할 마스크 PNG 경로를 기록하는 값이다. 현재 액터가 마스크를 자동 생성하지는 않는다.

`Map Capture|Import`

- `ImportDestinationPath`: 캡처 PNG를 임포트할 콘텐츠 경로다. 기본값은 `/Game/UI/Map`이다.
- `ImportAssetNamePattern`: 임포트할 텍스처 에셋 이름 규칙이다. 기본값은 `T_UIMap_{level}_RGB`이며 `{level}`은 현재 레벨 이름으로 치환된다.
- 임포트 버튼은 기존 에셋이 있으면 항상 덮어쓴다.

`Map Capture|Last Result`

- `LastDetectedLocalMin`, `LastDetectedLocalMax`: 마지막 자동 검출 결과의 로컬 바운더리다.
- `LastCaptureResolution`: 마지막 캡처 출력 해상도다.
- `LastContentPixelMin`, `LastContentPixelSize`: `2048x2048` 텍스처 안에서 실제 지도가 차지하는 픽셀 사각형이다.
- `LastWrittenRgbPngAbsolutePath`: 마지막으로 저장된 RGB PNG 절대 경로다.

## 고정 해상도와 여백

캡처 PNG와 임포트 텍스처는 항상 `2048x2048` RGBA로 생성한다. 월드 캡처 영역의 가로세로 비율은 유지하며, 남는 영역은 투명 픽셀로 가운데 정렬한다.

- 가로로 긴 지도: 위·아래에 투명 여백을 둔다.
- 세로로 긴 지도: 왼쪽·오른쪽에 투명 여백을 둔다.
- 정사각형 지도: 전체 `2048x2048`을 사용한다.

런타임 좌표 변환은 전체 텍스처가 아니라 메타데이터의 `ContentPixelMin`과 `ContentPixelSize`를 사용한다. 따라서 플레이어와 마커는 투명 여백을 제외한 실제 지도 영역에 맞춰 표시된다.

## 바운더리 조정 방식

자동 검출은 캡처 액터 위치를 기준으로 `AutoDetectSearchExtent` 영역을 그리드로 훑는다. 각 셀마다 위에서 아래로 라인트레이스를 쏘고, 유효한 충돌이 있는 셀의 min/max를 잡아 캡처 영역을 만든다.

자동 검출 결과가 마음에 들지 않으면 다음 중 하나로 조정한다.

- `BoundsPaddingCm`을 키워 여백을 늘린다.
- `AutoDetectSearchExtent`를 키워 더 넓은 범위를 훑는다.
- 액터 위치를 원하는 대략 중심으로 옮긴 뒤 다시 `AutoDetectCaptureBounds`를 누른다.
- 수동으로 쓰고 싶으면 `bAutoDetectBoundsBeforeCapture`를 끄고, `CaptureWorldSize`와 액터 위치를 직접 조정한 뒤 `CaptureOpaqueRgbPng`만 누른다.

## 마스크 제작 규칙

현재 캡처 액터는 실제 지도 영역은 불투명하고 비율 보정 여백은 투명한 RGBA PNG를 저장한다. 지형 자체를 별도 형태로 오려내는 마스크가 필요하면 같은 `2048x2048` 크기의 그레이스케일 PNG를 수동 제작한다.

권장 규칙:

- 흰색: 완전히 보임
- 검정: 완전히 투명
- 회색: 반투명

마스크 파일은 RGB PNG와 같은 해상도로 만들어야 한다. 런타임에서 표시할 때는 지도 RGB 텍스처를 색상으로 쓰고, 마스크 텍스처의 R 채널을 opacity로 쓰는 머티리얼을 사용한다.

마스크 텍스처를 Unreal에 가져올 때는 sRGB를 끄고 Masks/Grayscale 계열 압축 설정을 쓰는 것이 좋다.

## 런타임 적용 절차

캡처 결과는 `Saved/MapCaptures`에 저장되는 파일이다. 이 파일은 자동으로 패키징 대상이 되지 않는다.

런타임 지도에 쓰려면 다음 절차가 필요하다.

1. 생성된 RGB PNG를 확인한다.
2. 지형 외곽 마스크가 필요하면 같은 `2048x2048` 크기의 마스크 PNG를 만든다.
3. RGB PNG는 Details 패널의 `Capture + Import` 또는 `Auto Detect + Capture + Import` 버튼으로 패키징 대상 콘텐츠 경로에 임포트할 수 있다.
   - 기본 경로: `/Game/UI/Map/T_UIMap_{level}_RGB`
   - 기존 에셋이 있으면 덮어쓴다.
4. `/Game/UI/Map/DA_UIMap_{level}` 메타데이터와 `/Game/UI/Map/DA_UIMapRegistry`가 자동으로 생성 또는 갱신된다.
5. 마스크 PNG는 아직 수동으로 임포트한다.

메타데이터에는 레벨, 텍스처, 캡처 중심, 월드 크기, yaw, 텍스처 크기, 실제 콘텐츠 픽셀 사각형이 저장된다. `UTunaSweeperMapWidget`은 레지스트리에서 현재 월드의 정의를 찾아 텍스처와 좌표 변환을 자동 적용한다.

주의: 런타임 코드가 `/Game/EditorOnly/MapCapture/BP_Editor_MapCaptureActor`나 `TS_Editor_MapCapture_Raid`를 직접 찾거나 참조하면 안 된다. 이 액터는 제작 도구이고 cooked 빌드에서는 제외된다.

## 현재 런타임 연결

`RaidMap`과 `BunkerMap` 캡처 결과는 현재 UI 텍스처로 임포트되어 지도 시스템에서 사용한다.

- 소스 PNG: `TunaSweeper/Saved/MapCaptures/RaidMap_Map_RGB.png`
- 런타임 텍스처: `/Game/UI/Map/T_UIMap_RaidMap_RGB`
- 메타데이터: `/Game/UI/Map/DA_UIMap_RaidMap`
- `RaidMap` 월드 바운즈: X `-12700.0` ~ `9700.0`, Y `-13950.0` ~ `10450.0`
- 콘텐츠 픽셀 사각형: min `(0, 84)`, size `(2048, 1880)`

- 소스 PNG: `TunaSweeper/Saved/MapCaptures/BunkerMap_Map_RGB.png`
- 런타임 텍스처: `/Game/UI/Map/T_UIMap_BunkerMap_RGB`
- 메타데이터: `/Game/UI/Map/DA_UIMap_BunkerMap`
- `BunkerMap` 월드 바운즈: X `-1751.3` ~ `1948.7`, Y `-1787.5` ~ `1912.5`
- 콘텐츠 픽셀 사각형: min `(0, 0)`, size `(2048, 2048)`

적용 코드는 `UTunaSweeperMapWidget`이다. 신규 레벨 지도는 캡처·임포트 시 레지스트리에 자동 등록되므로 위젯 코드를 수정할 필요가 없다.

## 좌표 매핑 기준

캡처 액터는 다음 변환을 제공한다.

- `WorldLocationToMapUV`
- `MapUVToWorldLocation`

지도 UV 기준:

- `(0, 0)`: 지도 이미지 좌상단
- `(1, 1)`: 지도 이미지 우하단

캡처 액터의 yaw가 지도 회전에 영향을 준다. 레벨 지형과 UI의 플레이어 위치가 맞으려면 캡처에 사용한 actor location, yaw, `CaptureWorldSize`와 런타임 매핑 데이터가 일치해야 한다.

## 문제 해결

자동 검출이 아무것도 찾지 못한다면:

- 대상 지형이 `AutoDetectTraceChannel`에 반응하는지 확인한다.
- `AutoDetectSearchExtent`가 너무 작지 않은지 확인한다.
- `AutoDetectTraceStartHeight`와 `AutoDetectTraceDepth`가 지형 높이를 충분히 감싸는지 확인한다.
- `bRequireIncludedActorTag`가 켜져 있다면 대상 액터에 `MapCapture` 또는 지정 태그가 붙어 있는지 확인한다.
- `bIgnoreMovableComponentsForBounds`가 켜져 있다면 대상 컴포넌트가 Static인지 확인한다.

캡처 결과가 너무 작거나 잘렸다면:

- `BoundsPaddingCm`을 늘린다.
- `CaptureWorldSize`를 직접 키운다.
- 자동 검출 전 액터를 레벨 중심 근처로 옮긴다.

파일이 보이지 않는다면:

- `LastWrittenRgbPngAbsolutePath` 값을 확인한다.
- 기본 경로 기준으로 `TunaSweeper/Saved/MapCaptures` 폴더를 확인한다.
- `RgbPngOutputPath`가 비어 있거나 잘못된 절대 경로로 되어 있지 않은지 확인한다.

## 현재 제한

- 마스크 PNG는 자동 생성하지 않는다.
- 신규 레벨의 생성된 RGB PNG는 버튼으로 UI 텍스처 임포트까지 할 수 있지만, 마스크 임포트는 수동이다.
- 런타임 지도 위젯이 캡처 액터를 직접 읽는 구조가 아니다.
- 마스크 텍스처를 런타임 지도 표시 알파로 적용하는 작업은 별도 구현이 필요하다.
