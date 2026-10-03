# SSTO 검증

평원맵 중앙의 `TS_Plains_SSTOLander`는 항상 표시되는 선체·바닥·설비 10개와 상부 디졸브 대상 3개로 구성한다. 상부 ChildActor의 VerticalOcclusionReveal 컴포넌트가 기존 마스크를 사용하며, 재질 인스턴스의 배색을 유지한다. `M_SSTO_VerticalReveal`은 기존 `M_OcclusionVerticalRevealMasked`의 opacity 그래프를 복제하고 외관 매개변수만 추가한 재질이다.

외부 `TS_Plains_SSTO_Entry`와 내부 `TS_Plains_SSTO_Exit`를 사다리 상호작용으로 연결한다. 도착 지점은 바닥 좌표이며 실제 플레이어 캡슐의 높이·충돌·지면 지지를 검사한다. 기존 플레이어와 컨트롤러를 유지한다. UI 문구는 `ui.interaction.ssto_enter`, `ui.interaction.ssto_exit`로 조회한다.

UE 5.7에서 다음 명령으로 검사한다. 에디터는 명시된 프로젝트로 숨김 실행되며, 캐시 쓰기 권한이 필요하다.

```powershell
./Tools/SSTOLander/run_verify.ps1
./Tools/SSTOLander/run_review.ps1 -Mode Runtime
./Tools/SSTOLander/run_review.ps1 -Mode Capture
```

- Structure: 저장 맵 재로드, 원본 좌표·UE 빌드 삼각형 수·재질·경계 보존, 지면 추적과 Pawn 캡슐 통로 검사, Map Check.
- Runtime: SIE의 테스트용 관전자 제한을 해제하고 게임 캐릭터를 제어해 실제 상호작용으로 두 번 왕복한다. 기존 컴포넌트의 틱이 배색을 보존한 MID에 활성화·절단 높이·디졸브 폭을 전달하는지 확인한다.
- Capture: 외관과 기존 마스크가 적용된 내부를 렌더링한다. 실제 디졸브 매개변수를 설정하며, 메시를 숨겨 내부를 표시하지 않는다.

검증·캡처는 에셋 패키지를 저장하지 않으며 전후 해시를 비교한다. 결과는 `TunaSweeper/Saved/SSTOLander_20261003`에 기록된다. 원본 FBX·GLB·배치 명세와 리뷰 이미지는 `TunaSweeper/SourceArt/Environment/SSTO_Lander`에 있다. 원본과 UE 빌드에서 제거된 미세 삼각형 수를 명세에 구분해 기록한다. 일회성 모델·에셋 생성 경로는 최종 소스에 포함하지 않는다.
