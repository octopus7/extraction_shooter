# RaidForest 수역 편집

`/Game/MainRaid/RaidForest`의 아웃라이너에서 `Environment/Forest/Water/River`를 열면 강이 흐르는 순서대로 다섯 액터가 있다. 오른쪽으로 넓어지는 `Water_04_EastFan`과 절벽 끝에 닿는 `Water_05_RiverMouth`는 각각 독립된 BP다. `Environment/Forest/Water/Ponds`에는 코끼리 호수와 얕은 웅덩이가 있다.

| 아웃라이너 액터 | 위치 | 바닥 소품 인스턴스 |
| --- | --- | ---: |
| `Water_01_WestPool` | 상류의 넓은 소 | 31 |
| `Water_02_WestFord` | 서쪽 여울 | 11 |
| `Water_03_MiddleBend` | 가운데 굽이 | 36 |
| `Water_04_EastFan` | 동쪽 여울부터 넓어지는 구간 | 30 |
| `Water_05_RiverMouth` | 외곽 암반까지 이어지는 하구 | 60 |
| `Water_06_ElephantLake` | 코끼리 앞 호수 | 12 |
| `Water_07_ShallowPuddle` | 얕은 웅덩이 | 1 |

## 소품과 물색

액터에서 **Edit Blueprint**를 열거나 콘텐츠 브라우저의 `/Game/Environment/ForestWater/Blueprints`에서 해당 BP를 열 수 있다. `WaterSurface`는 수면, `ShoreFoam`은 물가, `Riverbed`는 퇴적물이 깔린 바닥이다. `Bed_*` 컴포넌트에는 종류별로 납작한 자갈 무더기, 이끼 낀 돌, 갈라진 유목, 수초, 붉은 잔자갈이 들어 있다. 컴포넌트의 **Instances**에서 개별 위치·회전·크기를 수정한다. 변환은 해당 수역 액터 기준이다.

`M_ForestWaterDepth`의 **Shallow Tint**, **Deep Tint**, **Deep Depth Metres**는 얕은 물과 깊은 물의 색, 짙은 색에 이르는 수심을 조절한다. **Shallow Opacity**와 **Deep Opacity**로 바닥이 보이는 정도를 바꾼다. 한 구역만 조정할 때는 머터리얼 인스턴스를 만들어 그 BP의 `WaterSurface` 슬롯에 지정한다.

## 물가와 지형 수정

수면의 UV0은 구역 전체에 이어지는 미터 단위 평면 좌표다. UV1의 U에는 수면에서 실제 바닥까지의 수직 거리(미터)가 들어 있다. 지형을 다시 조각해 강바닥 높이나 강폭을 바꾸면 해당 수면의 UV1과 물가 윤곽도 함께 갱신한다. 편집 원본과 메시별 FBX는 `TunaSweeper/SourceArt/Environment/ForestWater`에 있다.

`Riverbed`는 실제 강바닥의 굴곡을 따르며 수심 약 2–8cm 사이에서 마른 지면과 섞인다. 하상과 수중 소품은 배치도 데칼을 받지 않는다. 숲의 `MI_Layout_Forest`는 외곽 5.6m에서 배치도를 서서히 흐리게 표시하고, 식생 분류에는 원래의 `T_Layout_Forest_Draft`를 사용한다.

`M_ForestShoreFoam`은 물가 리본을 따라 긴 물결, 짧은 포말, 옅게 풀어지는 물자국 텍스처를 겹친다. 리본의 UV0은 강둑을 따라 누적한 거리(U)와 물가에서 물 안쪽으로 들어가는 비율(V)이다. 닫힌 호수의 UV1 U에는 전체 둘레가 들어가서 시작점과 끝점의 무늬가 이어진다. **Foam Opacity**, **Repeat Metres**, **Motion Speed**로 강도와 무늬의 길이, 속도를 조절한다. 하구에서 절벽 밖으로 열리는 가장자리에는 물가 리본을 두지 않았다.

원래의 `Forest_ShallowPuddle_TestWater`는 웅덩이 발소리와 발걸음 파문을 처리한다. 수면과 물가의 표시는 `Water_07_ShallowPuddle`에서 편집한다. 코끼리 호수의 도전과제는 기존 `Forest_ElephantLake_Achievement13` 트리거가 담당한다.
