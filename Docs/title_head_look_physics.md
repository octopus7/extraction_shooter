# 타이틀 머리 추적과 양갈래 물리

타이틀의 커서 추적은 `ATunaSweeperTitlePresentationActor::UpdateCursorLook`에서 기존 각도 제한과 보간을 적용한 목표 Yaw/Pitch를 계산한다. `UTunaSweeperTitleSkeletalMeshComponent`는 이 목표를 보관하며, `FAnimNode_TunaSweeperTitleHeadLook::PreUpdate`가 게임 스레드에서 목표와 본 이름을 복사한다. 병렬 AnimGraph 평가는 복사된 값과 해당 프레임의 입력 포즈만 사용한다.

`ABP_LunaMk2`의 컴포넌트 공간 포즈 연결 순서:

```text
기존 애니메이션 → Local To Component → Title Head Look
 → 기존 Rigid Body (PA_LunaMk2_SideTail) → Component To Local → Output
```

머리 보정 노드는 입력 포즈의 head 본 +Y 방향을 목표 방향에 맞추는 차이 회전을 계산하고 neck_01의 회전에 적용한다. 목의 위치와 나머지 몸 포즈는 유지된다. 목의 자손은 보정된 포즈를 이어받으며 Rigid Body는 보정된 head 앵커를 기준으로 양갈래를 시뮬레이션한다.

머리 보정을 `FinalizeBoneTransform`에 다시 추가하지 않는다. 그 시점에는 Rigid Body 계산이 끝났으므로 머리카락까지 한꺼번에 돌리면 커서 움직임의 관성이 전달되지 않는다. 안구 보정은 기존 `UTunaSweeperGazeSkeletalMeshComponent`의 최종 보정 경로를 유지한다.

목표가 없거나 비활성화된 경우와 일반 인게임 SkeletalMeshComponent에서는 머리 보정 노드가 입력 포즈를 그대로 통과시킨다. 같은 AnimBP를 쓰는 인게임 캐릭터의 기존 물리 설정은 유지된다. 본이 없는 LOD에서는 보정을 건너뛴다.

실행용 AnimNode는 TunaSweeper 모듈에 있고, 편집용 AnimGraphNode는 TunaSweeperAnimGraph (`UncookedOnly`) 모듈에 있다. 후자는 게임에 포함할 실행 코드가 아니다. 모듈 로딩은 `Default` 단계이며 .uproject의 Modules 배열에서 TunaSweeperAnimGraph를 TunaSweeper보다 먼저 선언한다. 그래야 런타임 생성자가 ABP_LunaMk2를 로드하기 전에 편집용 노드가 등록된다. `PreDefault`로 앞당기면 런타임 모듈의 기존 생성자 에셋 로딩이 AnimationData 등 플러그인의 준비보다 먼저 실행될 수 있다.

검증 항목:

- `TunaSweeper.Gaze.TitleHairInertia`: 같은 애니메이션의 두 메시 중 하나의 머리만 돌려 양갈래 끝이 머리 기준으로 다르게 움직이는지와 머리 목표 방향을 검사한다.
- `TunaSweeper.Gaze.TitleHeadLookAnimatedPose`: 기울어진 입력 포즈의 절대 시선 방향, 목 피벗과 몸 포즈 보존, 목표 해제, 인게임 비활성화 및 최종 단계의 중복 회전 방지를 검사한다.
- 나머지 `TunaSweeper.Gaze` 테스트는 기존 안구·타이틀 연결을 확인한다.
