# 숲 웅덩이 수생식물

작은 숲 웅덩이에 사용하는 저폴리 수련잎, 개구리밥, 어리연꽃 모델이다. 3개 독립 Blender 작업에서 제작했으며, 이미지 생성으로 만든 공용 불투명 베이스 컬러 아틀라스와 종별 머티리얼 하나를 사용한다.

| 종 | FBX 변형 | 실제 크기 | 숲 배치 수 |
| --- | --- | --- | ---: |
| 수련잎 | `WaterLily/Models/SM_WaterLily_A`, `B`, `C` | 폭 27~38cm | 6 |
| 개구리밥 | `Duckweed/Models/SM_Duckweed_A`, `B`, `C` | 군집 폭 21~30cm, 두께 2mm | 11 |
| 어리연꽃 | `Nymphoides/Models/SM_Nymphoides_A`, `B` | 군집 폭 26.6~30.2cm, 꽃 수면 위 약 3.5~4.2cm | 3 |

각 종 폴더의 `.blend`가 편집 원본이고 `Models/*.fbx`가 UE 전달 파일이다. 공용 텍스처는 `Textures/T_AquaticPlants_BaseColor.png`, 미리보기는 각 종의 `Previews` 또는 `Renders`, Blender 검증 결과는 `Validation` 또는 `Models/Nymphoides_Validation.json`에 있다. Blender 원본에도 이미지를 패킹하고 상대 경로를 저장한다.

## 단위와 가져오기

- Blender 1단위는 1m, Z-up이며 모델 원점 Z=0은 수면이다. 수련잎 아랫면과 어리연꽃 줄기는 수면 아래로 조금 내려간다.
- FBX는 `-Y Forward / Z Up`, `FBX_SCALE_UNITS`, 단위 메타데이터를 포함한다. UE에서 `Convert Scene Unit`을 켜고 `Import Uniform Scale=1.0`을 사용한다.
- 저장된 UE 에셋은 `/Game/Environment/AquaticPlants/<종>`에 있다. 메시 8개, 머티리얼 3개, 공용 텍스처 1개다. 텍스처는 하위 `Textures` 폴더에 있다.
- UV0은 실제 표면 형태에 맞춰 펼친 베이스 컬러 좌표이고, UE의 UV1은 자동 생성한 라이트맵 좌표다. 같은 종의 반복되는 잎은 텍스처 영역을 공유한다. 한 점 UV를 사용하지 않으며 FBX 노멀과 고정밀 UV를 사용한다. 식물은 불투명이며, 어리연꽃 재질은 작은 꽃잎의 양면 표시를 허용한다.
- 장식용이므로 메시 충돌, 오버랩, 내비게이션과 거리장 기여를 끈다. 물 발걸음 판정은 기존 웅덩이가 담당한다.

## 숲 배치

`/Game/MainRaid/RaidForest`의 `Forest_ShallowPuddle_TestWater` 주변에 20개를 저장했다. 액터 라벨은 `Forest_PuddlePlant_...`, 폴더는 `Environment/Water/AquaticPlants`다. 수면 원점 `(-7780.16, -2084.13, 3.0)cm`을 기준으로 식물 원점은 Z+0.12cm다. 잎과 꽃은 여러 군집으로 모으고 중앙 수면은 비워 두었다. 각 액터의 위치·회전·스케일을 일반 레벨 배치로 편집할 수 있다.

어리연꽃은 흰 다섯 갈래 꽃과 노란 중심, 꽃잎 가장자리의 가는 솜털을 표현했다. 외형 참고: [NParks Nymphoides indica](https://www.nparks.gov.sg/florafaunaweb/flora/2/2/2272).

## 생성 텍스처와 UV

내장 `image_gen`으로 만든 2×2 아틀라스에 잎맥과 꽃잎 결을 넣었다. 원본 생성 프롬프트와 영역 정보는 `texture_generation.json`에 기록한다. 생성 PNG는 1254×1254이며 래스터 필터나 후처리 없이 보존한다. UE 텍스처는 `Stretch To Power Of Two`로 2048×2048 빌드와 밉맵을 지원하며, 정규화한 UV 영역은 유지한다.

| 이미지 영역 | UV 범위 | 용도 |
| --- | --- | --- |
| 왼쪽 위 | U 0–0.5, V 0.5–1 | 수련잎의 중심에서 가장자리로 이어지는 방사형 잎맥 |
| 오른쪽 위 | U 0.5–1, V 0.5–1 | 어리연꽃 잎의 방사형 잎맥 |
| 왼쪽 아래 | U 0–0.5, V 0–0.5 | 개구리밥의 장축을 따라 흐르는 잎맥 |
| 오른쪽 아래 | U 0.5–1, V 0–0.5 | 흰 꽃잎 결과 아래쪽 노란 꽃 중심 |

UV 원점은 이미지 왼쪽 아래다. 각 섬은 사분면 경계에서 여백을 두며, 꽃잎은 뿌리에서 끝으로 결이 이어지도록 방향을 맞춘다. 잎 측면·줄기·꽃 중심·솜털도 각 표면에 맞는 면적 UV를 사용한다. 불필요해진 종별 단색 팔레트는 제거한다.

새 Blender 프로세스에서 FBX 단위·토폴로지·UV·노멀·텍스처 로드를 검사했다. UE 저장 후 재로드·실제 렌더 검증 결과는 `unreal_validation.json`에 기록한다. 원본 생성기와 UE 일회성 임포트 코드는 최종 소스에 포함하지 않는다.
