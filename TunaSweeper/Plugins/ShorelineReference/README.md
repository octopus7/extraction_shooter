# Shoreline Reference

UE 5.7에서 잔잔한 흰 해안 띠를 검토하는 독립 무인도 테스트 장면이다. 이번 구현은 원본 [three-stylized-water](https://norio.github.io/three-stylized-water/)의 tropical 해안 수식과 사용자가 제공한 이미지를 기준으로 한다.

## 열기

- 콘텐츠 브라우저의 **Show Plugin Content**를 켠다.
- 맵: `/ShorelineReference/Maps/L_ShorelineReferenceIsland`
- 물 재질 인스턴스: `/ShorelineReference/Materials/MI_ShorelineReference`
- 저장된 검토 카메라: `SR_ReviewCamera`
- 에디터 뷰포트의 Realtime을 켜면 띠가 움직인다.

이 콘텐츠 전용 플러그인은 기본 활성화되며 C++ 모듈, 시작 시 생성 코드, 기존 물에 대한 참조가 없다. 모든 신규 맵·메시·재질·텍스처·원본·검증 도구는 이 폴더에 있다. 기존 `/StylizedWater`, `/Game/Environment/TestWater`, 게임플레이 맵은 수정하지 않는다.

## 구성과 단위

- `SR_Island`: 약 36m 크기의 유기적인 섬과 1:10 경사의 해변. 정점색으로 모래와 초지의 기본색을 표현한다.
- `SR_Water`: Z=0cm의 평평한 비충돌 수면. Translucent/Unlit 재질을 사용한다.
- 검토 수면의 XY 배율은 16000으로 넓혀 낮은 카메라에서도 가까운 평면 끝이 드러나지 않게 했다. 섬·수심 데이터 범위는 그대로이며, 데이터 가장자리 이전에 불투명한 깊은 물로 부드럽게 연결한다.
- `T_IslandShoreData`: 섬과 같은 높이 함수에서 만든 선형 1024² RGBA16F 데이터. R은 해안까지의 부호 있는 방사 거리, G는 수심이다.
- 월드 범위는 XY ±4096cm. UE 데이터 UV는 `((X+4096)/8192, (Y+4096)/8192)`이다. FBX 수입 시 원본 Y가 반전되므로 EXR의 원본 좌표 UV `((sourceX+4096)/8192, (4096-sourceY)/8192)`와 구분한다. 이 변환은 UE 정점으로 검증한다. R 복원은 `(R-0.5)*2000`, 수심은 `G*700`cm이다.
- 물 높이·섬 메시·섬 위치를 바꾸면 수심 데이터도 다시 제작해야 한다. 이 장면에는 임의 지형을 자동으로 베이크하는 도구가 없다.

흰 띠는 `exp(-DepthCm/ShoreDepthCm)`을 해안 좌표로 사용한다. 시간 위상, 큰 반복 노이즈, 방향성 꼬리를 합성하고, 해안 바깥과 깊은 물에서는 제거한다. 색을 하얗게 보간하는 양과 물의 불투명도를 높이는 양을 별도로 계산한다. 화면 깊이 대신 저장된 수심을 사용하므로 카메라 이동이나 전경 물체에 해안 전체가 끌려가지 않는다.

꼬리는 일정한 그라데이션으로만 사라지지 않도록 기존 `T_ShoreDissolve`를 회전·오프셋·크기 변경하여 한 번 더 샘플링한다. 밝은 앞부분과 물결 윤곽은 보호하고 옅은 뒤쪽 마스크만 부분적으로 깎아, 부드러운 빈틈과 거품 덩어리가 남는다. 새 텍스처는 추가하지 않으며 이 보정은 원본 해안 수식에 더한 미술 조정이다.

수면은 서로 다른 방향의 잔물결을 큰 노이즈로 흐트러뜨려 표현한다. 시선에 따라 하늘색과 부드러운 햇빛을 합성하며, 먼 잔물결은 화면상의 크기에 따라 약해진다. 얕은 곳의 카스틱은 휘어진 불규칙 셀과 노이즈 마스크로 일부만 드러나게 한다. 기존 해안 거품 마스크는 이 레이어와 독립적이다. 이 표현은 스타일화한 셰이딩이며 실제 장면 반사/굴절이나 해저 투영 시뮬레이션은 아니다.

## 조정

| 파라미터 | 초기값 | 역할 |
| --- | ---: | --- |
| ShoreDepthCm | 45 | 거품이 분포하는 얕은 수심 범위 |
| PhaseSpeed | 0.079 | 해안 방향 진행 속도; 음수는 반대 방향 |
| BandAmount | 4.06 | 수심 좌표 안의 반복 수 |
| BandThickness | 0.868 | 띠의 폭 |
| DissolveScale | 1.75 | 큰 노이즈 크기; 월드 반복 길이 약571cm |
| DissolveStrength | 2.74 | 띠가 끊기는 정도 |
| GradientDissolve | -1 | 원본의 수심별 노이즈 확대 보간 |
| TrailFade | 1.27 | 뒤쪽 꼬리 소멸 보간 |
| TailBreakupStrength | 0.85 | 꼬리의 불규칙한 빈틈 강도; 0이면 보정 전 모습 |
| TailBreakupScale | 2.6 | 꼬리 노이즈 UV 배율; 커질수록 무늬가 작아짐 |
| FoamOpacity | 0.68235296 | 흰색/알파 합성 강도 |
| OpaqueDepthStartCm / OpaqueDepthEndCm | 90 / 290 | 깊은 물 색과 완전 불투명으로 부드럽게 전환; 데이터 외곽에서는 별도 경계 페이드 적용 |
| ColorDepthCm | 125 | 얕은 청록색에서 깊은 청색으로 전환하는 수심 |
| ShallowOpacity | 0.45 | 얕은 물 기본 불투명도 |
| SurfaceDetailStrength | 1 | 새 표면 셰이딩 전체 강도; 0은 해당 레이어만 끔 |
| SurfaceScale | 0.7 | 잔물결의 공간 빈도 |
| SurfaceSpeed | 0.45 | 잔물결과 카스틱의 시간 배율 |
| RippleStrength | 0.11 | 표면 기울기 크기 |
| ReflectionStrength | 0.45 | 스타일화한 하늘색 반사 강도 |
| GlintStrength | 0.09 | 부드러운 햇빛 광택 강도 |
| CausticsStrength | 0.085 | 얕은 물의 불규칙한 빛무늬 강도 |
| AnimationSpeed | 1 | 시간 재생 배율; 0은 시간0에 정지 |
| FreezeTime / PreviewTime | 0 / 0 | FreezeTime=1이면 지정 초에 고정 |
| DebugMask | 0 | 1이면 검증용 거품 마스크 표시 |

`GradientDissolve`와 `TrailFade`의 0~1 범위 밖 값은 원본의 의도된 보간이다. 중간에 임의로 clamp하지 않는다. 최종 거품은 clamp하며 경계에 좁은 fwidth AA를 적용한다. `Resources/Shaders/CalmShoreline.hlsl`은 재질 Custom 노드에 저장한 코드와 같은 원본이다. 이 텍스트를 수정하는 것만으로 저장된 `.uasset`이 갱신되지는 않는다.

## 검증과 범위

`Tools/verify_assets.py`는 필수 자산, 의존성 격리, 재질 연결/코드, 선형 데이터, UE에 수입한 지형 방향, 수면 높이와 비충돌 설정, 저장 파일 불변성을 검사한다. `Tools/capture_review.py`는 실제 D3D12/SM6에서 고정 시각·거품만 표시한 화면·20초 움직임을 렌더한다. 두 도구는 전용 `UnrealEditor-Cmd -ExecutePythonScript=...` 프로세스에서 실행하며 완료 시 해당 프로세스를 종료한다. 이미 열려 있는 편집 세션에서는 실행을 거부한다. 저장된 에셋을 수정하지 않고 결과를 프로젝트 `Saved/ShorelineReferenceWork`에 출력한다.

맵 자체 GameMode는 Engine.GameModeBase다. 프로젝트 GameInstance는 PIE에서도 초기화되므로 이번 검토는 에디터 뷰포트와 SceneCapture를 기준으로 한다.

캡처 종료 후 `Tools/analyze_captures.py`를 일반 Python(Pillow, NumPy 필요)으로 실행한다. 실제 GPU 이미지에서 거품 표시/끄기, 얕은 해안 범위, 시간 변화, 고정 시각 반복 일치, 240프레임과 셰이더 오류를 검사한다. 캡처 도구의 `captured_pending_pixel_validation`만으로 통과를 판단하지 않는다.

현재 단계는 해안 흰 띠와 스타일화한 표면 반사·햇빛·얕은 빛무늬를 검토하는 장면이다. 파도 지오메트리, 실제 장면 반사/굴절, 바위 교차 거품, 캐릭터 상호작용은 후속 단계다. 게임플레이 연결이나 패키지 포함 목록은 변경하지 않는다. SceneCapture 결과는 실제 게임 카메라의 DLSS 품질 또는 출시 성능 검증을 대신하지 않는다.

## 반복 캡처

`Tools/capture_iteration.py`를 전용 UE 5.7 프로세스의 `-ExecutePythonScript`로 실행한다. `-RenderOffscreen -dx12 -sm6 -ShorelineRun=<단계명> -abslog=<로그경로>`를 함께 지정한다. 저장 자산은 바꾸지 않는다.

각 실행은 `Resources/Iterations/<실행시각-단계>/<뷰>/yyyymmdd-hhmm.png`에 원본 PNG를 보존한다. 같은 분의 다른 뷰도 디렉터리가 달라 덮어쓰지 않는다. `manifest.json`에 카메라·시간·파라미터·해시를, `shader_source.hlsl`에 해당 단계의 소스를 보존한다. `Tools/analyze_iteration.py <실행폴더>`는 PNG/해시, 고정 시각 재현, 레이어 A/B, 움직임과 셰이더 오류를 검사한다. `captured`는 촬영 완료만 의미하며 시각적 승인이나 검증 통과를 뜻하지 않는다.

`Resources/Island/Island_SourcePreview.png`는 Blender의 지형 원본 확인용이며 UE 최종 결과가 아니다. UE 캡처는 `Resources/Validation`에 별도로 보관한다.

이전 해안 꼬리 개선 검증(2026-10-23 00:49): UE5.7 D3D12/SM6 셰이더 오류 0, 거품의 얕은 해안 범위 밖 픽셀 0, 같은 시각 반복 차이 0, 서로 다른 시각의 변화 픽셀 17,461개, 240프레임/20초 캡처의 239개 연속 쌍 변화 확인. 꼬리 보정 강도 0의 마스크는 수정 전과 정확히 일치하며, 현재 강도에서 4,778픽셀이 달라지고 꼬리 1,192픽셀이 제거되었다. 밝은 앞부분의 평균 차이는 255단계 중 0.34 미만이다. 일회성 갱신기 삭제 후 새 UE 프로세스에서 자산 검증도 통과했다. [수정 전](Resources/Validation/BeforeTailBreakup/Close_1.png), [근접 결과](Resources/Validation/Close_1.png), [전체 섬](Resources/Validation/Wide.png), [20초 영상](Resources/Validation/Shoreline_20s.mp4), [자산 검증](Resources/Validation/asset_validation.json), [픽셀 검증](Resources/Validation/pixel_validation.json)을 보관한다.

## 2026-10-23 반복 개선 결과

10개 단계의 스크린샷 103장과 최종 20초 영상을 [반복 캡처 목록](Resources/Iterations/README.md)에 보존했다. 최종 저장 자산/파라미터/의존성 검증, 표면·카스틱·햇빛 A/B, 고정 시각 재현, 240프레임 거품 회귀가 통과했다. 셰이더 오류와 얕은 해안 범위 밖 거품은 0이며, PNG 프레이밍/CRC 검사는 렌더 파일 250개와 보관 스크린샷 103개를 통과했다.

`png_integrity.py`는 내보낸 PNG의 정상 청크와 CRC를 검사하고 UE 내보내기가 IEND 뒤에 남긴 여분 바이트만 제거한다. 렌더 픽셀은 변경하지 않는다. 원본/정리 후 해시와 검증 결과는 캡처 기록에 남긴다. 반복 중간 커밋 없이 검증 자료와 함께 보존했으며, 커밋은 사용자의 명시적 지시에 따른다.

## 출처

해안 알고리즘과 `noise5.png`: [norio/three-stylized-water](https://github.com/norio/three-stylized-water), MIT, ©2025 MatrixRex / ©2026 norio. 전체 라이선스는 `Resources/Reference/LICENSE.txt`에 보관한다. 섬 메시·높이 데이터·기본 모래/초지 정점색은 이 작업에서 제작했다.
