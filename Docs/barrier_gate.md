# 무인 봉 차단기

- 배치용 BP: `/Game/Environment/BarrierGate/BP_BarrierGate`
- C++: `ATunaSweeperBarrierGateActor`
- Blender 원본/FBX/PNG: `TunaSweeper/SourceArt/Environment/BarrierGate/`
- 기존 레벨에 배치하거나 레벨을 저장하지 않는다. 사용자가 BP를 직접 배치한다.

## 외형과 방향

높이 약 112.5cm 제어함, 길이 350cm 적백색 봉, 제어함 전면의 붉은색/녹색 LED로 구성된다. 초소는 없다. 로컬 +X가 닫힌 봉의 길이 방향이고, 통행 방향은 양쪽 Y다. 회전 피벗은 제어함 바닥 중심 기준 `(0, 36, 100)cm`이며 +Z로 올라간다. 액터 전체를 회전해 통행 방향을 맞춘다.

제어함은 1024² 전용 BaseColor 텍스처를 사용한다. UV0은 면적 기준으로 정규화하고 겹침 없이 패킹했으며, 재질은 해당 UV를 그대로 샘플링한다. 실제 부품 형상에서 구운 색과 미세한 접합부 음영을 포함한다. 반복 패턴은 봉의 256×128 적백색 텍스처에만 사용한다. 원본 `.blend`에는 텍스처가 내장되어 있다. Blender 미리보기의 바닥/카메라/조명은 UE로 임포트하지 않는다.

## 동작과 조절

기본값은 닫힘이다. 플레이어가 조종하는 Pawn의 충돌 바운드가 감지 영역에 진입하면 열린다. 도보 플레이어와 플레이어가 조종하는 차량 Pawn을 같은 규칙으로 감지한다. 비플레이어 Pawn은 자동 감지 대상이 아니다.

| BP 설정 | 기본값 | 의미 |
|---|---:|---|
| Open Angle | 85° | 봉 개방 각도 |
| Open Duration | 1.2초 | 완전 닫힘에서 완전 개방까지 시간 |
| Close Duration | 1.8초 | 완전 개방에서 완전 닫힘까지 시간 |
| Detection Distance | 300cm | 봉 전후 각 방향의 감지 거리, 최소 100cm |
| Auto Close Delay | 1.5초 | 마지막 플레이어 이탈 뒤 닫힘 지연 |
| Auto Open | 켬 | 근접 자동 개폐 활성화 |
| Starts Open | 끔 | 최초 개방 상태 |
| LED Intensity | 8 | LED 발광과 주변 보조광 강도 |

0.1초마다 감지를 갱신한다. 닫히는 중에는 매 프레임 재진입도 확인한다. 플레이어가 영역 안에 있는 동안 닫힘 요청은 다시 열림으로 전환한다. `OpenGate`, `CloseGate`를 BP에서 직접 호출할 수 있다. Auto Open을 꺼도 수동 닫힘의 플레이어 안전 확인은 유지된다. 완전 개방은 녹색, 닫힘과 이동 중은 붉은색이다. 실제 봉 충돌체는 봉과 함께 회전한다.

상태는 런타임 일시 상태이며 저장 데이터나 저장 버전을 추가하지 않는다. 로드 시 Starts Open 설정에서 다시 시작하고 근접 감지를 갱신한다. 이 액터는 현재 프로젝트의 싱글플레이어 환경용이며 네트워크 복제를 추가하지 않는다.

## 검증

- `Tools/BarrierGate/run_tests.ps1`: 개폐 진행, 재요청/반전, LED 전환, 감지 양방향, 지연 닫힘, 점유 중 닫힘 방지, 잘못된 시간 설정, 저장 BP 생성, BeginPlay 감지 타이머, 점유 Pawn 제거.
- `Tools/BarrierGate/run_unreal.ps1`: 새 UE 프로세스에서 BP/에셋 바인딩, 메시 UV, 봉 길이와 피벗을 읽기 전용으로 검증.
- Blender에서 `Tools/BarrierGate/verify_model.py`: UV 퇴화/겹침, 텍셀 밀도, 봉 원점을 검증. 결과는 `TunaSweeper/Saved/Automation/BarrierGate/`에 기록한다.

일회성 모델 생성기와 임포트 코드는 에셋 생성 커밋 직후 별도 커밋으로 제거한다. 검증 도구만 유지한다.
