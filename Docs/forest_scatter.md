# 숲 식생 배치

`/Game/MainRaid/RaidForest`의 나무와 수국은 개별 액터로 편집한다. 랜드스케이프의 풀·덤불·들꽃·자갈·큰돌은 다섯 구역이 `/Game/Environment/ForestScatter/PCG_ForestScatter` 그래프를 공유하며, 각 구역의 닫힌 스플라인과 지면에 투영된 배치도의 색으로 배치한다.

## 심어 둔 나무와 외곽 바위 찾기

- 아웃라이너의 `Environment/Forest/Trees`를 열면 북서·북동·남서·남동 구역별로 심어 둔 나무가 있다. 이름은 `Tree_Planted_구역_번호_수종_X좌표_Y좌표` 형식이다. `P`와 `N`은 월드 원점 기준 양수와 음수, 숫자는 미터 단위의 최초 식재 위치다. 나무를 선택해 위치·회전·크기를 바로 편집할 수 있다.
- 벚나무는 `Environment/Forest/Trees/CherryBlossom` 아래에 군락별로 모았다. `ElephantLake`는 코끼리 호숫가 5그루, `WestPool`은 상류 물가 3그루, `MiddleBend`는 중류 굽이 3그루다. 줄기 중심은 물가에서 2.7~6.7m 떨어진 녹지에 있다.
- 플레이어나 커서가 나무의 수평 경계에서 3.5m 이내로 들어오면 수관이 사라져 시야가 열린다. `/Game/Effects/DA_OcclusionRevealSettings`의 **Vertical Reveal Proximity Radius Cm**, **Vertical Reveal Start Above Player Cm**, **Vertical Reveal Fade Height Cm**에서 접근 거리, 캐릭터 기준 시작 높이(60cm), 전환 폭(30cm)을 조절한다. 숲의 참나무·느릅나무·벚나무와 마을의 벚나무·안뜰 나무가 이 설정을 공유한다.
- `Environment/Forest/Hydrangeas` 아래의 `Hills`, `Buildings`, `Passages`에서 수국을 찾을 수 있다. 마을 통로 양쪽 바위 기슭, 창고·차고·전력 시설 주변, 서쪽 벽과 철망문 옆에 심었다. 이름은 `Hydrangea_Planted_장소_번호_X좌표_Y좌표` 형식이다. 위치를 옮기거나 새로 심을 때는 해당 장소 폴더에서 개별 액터를 편집한다.
- `Environment/Forest/GroundCover`에서 서쪽 숲, 중앙 초지, 남쪽 공터, 동쪽 강둑, 동쪽 호숫가 구역을 찾을 수 있다. 작은 연못과 코끼리 호숫가에서는 식재 스플라인이 물가를 따라간다.

## 구역 그리기

1. 새 구역을 그릴 때는 기존 액터 선택을 해제한다. 레벨 에디터의 모드를 **PCG**로 바꾸고 **Draw Spline Surface**에서 `PCG_ForestScatter` 프리셋을 선택한다. 기존 구역을 선택하고 그리면 첫 클릭부터 그 구역의 외곽을 다시 그린다.
2. 지면을 클릭해 식생을 심을 외곽을 그리고 도구의 **Accept**로 확정한다. 액터 배치 패널에서 **Forest Scatter Region**을 끌어온 뒤 `Boundary` 스플라인의 점을 편집해도 된다.
3. 구역 액터의 디테일에서 **Grass**, **Shrubs**, **Flowers**, **Gravel**, **Boulders**의 메시와 **Instances per 100 Square Metres**를 조절한다. 수치는 배치 가능한 수평 면적 100㎡당 평균 개수다. 숲의 다섯 구역은 모두 잔디 밀도가 200이다. 밀도 0 또는 빈 메시는 해당 종류를 생략한다.
4. **Regenerate Forest**를 누른다. 결과는 구역 액터에 인스턴스로 모인다. 확정할 배치는 생성한 상태에서 레벨을 저장하면 플레이할 때 그대로 보인다. 작업 중 결과를 걷어낼 때는 해당 PCG 컴포넌트의 **Cleanup**을 사용한다. 스플라인과 구역 설정은 다시 생성할 때도 쓸 수 있다.

꽃의 줄기와 잎은 `SM_ForestFlower`의 재질 슬롯 0, 꽃잎은 슬롯 1을 쓴다. **Flower Petal Materials**에서 흰색·노란색·장미색·라일락색 재질을 바꾸고, **Flower Color Patch Size**에서 같은 색이 모여 피는 범위(cm)를 조절한다. 수국은 보라색 꽃송이가 여러 개 뭉친 별도 메시다.

잔디 인스턴스 하나는 가까운 거리에서 넓은 잎 62장이 열두 무리로 모여 보인다. 잎 밑동 폭은 약 5.5~8.3cm다. 가장 가까운 LOD의 원본 메시에서 잎 높이의 중앙값은 약 34cm, 뿌리부터 잎끝까지의 수평 거리는 약 6cm다. `SM_ForestLawnGrass`의 세 LOD 모두 노멀을 위로 맞추고 그림자 생성을 껐다. PCG의 **Grass** 레이어는 메시를 교체하고 다시 생성해도 이 설정을 적용한다. 나무와 바위가 드리우는 그늘은 잔디에 그대로 보인다.

숲 지면과 잔디 재질은 `MF_ForestMeadowField`에서 같은 월드 좌표의 녹색을 읽는다. `Meadow Open Green`과 `Meadow Rich Green`은 열린 초지와 풀이 짙은 곳의 색이고, `Meadow Patch Size cm`은 색이 달라지는 공간의 크기다. 군락 안쪽은 부드러운 색면으로 이어지고 경계의 일부에서 풀잎이 길게 올라온다. `M_ForestLawnGrass`의 `Quiet Tuft Height`는 낮은 풀의 높이 비율이며 기본값은 0.72다. `Blade Tip Color Multiplier`는 잎 윗부분의 색을 조절한다. 기본 선형 RGB 배율은 (1.16, 1.14, 0.97)로, 잎끝에 연한 녹색 차이를 준다. 바람의 편측 진폭은 **Forest Wind → WindDisplacementCm** 벡터의 길이로 정한다. 현재 X 13.76cm, Y 5.98cm로, 완전히 휘는 잎끝은 중심에서 한쪽으로 최대 15cm 움직인다. 뿌리는 고정되고 포기마다 바람의 위상이 조금씩 다르다.

## 외곽 절벽과 하구

- 아웃라이너의 `Environment/Forest/Cliffs`에서 `Escarpment_RisingCliffs_01`부터 `Escarpment_RisingCliffs_20`까지 개별 선택해 이동하거나 회전할 수 있다. 각 조각의 기슭은 중앙 랜드스케이프 안쪽에 걸쳐 있고, 암벽의 실제 표면이 충돌을 담당한다. 편집 원본은 `TunaSweeper/SourceArt/Environment/ForestScatter/CliffSections/ForestCliffSections.blend`다.
- 하구 양쪽에서 암벽이 끝난다. 그 사이로 강물이 외곽 랜드스케이프 끝까지 이어진다. 수로와 양안을 조각할 때는 **Landscape** 모드에서 `Environment/Forest/Cliffs/OuterTerrain`의 해당 `TS_Forest_OuterTerrain_*`를 선택한다. 네 구역의 높이 원본은 `TunaSweeper/SourceArt/Environment/ForestScatter/RaidForest_OuterRidge.r16`이다.

## 녹지와 건물

- 랜드스케이프의 북서쪽 고지는 배치도 10시 방향의 넓은 회색 절벽을 따른다. 강바닥은 파란 수계를 따라 북동쪽 암반 출구까지 이어지고, 노란 길이 강을 가로지르는 두 곳은 얕은 여울이다. 에디터의 Landscape 모드에서 높이를 편집할 수 있다. 원본 505×505 높이맵은 `TunaSweeper/SourceArt/Environment/ForestScatter/RaidForest_Relief.r16`에 있다.
- **Landscape → Paint**에서 `TS_Forest_Landscape`의 **Dirt**를 칠하면 강·호수·작은 연못의 흙 물가를 고칠 수 있다. 물가에서 멀어지면서 흙을 풀과 섞고, 드러난 비탈과 큰 바위 기슭에는 **Rock**, 나무 아래에는 **GrassDark**를 사용했다. 길 위 장식용 웅덩이 아래의 흙도 같은 **Dirt** 레이어다.
- 중앙 지형의 `M_ForestLandscapeGround`는 **Dirt** 비중이 60%를 넘는 부분부터 **Loam Tint**를 섞어 물가와 젖은 길에 짙은 흙색을 낸다.
- 숲의 다섯 구역은 **Vegetation Paint Layers**에 **Grass**와 **GrassDark**를 사용한다. 두 레이어의 합이 **Minimum Vegetation Paint Weight**(0.16)보다 작은 곳에서는 풀·들꽃·덤불을 비운다. 물가를 다시 칠한 뒤 해당 구역의 **Regenerate Forest**를 누르면 새 페인트를 따른다. 자갈과 큰돌은 배치도와 구역 설정으로 조절한다.
- `RaidForest`는 배치도 데칼의 머터리얼에서 `PreviewTexture`를 읽는다. 데칼의 이동·회전·크기를 반영하고 실제 랜드스케이프 높이에 뿌린다. 녹지에 둘러싸인 작은 원형 회색 표식도 녹지로 취급한다. 노란 길·불모지, 파란 물, 넓은 회색 절벽은 비워 둔다.
- **Projection Decals**가 비어 있으면 레벨의 배치도 데칼을 자동으로 찾는다. 데칼을 지정하면 해당 목록을 사용한다. 배치도가 덮지 않는 지면은 비워 둔다.
- 숲 전용 배치도(`MI_Layout_Forest`)는 데칼 표시나 불투명도를 꺼도 녹지 판정에 쓰인다. 에디터와 게임 화면에서 배치도를 숨긴 채 **Regenerate Forest**를 눌러도 된다.
- 건물이 지면을 차지할 때는 그 건물을 **Grounded Buildings**에 추가한다. 기본은 정적 메시의 충돌 형상이 지면에 닿는 부분을 비운다. 건물 메시의 충돌이 단순해서 안뜰이나 기둥 사이까지 막는 경우, 바닥을 차지하는 부분마다 박스 컴포넌트를 두고 **Component Tags**에 `ForestScatterFootprint`를 넣는다. 태그가 있는 건물은 표시한 컴포넌트만 사용한다. 공중에 띄운 구조물 아래에는 식생이 자랄 수 있다.
- 스플라인, 밀도, 메시, 배치도, 건물 위치를 편집한 뒤 다시 **Regenerate Forest**를 누르면 현재 설정을 반영한다.
- 구역은 각각 독립적으로 심는다. 두 구역을 맞붙여 사용할 때는 외곽이 겹치지 않게 그리고, 의도적으로 겹쳐 심을 때는 한쪽 PCG 컴포넌트의 **Seed**를 바꾸면 같은 위치에 같은 메시가 포개지는 것을 피할 수 있다.

## 마을 벚나무

마을의 벚나무 6그루는 `Environment/Village/Trees/CherryBlossom` 아래의 `WellGreen`, `OrchardLane`, `EastGardens`, `MarketEdge`에서 편집한다. 두 맵은 `/Game/Environment/CherryBlossom/SM_CherryBlossom`을 공유한다. Blender 편집 원본은 `TunaSweeper/SourceArt/Environment/CherryBlossom/CherryBlossom_XHigh.blend`다. UV0은 줄기 방향과 꽃무리 형태를 따라 펼친 2048×2048 채색용 아틀라스, UV1은 라이트맵용이다.

## 마을 잔디

`/Game/MainRaid/RaidVillage`의 `Environment/Village/Grass` 폴더에는 해안 풀밭(`CoastalVerge`), 서쪽 주택가(`WestGardens`), 동쪽 주택가(`EastGardens`), 북쪽 초지(`NorthMeadow`)가 있다. 각 구역의 **Grass → Instances per 100 Square Metres** 값은 네 구역 모두 200이다. 경계 스플라인이나 밀도를 바꾼 뒤 **Regenerate Forest**를 누르면 해당 구역의 잔디 인스턴스를 다시 심는다. 잔디 메시와 잎끝 바람은 숲과 공유한다.

`VillageGrass_PlantingGuide`는 포장도로·현관길·건물·선로 자갈·승강장·모래사장과 물을 비우는 에디터 전용 가이드다. 식재 가능 지면에서 랜드스케이프 **Grass**와 **GrassDark**의 합이 0.44 이상인 곳에만 풀을 심는다. 길을 옮길 때는 `TunaSweeper/SourceArt/Environment/VillageYards/T_VillageMeadowPlanting.png`의 녹색 식재 범위와 황색 제외 범위를 수정하고 같은 이름의 텍스처를 재임포트한다. 이미지의 위쪽이 북쪽이고 오른쪽이 동쪽이다. 가이드 데칼의 **Component Tags**에 있는 `PCGPlantingGuide`가 화면 표시와 독립적으로 이 배치도를 활성화한다.
