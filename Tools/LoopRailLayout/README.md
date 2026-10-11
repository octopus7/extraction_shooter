# 마을 철로 검증

저장된 `/Game/MainRaid/RaidVillage`를 새 UE 5.7 에디터 프로세스에서 읽기 전용으로 검사한다. 맵이나 에셋을 저장하지 않으며 전후 맵 SHA-256을 비교한다.

```powershell
./Tools/LoopRailLayout/run_verify.ps1
./Tools/LoopRailLayout/run_verify.ps1 -Runtime
./Tools/LoopRailLayout/run_arrival_display.ps1
```

- 구조: 300m 마을 안의 둥근 사각형 루프, 3개 역, 4량 편성, 원래 중앙 테스트 액터 156개의 저장된 transform 보존.
- 철로: 1m 간격으로 양쪽 레일과 중앙의 지형 지지, 차량 공간, 최대 6도 경사, 곡선 접합 연속성, 경계 안의 배치를 검사한다. 침목 하단 기준 지면 오차 허용치는 ±10cm다.
- 정차 구간: 각 역 정차점 뒤 38.4m부터 앞 5m까지 수평 직선과 진행 방향을 검사한다.
- 승강장: 열차 바닥과 같은 높이, 승차 간격 10cm 이하, 기초 지면 지지, 진입 램프 외곽 세 지점의 지면 단차 40cm 이하를 검사한다.
- Runtime: PIE 복사본에서만 가속·속도·정차 시간과 시간 배율을 조절한다. 실제 `OnStationArrived` 이벤트로 세 역의 정차 오차와 정지 속도를 확인하고, 누적 이동 거리로 한 바퀴 이상 순환을 검사한 다음 PIE를 정상 종료한다.

결과는 `TunaSweeper/Saved/LoopRailLayout/verification.json` 또는 `runtime.json`에 기록한다. 기준 중앙 배치는 `Docs/level_layout_snapshots/RaidVillage_20261012.json`의 `after` transform이다.

`run_arrival_display.ps1`은 렌더링 가능한 PIE에서 세 역의 `m:ss` 문자열, 카운트다운, 운행 중지·도착 상태와 두 카메라 방향의 패널 정렬을 검사한다. `Saved/LoopRailLayout/ArrivalDisplay/`에 JSON과 Countdown/Arrived PNG를 남긴다. PNG의 검은 바탕·푸른 상단 도트·빈 하단은 육안으로 확인한다. 카메라와 열차 상태 변경은 저장하지 않는다.
