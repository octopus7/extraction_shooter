# RaidForest 수역 편집

`/Game/MainRaid/RaidForest`의 아웃라이너에서 `Environment/Forest/Water/River`를 열면 강물 BP 다섯 개와 하구 바깥 수면이 있다. 오른쪽으로 넓어지는 `Water_04_EastFan`과 하구로 이어지는 `Water_05_RiverMouth`는 각각 독립된 BP다. `Environment/Forest/Water/Ponds`에는 코끼리 호수와 얕은 웅덩이가 있다.

| 아웃라이너 액터 | 위치 | 바닥 소품 인스턴스 |
| --- | --- | ---: |
| `Water_01_WestPool` | 상류의 넓은 소 | 31 |
| `Water_02_WestFord` | 서쪽 여울 | 11 |
| `Water_03_MiddleBend` | 가운데 굽이 | 36 |
| `Water_04_EastFan` | 동쪽 여울부터 넓어지는 구간 | 30 |
| `Water_05_RiverMouth` | 숲에서 열린 하구로 이어지는 구간 | 60 |
| `Water_06_ElephantLake` | 코끼리 앞 호수 | 12 |
| `Water_07_ShallowPuddle` | 얕은 웅덩이 | 1 |

## 소품과 물색

액터에서 **Edit Blueprint**를 열거나 콘텐츠 브라우저의 `/Game/Environment/ForestWater/Blueprints`에서 해당 BP를 열 수 있다. `WaterSurface`는 수면, `ShoreFoam`은 물가, `Riverbed`는 퇴적물이 깔린 바닥이다. `Bed_*` 컴포넌트에는 종류별로 납작한 자갈 무더기, 이끼 낀 돌, 갈라진 유목, 수초, 붉은 잔자갈이 들어 있다. 컴포넌트의 **Instances**에서 개별 위치·회전·크기를 수정한다. 변환은 해당 수역 액터 기준이다.

하구 BP의 `GorgeWaterSurface` 끝에서 `Water_08_OpenRiverOutlet`이 외곽 랜드스케이프 끝까지 이어진다. 접합선의 정점 위치와 수면·수심 UV가 일치한다. 하류의 바닥과 양안은 외곽 랜드스케이프에서 편집한다.

`M_ForestWaterDepth`의 **Shallow Tint**, **Deep Tint**, **Deep Depth Metres**는 얕은 물과 깊은 물의 색, 두 색 사이의 전환 수심을 조절한다. 얕은 강에서도 수십 cm의 깊이 차이로 물색이 변한다. 코끼리 호수는 수심 1.18m부터 **Lake Basin Tint**로 설정한 남색이 점차 짙어진다. **Shallow Opacity**와 **Deep Opacity**로 바닥이 보이는 정도를 바꿀 수 있다. 한 구역만 조정할 때는 머터리얼 인스턴스를 만들어 그 BP의 `WaterSurface` 슬롯에 지정한다.

## 강물의 흐름

**River Speed Metres Per Second**는 물마루가 하류로 이동하는 속도(기본 0.95m/s), **River Current Strength**는 물결의 세기, **River Crest Radiance**는 물마루 끝에 맺히는 빛의 밝기다. 흐름과 발광을 조정한 뒤에는 강이 굽는 `Water_03_MiddleBend`와 폭이 넓어지는 `Water_04_EastFan`에서 움직임을 확인할 수 있다.

강 다섯 구간은 같은 `T_ForestRiverFlowCoordinates`를 읽는다. R은 상류에서 흐름을 따라 이동한 거리(m), G는 물줄기를 가로지르는 좌표, B는 강물 효과를 적용할 가중치다. 구간을 가르는 단면에서도 값이 연속이라 물결이 다음 BP로 이어진다. 호수와 웅덩이의 B는 0이다. 강의 흐름이나 폭이 바뀌면 `TunaSweeper/SourceArt/Environment/ForestWater/Textures/T_ForestRiverFlowCoordinates.exr`도 함께 수정한다. UE에서는 이 텍스처를 선형 부동소수점 데이터로 읽는다.

## 물가와 지형 수정

수면의 UV0은 구역 전체에 이어지는 미터 단위 평면 좌표다. UV1의 U에는 수면에서 실제 바닥까지의 수직 거리(미터)가 들어 있다. 지형을 다시 조각해 강바닥 높이나 강폭을 바꾸면 해당 수면의 UV1과 물가 윤곽도 함께 갱신한다. 편집 원본과 메시별 FBX는 `TunaSweeper/SourceArt/Environment/ForestWater`에 있다.

코끼리 호수는 수면 높이 −15cm, 면적 약 98.5㎡다. 코끼리가 서 있는 쪽에는 약 30cm 깊이의 얕은 턱이 있고 맞은편 바닥은 최대 약 3.06m까지 내려간다. 수면·하상·물가 메시와 같은 윤곽으로 식생 배치도 `TunaSweeper/SourceArt/Environment/ForestScatter/T_Layout_Forest_Draft.png`에 물을 표시한다. 호수 크기를 바꿀 때는 이 원본을 수정한 뒤 같은 이름의 UE 텍스처를 재임포트한다.

`Riverbed`는 실제 강바닥의 굴곡을 따르며 수심 약 2–8cm 사이에서 마른 지면과 섞인다. 하상과 수중 소품은 배치도 데칼을 받지 않는다. 숲의 `MI_Layout_Forest`는 외곽 5.6m에서 배치도를 서서히 흐리게 표시한다. 이 인스턴스에서 **Reveal painted shores**를 켜면 파란 수계 주변의 배치도 색이 옅어져 칠해 둔 흙이 드러난다. 노란 길은 원래 투영 농도로 표시하며, 식생 분류는 원본 `T_Layout_Forest_Draft`를 읽는다.

`M_ForestShoreFoam`은 물가 리본을 따라 긴 물결, 짧은 포말, 옅게 풀어지는 물자국 텍스처를 겹친다. 리본의 UV0은 강둑을 따라 누적한 거리(U)와 물가에서 물 안쪽으로 들어가는 비율(V)이다. 닫힌 호수의 UV1 U에는 전체 둘레가 들어가서 시작점과 끝점의 무늬가 이어진다. **Foam Opacity**, **Repeat Metres**, **Motion Speed**로 강도와 무늬의 길이, 속도를 조절한다. 하구 본체와 연장 수면이 만나는 단면은 그대로 이어져 물가 리본을 생략한다.

원래의 `Forest_ShallowPuddle_TestWater`는 웅덩이 발소리와 발걸음 파문을 처리한다. 수면과 물가의 표시는 `Water_07_ShallowPuddle`에서 편집한다. 코끼리 호수의 도전과제는 기존 `Forest_ElephantLake_Achievement13` 트리거가 담당한다.

## 길 위에 고인 빗물

아웃라이너의 `Environment/Forest/Water/RainPuddles`에 지면에 투영한 빗물 웅덩이 25개가 있다. 기존 7개와 같은 크기로 구부러진 물자국(BentRill)과 두 덩이가 이어진 모양(TwinLobe)을 각각 7개 배치했다. 부채꼴(FloodFan)과 긴 물받이 모양(LongBasin)은 대형으로 각각 2개다. 여울로 내려가는 길에서는 `Puddle_FordApproach_01`을 선택해 확인할 수 있다. 각각 지면에 직접 투영하는 **Decal Actor**다. 옮기거나 복제할 때 투영 화살표를 바닥 쪽으로 향하게 하고 **Decal Size**의 Y와 Z로 너비와 길이를 정한다. X는 투영 깊이로, 숲길에서는 반깊이 9cm를 사용한다. 랜드스케이프나 **Receives Decals**를 켠 평평한 메시 위에 쓸 수 있다. 경사진 바닥에는 화살표를 그 면에 수직으로 맞춘다.

`/Game/Environment/ForestWater/Materials`의 `MI_ForestGroundPuddle_Pool`은 넓게 고인 모양, `MI_ForestGroundPuddle_Runoff`는 길을 따라 가늘게 이어진 모양이다. 인스턴스를 복제한 뒤 **FootprintMask**에 선형 흑백 마스크를 지정하면 윤곽을 바꿀 수 있다. 흰색은 고인 물, 회색은 젖은 흙으로 이어지는 가장자리, 검은색은 마른 지면이다. **WetOpacity**와 **WetGroundTint**로 바닥의 비침과 색을 조절한다.

작은 물결의 노멀과 카메라 방향으로 반사된 하늘 무늬가 움직이면서 밝은 부분에 블룸이 맺힌다. **RippleSlope**는 수면 결의 세기, **WaterRoughness**는 광택의 거칠기, **HighlightIntensity**는 밝은 반사의 세기다. 이 빗물은 장식용이며, 발소리와 파문이 필요한 물은 위의 수역 액터에서 편집한다.
