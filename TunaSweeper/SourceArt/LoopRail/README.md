# LoopRail 상세 외형 원본

마을 순환선의 전기 기관차는 높은 중앙 운전실, 경사진 긴 전방 후드, 짧은 후방 후드와 원형 전조등을 갖는다. 객차에는 양끝 승강대, 개방형 아치 창, 목재 좌석과 환기용 이중 지붕이 있다. 두 차량 모두 차체 하단에 옅은 선로 먼지를 입혔다. 발판 모서리와 황동 손잡이는 접촉 부위의 색과 거칠기가 달라진다.

## 파일

- `Concept_Locomotive.png`와 `Concept_Carriage.png`는 외형과 도장에 사용한 시안이다.
- `LoopRail_Detailed.blend`는 Blender 4.5 원본이다. 첫 장면에는 4량 편성을 배치했다. 장면 선택기에서 기관차, 객차, 지붕 또는 연결 발판을 골라 원점에서 부품을 편집할 수 있다. 객차 장면에서 `CarriageRoof` 컬렉션을 끄면 좌석과 하부 차체가 보인다.
- `SM_Locomotive.fbx`, `SM_Carriage.fbx`, `SM_CarriageRoof.fbx`, `SM_CarriageConnection.fbx`: 차량별 합쳐진 렌더 메시. 기관차와 객차에는 `UCX_SM_…` 단순 충돌 메시가 함께 있다.
- `T_<차량>_BaseColor.png`, `T_<차량>_Roughness.png`, `T_<차량>_Metallic.png`: 차량별 2048×2048 텍스처 3장. BaseColor는 sRGB, 나머지는 선형 데이터다.
- `UV_<차량>_Layout.png`: 실제 채색 위에 각 UV 아일랜드의 경계를 표시한 참고 이미지. 재질에는 연결하지 않는다.
- `uv_islands.json`: Blender 부품 이름·폴리곤 번호·표현 종류와 atlas 픽셀 영역의 대응표. 좌표 원점은 텍스처 왼쪽 아래다.
- `Preview_Locomotive.png`, `Preview_Carriage.png`, `Preview_CarriageInterior.png`, `Preview_CarriageConnection.png`, `Preview_Consist.png`는 실제 모델과 최종 텍스처를 사용한 렌더다.
- `Preview_VillagePair.png`는 새 기관차와 객차를 연결한 모습이다. `Preview_VillageCoach.png`와 `Preview_VillageReveal.png`는 같은 위치에서 찍은 마을 맵의 UE 렌더다. 후자는 창틀까지 사라지는 경계를 볼 수 있도록 검증용 반경을 사용했다.
- `validation.json`, `fbx_validation.json`: 원본 수치와 FBX 재수입 검사 결과.

## 규격

| 항목 | 기관차 | 객차 | 분리 지붕 | 연결 통로 |
|---|---:|---:|---:|---:|
| 삼각형 | 11,956 | 12,372 | 3,248 | 2,228 |
| 단순 충돌 상자 | 5 | 42 | 0 | 0 |
| 렌더 재질 슬롯 | 1 | 1 | 1 | 1 |

객차와 지붕의 합계는 15,620삼각형이다. 차량 길이는 900cm, 폭은 약 220cm다. 차량 중심의 레일 상면을 원점으로 사용한다. +X가 전방이고 +Z가 위다. Blender 1단위는 1cm이며 `Unit Scale=0.01`이다. 바퀴 최저점은 Z=0, 바닥 윗면은 Z=60이다.

객차는 8열×좌우 1석, 총 16석이다. 좌석 중심은 X=`-297.5+85×열 번호`, Y=`±70cm`다. 좌석 사이의 충돌 유효 폭은 96cm이며 양 끝 X=±390cm에서는 좌우로 탑승할 수 있다. 좌석과 하부 안전 난간의 단순 충돌은 객차 본체에 둔다. 이동 바닥 컴포넌트가 승객을 지지한다.

## 창틀과 지붕

베이지색 아치 기둥과 창틀 상단, 양끝 승강대의 지붕 받침은 모두 `CarriageRoof`에 속한다. 이 상부 조립체는 로컬 Z=60cm에서 약 280cm까지 이어지며 충돌이 없다. 청록색 하판·목재 창턱·손잡이·좌석·바닥은 `Carriage`에 남는다. 마을 열차는 이 상부 FBX의 게임 전용 사본 `/Game/MainRaid/Rail/SM_CarriageRoofReveal`을 사용한다. 플레이어와 커서 위치에 따른 마스크가 지붕과 창틀에 함께 적용된다.

## 텍스처 수정

모든 면은 `UV_DetailAtlas`라는 같은 1차 UV 이름을 쓴다. 한 차량 안에서 UV를 겹치지 않았으며 각 아일랜드 둘레에 4픽셀 여백을 두었다. 좌석별 나무결과 작은 흠집도 서로 다른 영역에 있다. 완성된 객차를 여러 량 배치할 때는 같은 메시와 텍스처를 재사용한다.

색·마모 수정은 atlas의 대응 부품 영역을 편집하면 된다. 차체 도장은 바퀴 위의 낮은 부분에 먼지와 가는 빗자국이 집중된다. 목재는 판재별 결 방향을 따르고, 승강 발판은 요철과 모서리 마모를 갖는다. 유리·도장·고무·황동의 차이는 색상·거칠기·금속성 지도에 나누어 표현했다.

## UE 재수입

각 FBX를 기존 `/LoopRail/Meshes/SM_<차량>`에 정적 메시로 가져온다. 단위 변환을 켜고 추가 스케일은 1로 둔다. `Combine Meshes=true`, `Auto Generate Collision=false`, `One Convex Hull Per UCX=true`를 사용한다. 재수입 후 길이 900cm·폭 220cm를 확인한다. UV0를 유지하고 라이트맵 UV가 필요하면 UV1로 생성한다.

재질은 아래처럼 각각 하나씩 할당한다. FBX의 자동 재질 생성 대신 PNG들을 같은 `/LoopRail` 플러그인 안으로 가져와 Base Color·Roughness·Metallic에 연결한다.

- `SM_Locomotive` → `/LoopRail/Materials/M_LocomotiveDetail`
- `SM_Carriage` → `/LoopRail/Materials/M_CarriageDetail`
- `SM_CarriageRoof` → `/LoopRail/Materials/M_CarriageRoofDetail`
- `SM_CarriageConnection` → `/LoopRail/Materials/M_CarriageConnectionDetail`
- 마을의 `SM_CarriageRoofReveal`도 같은 `SM_CarriageRoof.fbx`에서 재수입한다. 재질 슬롯에는 `/Game/MainRaid/Rail/M_CarriageRoofReveal`을 할당한다.

원본과 최종 FBX에서 크기, 삼각형 수, 1차 UV 범위, 16석 충돌 및 68cm 캡슐이 지나는 통로·양 끝 출입구를 검사했다. FBX를 다시 출력할 때도 모든 부품의 UV 이름을 유지해야 합친 메시에서 지붕 등의 UV가 별도 채널로 분리되지 않는다.

재수입 수치와 부품별 결과는 `fbx_validation.json`과 `unreal_validation.json`에 기록한다.

## 차량 사이 연결 통로

객차 양 끝 난간을 좌우로 나누어 중앙 90cm를 비웠다. 끝 난간 충돌도 같은 개구부를 유지하며, 원래의 측면 출입구와 16석은 그대로다.

`SM_CarriageConnection.fbx`는 길이 100cm(+X), 외곽 폭 100cm, 바닥 윗면 Z=0·아랫면 Z=-10cm이고 원점은 바닥 윗면 중심이다. 가운데에는 횡방향 고무 리브를 놓고, 양끝에는 경첩이 달린 금속판을 배치했다. 교차 보강대가 있는 난간은 Z=90cm까지 올라오며 내부 유효 폭은 92cm다. 움직이는 바닥과 난간 충돌은 런타임 컴포넌트가 담당한다.

연결 통로는 두 차량 끝 사이의 실제 거리와 겹침 여유에 맞춰 X축만 늘려 사용한다. 발판의 고무와 금속은 전용 2048×2048 BaseColor·Roughness·Metallic atlas를 사용하며 재질은 `/LoopRail/Materials/M_CarriageConnectionDetail`이다.

`Preview_CarriageConnection.png`는 단독 모듈이고 `Preview_Consist.png`는 기본 80cm 차량 간격에 길이 120cm로 늘려 양 끝 20cm씩 겹친 예시다. 연결된 실제 곡선 주행의 지면 이동과 충돌 검증은 엔진에서 수행한다.
