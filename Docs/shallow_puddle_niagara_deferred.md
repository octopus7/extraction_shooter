# 물웅덩이 Niagara — 구현 상태와 후속 시각 조정

## 상태와 재개 조건

**2026-09-20: Computer Use로 물방울·파문과 BP 발걸음 연결을 제작·저장했다. 사용자 요청으로 현재 상태를 먼저 커밋한다. 실제 게임 카메라 거리의 크기·가시성 조정과 최종 시각 수용 확인은 남아 있다.**

첫 시도는 원격 화면 캡처 오류로 중단했으며 복구 후 동일 프로젝트 에디터에서 제작했다. 마지막 Computer Use는 사용자의 Esc 입력으로 중단됐다. 현재 에셋은 저장됐고 크기 조정은 아직 적용하지 않았다. 후속 UI 작업은 재개 지시 후 충돌 여부를 조율한다.

사용자 요청: 물웅덩이는 먼저 구현하고, Niagara는 나중에 Computer Use로 제작한다. 재개 시 다른 프로젝트 작업의 에디터 조작이나 빌드가 충돌하면 일시 보류를 요청해 이 작업을 우선한다. 기존 물 구현을 재사용하는 작업으로 바꾸지 않는다.

반드시 `D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject`를 열거나 해당 프로젝트의 기존 에디터를 재사용한다. UE 5.7 기준이다. 시작 전에 현재 Computer Use 스킬을 읽고, 사용자 작업 화면을 점유해도 되는 시점인지 확인한다.

## 현재 연결

- 배치 액터: `/Game/Environment/ShallowPuddle/BP_ShallowPuddle`.
- 부모 클래스: `ATunaSweeperShallowPuddleActor`.
- 이벤트: `OnPuddleFootstep(SurfaceLocation, SpeedCmPerSecond, bSprinting, StepInstigator)`.
- `SurfaceLocation`: 실제 발걸음의 XY와 물 수면의 Z. 월드 좌표, cm 단위.
- `SpeedCmPerSecond`: 캐릭터 수평 속도. 질주 여부는 별도 bool로 전달된다.
- 발걸음 주체: `StepInstigator`.
- 발걸음 판정은 기존 플레이어 이동 타이밍을 사용한다. 정지·공중·구르기 때 재생하지 않으며, 물의 윤곽과 수심 범위를 확인한 뒤 이벤트를 발생시킨다.
- 물소리와 AI 청각 소음은 C++에서 처리한다. Niagara Blueprint에서 소리를 추가 재생하거나 AI 소음을 다시 발생시키지 않는다.
- 수면 머티리얼의 잔물결은 배경용 노멀 애니메이션이며, 발걸음에서 퍼지는 원형 파문은 별도 Niagara sprite다.

## 저장된 에셋과 설정

아래 에셋은 UE 편집기 UI로 제작했으며 일회성 에셋 생성기는 사용하지 않았다.

- `/Game/Environment/ShallowPuddle/FX/NS_ShallowPuddle_Footstep`
- `/Game/Environment/ShallowPuddle/Materials/M_ShallowPuddle_Splash`
- `/Game/Environment/ShallowPuddle/Materials/M_ShallowPuddle_Ripple`

- CPU, 월드 공간, 한 번 재생. 물방울 8개, 수명 0.18~0.35초, 초기 속도 60~120cm/s, 크기 0.7~1.8cm.
- 파문 1개, 수명 0.6~0.75초, 기본 크기 28~36cm에 수명 0→1 확장 곡선을 곱한다. 알파는 1→0으로 감소한다. 월드 +Z facing과 +Y alignment로 수평을 유지한다.
- `User.StepStrength`: 걷기 1.0, 질주 1.4. 현재 파문 크기에 적용하며 물방울 속도에는 적용하지 않았다.
- `BP_ShallowPuddle`: BeginPlay → 부모 BeginPlay → 이벤트 바인딩. 발걸음 시 전용 서버를 제외하고 수면 위치 +Z 0.3cm에 생성한다. Auto Activate를 끄고 유효성 검사 → 강도 설정 → Activate 순서다. Auto Destroy 사용, pooling은 None이다. 추가 Tick·오디오·AI 소음 노드는 없다.
- 시스템 fixed bounds는 각 축 -100~100cm다. 별도 Effect Type/거리 scalability 제한은 아직 추가하지 않았다.

## 검증과 남은 작업

- BP UI 컴파일 성공. 새 UE 5.7 프로세스에서 저장된 에셋과 실제 PIE BeginPlay 바인딩을 읽어 맑은 물·기본 물·탁한 물의 걷기/질주 6개 조건을 검사했다. 발걸음당 컴포넌트 1개, 강도 값, 수면 +0.3cm 위치, 활성화, 종료 후 제거 검증 통과.
- 60초 분량의 180회 질주 이벤트를 수동 시뮬레이션했다. 최대 동시 컴포넌트 4개, 종료 후 0개. 실제 사용자가 1분간 직접 달린 검사와는 구분한다.
- Niagara SimCache에서 실제 입자 위치·크기·알파와 파문 +Z facing/+Y alignment를 읽었다. 근접 렌더에서 가운데가 빈 파문을 확인했다. 원거리 캡처만으로 게임 카메라 가시성은 승인하지 않았다.
- 기존 `Footprint`, `Selection`, `PlayerFootsteps` 자동 테스트 3개 성공. 저장 에셋/오디오/이벤트 검증도 통과했다. 이번 변경의 C++ 부분은 주석만 바뀌었으며 새 C++ 빌드는 수행하지 않았다.
- 결과: `TunaSweeper/Saved/Automation/ShallowPuddle/niagara.json`, `niagara_render_verify.log`, `regression_after_niagara.log`, `ShallowPuddle_Niagara_Close.png`.
- **후속 작업**: 실제 게임 카메라 거리에서 파문 크기·가시성을 조정하고 직접 걷기/질주 시 모습을 최종 확인한다. 거리 scalability/pooling은 필요성을 확인한 뒤 결정한다. 아래 원래 수용 목록 전체가 완료된 것은 아니다.

검증 도구 `Tools/ShallowPuddle/verify_niagara.py`는 별도 프로세스의 저장하지 않는 PIE 월드에서 실행하는 검사·캡처 도구다. 에셋을 생성하거나 저장하지 않는다. `UnrealEditor-Cmd.exe`에 프로젝트와 검토 맵을 명시하고 `-ExecutePythonScript="D:/github/extraction_shooter/Tools/ShallowPuddle/verify_niagara.py" -ExecCmds="fx.Niagara.ForceWaitForCompilationOnActivate 1" -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -ddc=InstalledNoZenLocalFallback -unattended -nosplash -RenderOffscreen`으로 실행한다. 스크립트는 검증 후 해당 프로세스를 종료하므로 사용 중인 에디터에서 실행하지 않는다. 성공 판정은 프로세스 종료 코드만 보지 말고 `niagara.json`의 `status=passed`와 로그를 확인한다.

시스템 하나에 소량의 물방울과 수면 위 링 파문을 만든다. Niagara Fluids나 지속적인 유체 시뮬레이션은 필요하지 않다. CPU emitter와 짧은 one-shot burst로 시작한다.

## 원래 Computer Use 제작 지침

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
