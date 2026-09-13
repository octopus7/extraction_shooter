# 배관 수리 BP

- 레벨에는 `/Game/Interaction/BunkerPipe/BP_BunkerPipe_Broken`을 배치한다.
- 부모는 `ATunaSweeperWorldProgressActor`. 방수 테이프(6005) 1개를 소비하면 `BP_BunkerPipe_Repaired`로 교체된다.
- 기존 퀘스트 이벤트 `demo.bunker_pipe.repair`를 사용한다.
- Details의 World Progress에서 `ProgressObjectId`를 배치한 배관마다 고유하고 안정적인 값으로 지정한다(예: `bunker.pipe.01`). BP 기본값은 None이며 미지정 시 액터 이름을 사용한다.
- `RequiredQuantity`로 테이프 소모량을 바꿀 수 있다.
- 두 메시 모두 에디터 C++ 코드로 만든 32면 세로 실린더이며 높이 240cm, 지름 40cm, 피벗은 바닥 중앙이다. 수리 전은 빨강, 완료 후는 회색 머터리얼을 메시 애셋에 지정했다.
- `ProgressVisualMesh`에 애셋을 지정하면 기본 다리 시각화를 대체한다. 비워 둔 기존 다리 BP는 기존 동작을 유지한다.
- 교체 시 위치·회전·스케일을 유지한다. 배관 중심 충돌 상자는 로컬 Z=120cm이며 완료 BP에도 물리 충돌이 있다.
- 수리 상태는 기존 WorldProgressStates 저장/복원 경로를 사용한다. 별도 세이브 형식은 추가하지 않았다.
- 자동 검사: `Automation RunTests TunaSweeper.WorldProgress.BunkerPipe`.
- 애셋 생성기는 애셋과 함께 커밋한 뒤 프로젝트 규칙에 따라 바로 제거한다. 최종 프로젝트는 저장된 애셋만 사용한다.
