# LoopRail 상세 외형 원본

청록색 차체, 크림색 창틀, 황동 테두리와 목재 좌석을 사용한 전기 지상열차다. 기관차의 굴뚝은 장식용 1개이며, 기관실 내부와 작은 기계 부품은 만들지 않았다. 큰 윤곽에는 형상을 쓰고 패널 이음선, 환기구, 창 반사, 나무결, 체결부와 부분 마모는 해당 면의 고유 UV에 그린 텍스처로 표현했다.

## 파일

- `LoopRail_Detailed.blend`: 부품별로 편집 가능한 Blender 4.5 원본. 기관차·객차·지붕 컬렉션은 같은 원점을 사용한다. 처음에는 기관차만 보인다. 편집할 컬렉션을 켜고 나머지를 숨기면 된다.
- `SM_Locomotive.fbx`, `SM_Carriage.fbx`, `SM_CarriageRoof.fbx`, `SM_CarriageConnection.fbx`: 차량별 합쳐진 렌더 메시. 기관차와 객차에는 `UCX_SM_…` 단순 충돌 메시가 함께 있다.
- `T_<차량>_BaseColor.png`, `T_<차량>_Roughness.png`, `T_<차량>_Metallic.png`: 차량별 2048×2048 텍스처 3장. BaseColor는 sRGB, 나머지는 선형 데이터다.
- `UV_<차량>_Layout.png`: 실제 채색 위에 각 UV 아일랜드의 경계를 표시한 참고 이미지. 재질에는 연결하지 않는다.
- `uv_islands.json`: Blender 부품 이름·폴리곤 번호·표현 종류와 atlas 픽셀 영역의 대응표. 좌표 원점은 텍스처 왼쪽 아래다.
- `Preview_Locomotive.png`, `Preview_CarriageInterior.png`, `Preview_Consist.png`: 원본의 가까운 모습과 4량 편성 렌더. 편성 렌더는 지붕의 유무를 비교하도록 첫 객차 지붕을 숨겼다.
- `validation.json`, `fbx_validation.json`: 원본 수치와 FBX 재수입 검사 결과.

## 규격

| 항목 | 기관차 | 객차 | 분리 지붕 | 연결 통로 |
|---|---:|---:|---:|---:|
| 삼각형 | 4,916 | 9,012 | 188 | 396 |
| 단순 충돌 상자 | 3 | 52 | 0 | 0 |
| 렌더 재질 슬롯 | 1 | 1 | 1 | 1 |
| UV 아일랜드 | 2,170 | 4,806 | 102 | 234 |

객차와 지붕을 합쳐 9,200삼각형이다. 길이 900cm, 폭 220cm, 차량 중심의 레일 상면을 원점으로 사용한다. +X가 전방이고 +Z가 위다. Blender 1단위는 1cm이며 `Unit Scale=0.01`이다. 바퀴 최저점은 Z=0, 바닥 윗면은 Z=60이다. 지붕은 Z=248–280cm에 별도 배치된다.

객차는 8열×좌우 1석, 총 16석이다. 좌석 중심은 X=`-297.5+85×열 번호`, Y=`±70cm`이고 통로 폭은 90cm다. 양 끝 X=±390cm에서는 좌우로 탑승할 수 있다. 좌석·벽·기둥·끝 난간에만 단순 충돌을 넣었으며, 바닥 충돌은 플러그인의 이동 바닥 컴포넌트가 담당한다. 지붕은 충돌을 사용하지 않는다.

## 텍스처 수정

모든 면은 `UV_DetailAtlas`라는 같은 1차 UV 이름을 쓴다. 한 차량 안에서 UV를 겹치지 않았으며 각 아일랜드 둘레에 4픽셀 여백을 두었다. 좌석별 나무결과 작은 흠집도 서로 다른 영역에 있다. 완성된 객차를 여러 량 배치할 때는 같은 메시와 텍스처를 재사용한다.

색·마모 수정은 atlas에서 대응 부품 영역을 편집하면 된다. 바닥에는 목재와 중앙 미끄럼 방지 통로, 창에는 프레임과 반사, 후드 측면에는 환기구처럼 부품의 용도에 맞는 내용이 배치되어 있다. 텍스트나 차량 번호를 굽지 않았으므로 게임 UI 문자열 의존성이 없다.

## UE 재수입

각 FBX를 기존 `/LoopRail/Meshes/SM_<차량>`에 정적 메시로 가져온다. 단위 변환을 켜고 추가 스케일은 1로 둔다. `Combine Meshes=true`, `Auto Generate Collision=false`, `One Convex Hull Per UCX=true`를 사용한다. 재수입 후 길이 900cm·폭 220cm를 확인한다. UV0를 유지하고 라이트맵 UV가 필요하면 UV1로 생성한다.

재질은 아래처럼 각각 하나씩 할당한다. FBX의 자동 재질 생성 대신 PNG들을 같은 `/LoopRail` 플러그인 안으로 가져와 Base Color·Roughness·Metallic에 연결한다.

- `SM_Locomotive` → `/LoopRail/Materials/M_LocomotiveDetail`
- `SM_Carriage` → `/LoopRail/Materials/M_CarriageDetail`
- `SM_CarriageRoof` → `/LoopRail/Materials/M_CarriageRoofDetail`

원본과 최종 FBX에서 크기, 삼각형 수, 1차 UV 범위, 16석 충돌 및 68cm 캡슐이 지나는 통로·양 끝 출입구를 검사했다. FBX를 다시 출력할 때도 모든 부품의 UV 이름을 유지해야 합친 메시에서 지붕 등의 UV가 별도 채널로 분리되지 않는다.

최종 FBX 검사에서는 256개 원본 부품 모두가 닫힌 형상이고 면이 바깥쪽을 향함을 확인했다. 원본과 재수입 FBX의 전체 43,536개 코너 UV 대응도 일치한다.

## 차량 사이 연결 통로

객차 양 끝 난간을 좌우로 나누어 중앙 90cm를 비웠다. 끝 난간 충돌도 같은 개구부를 유지하며, 원래의 측면 출입구와 16석은 그대로다.

`SM_CarriageConnection.fbx`는 길이 100cm(+X), 외곽 폭 100cm, 바닥 윗면 Z=0·아랫면 Z=-10cm이고 원점은 바닥 윗면 중심이다. 난간은 Z=90cm까지 올라오며 내부 유효 폭은 92cm다. 메시에는 UCX가 없다. 움직이는 바닥과 측면 난간 충돌은 런타임 컴포넌트가 담당한다.

연결 통로는 두 차량 끝 사이의 실제 거리와 겹침 여유에 맞춰 X축만 늘려 사용한다. 바닥의 고무 리브와 금속 끝판은 고유 UV 텍스처로 표현했다. 새 atlas도 2048×2048 BaseColor·Roughness·Metallic이며 재질은 `/LoopRail/Materials/M_CarriageConnectionDetail`이다. FBX 규격 확인 시 이 메시의 길이는 차량 본체와 달리 100cm다.

`Preview_CarriageConnection.png`는 단독 모듈이고 `Preview_Consist.png`는 기본 80cm 차량 간격에 길이 120cm로 늘려 양 끝 20cm씩 겹친 예시다. 연결된 실제 곡선 주행의 지면 이동과 충돌 검증은 엔진에서 수행한다.
