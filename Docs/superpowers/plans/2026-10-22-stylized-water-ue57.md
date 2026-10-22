# Stylized Water UE 5.7 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** 첨부 이미지처럼 잔잔한 흰 해안 파도와 투명한 청록색 물을 기존 StylizedWater 플러그인의 독립 검토 재질로 재현한다.

**Architecture:** 기존 반투명 수면·배치 액터·거리/수심 마스크를 재사용한다. 원본의 수심 기반 해안 위상·노이즈·방향성 꼬리를 분리하고, 저장 수심을 기본 해안 좌표로 사용한다. 기존 재질은 유지하며 새 Reference 재질을 검토용 액터에 명시적으로 선택한다.

**Tech Stack:** UE 5.7, C++, HLSL Custom expressions/material functions, Unlit Translucent, 선형 RG 수심/거리 텍스처, DX12/SM6.

**Spec:** [분석 및 설계안](../specs/2026-10-22-stylized-water-reference-design.md)

**Status:** 검토용 구현 계획. 구현·엔진 실행·빌드·렌더 시험은 아직 수행하지 않았다. 아래 새 이름은 계획된 자산/인터페이스다.

## Global Constraints

- 모든 새 물 구현은 `TunaSweeper/Plugins/StylizedWater`에 둔다. 기존 맵·웅덩이·TestWater를 자동 교체하지 않는다.
- UE 5.7, +Z 위쪽, 수면 XY, 100uu=1m. 거리/수심은 cm, 시간은 초로 전달한다.
- 저장 마스크 계약 R=`0.5 + signedDistanceCm/(2*MaskDistanceRange)`, G=`saturate(verticalDepthCm/MaskDepthRange)`, 물 안쪽 R>0.5를 유지한다.
- 현재 지형 맞춤의 정점 수심은 색 범위로 정규화되어 있다. 새 거품 계산에는 원시 cm 수심 또는 G×MaskDepthRange를 사용하고 기존 포장된 `Surface.w` 값을 수심 cm로 오인하지 않는다.
- 게임플레이 저장 데이터 변경은 없다. 맵에 저장되는 authoring 데이터의 의미는 플러그인 README에 기록한다.
- 새 UI 안내·버튼·경고는 프로젝트 문자열 키 체계를 사용한다. 플레이어 노출 한국어는 `기지`로 통일한다.
- 에디터 감지/열기/종료/빌드 전 `Docs/unreal_editor_coordination.md`를 따른다.
- 일회성 생성기를 쓴다면 자산 검증→생성기·전용 의존성 제거→재검증→로그까지 하나의 해당 구현 커밋에 포함한다. 재사용 에디터 베이크 도구는 유지하되 자동 실행하지 않는다.
- 현재 작업은 계획만 저장한다. 아래 구현 단계의 체크박스는 실행하지 않는다.

## Review Focus

1. 카메라 이동에 해안 띠가 끌려가거나 카메라 앞 물체에 따라 해안 전체가 바뀌지 않는가 — Task 2/3.
2. 완만/급경사와 섬·오목한 해안에서 거품이 전역에 번지거나 마스크 밖 사각 테두리가 생기지 않는가 — Task 1/3.
3. 원거리와 실제 DLSS/TSR에서 얇은 흰 부분이 점멸·잔상·점 노이즈로 바뀌지 않는가 — Task 2/5.
4. 전경 캐릭터, 바위, 화면 가장자리에서 굴절과 투명도 정렬이 무너지지 않는가 — Task 4/5.
5. 저장/재로드·맵 이동 후 재질/마스크 참조가 유지되고 기존 물·웅덩이가 바뀌지 않는가 — Task 1/3/5.

## 파일 구성

기존 수정 대상:

- `Source/StylizedWater/Public/StylizedWaterBodyActor.h` 및 `Private/StylizedWaterBodyActor.cpp`: 새 외형 선택과 파라미터 전달. 기존 기본 선택 유지.
- `Source/StylizedWaterEditor/Private/Tests/StylizedWaterEditorTests.cpp`, `WaterRenderTests.cpp`: 저장 계약과 렌더 검증 확장.
- `Tools/verify_saved_water.py`: 기존 10개 고정 자산 가정을 명시적 기존/신규 자산 목록으로 갱신.
- `README.md`: 단위, 마스크 베이크·갱신 시점, 신규 외형과 제한.

신규 대상(모두 플러그인 상대 경로):

- `Shaders/Private/ReferenceShoreline.ush`: 수심 좌표, 띠·노이즈·꼬리 마스크만 계산.
- `Shaders/Private/ReferenceWater.ush`: 물색·거품 알파·광택 합성. 깊이/색 샘플은 그래프 입력으로 전달.
- `Content/Reference/M_WaterReference.uasset`: 저장된 반투명 재질. 기존 MaskWater 재질과 독립.
- `Content/Reference/Textures/T_ShoreDissolve.uasset`, `T_WaterNormal.uasset`, `T_Caustics.uasset`: 선형 데이터 노이즈/노멀/빛무늬. 출처와 인코딩 기록.
- `Content/Review/WaterShoreReference.umap`: 작은 경사 해변, 급경사, 바위, 섬, 기존/신규 수면 비교.
- `Source/StylizedWaterEditor/Private/StylizedWaterShoreBake.h/.cpp`: 명시적 지형 수심/거리 베이크. 런타임에는 포함하지 않는다.
- `Source/StylizedWaterEditor/Private/Tests/ReferenceWaterTests.cpp`: 신규 계약 및 기능 시험.

## Task 1: 비교 장면과 안전한 재질 선택

**Interfaces:** `EStylizedWaterLook { ExistingMask, ReferenceShore }`; 액터의 새 `WaterLook` 기본값은 `ExistingMask`. 신규 재질은 reflected soft reference로 cooker에 연결한다. 기존 sky 토글은 기존 외형에서 의미를 유지한다.

- [ ] 원본 tropical 설정과 첨부 화면을 비교 기준으로 기록한다. 실제 웹 데모에서 시간 고정과 shoreline/intersection 토글 A/B를 캡처해 소스 분석을 시각 확인한다.
- [ ] 기존 검토 맵을 수정하지 않고 `WaterShoreReference`에 수평 수면, 1:10/1:3 경사, 수직 벽, 섬, 바위를 배치한다. 신규 수면 충돌·내비게이션 영향은 끈다.
- [ ] `StylizedWater.Reference.AssetsAndSelection` 시험을 추가한다. 이전 액터 기본 선택 유지, 신규 재질 명시 선택, 필수 참조 누락 시 명확한 진단, 다른 외형으로 몰래 대체하지 않음을 검사한다.
- [ ] 독립 Reference 재질과 액터 선택을 연결하고 저장/재로드한다. 새 재질의 DX12/SM6/Substrate 컴파일 성공과 저장 패키지 참조를 확인한다.
- [ ] `ReferenceShore`에서는 기존 액터의 `FittedHeights`를 정점에 적용하지 않는다. 저장된 기존 값은 보존하고 `ExistingMask`로 돌아가면 다시 사용한다. 지형 맞춤된 액터의 외형 왕복 전환과 저장/재로드에서도 수평 수면 및 기존 외형이 각각 복원되는지 시험한다.
- [ ] 고정 카메라/노출의 기존 결과를 보관한다. 계획 당시 C++ 기본 카메라 값은 참고만 하고 실제 게임 BP 설정을 확인한다.

완료 조건: 새 검토 장면에서만 새 외형을 선택할 수 있고 기존 액터와 재질의 동작은 유지된다.

## Task 2: 가장 먼저 흰 해안 띠 완성

**Interfaces:** `ReferenceShoreline.ush`는 `DepthCm`, `ShoreDistanceCm`, `WorldXYCm`, `TimeSeconds`, `DissolveSample`, 파라미터를 받아 0~1 `FoamMask`를 반환한다. 기존 `EvaluateMaskWater` 시그니처는 바꾸지 않는다.

초기 파라미터: `ShoreDepthCm=45`, `BandAmount=4.06`, `PhaseSpeed=0.079`(exp 수심 좌표/초), `BandThickness=0.868`, `FoamOpacity=0.68235296`, `TrailFade=1.27`, `GradientDissolve=-1`. 원본 비교용 시작값이며 최종 미술값은 캡처로 조정한다. phase는 `(expDepth-Time*PhaseSpeed)*BandAmount`로 정의해 0.1을 중복 곱하지 않는다.

- [ ] 얕은 마스크 `exp(-max(DepthCm,0)/max(ShoreDepthCm,epsilon))`와 제한 영역을 먼저 디버그 색으로 표시한다. 정확한 물 경계는 R coverage로 제한한다.
- [ ] 원본의 양/음 frac 띠, 진행 방향 선택, 큰 노이즈, dissolve, trail을 이식한다. TSL의 메서드 인자 순서를 HLSL로 옮길 때 직접 확인한다. 0~1 밖 보간값을 중간에서 임의 clamp하지 않는다.
- [ ] 원본의 단단한 경계는 `fwidth` 기반 아주 좁은 AA 구간으로 안정화한다. 앞부분 전체를 blur하지 않고 꼬리에 별도 연속 알파를 준다.
- [ ] `ReferenceWater.ush`에서 거품 색 보간과 거품 불투명도 누적을 분리한다. 물색만 흰색으로 바꾸고 알파를 그대로 두는 구현은 제외한다. 경계 fade를 마지막 알파에 한 번 적용한다.
- [ ] `StylizedWater.Reference.ShorelineMasks`에서 깊은 영역=0, 해안 밖=0, 시간 정지 결과 동일, 속도 부호 전환 시 이동 방향 반전, NaN/Inf 없음과 최소 2주기의 연결성을 검사한다. 내부 수식을 그대로 복사한 CPU 테스트를 정답으로 삼지 않는다.
- [ ] 저장 G 수심 모드와 화면 깊이에서 world Z를 복원하는 원본 비교 모드를 같은 카메라에서 캡처한다. 비교 모드는 Unreal의 뷰/깊이 유틸리티를 사용해 reverse-Z와 화면 UV를 처리하며 Y-up 카메라 공식을 복사하지 않는다.
- [ ] 거품만 켠 화면→물색 추가→바닥 표시 순으로 20초를 관찰한다. 큰 연결 형태, 선명한 앞선, 반투명 꼬리, 수면에 붙은 낮은 움직임을 확인한다.

완료 조건: 다른 효과 없이도 첨부 이미지의 흰 파도 형태가 설명되고, 고정 카메라와 이동 카메라에서 지속적인 점멸·직선 반복·전체 흰 도배가 없다. 결과를 사용자 검토 대상으로 제시한다.

## Task 3: 지형에 맞는 수심과 해안 데이터

**Interfaces:** `FStylizedWaterShoreBakeSettings`에 월드 XY 범위, 물 높이 cm, 해상도, trace 대상, 거리/수심 인코딩 범위를 둔다. `bool BakeShoreData(UWorld& World, const FStylizedWaterShoreBakeSettings& Settings, FStylizedWaterShoreBakeResult& OutResult)`는 성공 여부를 반환하며 OutResult에 R/G 데이터, 해상도, 유효 hit 수, 실패 사유를 담는다. `bool SaveShoreDataTexture(const FStylizedWaterShoreBakeResult& Result, const FString& PackagePath, UTexture2D*& OutTexture, FText& OutError)`는 명시 실행 시에만 지정 패키지에 저장한다. OutError의 UI 문구는 문자열 키로 해결한다.

- [ ] 수직 trace로 지정한 고정 지형만 샘플링한다. 캐릭터·나무·교량 등 원치 않는 물체를 제외하며 일반 Visibility trace에만 의존하지 않는다.
- [ ] 물 높이와 지형 높이에서 G를 계산하고 물 안/밖 분류에서 2D signed distance R을 만든다. XY 샘플 간격이 다르면 실제 cm 거리를 사용한다.
- [ ] `StylizedWater.Reference.ShoreBake`에서 1:10 경사의 알려진 수심, 물 안쪽 R 양수, 작은 섬의 독립 경계, UV 범위 밖 fade, trace miss 시 불완전 데이터 저장 거부를 검사한다.
- [ ] 수위·범위·지형 변경 이후 마스크가 오래되었다는 상태와 재베이크 동작을 제공한다. 자동 재생성·매 프레임 SceneCapture는 추가하지 않는다.
- [ ] 물 평면은 유지한다. 젖은 막이 필요하면 별도의 범위 제한 표현으로 처리하며 물 전체를 바위나 육지 위로 들어 올리지 않는다.
- [ ] 해안 띠가 최소 여러 texel에 걸치도록 해상도를 정한다. 예를 들어 50m/1024는 약 4.9cm/texel이므로 30cm 띠는 약 6texel이다. 훨씬 큰 영역은 구역 분할·해상도를 먼저 조정한다.
- [ ] 새 파일을 저장/재로드하고 R/G 인코딩·선형 설정·참조를 재검사한다. 베이크 수심과 정점 기반 기존 지형 맞춤 수심은 구분해 문서화한다.

완료 조건: 지정 지형에서 일관된 R/G 자료가 만들어지고, 지형/수위 변경 시 사용자가 언제 갱신해야 하는지 알 수 있다.

## Task 4: 물 본체와 전체 효과 합성

**Interfaces:** 얕은/깊은 물색과 불투명도, `ColorDepthCm`, 독립 노멀 2개, `RefractionStrength`, `CausticsStrength`, `ReflectionStrength`를 재질 파라미터로 노출한다. `ColorDepthCm`와 `ShoreDepthCm`는 독립이다.

- [ ] 수심별 색과 알파를 구현한다. 원본 m→cm 및 색 공간을 변환하고 노출 고정 상태에서 보이는 색으로 조정한다.
- [ ] 두 방향 노멀과 거리 fade를 추가한다. 원본과 동일한 법선 평균이 필요한 비교 모드와 프로젝트 노멀 인코딩의 차이를 확인한다.
- [ ] 원본 대응 검토 재질은 SceneColor를 직접 읽는 굴절 합성 한 경로를 사용하고 엔진 Refraction 입력을 중복 사용하지 않는다. 굴절 UV가 전경 또는 화면 밖을 가리키면 비굴절 UV로 되돌린다. foam mask가 큰 지점의 왜곡은 줄인다.
- [ ] scene depth의 작은 접촉 마스크를 별도로 더해 바위 근처를 처리한다. 이 값으로 저장된 해안 띠 전체를 대체하지 않는다.
- [ ] 카우스틱 두 샘플을 얕은 범위에 제한하고 낮은 대비로 합성한다. 실제 바닥에 광원이 투영된다는 설명은 하지 않는다.
- [ ] 기존 painted sky 경로와 낮은 광택을 선택 기능으로 연결한다. 하늘 UV에 시간 흐름을 추가하지 않는다. 기본 Unlit에서 실시간 그림자/엔진 반사가 자동 적용된다고 가정하지 않는다.
- [ ] 실제 장면의 조명 수신이 필요할 경우 같은 마스크를 쓰는 Lit Translucent/Surface ForwardShading 비교 재질을 시험한다. 노출·밤낮·그림자·GPU 비용을 비교해 선택하며 기본 Unlit 결과를 무조건 덮어쓰지 않는다.
- [ ] 각 레이어를 독립적으로 끄고 켠 캡처를 남긴다. 반사나 광택이 흰 띠를 지워버리지 않는지 확인한다. 입체 파도와 전체 표면 거품은 꺼 둔다.

완료 조건: 얕은 바닥 투시, 청록색 물, 흰 해안 띠, 잔물결, 절제된 굴절/반사가 함께 유지된다.

## Task 5: 실제 게임 조건 검증과 납품

- [ ] 에디터 조율 규칙을 먼저 적용한다. UE 5.7 `Build.bat TunaSweeperEditor Win64 Development`에 프로젝트 절대 경로, `-WaitMutex`, 고유 `-Log`를 전달한다. 자동 실행 배치 대신 조율된 직접 빌드를 사용한다.
- [ ] GPU 사용이 가능한 DX12/SM6 에디터 자동화에서 `StylizedWater.MaskWater`와 `StylizedWater.Reference` 접두사 시험을 실행한다. `-NullRHI` 결과를 재질 렌더 성공으로 사용하지 않는다.
- [ ] `Tools/verify_saved_water.py`로 저장 자산 참조·구형 생성기 부재·필수 신규 자산 존재를 확인한다. 기존 고정 자산 수 10개 가정을 갱신한다.
- [ ] 실제 게임 카메라·노출·사용 중인 업스케일러에서 20초 영상과 정지 이미지를 저장한다. AA-off 자동화와 구분한다. 화면 전환, 원거리, 낮은 각도, 캐릭터 통과, 바위, 섬, 두 물 영역의 인접 배치를 포함한다.
- [ ] GPU 프로파일로 물 off/on의 차이와 반사/굴절 옵션별 비용을 기록한다. 해상도·GPU·업스케일러를 같이 기록하고 미측정 성능 수치를 주장하지 않는다. 넓은 반투명 면적의 overdraw를 우선 확인한다.
- [ ] 기존 물과 웅덩이·본 맵의 의도하지 않은 변경이 없는지 diff/참조를 확인한다. 해안 표현과 실사용 성능은 캡처를 근거로 사용자가 최종 판단하도록 한다.
- [ ] 일회성 생성기 사용 시 제거 후 빌드·저장 재로드·해당 자동화를 다시 수행한다. 구현·검증·정리·README·요청 로그를 해당 작업 단위로 함께 커밋한다.

완료 조건: 검토 가능한 영상·이미지·빌드/시험 결과·성능 측정과 저장된 자산을 제공한다. 실제 맵 적용은 대상 맵이 지정되는 후속 범위로 다룬다.

## 실행 순서와 병렬화

Task 1 → Task 2 → Task 3 → Task 4 → Task 5 순서가 기본이다. 흰 해안 띠를 먼저 검토하면 바닥·반사로 원인을 가리는 일을 줄일 수 있다. Task 2에서 입력 계약을 확정한 뒤 Task 3의 베이크 도구와 Task 4의 노멀/색 기능은 서로 다른 파일에서 병렬 진행할 수 있다. 바이너리 재질·검토 맵 저장은 한 담당자가 순서대로 수행한다.

첫 납품 단위는 Task 1~2의 작은 해변 비교 장면이다. 전체 구현 착수 전 이 계획과 레퍼런스 해석을 검토할 수 있으며, 이번 작업에서는 구현을 실행하지 않았다.
