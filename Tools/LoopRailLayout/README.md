# 마을 철로 검증

저장된 `/Game/MainRaid/RaidVillage`를 새 UE 5.7 에디터 프로세스에서 읽기 전용으로 검사한다. 맵이나 에셋을 저장하지 않으며 전후 맵 SHA-256을 비교한다.

```powershell
./Tools/LoopRailLayout/run_verify.ps1
./Tools/LoopRailLayout/run_verify.ps1 -Runtime
./Tools/LoopRailLayout/run_arrival_display.ps1
```

- 저장된 순환선의 평면 경로를 2026년 11월 15일 선형과 대조한다. 세 역과 4량 편성, 레일과 침목 인스턴스 수, 마을 경계를 검사한다.
- 선로를 약 1m 간격으로 따라가며 양쪽 레일과 중앙에서 랜드스케이프와 자갈 도상을 각각 추적한다. 레일 윗면과 침목을 받치는 자갈 사이의 간격, 최대 종단 경사 6도, 차량 통과 공간을 확인한다.
- 세 역에서 열차를 정차시켜 객차 출입구와 승강장의 높이 차이 5cm 이하, 수평 승차 간격 10cm 이하, 진입 램프 끝과 지면의 높이 차이 5cm 이하를 확인한다.
- 북쪽 흙길의 건널목 표지 두 개와 목재 바닥을 확인한다. 건널목 중앙과 양옆에서 지면 단차를 측정하고 캡슐 통과와 보행 경로를 검사한다.
- Runtime: PIE 복사본에서만 가속·속도·정차 시간과 시간 배율을 조절한다. 실제 `OnStationArrived` 이벤트로 세 역의 정차 오차와 정지 속도를 확인하고, 누적 이동 거리로 한 바퀴 이상 순환을 검사한 다음 PIE를 정상 종료한다.

결과는 `TunaSweeper/Saved/LoopRailLayout/verification.json` 또는 `runtime.json`에 기록한다. 평면 경로의 기준 제어점과 접선은 `Docs/level_layout_snapshots/RaidVillage_RailLayout_20261115.json`의 `nodes`에 있다.

`run_arrival_display.ps1`은 렌더링 가능한 PIE에서 세 역의 `m:ss` 문자열, 카운트다운, 운행 중지·도착 상태를 검사한다. 220×100×25cm 몸체의 승강장 접지와 상단 표시 위치, 두 경사 시점의 Z축 회전, 정수직 시점의 마지막 방향 유지 및 카메라 이동 중 충돌체 불변도 확인한다. `Saved/LoopRailLayout/ArrivalDisplay/`에 JSON과 Countdown/Arrived PNG를 남긴다. PNG의 검은 입체 몸체·푸른 상단 도트·빈 하단은 육안으로 확인한다. 카메라와 열차 상태 변경은 저장하지 않는다.
