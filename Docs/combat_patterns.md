# 재사용 전투 패턴

보스와 중간보스가 같은 예고·회피 규칙을 공유하도록 `UTunaSweeperCombatPatternComponent`에 세 패턴을 구성한다. 특정 보스 맵이 정해지지 않아 기존 맵에는 적을 추가 배치하지 않는다.

## 사용

- 바로 배치하는 예제는 `TunaSweeperPatternEnemyCharacter`다. 세 패턴의 자동 실행이 켜져 있으며, 적대 대상을 인지하면 미사일 터렛 → 돌진 → 부하 로봇 순서로 사용한다. 실행 조건이 맞지 않는 패턴은 건너뛸 수 있다.
- 기존 `TunaSweeperEnemyCharacter`에도 `CombatPatterns` 컴포넌트가 있다. 기본 `bAutomaticPatterns`는 꺼져 있으므로 원하는 적에서만 켠다.
- 학습용 중간보스는 아래의 돌진형·로봇 소환형 두 네이티브 캐릭터를 사용한다. 미사일은 메인보스 고유 기술로 남긴다. `EnabledPatterns`는 수동·자동 호출 모두의 허용 목록이고, `PatternSequence`는 그중 자동 실행 순서다. 기존 조합 보스의 터렛과 부하는 독립적으로 동작하므로 다음 패턴과 겹칠 수 있다.
- 수동 제어는 컴포넌트의 `TryStartPattern(Pattern, TargetActor)`를 호출한다. 유효한 적대 대상, `ActivationRange`, 재사용 대기시간을 만족해야 하며 다른 패턴이나 로봇 전개가 진행 중이면 시작하지 않는다. 총기 적은 사격·재장전·전술 이동 중에는 시작을 거절하고, 일반 AI의 Idle/Observe 시점에 실행한다. 패턴 실행 중에는 분대 역할을 반납하고 종료 후 다시 합류한다. 반환값으로 시작 성공을 확인한다.
- 네이티브 예제 적과 부하 로봇에는 기본 근접 전투 프로필이 있다. 기존 스폰 경로의 `ConfigureCombatProfile`로 프로필과 팩션·분대 배정을 전달할 수 있고, `ConfigureSpawnData`로 체력·보상 등을 설정할 수 있다. 패턴 밖에서는 기존 적 AI가 이동과 공격을 담당한다.

## 사전 학습용 중간보스

에디터 Place Actors 또는 C++ Classes에서 각각 독립적으로 배치한다. 두 클래스 모두 Blueprint 파생·세부 수치 조정이 가능하다. 배치할 전투 맵은 지정되지 않았으므로 기존 맵에는 추가하지 않는다.

| 캐릭터 | 역할 | 학습용 기본값 |
| --- | --- | --- |
| `TunaSweeperChargeTeachingMiniboss` / Teaching Miniboss - Charge | 돌진 몸통 공격 전용. 앞쪽 충각이 있는 무한궤도 로봇. | 체력 260, 3초 고정 경로 예고 → 850cm/s로 최대 8m 돌진, 피해 12. 후딜 2.2초 후 다음 패턴까지 7초 대기. |
| `TunaSweeperRobotTeachingMiniboss` / Teaching Miniboss - Robot Carrier | 부하 로봇 소환 전용. 등에 구체 포드를 실은 고정형 운반 로봇. | 체력 240, 청록색 2.5초 준비 표시 → 0.85초 간격으로 3기 소환. 후딜 2.5초와 재사용 대기 9초를 거치며 기존 부하가 모두 처치되어야 다음 웨이브 허용. |
| `TunaSweeperTeachingRollingRobotMinion` | 소환형이 사용하는 학습 전용 부하. | 체력 14, 구르기 350cm/s·3초, 기립 1.2초. 보행 180cm/s, 근접 피해 4, 공격 간격 1.8초. 소환된 순간부터 피격 가능. 벽에 막히면 기존 규칙대로 일찍 기립을 시작할 수 있다. |

두 중간보스는 각자의 패턴만 허용한다. 미사일과 반대쪽 중간보스 패턴은 수동 호출도 거절한다. `bPatternAttacksOnly`가 켜져 있어 패턴 사이에 예고 없는 일반 근접·총기 공격을 섞지 않는다. 돌진형은 패턴 거리 밖에서만 천천히 접근하고, 소환형은 제자리에서 부하 처리에 집중할 시간을 준다. 예고 도중 대상 상실·취소·사망 시 준비 효과를 정리한다.

메인보스용 `TunaSweeperPatternEnemyCharacter`와 기본 부하의 타이밍은 유지한다. 기본 돌진 예고는 1.5초·1500cm/s, 기본 소환은 즉시 준비 후 0.18초 간격 5기, 기본 부하는 800cm/s·1.4초 구르기와 0.45초 기립이다. 학습 캐릭터는 같은 회피·피격 규칙을 더 여유로운 별도 설정으로 사용한다.

재사용 설정 `MinionWarningSeconds`는 소환 준비 시간(기본 0초), `bWaitForMinionsDefeated`는 기존 부하 처치 대기(기본 false)다. 새 실행 제한과 예고·웨이브 상태는 런타임 전투 설정이며 저장 필드나 세이브 버전 변경은 없다.

## 패턴과 조정값

| 패턴 | 동작 | 주요 조정값 |
| --- | --- | --- |
| `MissileTurret` | 파괴 가능한 터렛을 소환한다. 터렛은 목표 바닥 위치를 고정하고 원형 경고를 채운 뒤 그 위치에 미사일 피해를 준다. 경고 이후 목표를 따라가지 않는다. | 컴포넌트의 `MissileTurretClass`, `MaxActiveTurrets`. 터렛 클래스의 `WarningDuration`, `ImpactRadius`, `ImpactDamage`, `SalvoInterval`, `Lifetime`, `MaxHealth`. |
| `Charge` | 바닥에 돌진 경로를 표시하고 예고가 끝나면 몸통으로 돌진한다. 장애물이나 이동 가능한 바닥의 끝에서 멈추며, 한 돌진에서 같은 대상에게 피해를 반복하지 않는다. | `ChargeTelegraphClass`, `ChargeWarningSeconds`, `ChargeDistance`, `ChargeSpeed`, `ChargeDamage`. 경고 폭은 시전자 캡슐 반지름을 따른다. |
| `RollingMinions` | 부하 로봇을 부채꼴 방향으로 시간차를 두고 쏟아낸다. 로봇은 공처럼 구른 뒤 다리를 펴고 근접 추격한다. 구르기와 전개 중에도 사격으로 처치할 수 있다. | 컴포넌트의 `RollingMinionClass`, `MinionsPerWave`, `MaxActiveMinions`, `MinionSpawnInterval`, `MinionFanAngle`. 로봇 클래스의 `RollSpeed`, `RollDurationSeconds`, `LaunchUpwardSpeed`, `UnfoldDurationSeconds`, `BallRadius`, `StandingHalfHeight`. |

공통 실행 거리는 `ActivationRange`, 패턴 뒤 회복 시간은 `RecoverySeconds`, 다음 패턴까지의 간격은 `CooldownSeconds`로 정한다. 거리·속도는 Unreal의 cm와 cm/s, 시간은 초 기준이다. 같은 팩션에는 전투 피해를 주지 않는다.

부하를 별도로 소환할 때는 `InitializeRoll(Direction, TargetActor)`를 사용한다. `FinishSpawning` 전에도 호출할 수 있다. 배치 높이는 `GetRollRadius()`를 기준으로 잡으며, 로봇은 바닥에 닿고 서 있을 공간이 확보된 뒤에만 캡슐을 확장한다. `GetDeploymentPhase()`와 `IsDeployingFromRoll()`로 전개 상태를 읽을 수 있다.

## 표현과 수명

적·터렛·부하는 `/Game/Characters/CombatPatterns/Meshes`의 전용 모델을 기본으로 사용한다. 터렛은 지지대·헤드·발사관, 부하는 장갑 구체·눈·다리·발로 분리되어 있다. 예제 적은 앞쪽 충각과 무한궤도가 있는 중형 로봇이며, 구체 부하의 몸체 회전·다리 전개·보행 포즈는 기존 네이티브 코드가 구동한다. 모델의 정면은 +X, 단위는 cm다. Blender 원본과 FBX, 반복 실행 가능한 모델링 스크립트는 `Art/CombatPatterns`와 `Tools/CombatPatternArt`에 있다. 자식 Blueprint에서 메시·재질·소환 클래스를 교체하여 다른 적에게 재사용할 수 있다.

원형 경고는 정확한 피해 반경의 연속 외곽선, 분절 링과 시간에 비례해 면적이 차오르는 반투명 채움으로 표시한다. 돌진 경고는 캡슐 폭의 경계와 이동 방향 화살표를 표시한다. 지형 높이를 따르는 경고와 실제 피해 시간·범위는 동일한 기존 로직을 사용한다.

`TunaSweeperCombatPatternEffectActor`는 소환, 발사, 미사일 배기, 충돌, 돌진 먼지, 로봇 기립·파괴의 일곱 표현을 제공한다. Blueprint 또는 C++의 `Spawn`에 종류·위치·시각 반경·방향을 전달한다. 효과는 발광 불꽃·충격 링과 반투명 연기 메시로 구성되며 충돌·피해가 없고 0.24~0.95초 후 정리된다. 효과는 시야 시스템을 따르고 전용 서버에는 생성하지 않는다. 재질 8개는 장갑·기계부·발광부와 경고·불꽃·연기를 공용으로 사용한다.

소환 위치의 충돌을 확인하며, 막힌 위치에서 소환에 실패한 개체는 해당 웨이브에서 생략한다. 활성 개체 수 제한으로 터렛과 부하가 무한히 누적되지 않게 한다. 시전자 사망·제거 시 진행 중인 경고와 남은 소환물을 정리하고, 터렛 자체가 파괴되면 예고 중인 미사일도 취소한다. `CancelPatterns(false)`는 현재 패턴을 취소하면서 이미 나온 소환물은 유지하고, 기본 `CancelPatterns()`는 소환물까지 정리한다.

패턴 진행도, 경고, 재사용 대기시간, 소환물 목록과 부하 전개 상태는 전투 중에만 유지하는 런타임 상태다. 이번 구현은 저장 필드나 세이브 버전을 변경하지 않는다.

## 검증

- UE 5.7 `TunaSweeperEditor Win64 Development` 빌드 성공.
- 공통 변경 후 기존 `TunaSweeper.Combat` 회귀 29개와 새 `TunaSweeper.Combat.TeachingMinibosses` 5개, 총 34개 성공. 경고·실패는 0개.
- 새 테스트는 예고 시간·고정 위치, 돌진 실제 접촉점과 단일 피해, 벽 차단, 즉시 피격 가능한 구르기, 기립 공간, 시간차 소환·상한, 사망 정리, 재장전·사격과의 전환을 검사한다.
- 검증 보고서: `TunaSweeper/Saved/Automation/TeachingMinibossRegression/index.json`, `TunaSweeper/Saved/Automation/TeachingMinibosses/index.json` (로컬 실행 결과). 전투 밸런스는 실제 플레이에서 조정할 수 있다.

- 아트 검증은 네이티브 액터의 메시 9개 연결, 효과 7종의 유한 기하·수명·충돌 없음, 진행도별 원형·돌진 경고 범위를 검사한다. 저장된 에셋의 독립 읽기 검증은 크기·피벗·UV·재질 8개·충돌 없음과 패키지 무변경을 확인한다. 재검증 스크립트는 `Tools/CombatPatternArt/verify_unreal.py`, 검증 결과는 `Art/CombatPatterns/Validation`에 있다.

- 학습 중간보스 검증은 자동·수동 패턴 격리와 메인보스 미사일 유지, 3초 돌진 예고·고정 경로·후딜, 2.5초 소환 예고·0.85초 배출 간격·전원 처치 대기, 구르기 중 피격과 3초/1.2초 전개, 예고 중 사망 정리를 검사한다.
