# 교전 이동 시뮬레이터

## 목적

`Tools/CombatMovementSimulator`는 TunaSweeper 본체 코드, 플레이어 캐릭터, 플레이어 컨트롤러에 영향 없이 교전 AI 이동 감각을 빠르게 반복 검증하기 위한 .NET 10 Windows 도구다.

이 도구의 1차 목표는 최종 이식 비용 최소화가 아니라, 여러 번 실행하며 거리 유지, 추적, 후퇴, 측면 이동, 직사 화선 확보, 장애물 파괴/차단 감각을 빠르게 판단하는 것이다.

## 실행

```bat
Tools\CombatMovementSimulator\RunCombatMovementSimulator.bat
```

또는:

```bat
dotnet run --project Tools\CombatMovementSimulator\CombatMovementSimulator.csproj
```

헤드리스 자동 평가는 GUI 없이 실행한다.

```bat
dotnet run --project Tools\CombatMovementSimulator\CombatMovementSimulator.csproj -- --headless-eval --runs=16 --seconds=45
```

- 자동 평가는 `EvaluationCandidate`가 만든 튜닝 후보를 여러 시드로 반복 실행한다.
- 자동 플레이어는 `BalancedKite`, `PressureStrafe`, `SurvivalKite`, `CoverProbe` 프로필로 WASD/조준/사격 입력을 생성한다.
- 평가기는 플레이어 파일럿 내부 판단을 보지 않고 `SimulationWorld` 상태와 `SimulationTelemetry` 결과만 읽는다.
- 실행 결과는 `Tools/CombatMovementSimulator/evaluation_logs/*.jsonl`과 `Docs/combat_movement_self_eval.md`에 타임스탬프와 함께 기록된다.

## 현재 구현

- .NET 10 WinForms 기반 2D 시뮬레이터.
- 실행 직후에는 시뮬레이션이 정지된 편집 모드로 시작한다.
- 우측 패널의 `Start` 버튼을 누르면 시뮬레이션 모드로 전환되고, 그때부터 플레이어 이동, 사격, 적 AI가 업데이트된다.
- 월드 단위는 Unreal 기준과 맞춰 cm로 사용한다.
- 화면 12시 방향은 프로젝트 기준 북쪽인 월드 `+X`다.
- 화면 오른쪽은 월드 `+Y`다.
- 플레이어는 WASD로 이동하고, `Shift`가 눌린 동안 2배 속도로 스프린트하며, 마우스 방향으로 조준하고, 좌클릭으로 사격한다.
- 장애물은 회전 가능한 직사각형이다.
- 장애물 유형은 파괴 가능 장애물과 무적/불멸 장애물 두 가지다.
- 파괴 가능 장애물은 총알을 맞으면 내구도가 감소하고 0 이하가 되면 제거된다.
- 무적/불멸 장애물은 총알과 이동을 막지만 파괴되지 않는다.
- 적 유형은 근접 적과 총기 적 두 가지다.
- 비교전 상태의 적은 플레이어를 계속 바라보지 않고 `Idle`과 `Wander`를 랜덤 시간 범위로 반복한다.
- 비교전 상태의 적은 자신의 전방 시야 부채꼴 안에 플레이어가 들어오고 무적 장애물에 시야가 막히지 않을 때 교전에 진입한다.
- 근접 적은 접근 후 공격 거리에서 짧은 선딜 뒤 피해를 준다.
- 총기 적은 UE 본체의 `TunaSweeperEnemyAIController` 원거리 교전 흐름을 따라 `AdvanceBurst`, `HoldFire`, `SeekLineOfFire`, `KeepDistance` 상태로 움직인다.
- 총기 적은 교전 진입 즉시 접근하지 않고 먼저 `HoldFire` 상태로 멈춰 사격을 시도한 뒤, 홀드 시간이 끝난 다음 거리/시야 상태에 따라 전진 여부를 다시 판단한다.
- 총기 적은 별도 공격 거리 제한 없이 직접 시야와 발사 쿨다운 조건을 만족하면 사격한다.
- 총기 적은 멀리 있을 때 짧은 전진 버스트를 수행한 뒤 일정 시간 멈춰 사격하며, 계속 접근만 하지 않는다.
- 총기 적은 선호 거리 유지, 근접 압박 시 후퇴, 직사 화선 탐색을 수행한다.
- 총기 적은 파괴 가능 장애물 너머의 목표에는 사격해 장애물을 깎을 수 있고, 무적 장애물에 막히면 측면 이동으로 화선을 찾는다.
- 적은 타입별 감지 거리 안에서 교전을 시작하고, 한번 교전에 들어가면 더 긴 `CombatDisengageRange` 밖으로 나가기 전까지 전투를 유지한다.
- 디버그 표시가 켜져 있으면 비교전 상태의 적은 교전으로 이어지는 시야 부채꼴을 25% 투명도 점선 외곽선으로 표시한다.
- 교전 중인 적은 플레이어를 지나 `CombatDisengageRange`까지 남은 구간만 두께 2의 25% 투명도 점선으로 표시한다.

## 조작

- `WASD`: 이동
- `Shift`: 누르고 있는 동안 스프린트, 플레이어 이동 속도 2배
- `Mouse`: 조준
- `LMB`: 사격
- `Start` 버튼: 편집 모드에서 시뮬레이션 시작
- `R`: 시나리오 리셋
- `P`: 일시정지
- `T`: 디버그 표시 토글
- `F`: 적 AI 정지 토글
- `B`: 배치 모드 순환
- `1`: 근접 적 배치 모드
- `2`: 총기 적 배치 모드
- `3`: 파괴 가능 장애물 배치 모드
- `4`: 무적/불멸 장애물 배치 모드
- `RMB`: 현재 배치 모드의 적 또는 장애물 배치
- `MouseWheel` 또는 `Q/E`: 장애물 배치 미리보기 회전
- `- / +`: 배치 미리보기 너비 조정
- `[ / ]`: 배치 미리보기 높이 조정
- `Delete`: 마우스 아래 적 또는 장애물 제거
- `Esc`: 배치 모드 해제

## 튜닝 데이터

기본 수치는 `Tools/CombatMovementSimulator/combat_tuning.json`에 있다.

현재 분리된 주요 수치:

- 플레이어 반경, 이동 속도, 스프린트 배율, 체력, 투사체 속도, 피해량, 연사 간격
- 적 반경, 체력, 근접/총기 이동 속도
- 근접/소총/측면기동형 감지 거리, 교전 중 이탈 거리, 시야각, 비교전 대기/배회 시간, 배회 이동 속도, 근접 선딜/후딜, 근접 피해량
- 총기 적 선호 최소/최대 거리, 위험 근접 거리
- 총기 적 장거리/중거리 전진 거리 범위, 장거리/중거리/선호 거리 정지 사격 시간 범위
- 총기 적 직사 화선 탐색 시간 범위, 근접 거리 벌리기 시간 범위, 전진/화선 탐색/후퇴 측면 이동 가중치
- 총기 적 투사체 속도, 피해량, 발사 간격, 측면 이동 전환 시간
- 투사체 반경, 파괴 가능 장애물 기본 내구도, 렌더 스케일

현재 총기 적 정지 사격 시간:

- 장거리 홀드: `1.6~2.4초`
- 중거리 홀드: `2.4~3.6초`
- 선호 거리 홀드: `2.8~4.4초`

현재 적 교전 거리:

- 근접 적 감지 거리: `900cm`
- 권총 Flanker 감지 거리: `1050cm`
- 일반 소총 적 감지 거리: `1150cm`
- 교전 중 이탈 거리: `3600cm`
- 비교전 시야각: `100도`

## 판단 포인트

시뮬레이터에서 우선 판단할 항목:

- 근접 적이 직선 추적만 하는 느낌이 과하게 기계적인지.
- 근접 적이 공격 거리에서 멈추는 타이밍이 자연스러운지.
- 총기 적이 전진 버스트 후 적절히 멈춰서 교전하는지.
- 총기 적이 선호 거리 안에서 너무 많이 흔들리거나 너무 오래 정지해 보이지 않는지.
- 총기 적이 근접 압박을 받을 때 후퇴 판단이 충분히 빠른지.
- 파괴 가능 장애물 뒤 목표를 쏘는 행동이 전투를 풍부하게 만드는지.
- 무적 장애물에 막혔을 때 측면 이동으로 화선을 찾는 움직임이 납득 가능한지.
- 파괴 가능 장애물 내구도가 너무 빨리 녹거나 너무 오래 버티지 않는지.

## UE 이식 메모

현재 C# 구현은 의사결정 검증용이다. UE 이식 시 다음 경계를 유지하는 것이 좋다.

- 상태 enum은 `Idle`, `Wander`, `AdvanceBurst`, `HoldFire`, `Approach`, `HoldRange`, `Strafe`, `Retreat`, `KeepDistance`, `SeekLineOfFire`, `AttackCommit`, `Recover`를 기본으로 가져간다.
- 거리는 cm 단위 그대로 이식한다.
- 플레이어/적 위치는 `AActor` 위치 기반으로 읽고, 플레이어 캐릭터나 플레이어 컨트롤러에 직접 의존하지 않는다.
- 무적 장애물과 파괴 가능 장애물 판정은 라인트레이스 결과의 actor/component 태그나 인터페이스로 분리한다.
- 최종 이동은 시뮬레이터의 직접 위치 이동이 아니라 UE `CharacterMovement` 또는 `AIController` 이동 명령으로 변환한다.
- UE 검증 단계에서는 테스트 전용 Pawn/AIController에 먼저 붙이고, 실제 적 클래스 연결은 마지막 단계로 둔다.

## 현재 한계

- NavMesh, CharacterMovement, 애니메이션, 루트모션은 반영하지 않는다.
- 투사체와 충돌 판정은 2D 원/직사각형 기준으로 단순화되어 있다.
- 적끼리의 충돌 회피는 아직 단순화되어 있다.
- 엄폐 후보 점수화는 아직 없고, 직사 화선 탐색은 UE 본체와 같은 측면 이동 기반이다.
- 튜닝 파일 런타임 재로드 UI는 아직 없다.

## 2026-07-09 측면기동형 추가

- 적 타입에 `Flanker`를 추가했다.
- 기본 시나리오에 측면기동형 적 2마리를 배치했다.
- 배치 모드에서 `3` 키로 측면기동형 적을 선택하고, 기존 장애물 배치 단축키는 `4/5`로 옮겼다.
- 측면기동형은 `FlankerPreferredMin`~`FlankerPreferredMax` 거리 안에서 `Strafe` 상태로 플레이어 주변을 좌우 원호 이동하며 사격한다.
- 측면기동형은 너무 멀면 정면 접근보다 측면 가중치가 큰 `AdvanceBurst`로 접근하고, 너무 가까우면 대각 후퇴하는 `KeepDistance`로 빠진다.
- 무적 장애물에 직사 시야가 막히면 `SeekLineOfFire`로 측면 이동을 우선 수행한다.
- 측면기동형 전용 튜닝값은 `FlankerTrackingRange`, `FlankerMoveSpeed`, `FlankerPreferredMin`, `FlankerPreferredMax`, `FlankerDangerClose`, `FlankerOrbitSeconds`, `FlankerAdvanceDistance`, `FlankerProjectileSpeed`, `FlankerProjectileDamage`, `FlankerFireCooldown` 등으로 분리했다.
- 총기형은 `RangedPreferredMax` 안에 들어오면 기본적으로 더 이상 거리를 좁히지 않고 `HoldFire` 중심으로 유지한다. 접근 버스트도 `RangedPreferredMax`까지만 줄어들도록 제한했다.
