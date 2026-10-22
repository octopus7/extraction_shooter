# three-stylized-water 분석과 UE 5.7 설계안

작성 기준: 2026-10-22. 사용자 요청은 첨부 이미지의 잔잔한 흰 물가 표현 분석과 전체 UE 5.7 구현 계획이다. 이번 산출물은 소스 조사와 설계이며 엔진 구현·시각 검증 결과가 아니다.

## 목표와 판단 근거

- 우선 목표는 완만한 해변에서 보이는 넓은 흰 파도 앞부분, 반투명한 꼬리, 청록색 얕은 물과 비치는 바닥이다.
- 기본 링크의 `tropical` 프리셋을 기준으로 분석했다. 첨부 화면에서 사용자가 별도로 조절한 값까지 동일하다고 단정하지 않는다.
- 데모 페이지의 도구 로딩은 실패했다. 실제 웹 애니메이션 A/B 검증은 하지 않았고, 공개 소스·프리셋과 첨부 화면을 근거로 판단했다.
- 적용 대상은 TunaSweeper의 기존 StylizedWater 플러그인으로 가정한다. 신규 실험 재질과 검토 맵에서 먼저 비교한다.

## 원본의 흰 부분

흰 해안 파도는 `src/water/nodes/shoreline.ts`가 담당한다. 지형과 물의 수직 높이차를 지수 함수로 바꾼 얕은 수심 마스크에서 시간에 따라 반복 띠를 만들고, 흐르는 노이즈로 띠를 깎은 뒤 진행 방향의 꼬리와 알파를 합성한다. 파티클이나 유체 시뮬레이션이 없어도 이 모양을 만들 수 있다.

| 기본 tropical 설정 | 의미 |
|---|---|
| `ENABLESHORELINE = 1` | 움직이는 해안 띠 사용 |
| `SL_Color = (1, 1, 1, 0.68235296)` | 흰색이지만 완전 불투명이 아닌 레이어 |
| `SL_WaterDepth = 0.45` | 해안 띠용 수심 감쇠 길이 0.45m |
| `SL_Ammount = 4.06`, `SL_Speed = 0.79` | 수심 좌표에서의 반복과 위상 속도 |
| `SL_Thickness = 0.868`, `SL_EnableTrail = 1` | 넓은 띠와 방향성 꼬리 |
| `SL_Trail_Fade = 1.27`, `SL_GradientDissolve = -1` | 0~1 범위 밖 보간도 의도적으로 사용 |
| `SL_Dissolve_Mask = Noise 5.png` | 넓은 덩어리와 불규칙한 가장자리 |
| `Enable_SurfaceFoam = 0`, `ENABLEWAVE = 0` | 수면 전체 거품과 Gerstner 메시 변형은 기본 꺼짐 |

접촉 거품인 `intersection.ts`도 켜져 있지만 기본 색은 연한 노란색이다. 따라서 이 프리셋의 흰 해안 파도를 접촉 거품 하나로 설명하면 잘못된 분석이다. 미지정 접촉 마스크는 흰 텍스처로 대체된다.

계산 개념은 다음과 같다. 아래 식은 이해를 위한 축약이며 완전한 원본 이식 코드가 아니다.

```text
h = 수면 높이 - 화면 깊이에서 복원한 장면 지점의 높이
shallow = saturate(exp(-h / ShoreDepth))
phase = (shallow - Time * Speed * 0.1) * Amount
띠 = frac(+phase), frac(-phase)를 두께와 진행 방향에 따라 조합
해안 거품 = 얕은 영역 제한 × 노이즈로 깎은 띠 × 방향성 꼬리
색/알파 = 물의 색/알파와 거품의 색/알파를 각각 합성
```

원본은 수평 해안 거리가 아니라 수심 등고선을 사용한다. 완만한 경사에서는 넓고, 급경사에서는 좁아진다. 화면 깊이에서 복원한 지점은 카메라 광선이 보는 바닥이므로, 같은 XY 아래의 실제 수심을 담은 지형 지도와 완전히 같지는 않다. `SceneDepth - PixelDepth`만으로 수직 수심이나 해안까지의 수평 거리를 얻는 것도 아니다.

`SL_Speed`는 m/s가 아니며 두께도 cm가 아니다. 위상식에서 고정 수심의 반복 주기는 `1 / (0.1 × 0.79 × 4.06)`, 약 3.12초다. 잔잔한 인상은 느린 속도 하나보다 낮은 입체 변형, 큰 형태, 투명한 꼬리에서 나온다.

원본의 띠 판정에는 단단한 임계값도 있다. 전체를 흐리게 만드는 것과 구분해야 한다. 앞쪽 실루엣은 어느 정도 선명하게, 뒤쪽은 얇고 투명하게 만드는 것이 목표다. 최종 물가 알파 페이드가 다시 적용되고, 반사와 하이라이트가 뒤에 합성되므로 최종 색이 설정한 흰색과 항상 같지는 않다.

## 현재 프로젝트와 권장 구조

기존 `TunaSweeper/Plugins/StylizedWater`는 Unlit Translucent 수면과 R=해안 signed distance, G=정규화 수심 마스크를 사용한다. 현재 `MaskWater.ush`에는 앞쪽 거품, 안쪽 반복 띠, `fwidth` 경계 처리가 있으나 노이즈 파괴는 정적인 삼각함수 조합이고 원본의 방향성 꼬리·수심 지수 좌표와 다르다.

| 후보 | 판단 |
|---|---|
| 기존 반투명 플러그인에 독립적인 Reference 재질 추가 | 권장. 마스크·배치·검토 틀을 재사용하고 원본 레이어의 색/알파 제어를 보존한다. |
| Single Layer Water로 전환 | 물리 기반 투과·반사에는 장점. 일반 알파와 다른 합성이고 기존 프로젝트의 실패 이력도 있어 이번 기본안으로 선택하지 않는다. |
| 별도 거품 리본/데칼 중심 | 특정 해안의 수작업 연출에는 유용하지만 이음새·정렬·중첩 관리가 필요하다. 기본 경로에 추가하지 않는다. |

권장 데이터 흐름:

```text
저장된 R 해안 거리 ─→ 수면 범위 / 가장자리 / 거품 최대 범위
저장된 G 수직 수심 ─→ 얕은 물 색 / exp 수심 좌표 / 해안 띠
월드 XY + 시간 ────→ 노이즈 / 노멀 / 파도 위상
화면 깊이 ─────────→ 바위 등 국소 접촉 / 교차 페이드 / 굴절 유효성
각 레이어 ─────────→ 색과 알파 분리 합성 → 반사·하이라이트
```

G 수심 기반은 원본과 다른 안정화 선택이다. 화면 깊이 기반 원본 비교 모드를 검토용으로 두고, 게임용 기본값은 지형에 고정된 저장 수심을 사용한다. 해안 거리 R은 범위와 안정성을 제어하며 수심 G를 대신하지 않는다. 따라서 경사에 따른 띠 폭 변화는 유지하되 카메라의 가림 변화가 해안 전체를 흔드는 일을 줄인다.

R/G 마스크는 현재 지형 자동 검출기가 아니다. 재사용 가능한 명시적 에디터 베이크 동작으로 지정 지형의 높이·물 경계·해안 거리를 저장하고 지형/수위 변경 시 다시 실행한다. 런타임 매 프레임 캡처나 시작 시 자동 재생성은 도입하지 않는다. 물의 기본 평면을 지형에 따라 들어 올리지 않으며 기존 지형 맞춤은 별도 얇은 젖은 막에만 필요할 때 사용한다.

## 전체 표현의 우선순위

1. 해안 띠: 수심 기반 위상, 큰 노이즈, 방향성 꼬리, 별도 거품 알파. 미세한 점 노이즈와 강한 발광은 피한다.
2. 본체: 얕은 청록색→깊은 색, 얕은 바닥 투시, 투명한 교차 경계. 색용 수심 범위와 거품용 범위를 분리한다.
3. 잔물결: 서로 다르게 움직이는 두 노멀. 원거리에서는 강도를 줄이고 해안 거품의 굴절은 약하게 한다.
4. 굴절: 엔진 굴절과 직접 SceneColor 합성을 중복 적용하지 않는다. 전경 캐릭터가 물 안으로 끌려가는 왜곡, 화면 가장자리, 깊이 유효성을 검사한다.
5. 카우스틱: 얕은 영역에만 낮은 대비의 이동 무늬. 1차는 원본과 유사한 물 레이어의 가짜 투영으로 한정한다. 실제 바닥 수광은 별도 확장이다.
6. 반사/광택: 기존 painted sky를 선택 기능으로 재사용하고 Fresnel 및 노멀로 조절한다. Unlit이 Lumen 반사·동적 그림자를 자동 수신한다고 가정하지 않는다. 실제 조명 수신이 필요하면 같은 마스크를 사용하는 Lit Translucent/Surface ForwardShading 비교 재질로 먼저 검증한다.
7. 입체 파도: 기본 꺼짐. 해안 목표가 맞은 뒤 필요할 경우 작은 WPO만 추가한다. 큰 Gerstner 파도·수영·부력·잠수·Niagara 해안 거품은 이번 목표에 포함하지 않는다.

## 검증 및 제약

- UE 5.7, 월드 +Z 위쪽, XY 수면, 100uu=1m. 원본의 Y-up/m와 명시적으로 변환한다.
- 기존 DX12/SM6, Substrate, Lumen 설정을 유지한다. 일반 플레이 카메라는 소스 기본값 기준 perspective, pitch -60도, arm 1200cm, FOV 70도이며 BP override는 실제 검증 때 확인한다.
- 원본은 NoToneMapping이다. 프로젝트 전체 톤매퍼를 바꾸지 않고 고정 노출과 현행 색 관리에서 비교/조정한다.
- Single Layer Water의 Opacity는 일반 투명도와 다르며 일반 SceneDepth 노드는 translucent 전용이다. 두 경로를 같은 그래프로 간주하지 않는다.
- 원본 색/노멀/노이즈의 색 공간과 노멀 Y 방향을 검증한다. 데이터 마스크는 sRGB off. 출처 자산을 재사용하면 MIT 고지를 보존한다.
- 기존 물 자동화는 시각 품질의 증명이 아니다. 실제 게임 AA/DLSS/TSR 조건에서 20초 영상, 카메라 이동, 완만/급한 경사, 바위/캐릭터, 수위 변경, 원거리 얇은 띠를 검토한다.
- 기존 웅덩이 `BP_ShallowPuddle`, `/Game/Environment/TestWater`, 본 맵 배치는 이번 계획의 자동 교체 대상이 아니다.

## 근거

- [원본 프로젝트/구성](https://github.com/norio/three-stylized-water)
- [기본 프리셋](https://github.com/norio/three-stylized-water/blob/main/src/water/presets.json#L3631)
- [해안 띠 계산](https://github.com/norio/three-stylized-water/blob/main/src/water/nodes/shoreline.ts)
- [수심 복원](https://github.com/norio/three-stylized-water/blob/main/src/water/nodes/depth.ts)
- [색/알파 합성](https://github.com/norio/three-stylized-water/blob/main/src/water/WaterMaterial.ts)
- [UE 5.7 Single Layer Water](https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine?application_version=5.7)
- [UE 5.7 Depth Expressions](https://dev.epicgames.com/documentation/unreal-engine/depth-material-expressions-in-unreal-engine?application_version=5.7)
- [UE 5.7 Lit Translucency](https://dev.epicgames.com/documentation/en-us/unreal-engine/lit-translucency-in-unreal-engine?application_version=5.7)
- 프로젝트: `TunaSweeper/Plugins/StylizedWater/README.md`, `Docs/water_mask_rebuild_2026-09-06.md`, `Docs/game_conventions.md`, `Docs/shallow_puddle.md`.
