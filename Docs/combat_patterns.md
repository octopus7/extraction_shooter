# 재사용 전투 패턴

보스와 중간보스가 같은 예고·회피 규칙을 공유하도록 `UTunaSweeperCombatPatternComponent`에 세 패턴을 구성한다. 특정 보스 맵이 정해지지 않아 기존 맵에는 적을 추가 배치하지 않는다.

## 사용

- 바로 배치하는 예제는 `TunaSweeperPatternEnemyCharacter`다. 세 패턴의 자동 실행이 켜져 있으며, 적대 대상을 인지하면 미사일 터렛 → 돌진 → 부하 로봇 순서로 사용한다. 실행 조건이 맞지 않는 패턴은 건너뛸 수 있다.
- 기존 `TunaSweeperEnemyCharacter`에도 `CombatPatterns` 컴포넌트가 있다. 기본 `bAutomaticPatterns`는 꺼져 있으므로 원하는 적에서만 켠다.
- 학습용 중간보스는 `PatternSequence`에 하나만 넣고, 보스는 같은 패턴을 조합한다. 터렛과 이미 나온 부하는 독립적으로 동작하므로 다음 패턴과 겹칠 수 있다.
- 수동 제어는 컴포넌트의 `TryStartPattern(Pattern, TargetActor)`를 호출한다. 유효한 적대 대상, `ActivationRange`, 재사용 대기시간을 만족해야 하며 다른 패턴이나 로봇 전개가 진행 중이면 시작하지 않는다. 총기 적은 사격·재장전·전술 이동 중에는 시작을 거절하고, 일반 AI의 Idle/Observe 시점에 실행한다. 패턴 실행 중에는 분대 역할을 반납하고 종료 후 다시 합류한다. 반환값으로 시작 성공을 확인한다.
- 네이티브 예제 적과 부하 로봇에는 기본 근접 전투 프로필이 있다. 기존 스폰 경로의 `ConfigureCombatProfile`로 프로필과 팩션·분대 배정을 전달할 수 있고, `ConfigureSpawnData`로 체력·보상 등을 설정할 수 있다. 패턴 밖에서는 기존 적 AI가 이동과 공격을 담당한다.

## 패턴과 조정값

| 패턴 | 동작 | 주요 조정값 |
| --- | --- | --- |
| `MissileTurret` | 파괴 가능한 터렛을 소환한다. 터렛은 목표 바닥 위치를 고정하고 원형 경고를 채운 뒤 그 위치에 미사일 피해를 준다. 경고 이후 목표를 따라가지 않는다. | 컴포넌트의 `MissileTurretClass`, `MaxActiveTurrets`. 터렛 클래스의 `WarningDuration`, `ImpactRadius`, `ImpactDamage`, `SalvoInterval`, `Lifetime`, `MaxHealth`. |
| `Charge` | 바닥에 돌진 경로를 표시하고 예고가 끝나면 몸통으로 돌진한다. 장애물이나 이동 가능한 바닥의 끝에서 멈추며, 한 돌진에서 같은 대상에게 피해를 반복하지 않는다. | `ChargeTelegraphClass`, `ChargeWarningSeconds`, `ChargeDistance`, `ChargeSpeed`, `ChargeDamage`. 경고 폭은 시전자 캡슐 반지름을 따른다. |
| `RollingMinions` | 부하 로봇을 부채꼴 방향으로 시간차를 두고 쏟아낸다. 로봇은 공처럼 구른 뒤 다리를 펴고 근접 추격한다. 구르기와 전개 중에도 사격으로 처치할 수 있다. | 컴포넌트의 `RollingMinionClass`, `MinionsPerWave`, `MaxActiveMinions`, `MinionSpawnInterval`, `MinionFanAngle`. 로봇 클래스의 `RollSpeed`, `RollDurationSeconds`, `LaunchUpwardSpeed`, `UnfoldDurationSeconds`, `BallRadius`, `StandingHalfHeight`. |

공통 실행 거리는 `ActivationRange`, 패턴 뒤 회복 시간은 `RecoverySeconds`, 다음 패턴까지의 간격은 `CooldownSeconds`로 정한다. 거리·속도는 Unreal의 cm와 cm/s, 시간은 초 기준이다. 같은 팩션에는 전투 피해를 주지 않는다.

부하를 별도로 소환할 때는 `InitializeRoll(Direction, TargetActor)`를 사용한다. `FinishSpawning` 전에도 호출할 수 있다. 배치 높이는 `GetRollRadius()`를 기준으로 잡으며, 로봇은 바닥에 닿고 서 있을 공간이 확보된 뒤에만 캡슐을 확장한다. `GetDeploymentPhase()`와 `IsDeployingFromRoll()`로 전개 상태를 읽을 수 있다.

## 표현과 수명

현재 적·터렛·부하는 네이티브 기본 도형과 기존 재질을 사용하는 기능 시제품이다. 자식 Blueprint에서 메시와 재질을 교체하고, 컴포넌트의 소환 클래스에 해당 Blueprint를 지정할 수 있다. 로봇의 몸체 회전·다리 전개·보행 포즈와 캡슐 크기는 네이티브 코드가 구동하므로 교체 모델을 해당 치수와 피벗에 맞춘다.

소환 위치의 충돌을 확인하며, 막힌 위치에서 소환에 실패한 개체는 해당 웨이브에서 생략한다. 활성 개체 수 제한으로 터렛과 부하가 무한히 누적되지 않게 한다. 시전자 사망·제거 시 진행 중인 경고와 남은 소환물을 정리하고, 터렛 자체가 파괴되면 예고 중인 미사일도 취소한다. `CancelPatterns(false)`는 현재 패턴을 취소하면서 이미 나온 소환물은 유지하고, 기본 `CancelPatterns()`는 소환물까지 정리한다.

패턴 진행도, 경고, 재사용 대기시간, 소환물 목록과 부하 전개 상태는 전투 중에만 유지하는 런타임 상태다. 이번 구현은 저장 필드나 세이브 버전을 변경하지 않는다.

## 검증

- UE 5.7 `TunaSweeperEditor Win64 Development` 빌드 성공.
- 명령줄 자동화 `TunaSweeper.Combat` 15개 모두 성공: 새 패턴 회귀 테스트 10개와 기존 전투 테스트 5개. 테스트 결과의 경고·실패는 0개.
- 새 테스트는 예고 시간·고정 위치, 돌진 실제 접촉점과 단일 피해, 벽 차단, 즉시 피격 가능한 구르기, 기립 공간, 시간차 소환·상한, 사망 정리, 재장전·사격과의 전환을 검사한다.
- 검증 보고서: `TunaSweeper/Saved/Automation/CombatPatternsFinal/index.json` (로컬 실행 결과). 기본 도형 표현의 실제 맵 시각 품질과 전투 밸런스는 별도 플레이 조정 대상이다.
