# MoleDummy 리그

`SKM_MoleDummy.blend`는 Blender 4.5.12 LTS에서 제작/검증했다. 스켈레탈 메시 오브젝트 이름도 `SKM_MoleDummy`다. 리깅 전 원본 `SM_MoleDummy.blend`는 보존한다.

- 리그: `Armature`, 편집용 19개 뼈. 발 IK 컨트롤 `CTRL_foot.L/R`. FBX는 IK 평가 결과를 베이크하고 비변형 발 컨트롤을 제외한다.
- 배 앞면 중심은 다리 영향을 제거하고 하복부의 pelvis에서 윗배의 spine으로 부드럽게 연결했다. 가장자리와 다리 접합부는 기존 가중치로 점진적으로 전환해 걷기 중 배 중앙이 갈라져 접히는 현상을 보정했다.
- 30 fps, Blender 전방 -Y, 위쪽 +Z. 원본 크기 약 1.04 m 유지.
- Action Editor에서 리그의 액션을 선택한다. 모든 액션은 Fake User와 Asset 표시로 보존된다.

| 액션 | 프레임 | 동작 |
|---|---|---|
| Idle_Breathe | 1–120 반복 | 4초 주기의 미세한 호흡. 121은 이음 프레임. 런타임 기본 동작. |
| Turn_InPlace | 1–32 반복 | 루트 이동·회전 없이 발만 번갈아 드는 회전용 제자리 걸음. 33은 이음 프레임. |
| Walk_InPlace | 1–32 반복 | 보관용 제자리 걷기. 런타임에서는 사용하지 않는다. 33은 이음 프레임. |
| Walk_Forward | 1–33 | 한 주기 동안 root가 전방으로 0.32 m 이동. |
| Turn_Left_90 | 1–65 | 발을 번갈아 옮기며 왼쪽 90도 회전 후 정리. |
| Turn_Right_90 | 1–65 | 발을 번갈아 옮기며 오른쪽 90도 회전 후 정리. |

파일을 열면 Idle_Breathe가 선택되어 있으며 타임라인은 1–120이다. 다른 액션은 위 프레임 범위로 바꿔 재생한다. 원본에서 존재하던 발목 주변 표면 불연속은 모델 수정 범위에 포함하지 않았다.

Unreal FBX는 `TunaSweeper/SourceArt/Characters/Mole/`에 있다. Unreal 에셋은 `/Game/Characters/NPC/Mole/`에 있으며 기존 `M_Mole` 재질을 사용한다.

`/Game/Characters/Mole/BP_Mole` 및 `BunkerMap` 배치 액터는 `SKM_MoleDummy`를 사용한다. 기존 `DummyMesh` 스태틱 컴포넌트는 제거했다. `BS_Mole_IdleTurn`은 음수 입력에 `A_Mole_Turn_Left_InPlace`, 0에 `A_Mole_Idle_Breathe`, 양수 입력에 `A_Mole_Turn_Right_InPlace`를 연결한다. 방향별 제자리 클립은 원본 90도 회전 클립의 root 트랙을 아이들 첫 프레임의 위치·회전·스케일로 고정한 파생 에셋이다. 원본 FBX와 Blender 액션은 보존한다.

액터의 실제 yaw 변화 속도와 부호로 플레이어 추적 및 원래 방향 복귀의 발동작을 선택한다. 회전이 멈추면 기존 호흡 idle로 복귀한다. 액터 회전은 기본 초당 90도로 제한하고 클립 재생 속도를 실제 회전 속도에 맞추며, 메시 틱은 액터의 추적 회전 계산 뒤에 실행한다. 애니메이션은 액터 위치나 yaw를 추가로 변경하지 않는다.

방향별 클립은 아이들과 동일한 `Force Root Lock` 및 기준 포즈 루트 잠금을 사용한다. 골격의 root 스케일 100과 원본 애니메이션의 root 스케일 1 사이 단위 차이를 유지해야 하므로, raw root 트랙만 고정하고 루트 잠금을 끄면 회전 중 메시가 100분의 1로 축소된다.

`A_Mole_Walk_InPlace`, `A_Mole_Walk_Forward`, 기존 공통 `A_Mole_Turn_InPlace` 및 원본 90도 회전 클립은 에셋으로 보관하며 런타임 블렌드에 직접 연결하지 않는다.

`ABP_MoleCompanion`은 기존 `BS_Mole_IdleTurn` 포즈를 캐시하고, `MoleUpperBody` 슬롯을 `spine`부터 Layered Blend per Bone으로 합성한다. 루트·골반·다리는 기존 호흡 및 방향별 회전 포즈를 유지한다. `UTunaSweeperMoleAnimInstance`는 상체 모션을 1회 재생하고 블렌드아웃이 끝난 뒤 다음 휴식 시간을 무작위로 정한다. 슬롯 블렌드인은 0.3초, 블렌드아웃은 0.4초다.

UE의 기본 `A_Mole_Idle_Breathe`는 4초 호흡에 맞춰 양쪽 위팔 회전 폭을 약 3도, 팔꿈치를 약 1도로 보강했다. 루프 양끝은 같은 포즈이며 속도도 0으로 이어진다. 팔 4개 본의 회전 키만 수정했으며 몸통·머리·하체 및 본 위치·스케일은 그대로다. 이 보강은 UE 에셋에 적용했고 Blender/FBX 원본에는 아직 반영하지 않았다.

상체 모션은 UE 애니메이션 에셋으로 제작한 `A_Mole_Idle_Sniff`(주변 킁킁거리기), `A_Mole_Idle_ShoulderRoll`(어깨·앞발 풀기), `A_Mole_Idle_HeadTilt`(고개 갸웃하기), `A_Mole_Idle_PawWave`(한쪽 앞발 흔들기) 4종이다. 각각 기존 호흡 클립을 기준으로 상체 회전 키만 추가했으며 Blender 원본 액션은 변경하지 않았다. `ABP_MoleCompanion` 클래스 기본값의 `Idle Variations` 배열에서 모션 구성을 편집할 수 있다.

`BP_Mole`의 `Mole Companion > Idle Variations`에서 `Enable Idle Variations`, `Idle Variation Min Delay`, `Idle Variation Max Delay`를 조절한다. 기본 휴식 범위는 **4~15초**이고 첫 재생 전에도 같은 범위를 사용한다. 최대값이 최소값보다 작으면 최소값으로 보정한다. 휴식과 현재 모션은 저장 대상이 아닌 일시적 연출 상태다.

검증: 저장 후 Blender 4.5에서 재열기, 모든 버텍스 가중치 합계, 전체 액션 프레임의 유한 좌표, 걷기 이음 포즈, 루트 전진 거리 및 좌우 회전각 확인. 주요 포즈 렌더 및 걷기/왼쪽 회전 미리보기 영상 생성.
