# Tuna Guided Rocket

UE 5.7용 독립 C++ 런타임 플러그인. 소형 로켓이 제한된 각속도로 표적을 따라가다 유도가 끝나면 직진하고, 수명 만료 또는 충돌 시 폭발한다.

## 바로 확인하기

콘텐츠 브라우저에서 **플러그인 콘텐츠 표시**를 켜고 `/TunaGuidedRocket/Examples/L_GuidedRocketRange`를 열어 Play한다. 초록색 표적이 초당 4m로 옆으로 이동하는 동안 5발을 발사한다. 별도 경로의 한 발은 표적 없이 직진하다 시한폭발한다. 4초마다 반복한다. 샘플 피해량은 0이다.

- `BP_TestGuidedRocket`: 로켓 BP. Configuration에 `DA_TestRocket` 지정.
- `DA_TestRocket`: 비행·유도·신관·피해·연출 시간/크기 설정.
- `SM_TestRocket`: +X 전방, 길이 약 45cm, 72개 삼각형의 테스트 메시.
- `BP_TestRocketEffect`: 단순 메시 입자 연출. 짧은 연기와 섬광·방사형 입자를 생성.
- `SM_RocketExhaust`, `SM_RocketParticle`, `M_Rocket*`: 추진 불꽃과 이펙트용 메시·재질.

## BP에서 발사

`Spawn Actor from Class`에 로켓 BP 클래스를 지정한다. Spawn Transform의 +X가 초기 발사 방향이다. `Target Actor`에 추적할 액터, `Owner`와 `Instigator`에 발사자 정보를 전달한다. 캐릭터의 발 위치를 기준으로 추적하려면 `Target Offset`으로 높이를 보정한다. 스폰 시 Configuration 핀으로 다른 DA를 사용할 수도 있다.

DA는 BeginPlay에서 복사한다. 비행 중 DA나 Target Actor를 변경해도 이미 발사된 로켓은 재설정되지 않는다. 표적이 유효하지 않거나 전방 유도 원뿔을 벗어나거나 유도 시간이 끝나면 영구적으로 직진한다. 미래 위치 예측이나 다른 표적 탐색은 하지 않는다.

## 기본값

| DA 항목 | 기본값 | 의미 |
|---|---:|---|
| Speed | 1200cm/s | 전진 속도 |
| Turn Rate | 30°/s | 초당 최대 선회각 |
| Guidance Delay | 0.15초 | 발사 직후 직진 시간 |
| Guidance Duration | 1.1초 | 지연 이후 유도 시간 |
| Guidance Cone Half Angle | 65° | 현재 진행 방향 기준 허용 반각 |
| Lifetime | 3초 | 발사부터 시한폭발까지 |
| Collision Radius | 6cm | 메시와 별개인 충돌 구 |
| Damage / Damage Radius | 0 / 100cm | 선택적 폭발 피해 |
| Trail Interval / Lifetime | 0.08 / 0.45초 | 연기 간격과 수명 |
| Explosion Lifetime | 0.55초 | 기본 폭발 연출 수명 |
| Trail Size / Explosion Visual Radius | 9 / 100cm | 기본 연출 크기 |

기본 수치로 18m 거리에서 발사했을 때 미리 초당 4m로 횡이동하는 표적은 자동검증에서 1m 폭발 반경과 34cm 캐릭터 반경을 벗어난다. 모든 거리·속도 조합의 회피를 보장하는 수치는 아니므로 실제 교전 거리에서 튜닝한다. 런타임에서 비정상 DA 값은 안전한 범위로 보정하며 Lifetime은 0.05~30초다.

## 외형과 이펙트 교체

로켓 BP의 `RocketMesh`, `ExhaustMesh` 컴포넌트에서 메시·재질·상대 변환을 편집한다. `TrailNiagara`에 Niagara 시스템을 지정하면 연속 추진 연출을 추가할 수 있다. `Explosion Niagara`, `Explosion Sound`로 폭발 연출을 교체한다. `Simple Trail` / `Simple Explosion`을 꺼 기본 메시 연출과 중복되지 않게 한다. `Simple Effect Class`는 `ATunaRocketEffect` 파생 BP로 교체할 수 있다.

DA의 연출 시간과 크기는 기본 메시 연출에 적용된다. 사용자 지정 Niagara의 입자 수명과 크기는 해당 Niagara 시스템에서 설정한다. 커스텀 로직에는 `On Detonated` 이벤트와 `Detonate` 함수를 사용한다. 폭발은 한 번만 처리된다. 레벨 종료나 외부 Destroy는 추가 폭발을 만들지 않는다.

## 충돌과 피해

기본 충돌은 WorldStatic, WorldDynamic, Pawn, PhysicsBody를 막고 발사자 Owner/Instigator를 무시한다. 최대 1/120초 단계로 swept 이동하며 긴 프레임에서도 신관 이후 위치까지 이동하지 않는다. 필요하면 BP의 Collision 응답을 조정한다.

피해량을 0보다 크게 설정하면 반경 내 Pawn마다 한 번씩 엔진 `ApplyDamage`를 호출한다. 발사자는 제외한다. `Can Damage Actor`를 BP에서 재정의해 호스트의 팩션/팀 규칙을 연결한다. `Damage Cover Channel`은 기본 Visibility이며 해당 채널을 Block하는 장애물이 피해를 막는다. 플러그인은 게임의 팩션·방어구·시야 시스템을 직접 참조하지 않는다.

싱글플레이용이다. 네트워크 발사/이동/폭발 복제는 제공하지 않는다. 기존 적 AI나 무기 슬롯에는 자동 연결하지 않는다.

## 검증

UE Automation 필터: `TunaGuidedRocket`. 선회 상한, 유도 시간·이탈, 조기 횡이동, 비정상 DA, 긴 프레임 신관 위치, 벽/물리 충돌, 발사자 무시, 트리거/엄폐 피해, 중복 폭발 및 샘플 BP 에셋 참조를 검사한다.

콘텐츠 생성 도구는 작업 중에만 사용하고 제거한다. 에디터 시작 시 에셋을 다시 생성하지 않는다.
