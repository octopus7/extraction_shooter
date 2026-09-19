# 물웅덩이 Niagara — 보류 작업

## 상태와 재개 조건

**보류 중. 이 문서는 제작 지침이며 Niagara 에셋 제작·UI 조작·예약 실행을 시작하지 않는다.**

사용자 요청: 물웅덩이는 먼저 구현하고, Niagara는 사용자가 컴퓨터를 사용하지 않는 동안 나중에 Computer Use로 제작한다. 사용자가 재개를 요청하고 컴퓨터를 사용하지 않는 시점에 진행한다. 기존 물 구현을 재사용하는 작업으로 바꾸지 않는다.

반드시 `D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject`를 열거나 해당 프로젝트의 기존 에디터를 재사용한다. UE 5.7 기준이다. 시작 전에 현재 Computer Use 스킬을 읽고, 사용자 작업 화면을 점유해도 되는 시점인지 확인한다.

## 이미 준비되는 연결

- 배치 액터: `/Game/Environment/ShallowPuddle/BP_ShallowPuddle`.
- 부모 클래스: `ATunaSweeperShallowPuddleActor`.
- 이벤트: `OnPuddleFootstep(SurfaceLocation, SpeedCmPerSecond, bSprinting, StepInstigator)`.
- `SurfaceLocation`: 실제 발걸음의 XY와 물 수면의 Z. 월드 좌표, cm 단위.
- `SpeedCmPerSecond`: 캐릭터 수평 속도. 질주 여부는 별도 bool로 전달된다.
- 발걸음 주체: `StepInstigator`.
- 발걸음 판정은 기존 플레이어 이동 타이밍을 사용한다. 정지·공중·구르기 때 재생하지 않으며, 물의 윤곽과 수심 범위를 확인한 뒤 이벤트를 발생시킨다.
- 물소리와 AI 청각 소음은 C++에서 처리한다. Niagara Blueprint에서 소리를 추가 재생하거나 AI 소음을 다시 발생시키지 않는다.
- 현재 머티리얼의 잔물결은 배경용 노멀 애니메이션이다. 발걸음에서 퍼지는 원형 파문은 이 보류 작업에서 추가한다.

## 제작할 에셋

다음은 **예정 경로**이며 현재 제작 완료를 뜻하지 않는다.

- `/Game/Environment/ShallowPuddle/FX/NS_ShallowPuddle_Footstep`
- `/Game/Environment/ShallowPuddle/FX/M_ShallowPuddle_Splash`
- `/Game/Environment/ShallowPuddle/FX/M_ShallowPuddle_Ripple`

시스템 하나에 소량의 물방울과 수면 위 링 파문을 만든다. Niagara Fluids나 지속적인 유체 시뮬레이션은 필요하지 않다. CPU emitter와 짧은 one-shot burst로 시작한다.

## Computer Use 제작 순서

1. Git 상태와 에디터의 미저장 작업을 확인한다. 물웅덩이 검토 맵 `/Game/Environment/ShallowPuddle/Maps/L_ShallowPuddle_Review`를 연다.
2. Content Browser에서 위 FX 폴더를 만들고 one-shot Niagara System을 생성한다. 로컬 공간은 끄고 월드 공간에서 입자가 남도록 한다.
3. 물방울 emitter: 발자국당 약 5~10개, 수명 0.15~0.4초, 초기 위쪽 속도 60~120cm/s, 작은 수평 분산, 중력, 크기 약 0.5~2cm로 시작한다. 하얀 거품 기둥처럼 과장하지 않는다. 이 숫자는 시작값이며 실제 탑다운 카메라에서 조절한다.
4. 링 emitter: 수면에 평행한 sprite 1~2개를 0.4~0.8초 동안 바깥으로 확장하고 알파를 줄인다. 수면보다 약 0.2~0.5cm 위에 두어 깜빡임을 줄인다. 카메라를 향하는 billboard 대신 월드 +Z 법선 정렬을 사용한다.
5. 링 머티리얼은 가운데가 빈 부드러운 원환 형태로 만든다. 과도한 Emissive를 피하고 물 위에 얹힌 흰 원판처럼 보이지 않게 한다. 필요하면 별도의 약한 노멀 왜곡 방식을 검토한다.
6. 시스템 User Parameter로 `User.StepStrength`(float)를 만든다. 걷기 1.0, 질주 약 1.4로 시작하고 크기·방출 속도에 제한적으로 적용한다. 사용자 파라미터가 스폰 첫 프레임부터 반영되도록 Spawn System at Location의 Auto Activate를 끈 상태에서 설정 후 Activate한다.
7. `BP_ShallowPuddle`의 BeginPlay에서 `OnPuddleFootstep`에 custom event를 한 번 바인딩한다. 전달된 SurfaceLocation에 시스템을 스폰하고 bSprinting으로 강도를 결정한다. 부모 BeginPlay 호출을 유지한다. 물웅덩이마다 Tick이나 overlap 타이머를 추가하지 않는다.
8. 시스템은 Auto Destroy와 적절한 pooling을 사용하고 완료 후 잔여 컴포넌트가 계속 쌓이지 않는지 확인한다. 전용 서버에는 시각 효과를 스폰하지 않는다.
9. 수명과 최대 속도를 반영한 작은 fixed bounds를 설정한다. 멀리 있는 물웅덩이는 Niagara scalability 거리 제한을 검토한다.

## 수용 확인

- 검토 맵의 맑은 물·기본 물·탁한 물에서 실제 게임 카메라로 확인한다.
- 걷기와 질주 각각 한 발소리당 한 번 발생한다. 정지·점프·구르기에서 불필요하게 발생하지 않는다.
- 물 바깥, 젖은 테두리만 있는 위치, 물 위의 높은 바닥에서 발생하지 않는다.
- 두 웅덩이가 겹쳐도 같은 발걸음에 효과가 중복되지 않는다.
- 링은 수평이며 수면 높이에 맞고, 물방울은 이동하는 캐릭터에 붙어서 따라가지 않는다.
- 연속 질주 1분 후 emitter와 component 수가 지속적으로 증가하지 않는다.
- 저장 후 에디터를 재실행하고 BP 바인딩, 시스템, 머티리얼이 유지되는지 확인한다.
- 실제 동작 캡처와 Niagara 통계/관찰 결과를 기록한 뒤 이 문서의 상태를 완료로 바꾼다. 구현 전에는 체크 완료나 동작 보장을 기록하지 않는다.

게임 UI 텍스트를 추가하게 되면 프로젝트 string-key/localization 경로를 사용한다. 생성기를 사용할 경우에는 프로젝트 규칙대로 생성 에셋과 생성기를 함께 검증·커밋한 직후 다음 커밋에서 일회성 생성기를 제거한다.
