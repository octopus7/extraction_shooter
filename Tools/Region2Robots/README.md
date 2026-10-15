# 고양이 NPC와 바퀴형 로봇 검증

UE 5.7 자산 경로:

- `/Game/Characters/FactionCats/BP_MarineCat`, `BP_PirateCat`: 팔레트 머터리얼 하나를 공유하는 32³ 복셀 임시 NPC.
- `/Game/Characters/Region2Robots/BP_DeliveryRobot_Marine`, `BP_DeliveryRobot_Pirate`: 공통 흰색 몸체와 좌우 바퀴. 눈은 공통 `M_RobotEyes`를 부모로 하는 파랑·빨강 MI를 사용한다.
- `/Game/Characters/Region2Robots/BP_RabbitTrike`: 앞바퀴 하나와 뒤쪽 좌우 바퀴를 가진 토끼 로봇.

원본 blend, FBX, 텍스처와 미리보기는 `TunaSweeper/SourceArt/Characters/FactionCats`와 `Region2Robots`에 있다. 복셀 제작 규칙은 [별도 지침](../../Docs/voxel_placeholder_modeling.md)을 따른다.

마을의 `Region2/NPCs`, `Region2/EnemyAnchors`, `Region2/Navigation` 폴더에서 배치와 이동 영역을 찾을 수 있다. 앵커의 위치는 맵이, 적 종류와 확정 등장 설정은 `Content/Data/MainRuntimeDefaults/EnemySpawns.json` 및 `EnemySpawnProfiles.json`이 소유한다.

`verify_assets.py`는 저장된 메시 크기·UV·머터리얼, NPC 제공자, 로봇 BP 구성과 바퀴 수, 앵커와 JSON의 일치, 지형 접지 및 근처 이동 경로를 검사한다. 패키지 저장이나 자산 생성은 하지 않는다. 검사 중 임시 월드를 열고 마지막에 마을을 로드하므로, 먼저 [에디터 조율 규칙](../../Docs/unreal_editor_coordination.md)을 따라 작업을 저장한다. PIE 또는 미저장 변경이 있으면 검사를 거부한다.

깨끗한 프로젝트 에디터의 Python 콘솔에서 실행한다:

```python
import runpy
runpy.run_path("D:/github/extraction_shooter/Tools/Region2Robots/verify_assets.py")
```

검사는 에디터 틱을 통해 비동기로 진행한다. 최종 결과는 `TunaSweeper/Saved/Region2Work/asset_validation.json`의 `passed`와 Output Log의 `REGION2_ASSET_VERIFY_PASS`로 확인한다. 에디터는 자동 종료하지 않는다. 일회성 모델 생성기·임포터·맵 배치 스크립트는 완료 소스에 포함하지 않는다.
