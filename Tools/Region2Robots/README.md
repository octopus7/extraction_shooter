# 고양이 NPC와 바퀴형 로봇 검증

UE 5.7 자산 경로:

- `/Game/Characters/FactionCats/BP_MarineCat`, `BP_PirateCat`: 팔레트 머터리얼 하나를 공유하는 32³ 복셀 임시 NPC.
- `/Game/Characters/Region2Robots/BP_DeliveryRobot_Marine`, `BP_DeliveryRobot_Pirate`: 공통 흰색 몸체와 좌우 바퀴. 눈은 공통 `M_RobotEyes`를 부모로 하는 파랑·빨강 MI를 사용한다.
- `/Game/Characters/Region2Robots/BP_RabbitTrike`: 앞바퀴 하나와 뒤쪽 좌우 바퀴를 가진 토끼 로봇.

원본 blend, FBX, 텍스처와 미리보기는 `TunaSweeper/SourceArt/Characters/FactionCats`와 `Region2Robots`에 있다. 복셀 제작 규칙은 [별도 지침](../../Docs/voxel_placeholder_modeling.md)을 따른다.

## 토끼 바이크 이동

`BP_RabbitTrike`는 기존 CharacterMovement의 경로 탐색·접지·캡슐 충돌을 사용하며, `bTricycle`인 경우에만 전용 이동 컴포넌트가 조향한다. 차체는 전투 프로필의 회전 속도 상한으로 이동 방향을 향하고, 수평 이동 속도는 바퀴의 전후 축으로 제한된다. 방향 차이가 클수록 감속하며 60도 이상에서는 제동 후 방향을 맞춘다. 짧은 경로와 좁은 공간에서는 정지 회전을 허용한다. 비스듬한 벽에 부딪혀도 옆으로 미끄러지지 않고 조향 또는 기존 경로 복구를 기다린다.

주행 중에는 플레이어를 향한 강제 차체 회전과 사격을 차단한다. 이동 요청·입력이 끝나고 수평 속도가 5cm/s 이하가 되면 기존 정차 조준으로 돌아가며, 프로필의 공격 허용 각도 안에서 사격한다. 배회에서 의심·경계 상태로 바뀌면 정지 후 소리 또는 목표 방향을 볼 수 있다. 앞바퀴는 실제 주행 거리와 yaw 변화로 조향하고 구르며, 뒤쪽 바퀴는 거리와 회전에 맞춰 각각 구른다. 배송로봇의 기존 이동·근접 공격은 유지한다.

`Automation RunTests TunaSweeper.Enemy.Wheeled+TunaSweeper.Combat`으로 조향·감속·사격·경계 전환, 저장된 BP의 이동 컴포넌트, 입력/경로 속도에 따른 실제 바닥 이동, 정면/사선 벽 충돌, 기존 배송로봇과 전투 AI를 검사한다. 검증용 월드만 생성하며 에셋은 저장하지 않는다.

마을의 `Region2/NPCs`, `Region2/EnemyAnchors`, `Region2/Navigation` 폴더에서 배치와 이동 영역을 찾을 수 있다. 앵커의 위치는 맵이, 적 종류와 확정 등장 설정은 `Content/Data/MainRuntimeDefaults/EnemySpawns.json` 및 `EnemySpawnProfiles.json`이 소유한다.

`verify_assets.py`는 저장된 메시 크기·UV·머터리얼, NPC 제공자, 로봇 BP 구성과 바퀴 수, 앵커와 JSON의 일치, 지형 접지 및 근처 이동 경로를 검사한다. 패키지 저장이나 자산 생성은 하지 않는다. 검사 중 임시 월드를 열고 마지막에 마을을 로드하므로, 먼저 [에디터 조율 규칙](../../Docs/unreal_editor_coordination.md)을 따라 작업을 저장한다. PIE 또는 미저장 변경이 있으면 검사를 거부한다.

깨끗한 프로젝트 에디터의 Python 콘솔에서 실행한다:

```python
import runpy
runpy.run_path("D:/github/extraction_shooter/Tools/Region2Robots/verify_assets.py")
```

검사는 에디터 틱을 통해 비동기로 진행한다. 최종 결과는 `TunaSweeper/Saved/Region2Work/asset_validation.json`의 `passed`와 Output Log의 `REGION2_ASSET_VERIFY_PASS`로 확인한다. 에디터는 자동 종료하지 않는다. 일회성 모델 생성기·임포터·맵 배치 스크립트는 완료 소스에 포함하지 않는다.
