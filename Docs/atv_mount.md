# ATV 승하차

레벨에는 `/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA`를 배치한다. 부모는 `TunaSweeperATVActor`이며 기존 `SKM_ATV`, 차체 박스 충돌, `seat` 본에 연결된 `TunaSweeperVehicleMountComponent`, 시동/공회전/정지 사운드를 상속한다. 메시의 지면 원점은 액터 원점보다 45cm 아래다. 기존 StaticMeshActor에 둔 `SM_ATV`는 자동으로 교체하지 않는다.

## 상호작용

- `TunaSweeperVehicleMountComponent`는 기존 `TunaSweeperInteractableComponent`를 상속한다. 별도 키 탐색 없이 기존 상호작용 서브시스템과 F 입력을 사용한다.
- `VehicleMount` 상호작용을 요청하면 거리·높이·점유 상태·플레이어 상태를 확인하고 캐릭터를 좌석에 연결한다. 플레이어 possession과 기존 카메라/체력/인벤토리 소유권은 유지한다.
- 탑승 중 보행·점프·구르기·조준·사격 입력과 주변 상호작용을 막는다. 캡슐은 QueryOnly로 유지해 피격 쿼리는 계속 가능하다.
- 기존 X 입력(IA_Drop)은 탑승 중 하차를 우선 처리한다. 하차가 실패해도 아이템 버리기/취소로 이어지지 않는다. 인벤토리·일시정지·대화·하우징 UI가 열려 있으면 하차하지 않는다.
- 양옆/뒤쪽 후보에 대해 지면 경사, 실제 캡슐 공간, 좌석에서 후보까지의 장애물을 검사한다. 안전한 지점이 없으면 탑승 상태를 유지한다. 안내가 숨겨져 있어도 안전한 지점이 있으면 하차할 수 있다.
- 하차하면 부착·충돌·이동 모드를 복구한다. 사망과 액터 종료 시에도 좌석 상태를 해제한다. 강제 해제 시 주변에 공간이 없으면 마지막 탑승 직전 위치를 사용한다.

## 안내 및 사운드

- 좌석의 실제 위치 변화와 액터 속도를 함께 사용한다. 기본 5cm/s 이하에서 1.5초 정지하면 화면 아래쪽에 둥근 X 박스와 `내리기`를 표시한다. 이동 재개 시 바로 숨기고 시간을 초기화한다.
- `DismountHintDelay`, `StationarySpeedThreshold`, `DismountDistance`, 좌석의 위치/회전 오프셋은 컴포넌트에서 조정할 수 있다.
- `UITextStrings.csv`의 `ui.vehicle.mount`, `ui.vehicle.dismount`, `ui.key.x`를 기존 로컬라이징 경로로 해석한다. 한국어/영어/일본어를 제공한다.
- 탑승 시 `SW_ATV_Mount_Start` → `SW_ATV_Idle_Loop`, 하차 시 `SW_ATV_Dismount_Stop`을 재생한다. 시동 도중 하차하면 대기 중인 공회전 전환도 취소한다.

## 범위와 확장 지점

이번 구현은 상호작용을 통한 승하차다. Chaos 주행, 휠/서스펜션 런타임 구동, 운전자 앉는 자세와 손발 IK는 별도 구현 대상이다. 차량에 이미 주행 컴포넌트가 있다면 그 차량의 좌석에 `TunaSweeperVehicleMountComponent`를 추가해 같은 승하차 처리를 사용할 수 있다. `OnMounted`/`OnDismounted` 이벤트와 `GetRider`, 캐릭터의 `IsMountedInVehicle`/`GetVehicleMount`를 제공한다.

좌석 점유·안내 타이머·엔진 오디오는 일시적인 월드 상태이며 저장하지 않는다. 세이브 구조를 변경하지 않았다.

## 검증

- 에디터 자동 테스트: `TunaSweeper.Vehicle.MountInteraction`
- 별도 UE 프로세스에서 기본값/자산 참조 확인: `Tools/ATVRig/verify_mount_setup.py`
- UI의 실제 화면 배치와 청취, 운전자 자세의 최종 게임 내 확인은 별도 플레이 검수가 필요하다.
