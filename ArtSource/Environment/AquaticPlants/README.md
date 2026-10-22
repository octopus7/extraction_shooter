# 숲 웅덩이 수생식물

작은 숲 웅덩이에 사용하는 저폴리 수련잎, 개구리밥, 어리연꽃 모델이다. 3개 독립 Blender 작업에서 제작했으며, 각 종은 불투명 팔레트 텍스처와 머티리얼 하나를 사용한다.

| 종 | FBX 변형 | 실제 크기 | 숲 배치 수 |
| --- | --- | --- | ---: |
| 수련잎 | `WaterLily/Models/SM_WaterLily_A`, `B`, `C` | 폭 27~38cm | 6 |
| 개구리밥 | `Duckweed/Models/SM_Duckweed_A`, `B`, `C` | 군집 폭 21~30cm, 두께 2mm | 11 |
| 어리연꽃 | `Nymphoides/Models/SM_Nymphoides_A`, `B` | 군집 폭 26.6~30.2cm, 꽃 수면 위 약 3.5~4.2cm | 3 |

각 종 폴더의 `.blend`가 편집 원본이고 `Models/*.fbx`가 UE 전달 파일이다. 팔레트는 `Textures/T_<종>_Palette.png`, 미리보기는 `Previews` 또는 `Renders`, Blender 검증 결과는 `Validation` 또는 `Models/Nymphoides_Validation.json`에 있다.

## 단위와 가져오기

- Blender 1단위는 1m, Z-up이며 모델 원점 Z=0은 수면이다. 수련잎 아랫면과 어리연꽃 줄기는 수면 아래로 조금 내려간다.
- FBX는 `-Y Forward / Z Up`, `FBX_SCALE_UNITS`, 단위 메타데이터를 포함한다. UE에서 `Convert Scene Unit`을 켜고 `Import Uniform Scale=1.0`을 사용한다.
- 저장된 UE 에셋은 `/Game/Environment/AquaticPlants/<종>`에 있다. 메시 8개, 머티리얼 3개, 텍스처 3개다.
- 팔레트의 면 UV는 면적을 갖도록 펼쳤다. 한 점 UV를 사용하지 않는다. FBX 노멀을 유지하고 고정밀 UV를 사용한다. 식물은 불투명이며, 어리연꽃 재질은 작은 꽃잎의 양면 표시를 허용한다.
- 장식용이므로 메시 충돌, 오버랩, 내비게이션과 거리장 기여를 끈다. 물 발걸음 판정은 기존 웅덩이가 담당한다.

## 숲 배치

`/Game/MainRaid/RaidForest`의 `Forest_ShallowPuddle_TestWater` 주변에 20개를 저장했다. 액터 라벨은 `Forest_PuddlePlant_...`, 폴더는 `Environment/Water/AquaticPlants`다. 수면 원점 `(-7780.16, -2084.13, 3.0)cm`을 기준으로 식물 원점은 Z+0.12cm다. 잎과 꽃은 여러 군집으로 모으고 중앙 수면은 비워 두었다. 각 액터의 위치·회전·스케일을 일반 레벨 배치로 편집할 수 있다.

어리연꽃은 흰 다섯 갈래 꽃과 노란 중심, 꽃잎 가장자리의 가는 솜털을 표현했다. 외형 참고: [NParks Nymphoides indica](https://www.nparks.gov.sg/florafaunaweb/flora/2/2/2272).

새 Blender 프로세스에서 FBX 단위·토폴로지·UV·노멀·팔레트 로드를 검사했다. UE 저장 후 재로드·실제 렌더 검증 결과는 `unreal_validation.json`에 기록한다. 원본 생성기와 UE 일회성 임포트 코드는 최종 소스에 포함하지 않는다.
