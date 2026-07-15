# 파괴 토마토 에셋과 수동 Geometry Collection 작업

## 목적

`BP_BreakableTomato`는 레벨에 배치해 사용하는 자율형 파괴 토마토다. 플레이어가 가까워지면 짧게 튀어 접근하고, 피해를 모두 받으면 정적 메시를 숨긴 뒤 미리 분할한 Chaos Geometry Collection과 액체 파편 Niagara를 한 번 재생한다.

파괴 메시의 **조각 분할과 충돌 데이터는 에셋 제작 시 미리 베이크**한다. 런타임에는 새 메시를 자르지 않으며, 베이크된 조각에 Chaos 외부 변형 필드와 충격량을 적용해 물리 시뮬레이션만 시작한다.

## 산출물과 역할

| 파일 또는 에셋 | 역할 |
| --- | --- |
| `/Game/Interaction/BP_BreakableTomato` | 레벨에 배치하는 최종 Blueprint. C++ 토마토 액터를 부모로 사용하며 모든 기본 참조값을 보유한다. |
| `/Game/Interaction/GC_Tomato_Fractured` | `SM_Tomato`를 10~14개의 Voronoi 조각으로 베이크한 Chaos Geometry Collection. 평상시에는 숨겨져 있다가 파괴 순간에만 물리 시뮬레이션한다. |
| `/Game/Meshes/Props/TomatoHead/Materials/M_TomatoFlesh_Interior` | `T_TomatoFlesh`를 Base Color로 사용하는 불투명 양면 과육 재질. Geometry Collection의 새 내부 면에 배정된다. |
| `/Game/Effects/NS_Tomato_StickySplatter` | 기존 범용 액체 Niagara의 방출 모듈을 사용하되 모든 Sprite Renderer를 토마토 과육 방울 머티리얼로 교체한 Niagara System. 짧은 붉은 과육 방울 분사를 담당한다. |
| `/Game/Physics/PhysicalMaterials/PM_Flesh` | 토마토의 피격 충돌체가 사용하는 Flesh 물리 재질. 기존 표면 기반 피격 효과 체계가 Flesh로 해석할 수 있게 한다. |
| `SW_Tomato_A`, `SW_Tomato_B`, `SW_Tomato_C` | `/Game/Audio/Imported`에 있는 토마토 파괴음 후보 세 개. 토마토가 파괴될 때 하나를 무작위로 재생한다. |
| `TunaSweeperBreakableTomatoComponent.h/.cpp` | 체력, 현재 체력, 파괴 완료 상태를 전담한다. 시각 효과와 이동 상태가 체력 구현에 직접 의존하지 않도록 분리한다. |
| `TunaSweeperBreakableTomatoActor.h/.cpp` | 메시/충돌체/Geometry Collection을 조립하는 액터. 컴포넌트의 파괴 완료 시 Chaos와 Niagara를 실행하고, 근접 도약 이동도 담당한다. |
| `M_TomatoGooParticle` | `T_TomatoFlesh`와 원형 알파 마스크를 쓰는 반투명 Niagara 스프라이트 머티리얼. 연기 대신 붉은 과육 방울을 그린다. |
| `M_TomatoGooSplat` | 지면에 투사되는 반투명 데칼 머티리얼. 파괴 후 바닥에 남는 끈적한 과즙 자국을 그린다. |
| `TunaSweeperBreakableAppleCrateActor.h/.cpp` | 토마토가 재사용하는 기반 파괴 액터. 외부에서 파편 방향을 넘겨 파괴할 수 있도록 `BreakCrateInDirection` 진입점을 제공한다. |
| `TunaSweeperEditorSetupCore.cpp` | 과육 재질, 물리 재질, Niagara 복제, Geometry Collection, Blueprint 기본값을 생성·저장하는 Editor one-shot 로직이다. |
| `TunaSweeperEditorSetupShared.h` | 토마토용 에셋 이름, one-shot 작업 ID, 생성 함수 선언을 보관한다. |
| `TunaSweeperEditorOneShot_ToCleanupOnExplicitRequest.cpp` | `-TunaSweeperRebuildBreakableTomato` 명령행 옵션과 최초 실행 one-shot을 연결한다. |

## 런타임 동작

1. `BP_BreakableTomato`는 평상시에 `SM_Tomato`와 단순 박스 충돌체를 사용한다. Geometry Collection은 보이지 않고 충돌도 꺼져 있다.
2. 플레이어가 850cm 안에 들어오면 1~2초 동안 135cm/s 속도로 접근한다. 이때 20cm 높이의 상하 도약을 반복한다.
3. 접근 구간 뒤에는 2~3초 휴식한다. 플레이어와 110cm 이내가 되면 더 다가가지 않는다.
4. 투사체 피해는 `UTunaSweeperBreakableTomatoComponent`가 누적한다. 마지막 피해만 파괴 전환을 통과시킨다.
5. 파괴 시 `BreakSoundVariants` 배열에서 유효한 사운드 하나를 무작위로 선택해 재생한다. 기본값은 `SW_Tomato_A/B/C`다.
6. 원본 메시와 충돌체를 끄고 `GC_Tomato_Fractured`를 표시한다. 파편에는 외부 변형과 방사 충격을 가하며, 방향 충격은 마지막 투사체 진행 방향의 **반대**로 적용한다.
7. 탄착점에서 `NS_Tomato_StickySplatter`가 `M_TomatoGooParticle` 스프라이트를 짧게 분사한다. 기존 연기 머티리얼은 토마토 과육 머티리얼로 교체했다.
8. 동시에 탄착점 주변 지면을 추적해 4~7개의 `M_TomatoGooSplat` 데칼을 생성한다. 자국은 4초 유지 후 2초에 걸쳐 사라져 끈적한 과즙처럼 보인다.
9. 공중에 잔류할 수 있는 임시 물리 DropletActor는 제거했으며, Chaos 파편과 파괴 토마토 액터만 6초 후 정리한다.

## 사람이 직접 만드는 절차 (UE 5.7)

자동 생성 대신 아티스트가 Geometry Collection을 수정하거나 새로 만들 때는 다음 절차를 따른다.

### 1. 원본과 재질 준비

1. `SM_Tomato`를 복제하지 말고 원본의 스케일, 피벗, UV가 정상인지 확인한다.
2. 외피에는 기존 토마토 재질을 유지한다.
3. `M_TomatoFlesh_Interior`를 준비한다. 이 재질은 `T_TomatoFlesh`를 Base Color로 사용하고 Roughness는 약 `0.38`, Specular는 약 `0.52`로 시작한다.
4. 내부 면 전용 재질은 원본 Static Mesh의 외피 슬롯을 대체하지 않는다. Geometry Collection 재질 목록에 **추가 슬롯**으로 넣는다.

### 2. Geometry Collection 생성

1. Content Browser에서 `SM_Tomato`를 선택한다.
2. Fracture Mode로 전환한 뒤 **Create Geometry Collection**을 실행한다. 메뉴 위치는 Editor 레이아웃에 따라 Content Browser의 Fracture 메뉴 또는 Fracture 패널의 Create 버튼이다.
3. 생성 경로와 이름을 `/Game/Interaction/GC_Tomato_Fractured`로 지정한다.
4. 생성된 Collection을 열고 Materials 목록에서 `M_TomatoFlesh_Interior`를 추가한다. 외피 재질 다음 슬롯에 두는 것이 관리하기 쉽다.

### 3. 내부 과육 면과 조각 분할

1. Fracture 패널에서 루트 본을 선택한다.
2. **Uniform Fracture**를 선택한다.
3. Voronoi Site Count를 `10~14`로 설정한다. 토마토는 작으므로 조각을 지나치게 많이 만들지 않는다.
4. Random Seed는 재현 가능한 값으로 고정한다. 현재 자동 생성값은 `7162026`이다.
5. Fracture Settings의 **Internal Material ID**를 과육 재질 슬롯으로 지정한다. 이 설정이 빠지면 새 절단면도 외피 재질로 보인다.
6. Grout와 Fracture Noise는 `0`으로 시작한다. 필요할 때만 약한 노이즈를 더한다. 토마토는 돌처럼 거친 절단면보다 매끈한 과육 단면이 자연스럽다.
7. Fracture를 실행한 뒤 내부 면이 붉은 과육 재질인지, 외피가 그대로 유지되는지 확인한다.

### 4. Chaos 물리 설정

1. 클러스터링은 끈다. 이 토마토는 파괴 순간에 이미 개별 조각을 모두 해제하는 연출이다.
2. Damage Model은 User Defined Damage Threshold를 사용하고 Threshold는 `0`으로 둔다. 실제 파괴 판단은 `BreakableTomatoComponent`의 체력이 한다.
3. Mass는 약 `1.8`, Minimum Mass Clamp는 `0.02`에서 시작한다.
4. Size Specific Data의 첫 Collision Shape에 다음을 설정한다.
   - Collision Type: `Volumetric`
   - Implicit Type: `Convex`
   - Collision Sample Spacing: 약 `6cm`
5. Collection을 저장한다. 파괴 조각 수, 내부 재질, 충돌 형상 변경은 반드시 다시 저장해야 런타임에 반영된다.

### 5. Blueprint 연결과 확인

1. `BP_BreakableTomato`를 열어 `Crate Geometry Collection`이 `GC_Tomato_Fractured`를 참조하는지 확인한다.
2. `Sticky Splatter System`이 `NS_Tomato_StickySplatter`, `Tomato Physical Material`이 `PM_Flesh`를 참조하는지 확인한다.
3. Blueprint를 레벨에 배치하고, 플레이어가 850cm 안으로 들어왔을 때 도약-휴식 주기가 보이는지 확인한다.
4. 총알로 한 번 피격해 Chaos 조각, 붉은 내부 단면, 액체 Niagara가 한 번만 발생하는지 확인한다.

## 튜닝 기준

| 항목 | 현재 시작값 | 조정 방향 |
| --- | ---: | --- |
| 조각 수 | 10~14 | 성능이 부족하면 줄이고, 내부 단면이 너무 단순하면 조금 늘린다. |
| 접근 반경 | 850cm | 플레이어가 토마토를 발견하는 거리와 카메라 시야에 맞춘다. |
| 이동 구간 | 1~2초 | 짧을수록 불규칙하고 생물처럼 보인다. |
| 휴식 구간 | 2~3초 | 길수록 계속 달려들지 않는 인상이 강해진다. |
| 접근 속도 | 135cm/s | 너무 빠르면 적처럼 보이므로 낮은 값을 유지한다. |
| 도약 높이 | 20cm | 충돌체는 안정적으로 유지하고 시각 메시의 도약감만 강화한다. |
| Chaos 수명 | 6초 | 파편이 화면에 너무 오래 남으면 줄인다. |
| 파괴음 배열 | `SW_Tomato_A/B/C` | `BP_BreakableTomato`의 `Breakable > Audio > Break Sound Variants`에서 원하는 수만큼 추가·제거할 수 있다. |
| 바닥 과즙 자국 수 | 4~7 | 자국이 너무 많으면 줄이고, 파괴감이 부족하면 늘린다. |
| 과즙 반지름 | 5~13cm | 큰 덩어리를 늘리면 끈적한 느낌이 강해지지만 화면이 지저분해질 수 있다. |
| 방향 파괴 충격 | 1800 | 피격 반대 방향으로 적용한다. `BP_BreakableTomato`의 `Tomato Impact Directional Impulse`에서 조절한다. |

## 주의 사항

- `GC_Tomato_Fractured`를 다시 만들 때는 Internal Material ID가 과육 슬롯을 가리키는지 항상 재확인한다.
- `NS_Tomato_StickySplatter`는 모든 Sprite Renderer가 `M_TomatoGooParticle`을 사용하도록 교체됐다. 내부 Emitter 이름이 `Dust`로 남아 있어도 화면에는 연기가 아니라 과육 방울이 출력된다.
- `TunaSweeperTomatoGooDropletActor`는 초기 임시 물리 구체 방식이었으며 공중에 남을 수 있어 제거했다. 끈적한 착지는 지면 데칼 방식으로 처리한다.
- 토마토가 경사진 지형에서 이동해야 한다면 현재의 단순 상하 도약을 지면 추적 기반 이동으로 교체해야 한다.
- `BP_BreakableTomato`는 특정 레벨에 자동 배치되지 않는다. 의도한 레벨에서 직접 배치해 사용한다.
