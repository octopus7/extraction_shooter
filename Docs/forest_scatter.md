# 숲 식생 배치

`/Game/MainRaid/RaidForest`의 나무와 수국은 개별 액터로 편집한다. 랜드스케이프의 풀·덤불·들꽃·자갈·큰돌은 다섯 구역이 `/Game/Environment/ForestScatter/PCG_ForestScatter` 그래프를 공유하며, 각 구역의 닫힌 스플라인과 지면에 투영된 배치도의 색으로 배치한다.

## 심어 둔 나무와 외곽 바위 찾기

- 아웃라이너의 `Environment/Forest/Trees`를 열면 북서·북동·남서·남동 구역별로 심어 둔 나무가 있다. 이름은 `Tree_Planted_구역_번호_수종_X좌표_Y좌표` 형식이다. `P`와 `N`은 월드 원점 기준 양수와 음수, 숫자는 미터 단위의 최초 식재 위치다. 나무를 선택해 위치·회전·크기를 바로 편집할 수 있다.
- `Environment/Forest/Hydrangeas` 아래의 `Hills`, `Buildings`, `Passages`에서 수국을 찾을 수 있다. 마을 통로 양쪽 바위 기슭, 창고·차고·전력 시설 주변, 서쪽 벽과 철망문 옆에 심었다. 이름은 `Hydrangea_Planted_장소_번호_X좌표_Y좌표` 형식이다. 위치를 옮기거나 새로 심을 때는 해당 장소 폴더에서 개별 액터를 편집한다.
- `Environment/Forest/GroundCover`에서 서쪽 숲, 중앙 초지, 남쪽 공터, 동쪽 강둑, 동쪽 호숫가 구역을 찾을 수 있다. 작은 연못과 코끼리 호숫가에서는 식재 스플라인이 물가를 따라간다.

## 구역 그리기

1. 새 구역을 그릴 때는 기존 액터 선택을 해제한다. 레벨 에디터의 모드를 **PCG**로 바꾸고 **Draw Spline Surface**에서 `PCG_ForestScatter` 프리셋을 선택한다. 기존 구역을 선택하고 그리면 첫 클릭부터 그 구역의 외곽을 다시 그린다.
2. 지면을 클릭해 식생을 심을 외곽을 그리고 도구의 **Accept**로 확정한다. 액터 배치 패널에서 **Forest Scatter Region**을 끌어온 뒤 `Boundary` 스플라인의 점을 편집해도 된다.
3. 구역 액터의 디테일에서 **Grass**, **Shrubs**, **Flowers**, **Gravel**, **Boulders**의 메시와 **Instances per 100 Square Metres**를 조절한다. 수치는 배치 가능한 수평 면적 100㎡당 평균 개수다. 풀은 서쪽 숲 37, 중앙 초지 30, 남쪽 공터 33, 동쪽 강둑 39, 동쪽 호숫가 42로 설정했다. 밀도 0 또는 빈 메시는 해당 종류를 생략한다.
4. **Regenerate Forest**를 누른다. 결과는 구역 액터에 인스턴스로 모인다. 확정할 배치는 생성한 상태에서 레벨을 저장하면 플레이할 때 그대로 보인다. 작업 중 결과를 걷어낼 때는 해당 PCG 컴포넌트의 **Cleanup**을 사용한다. 스플라인과 구역 설정은 다시 생성할 때도 쓸 수 있다.

꽃의 줄기와 잎은 `SM_ForestFlower`의 재질 슬롯 0, 꽃잎은 슬롯 1을 쓴다. **Flower Petal Materials**에서 흰색·노란색·장미색·라일락색 재질을 바꾸고, **Flower Color Patch Size**에서 같은 색이 모여 피는 범위(cm)를 조절한다. 수국은 보라색 꽃송이가 여러 개 뭉친 별도 메시다.

`SM_ForestLawnGrass`는 짧은 잎의 버텍스 노멀을 위로 맞춘 잔디다. `M_ForestLawnGrass`는 뿌리에서 잎끝으로 갈수록 밝고 선명한 색을 쓰며, 월드 좌표를 기준으로 흐르는 바람에 맞춰 잎끝의 위치와 색을 조금씩 바꾼다.

## 외곽 절벽과 식재

- `Environment/Forest/Cliffs`에서 외곽을 편집한다. `Escarpment_ForestShelf`는 숲 바닥에서 절벽 기슭까지 이어지는 식재대다. `Escarpment_RisingCliffs`는 숲 안쪽을 향해 솟은 암벽으로, 길에서 보이는 암벽 윗선은 월드 높이 약 13~27m를 오간다. 남쪽은 카메라가 뒤로 물러날 공간을 두고 완만한 기슭에서 암벽으로 이어진다. `Escarpment_OuterLimit`은 숲 가장자리를 따라 이동을 막는다.
- 그 뒤의 능선은 `Environment/Forest/Cliffs/OuterTerrain`의 랜드스케이프 네 조각이다. **Landscape** 모드에서 방향별 `TS_Forest_OuterTerrain_*`를 선택해 조각하거나 기존 숲의 풀·흙·바위 레이어를 칠할 수 있다. 능선의 최고점은 월드 높이 약 50m다. 외곽 전체의 높이 원본은 `TunaSweeper/SourceArt/Environment/ForestScatter/RaidForest_OuterRidge.r16`, 암벽과 하구 연장부의 편집 원본은 같은 폴더의 `ForestRisingCliffs.blend`다.
- 윗면의 풀과 덤불은 에디터 **Foliage** 모드에서 수정한다. **Static Meshes** 표면 필터를 켜고 `/Game/Environment/ForestScatter/Foliage`에서 잔디는 `FT_ForestShelfGrass`, 키 큰 풀 포기는 `FT_ForestShelfTussock`, 덤불은 `FT_ForestShelfShrub`를 선택한 뒤 **Paint** 또는 **Erase**를 쓴다.
- 숲의 하늘과 조명은 레벨의 태양·하늘광·대기 액터에서 편집한다. 플레이 화면에서 보이는 외곽선은 암벽과 능선의 실제 형상을 따른다.

## 녹지와 건물

- 랜드스케이프의 북서쪽 고지는 배치도 10시 방향의 넓은 회색 절벽을 따른다. 강바닥은 파란 수계를 따라 북동쪽 암반 출구까지 이어지고, 노란 길이 강을 가로지르는 두 곳은 얕은 여울이다. 에디터의 Landscape 모드에서 높이를 편집할 수 있다. 원본 505×505 높이맵은 `TunaSweeper/SourceArt/Environment/ForestScatter/RaidForest_Relief.r16`에 있다.
- `RaidForest`는 배치도 데칼의 머터리얼에서 `PreviewTexture`를 읽는다. 데칼의 이동·회전·크기를 반영하고 실제 랜드스케이프 높이에 뿌린다. 녹지에 둘러싸인 작은 원형 회색 표식도 녹지로 취급한다. 노란 길·불모지, 파란 물, 넓은 회색 절벽은 비워 둔다.
- **Projection Decals**가 비어 있으면 레벨의 배치도 데칼을 자동으로 찾는다. 데칼을 지정하면 해당 목록을 사용한다. 배치도가 덮지 않는 지면은 비워 둔다.
- 건물이 지면을 차지할 때는 그 건물을 **Grounded Buildings**에 추가한다. 기본은 정적 메시의 충돌 형상이 지면에 닿는 부분을 비운다. 건물 메시의 충돌이 단순해서 안뜰이나 기둥 사이까지 막는 경우, 바닥을 차지하는 부분마다 박스 컴포넌트를 두고 **Component Tags**에 `ForestScatterFootprint`를 넣는다. 태그가 있는 건물은 표시한 컴포넌트만 사용한다. 공중에 띄운 구조물 아래에는 식생이 자랄 수 있다.
- 스플라인, 밀도, 메시, 배치도, 건물 위치를 편집한 뒤 다시 **Regenerate Forest**를 누르면 현재 설정을 반영한다.
- 구역은 각각 독립적으로 심는다. 두 구역을 맞붙여 사용할 때는 외곽이 겹치지 않게 그리고, 의도적으로 겹쳐 심을 때는 한쪽 PCG 컴포넌트의 **Seed**를 바꾸면 같은 위치에 같은 메시가 포개지는 것을 피할 수 있다.
