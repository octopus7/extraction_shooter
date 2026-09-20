# LunaMk2 손목 트위스트

2026-09-21 손목 캔디랩 원인 조사 및 포스트 프로세스 연결.

## 조사 결과

`SKM_LunaMk2`의 양팔에는 `lowerarm → cc_base_*_forearmtwist01 → cc_base_*_forearmtwist02` 체인이 존재한다. `hand_*`는 `lowerarm_*`의 별도 자식이므로 보조 본을 돌려도 손/손가락 포즈를 바꾸지 않는다.

UE에서 내보낸 메시의 LOD0 양의 스킨 웨이트 정점 수:

| 본 | 정점 수 | 최대 웨이트 |
|---|---:|---:|
| cc_base_l_forearmtwist01 | 272 | 0.9907 |
| cc_base_l_forearmtwist02 | 219 | 0.7644 |
| cc_base_r_forearmtwist01 | 293 | 0.9906 |
| cc_base_r_forearmtwist02 | 221 | 0.6762 |

본이나 웨이트가 빠진 상태는 아니었다. 수정 전 메시의 Post Process Anim Blueprint가 `None`이었고, `MF_Rifle_Idle_Hipfire1`과 타이틀 A의 전완 보조 본 로컬 회전은 기준 포즈와 동일했다. 손 회전은 변하지만 보조 본으로 분산되지 않아 손목에서 변형이 집중됐다.

## 적용

메시의 Post Process Anim Blueprint에 `/Game/Characters/Player/LunaMk2/Animations/ABP_LunaMk2_WristPostProcess`를 지정한다. 메시에 연결되므로 인게임, 타이틀, 단일 애니메이션 미리보기에 공통 적용된다. 컴포넌트에서 post-process를 비활성화하면 적용되지 않는다.

그래프는 Input Pose → Local To Component → Bone Driven Controller 4개 → Component To Local → Output이다. 각 손의 기준 자세 대비 Rotation X를 같은 쪽 두 전완 보조 본의 로컬 Rotation X로 보낸다. 이 리그의 전완 길이 축은 X이다. `AddToRefPose`, multiplier `1/3`을 사용한다. 직렬 본의 누적 보정은 첫 본 1/3, 두 번째 본 2/3이다. 기준 회전을 보존하며 기존 트위스트 트랙과 중복 가산하지 않는다. 기존 본·웨이트·메인 AnimBP·물리 에셋은 수정하지 않는다.

UE 기본 노드만 사용한다. 향후 별도 보정이 베이크된 모션이나 X축이 다른 리그로 교체할 때는 이 설정을 재검토해야 한다. Euler 기반 드라이버이므로 ±180도 경계를 넘는 손 회전도 별도 검토 대상이다.

## 검증

`TunaSweeper.Player.WristTwist`는 실제 SkeletalMeshComponent의 post-process 활성/비활성 결과를 비교한다. 기준 포즈 보존, 소총 대기와 타이틀 A/C를 30fps 간격으로 평가한 회전량·축·방향, 손을 포함한 비대상 본의 변환 보존과 유한값을 검사한다. 수정 전에는 누락된 post-process를 검출해 실패했다.

이 자동 검증은 본 변환을 검증한다. 사용자 첨부 화면과 같은 카메라에서 피부 실루엣이 완전히 해소됐는지에 대한 시각 검증은 별도다.
