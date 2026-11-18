# SSTO 검증

평원 착륙선은 흰 동체와 일체형 날개, 흑연색 하부 내열 타일을 사용한다. 조종석 앞유리는 기수를 감싸며 양쪽 끝이 어깨 쪽으로 올라간다. 주 객실 중앙 통로는 폭 180cm, 두 조종석 사이 통로는 폭 130cm다. 선체 로컬 +X가 기수, -Y가 좌현이다. [기체 형태 시안](../../chatgpt/Models/SSTO/ssto_redesign_concept_01.png) · [편집용 Blender 원본](../../TunaSweeper/SourceArt/Environment/SSTO_Lander/SSTO_Lander.blend)

평원맵 중앙의 `TS_Plains_SSTOLander`는 항상 표시되는 선체·바닥·설비 10개와 상부 디졸브 대상 3개로 구성한다. 상부 ChildActor의 VerticalOcclusionReveal 컴포넌트가 기존 마스크를 사용하며, 재질 인스턴스의 배색을 유지한다. `M_SSTO_VerticalReveal`은 기존 `M_OcclusionVerticalRevealMasked`의 opacity 그래프를 복제하고 외관 매개변수만 추가한 재질이다.

왼쪽 승강 계단 아래의 `TS_Plains_SSTO_Entry`와 조종석 뒤 통로의 `TS_Plains_SSTO_Exit`를 연결한다. 도착 지점은 바닥 좌표이며 실제 플레이어 캡슐의 높이·충돌·지면 지지를 검사한다. 기존 플레이어와 컨트롤러를 유지한다. UI 문구는 `ui.interaction.ssto_enter`, `ui.interaction.ssto_exit`로 조회한다.

UE 5.7에서 다음 명령으로 검사한다. 에디터는 명시된 프로젝트로 숨김 실행되며, 캐시 쓰기 권한이 필요하다.

```powershell
./Tools/SSTOLander/run_verify.ps1
./Tools/SSTOLander/run_review.ps1 -Mode Runtime
./Tools/SSTOLander/run_review.ps1 -Mode Capture
./Tools/SSTOLander/verify_ending.ps1
```

- 저장된 맵의 메시·재질·경계와 Pawn 캡슐 통로를 검사한다. 기체의 해치와 착륙 장치 기준점은 `Manifests/SSTO_Lander.json`에, 실제 지면에 맞춘 계단과 세 다리의 접점, 승하차 위치, 양쪽 장치의 위치·방향과 접근 경로는 `Manifests/SSTO_Placement.json`에 기록한다.
- Runtime: SIE의 테스트용 관전자 제한을 해제하고 게임 캐릭터를 제어해 실제 상호작용으로 두 번 왕복한다. 기존 컴포넌트의 틱이 배색을 보존한 MID에 활성화·절단 높이·디졸브 폭을 전달하는지 확인한다.
- 평원맵에서 외관, 앞유리 근접 화면, 상부 디졸브가 적용된 객실을 촬영한다.
- 엔딩 PIE 검사는 별도 슬롯에서 승선한 뒤 중앙 통로를 따라 각 장치 전면까지 플레이어 캡슐을 통과시킨다. 보관함 열기·아이템 이동과 기존 두 엔딩의 상호작용을 확인한다.

검증·캡처는 에셋 패키지를 저장하지 않으며 전후 해시를 비교한다. 결과는 `TunaSweeper/Saved/SSTOLander_20261003`에 기록된다. 원본 FBX·GLB·배치 명세와 리뷰 이미지는 `TunaSweeper/SourceArt/Environment/SSTO_Lander`에 있다. 원본과 UE 빌드에서 제거된 미세 삼각형 수를 명세에 구분해 기록한다. 일회성 모델·에셋 생성 경로는 최종 소스에 포함하지 않는다.
