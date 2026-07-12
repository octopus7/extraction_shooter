# 적 AI 인지·전투 상태 전이

기준 구현: `ATunaSweeperEnemyAIController`와 `ATunaSweeperEnemyCharacter`.
거리는 Unreal 단위인 cm이다.

```mermaid
stateDiagram-v2
    [*] --> Idle: Possess / 전투·인지 해제

    state "비전투" as NonCombat {
        Idle: Idle\n1.4~3.7초 정지
        Wander: Wander\n0.9~2.4초, 120cm/s
        Idle --> Wander: 대기 타이머 만료
        Wander --> Idle: 배회 타이머 만료
    }

    state "의심 수색" as Suspicious {
        Search: Suspicious Search\n소음 또는 피격 방향을 향해 회전·좌우 수색
    }

    state "발견 경고" as Alerted {
        Alert: Alerted\n정지·대상 주시·머리 위 ! 표시\n0.85초 반응 + 0.55초 대기
    }

    Idle --> Search: 청각 감지 / 피격
    Wander --> Search: 청각 감지 / 피격
    Search --> Alert: 플레이어를 시야로 확인
    Search --> Idle: 2.6~3.8초 수색해도 미발견

    Idle --> Alert: 플레이어를 시야로 확인
    Wander --> Alert: 플레이어를 시야로 확인
    Alert --> Melee: 경고·대기 완료 (근접 적)
    Alert --> Ranged: 경고·대기 완료 (원거리 적)

    state "근접 전투" as Melee {
        MeleeAdvance: Advance\n거리 > 130cm이면 접근
        MeleeAttack: Attack\n거리 ≤ 150cm, 재사용 대기 완료
        MeleeAdvance --> MeleeAttack: 거리 ≤ 95cm이면 이동 정지
        MeleeAttack --> MeleeAdvance: 거리 > 130cm
        MeleeAttack --> MeleeAttack: 1.25초마다 근접 공격
    }

    state "원거리 전투" as Ranged {
        HoldFire: Hold Fire\n정지, 사격 시도
        AdvanceBurst: Advance Burst\n전진 + 약한 좌우 이동
        SeekLine: Seek Line of Fire\n좌우 이동으로 사선 탐색
        KeepDistance: Keep Distance\n후퇴 + 좌우 이동
        HoldFire --> AdvanceBurst: 거리 > 1000cm
        AdvanceBurst --> HoldFire: 타이머 만료 / 목표 도달 / 거리 ≤ 1000cm
        HoldFire --> SeekLine: 비파괴물에 사선 차단
        AdvanceBurst --> SeekLine: 비파괴물에 사선 차단
        SeekLine --> HoldFire: 사선 확보 또는 탐색 만료·거리 ≤ 1000cm
        SeekLine --> AdvanceBurst: 탐색 만료·거리 > 1000cm
        HoldFire --> KeepDistance: 거리 ≤ 430cm
        AdvanceBurst --> KeepDistance: 거리 ≤ 430cm
        SeekLine --> KeepDistance: 거리 ≤ 430cm
        KeepDistance --> HoldFire: 거리 ≥ 650cm 또는 후퇴 타이머 만료
        HoldFire --> HoldFire: 사선 허용·사거리 내·재사용 대기 완료 시 사격
    }

    Melee --> Idle: 플레이어 사망 / 대상 없음 / 거리 > 3600cm
    Ranged --> Idle: 플레이어 사망 / 대상 없음 / 거리 > 3600cm
```

## 인지 규칙

- 소음은 `UTunaSweeperNoiseSubsystem` 이벤트를 구독해, 기본 청각 범위 `1800cm`와 최소 강도 `0.08`을 만족하면 `Suspicious Search`를 시작한다. 자신의 소음은 무시한다.
- 시야 확인은 플레이어 생존, 평면 거리 `≤2300cm`, 전방 시야각 `100°` 이내, 비파괴 차단물이 없는 사선을 모두 만족해야 한다.
- 적이 죽지 않고 피해를 받으면 공격자 방향을 의심 위치로 사용한다. 이 경우에도 즉시 전투로 넘어가지 않고 수색으로 시작한다.
- 직접 시야로 플레이어를 찾은 경우에도 즉시 공격하지 않는다. `Alerted` 상태에서 이동을 멈추고 플레이어를 바라보며 느낌표를 표시한 뒤, 총 `1.4초`가 지나야 실제 전투로 진입한다.
- `SM_Enemy_AlertExclamation` 메시를 적의 머리 위에 붙이고, 표시 중에는 로컬 플레이어를 향하는 빌보드로 회전한다.

## 원거리 거리별 동작

| 거리 | 상태 선택 | 기본 지속 시간 / 이동 |
| --- | --- | --- |
| `≤430cm` | `Keep Distance` | 0.5~0.9초 동안 후퇴·좌우 이동 |
| `650~1000cm` | `Hold Fire` | 2.8~4.4초 정지·사격 시도 |
| `>1000cm` | `Advance Burst` | 250~400cm(중거리) 또는 400~600cm(장거리) 전진 |
| 비파괴물에 사선 차단 | `Seek Line of Fire` | 0.8~1.4초 좌우 이동 |

## 구현 위치

- 인지·전투 상태: `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyAIController.cpp`
- 인지 기본값: `TunaSweeper/Source/TunaSweeper/Public/AI/TunaSweeperEnemyAIController.h`
- 피격 의심과 느낌표 표시: `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp`
- 느낌표 메시: `TunaSweeper/Content/Characters/Enemy/SM_Enemy_AlertExclamation.uasset`
