# 2관절 4족 로봇 IK 프리셋

현재 구현은 보행 타깃 생성과 스켈레톤 바인딩을 분리한다.

- `UQuadrupedComponent`: 네 발의 월드 위치, 대각선 보행 그룹, 지면 트레이스, 스텝 궤적을 계산한다.
- `Quadruped Robot IK`: AnimGraph에서 네 다리를 한 번에 푸는 재사용 가능한 Two Bone IK 노드다.
- `UQuadrupedRigProfile`: 특정 스켈레톤의 Upper, Lower, Foot 본 이름과 무릎 굽힘 설정을 보관한다.

## 바로 시험하기

1. `/Game/Blueprints/BP_QuadrupedDog`를 레벨에 배치한다.
2. 캐릭터의 `QuadrupedComponent`에서 `bDrawDebug`를 켠다.
3. PIE를 시작하고 캐릭터를 이동시킨다.
4. 초록 구는 고정된 발, 노란 구는 이동 중인 발 타깃이다.

총기 사용 적으로 시험하려면 `/Game/Blueprints/BP_QuadrupedGunEnemy`를 배치한다. 이 Blueprint는 기존 `ATunaSweeperEnemyCharacter`의 AI, 총기, 팩션, 피해, 드롭 기능을 유지하면서 RobotDog 스켈레탈 표현과 `UQuadrupedComponent`를 사용한다.

현재 연결된 자산은 다음과 같다.

- AnimBP: `/Game/Characters/Robot/ABP_RobotDog`
- 리그 프로필: `/Game/Characters/Robot/QRP_RobotDog_2Joint`
- 변경 전 백업: `/Game/Characters/Robot/ABP_RobotDog_LegacyTwoBone`

`ABP_RobotDog`의 AnimGraph에는 `Local To Component -> Quadruped Robot IK -> Component To Local` 흐름이 들어 있다. 노드의 `Rig Profile`은 `QRP_RobotDog_2Joint`로 설정되어 있다.

## 주요 튜닝값

`QuadrupedComponent`에서 먼저 다음 값을 조절한다. 거리 단위는 프로젝트 규칙대로 cm다.

- `Legs[].DefaultOffset`: 몸 중심 기준 기본 발 위치다. 순서는 Front Left, Front Right, Back Left, Back Right다.
- `StepThreshold`: 현재 발과 이상적인 발 위치가 이 거리보다 멀어지면 스텝을 시작한다.
- `StepHeight`: 이동 중 발이 올라가는 높이다.
- `StepDuration`: 한 스텝의 시간이다.
- `GroundCheckStartOffset`, `GroundCheckDistance`: 발 위치 위에서 아래로 수행하는 지면 트레이스 범위다.
- `LookAheadSeconds`: 현재 수평 속도와 관측된 yaw 속도로 몸체 위치를 미리 예측하는 시간이다.
- `MaxStepDistance`: 한 번에 선택할 수 있는 착지점의 최대 수평 이동 거리다.
- `GroundProbeRadius`, `MinGroundNormalZ`, `MaxLegReach`: 착지 후보의 접지 면적, 경사, 다리 도달 가능성을 제한한다.
- `bMoveGaitGroupTogether`: 켜면 대각선 다리를 함께 움직이고, 끄면 총기 적에 적합한 한 발씩의 안정 보행을 사용한다.

`GetFootPosition()`은 현재 IK 발 위치를, `GetNextFootPosition()`은 계속 평가되는 다음 착지 후보를 반환한다. 발을 드는 순간 후보 위치와 지지 컴포넌트 상대 좌표를 래치하므로 스윙 중 목표가 불안정하게 바뀌지 않는다. 움직이는 지지 컴포넌트에 심은 발은 해당 컴포넌트의 상대 좌표를 따라간다.

프로젝트 좌표 규칙에 따라 `+X`는 전방, `+Y`는 오른쪽, `+Z`는 위쪽이다. 따라서 왼발 오프셋의 Y는 음수이고 오른발은 양수다.

## 다른 리그에 재사용하기

솔버와 AnimGraph 노드는 스켈레톤에 종속되지 않지만 리그 프로필의 본 바인딩은 스켈레톤 종속이다.

1. Content Browser에서 `QuadrupedRigProfile` Data Asset을 만든다.
2. `TargetSkeleton`과 선택적인 `PreviewMesh`를 지정한다.
3. 네 슬롯 각각에 직접 연결된 `Upper -> Lower -> Foot` 본을 지정한다.
4. 해당 스켈레톤의 AnimBP에 `Quadruped Robot IK` 노드를 추가하고 새 프로필을 선택한다.
5. AnimBP를 사용하는 액터에 `UQuadrupedComponent`가 있어야 한다.

같은 Skeleton을 공유하는 여러 메시에는 하나의 프로필을 재사용할 수 있다. Skeleton 자산이 다르면 본 이름이 같아도 별도 프로필을 만드는 것이 현재 정책이다. 잘못된 Skeleton, 누락된 본, 직접 연결되지 않은 2관절 체인은 AnimBP 컴파일 때 오류로 표시된다.

## 현재 범위

이 버전은 네 다리의 분석적 Two Bone IK, 이동·yaw 예측 착지점, 구형 지면 스윕, 경사·도달 범위 판정, 이동 지지 컴포넌트 추적을 적용한다. 지면 법선은 발 상태에 저장하지만 현재 AnimGraph 노드는 위치만 소비하므로 발바닥 회전 정렬은 아직 적용하지 않는다. 몸통 높이·기울기 보정이나 Full Body IK가 필요해질 때 현재 발 타깃 생성기는 유지하고 별도 몸통/고품질 솔브 계층을 추가한다.
