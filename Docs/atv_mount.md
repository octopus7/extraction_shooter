# ATV 승하차와 주행

레벨에는 `/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA`를 배치한다. 부모 `TunaSweeperATVActor`는 APawn이며 기존 `SKM_ATV`, 차체 물리 애셋 `PA_ATV`, Chaos 차량 이동 컴포넌트, `seat` 본의 `TunaSweeperVehicleMountComponent`, 가솔린 엔진 사운드를 상속한다. 메시가 루트이며 액터 원점은 메시의 지면 원점과 같다. 배치 시 스케일 1을 사용하고 바퀴가 지면에 닿도록 놓는다. 기존 StaticMeshActor의 `SM_ATV`는 자동으로 교체하지 않는다.

## 주행과 기계 부품

- 기존 IA_Move 입력을 탑승 컴포넌트로 전달한다. W는 차량 전방 가속, S는 제동 후 후진, A/D는 좌우 조향이다. 카메라 방향을 기준으로 평행 이동하지 않는다. Shift+W는 가속 강도와 최고속을 높인다.
- 기본 최고속 목표는 전진 1,000cm/s(36km/h), Shift 1,600cm/s(57.6km/h), 후진 400cm/s(14.4km/h)이며 액터에서 조절한다. 목표 부근에서 구동 입력을 줄이므로 실제 속도는 지형·저항에 따라 달라진다.
- 340kg 차체, 네 바퀴 AWD, 자동 변속/후진, 전륜 Ackermann 조향과 속도별 조향 제한을 사용한다. 각 바퀴는 구면 접지 검사, 스프링/댐퍼, 위 8cm·아래 12cm 이동 범위를 가진다. 탑승자 Pawn은 바퀴 접지 대상에서 제외한다.
- `TunaSweeperATVAnimInstance`가 휠 회전·조향·서스펜션 이동을 본에 반영한다. 너클, 위/아래 암, 쇼크 양쪽과 스프링, 핸들도 연동한다. 별도 AnimBP 제작 없이 네이티브 애니메이션 인스턴스를 사용한다.
- UI 열기·행동 취소·하차 시 입력과 Shift 상태를 초기화한다. 탑승자가 없거나 UI가 주행을 막는 동안 주차 브레이크를 건다. 무인 차량에 입력을 보내도 움직이지 않는다.
- `PA_ATV`의 root 박스가 지형·장애물과 충돌하는 물리 차체다. Pawn과의 직접 물리 충돌은 제외하고 `ChassisCollision`의 QueryOnly 박스로 보행을 막아 캐릭터의 밀기 힘과 캡슐 침투 보정이 차체에 전달되지 않게 한다. 미탑승 상태에서도 차체 중력·서스펜션·환경 충돌은 계속 계산한다.
- 총알은 ProjectileMovement의 스윕으로 명중·피해를 처리하며 강체 물리 충돌에는 참여하지 않는다. ATV의 접지 검사에서도 Projectile 채널을 제외해 총알을 바닥으로 잘못 인식하지 않게 한다. 차체의 기존 포인트 대미지 충격과 탑승자의 피격 판정은 유지한다. 저장된 BP/레벨 인스턴스에도 시작 시 충돌 정책을 적용한다.

## 상호작용

- `TunaSweeperVehicleMountComponent`는 기존 `TunaSweeperInteractableComponent`를 상속한다. 별도 키 탐색 없이 기존 상호작용 서브시스템과 F 입력을 사용한다.
- `VehicleMount` 상호작용을 요청하면 거리·높이·점유 상태·플레이어 상태를 확인하고 캐릭터를 좌석에 연결한다. 플레이어 possession과 기존 카메라/체력/인벤토리 소유권은 유지한다.
- 탑승 중 보행·점프·구르기·조준·사격 입력과 주변 상호작용을 막는다. 캡슐은 QueryOnly로 유지해 피격 쿼리는 계속 가능하다.
- 기존 X 입력(IA_Drop)은 탑승 중 하차를 우선 처리한다. 하차가 실패해도 아이템 버리기/취소로 이어지지 않는다. 인벤토리·일시정지·대화·하우징 UI가 열려 있으면 하차하지 않는다.
- 양옆/뒤쪽 후보에 대해 지면 경사, 실제 캡슐 공간, 좌석에서 후보까지의 장애물을 검사한다. 안전한 지점이 없으면 탑승 상태를 유지한다. 안내가 숨겨져 있어도 안전한 지점이 있으면 하차할 수 있다.
- 하차하면 부착·충돌·이동 모드를 복구한다. 사망과 액터 종료 시에도 좌석 상태를 해제한다. 강제 해제 시 주변에 공간이 없으면 마지막 탑승 직전 위치를 사용한다.

## 안내 및 사운드

- 탑승 중 화면 아래쪽에 녹색/어두운 회색 내구도 막대를 표시한다. 외곽은 180×14이며 1px의 어두운 테두리를 포함한다. 기존 위치보다 28px 위로 올리고 하차 안내 위치는 유지한다. 주행 중에도 막대는 유지하며, 인벤토리·일시정지·대화·하우징 UI가 열리면 HUD를 숨긴다. 하차·사망·차량 파괴 시 제거한다.
- `/Game/UI/Vehicle/WBP_ATV_HUD`의 실제 WidgetTree에 바·흰 안내 패널·문구·우측 X 키캡을 저장한다. Designer에서 편집할 수 있으며 부모 `UTunaSweeperVehicleDismountWidget`은 내구도, 표시 상태, 로컬라이징만 갱신한다. 탑승 컴포넌트의 `VehicleHudClass`가 이 WBP를 참조한다.
- 하차 패널은 기존 상호작용과 동일한 18pt 검은 문구, 16pt 우측 키캡, 반경 5 및 키캡 외곽선 1px을 사용한다. 왼쪽 원과 액터 위치의 동심원은 포함하지 않는다. 탑승 F의 상호작용 컴포넌트 자체를 하차에 재사용하지는 않는다.
- UE 5.7의 `SetDesiredSizeInViewport`는 앵커를 초기화하므로 크기·오프셋·하단 앵커·정렬을 완성한 슬롯을 한 번에 적용한다.
- 좌석의 실제 위치 변화와 액터 속도를 함께 사용한다. 기본 5cm/s 이하에서 1.5초 정지하면 화면 아래쪽에 둥근 X 박스와 `내리기`를 표시한다. 이동 재개 시 바로 숨기고 시간을 초기화한다.
- `DismountHintDelay`, `StationarySpeedThreshold`, `DismountDistance`, 좌석의 위치/회전 오프셋은 컴포넌트에서 조정할 수 있다.
- `UITextStrings.csv`의 `ui.vehicle.mount`, `ui.vehicle.dismount`, `ui.key.x`를 기존 로컬라이징 경로로 해석한다. 한국어/영어/일본어를 제공한다.
- 탑승 시 `SW_ATV_Mount_Start` → `SW_ATV_Idle_Loop`, 하차 시 `SW_ATV_Dismount_Stop`을 재생한다. 시동 도중 하차하면 대기 중인 공회전 전환도 취소한다.
- 시동 완료 후 속도에 따라 `SW_ATV_Drive_Loop`, Shift 가속 시 `SW_ATV_Boost_Loop`을 부드럽게 섞는다. 엔진 RPM에 따라 주행/가속 루프의 피치를 조절하고 하차 시 모든 루프를 정리한다.

## 범위와 확장 지점

승하차·Chaos 주행·휠/서스펜션 구동과 Luna Mk2 탑승 자세를 구현했다. `OnMounted`/`OnDismounted` 이벤트와 `GetRider`, 캐릭터의 `IsMountedInVehicle`/`GetVehicleMount`를 제공한다. 탑승 컴포넌트는 다른 좌석에도 사용할 수 있지만 주행 입력 전달은 현재 ATV 액터에 연결되어 있다.

차량 내구도·피격 연기·파괴 시 부분 분해는 [구현 계획과 진행 상태](atv_damage_destruction_plan.md)에 기록했다. 현재 내구도/연기 상태/부품 분리 구현은 있으나, 막힌 출구의 하차 좌표 테스트 1건과 연기 최종 시각 검수가 남아 있다.

파괴 후 기본 3초 뒤 잔해의 현재 위치에서 `NS_Explosion_Tuna`를 한 번 재생한다. `BP_ATV_TypeA` 클래스 기본값 또는 레벨 인스턴스의 `ATV > Effects > Destruction Explosion Delay`로 지연 시간을 조정하며, 0이면 즉시 재생한다. 추가 피격은 타이머를 재시작하지 않고 차량 제거 시 예약을 취소한다. Steam Demo Shipping 패키징과 `TunaSweeper.Vehicle.DelayedExplosion` 자동 테스트를 통과했다. 폭발 시스템은 소프트 참조로 유지하고 BeginPlay에서 로드해 초기 CDO 생성 시 Niagara 컴포넌트 렌더러의 레지스트리 오류를 피한다. 최종 시각 검증은 별도다.

폭발 시 배럴과 동일한 `/Game/Audio/Imported/SW_barrel_explosion`을 폭발 위치에서 한 번 재생한다. `ATV > Effects > Destruction Explosion Sound`에서 사운드를 변경할 수 있다. 지연 폭발과 같은 중복 방지 처리를 사용한다. 사운드 연결은 Steam Demo Shipping 빌드에 포함했으며 최종 청취 검증은 별도다.

## 운전자 자세

- 탑승 중 `TunaSweeperATVRiderAnimInstance`로 전환한다. 자연스러운 몸 자세를 우선해 상체는 12°만 앞으로 기울이고 고개는 8° 반대로 보정한다. 골반과 다리를 기존 안장/발판에 억지로 맞추지 않으며, 엉덩이가 안장 안에 들어가거나 발이 공중에 떠도 허용한다. 이 자세에 맞춰 차량 모델링을 후속 조정한다.
- 손목은 `grip_l/r` 끝 표시보다 안쪽으로 좁혀 팔꿈치가 굽혀질 여유를 만든다. 다리는 실제 허벅지/정강이 길이에 비례해 허벅지를 앞쪽으로, 정강이는 거의 아래로 내리는 2본 IK를 사용한다. 무릎은 약간만 벌리고 발판 본은 목표로 사용하지 않는다. 조향 시 핸들 방향에 따라 자세와 손 위치를 갱신한다. 별도 AnimSequence 애셋 없이 매 프레임 계산하는 네이티브 자세다.
- 현재 Luna Mk2 비율과 +Y 메시 전방에 맞춘 자세다. 다른 스켈레톤에 대한 범용 리타기팅이나 별도 승차/하차 전환 모션은 포함하지 않는다.
- 탑승 중에는 보행 AnimBP를 전용 인스턴스로 교체하므로 기존 보행 그래프 안의 머리카락 RigidBody 등 부가 노드는 실행하지 않는다. 별도 Skirt 컴포넌트의 AnimBP는 유지한다. 탑승용 머리카락 물리는 후속 연결 대상이다.
- 탑승 중 장착 무기를 숨기고, 하차/사망/좌석 종료 시 기존 AnimBP·메시 갱신 설정·무기 표시 상태를 복원한다. 탑승 중에는 물리 갱신 뒤 매 프레임 자세를 평가해 손 위치가 늦게 따라오지 않게 한다.

좌석 점유·차량 주행 상태/이동 위치·안내 타이머·엔진 오디오는 일시적인 월드 상태이며 저장하지 않는다. 세이브 구조를 변경하지 않았다.

## 검증

- HUD의 실제 엔진 슬롯과 두 해상도에서 화면 안 배치: `TunaSweeper.Vehicle.HUDLayout`. `-ATVHUDPreview`로 렌더링하면 녹색/회색 픽셀과 X 안내 표시 전환도 검사하고 `Saved/ATVRigWork/HUD_Moving.png`, `HUD_Stopped.png`를 저장한다.
- 에디터 자동 테스트: `TunaSweeper.Vehicle.MountInteraction`
- 물리 주행/접지/조향/실제 휠·스프링·핸들 본/Shift 가속/입력 해제: `TunaSweeper.Vehicle.Driving`. 저장된 `BP_ATV_TypeA`로 실행한다.
- 실제 플레이어 BP의 완만한 상체 기울기, 팔꿈치/무릎 굽힘, 무릎 방향과 벌림, 하차 복원: `TunaSweeper.Vehicle.RiderPose`. `-ATVRiderPreview`와 렌더링을 활성화해 실행하면 `Saved/ATVRigWork/RiderPose0~2.png`에 검토 이미지를 저장한다.
- 실제 플레이어 BP의 전방·측면 보행 충돌 및 주차 차량 밀림: `TunaSweeper.Vehicle.PedestrianContact`.
- 총알의 접지 검사 제외, 물리 충돌 제외, 주행 중 연속 피격 안정성 및 탑승자 피해: `TunaSweeper.Vehicle.ProjectileContact`.
- 별도 UE 프로세스에서 기본값/자산 참조 확인: `Tools/ATVRig/verify_mount_setup.py`
- 주행 기본값/물리 애셋/애니메이션/사운드 참조 확인: `Tools/ATVRig/verify_driving_setup.py`
- UI의 실제 화면 배치와 청취, 운전자 자세의 최종 게임 내 확인은 별도 플레이 검수가 필요하다.
