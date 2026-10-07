# 지하 시설 모듈과 사격장

레벨: `/Game/Environment/Basement/Maps/L_Basement`

사다리실을 중심으로 북쪽(+X)에 사격장, 동·서·남쪽에 시설 확장용 통로를 배치했다. 기존 벙커 레벨과 별개인 편집 가능한 레벨이며, 통로와 방 입구에 문·문틀·문턱·천장이 없다. 사다리는 기존 FacilityRooms 메시를 재사용한 시각 모델이다. 다른 레벨로 이동하는 사다리 상호작용은 포함하지 않는다.

## 연습용 드론 표적

- `Target_01`~`Target_05`의 몸/머리 방어 등급은 각각 **맨몸(0) / 1 / 2 / 3 / 4**다. BP 기본값 및 각 배치 인스턴스의 `Practice Dummy > Armor`에서 `Body Armor Tier`, `Head Armor Tier`를 독립적으로 조절한다. 0은 해당 장비 해제다.
- 등급별 실제 방어구 아이템의 방어력·관통 계산을 사용하며, 현재 플레이어와 적처럼 두 부위의 유효 방어력을 합산한다. `RangeTarget/placement.json`이 배치 등급을 기록하고, `Tools/Basement/verify_target_armor.py`가 저장 맵과 PIE의 장착 아이템·피해·체력·회복을 검증한다. 결과는 `target_armor_validation.json`에 저장한다.

`/Game/Environment/Basement/Range/Blueprints/BP_RangePracticeTarget`은 `ATunaSweeperShootingPracticeDummyActor`를 상속한다. 레벨의 `Target_01`~`05`는 이 BP 인스턴스이며, `ROOT_Range`에 연결되어 방과 함께 이동·회전한다.

- 원래 토끼 얼굴과 전면 과녁판을 드론 전면 광학 센서로 교체했다. 머리 위에는 원래 높이 54.09cm의 절반인 27.05cm 토끼 귀 두 개를 다시 붙였다. 귀 밑면은 Z=125cm, 끝은 Z=152.05cm이며, 짧고 둥근 비율과 기존 흰색·청회색 아틀라스를 유지한다. 몸체와 다리는 변경하지 않았다.
- `BodyMesh`: 몸체·다리의 일반 피격 부위. `HeadMesh`: 중앙·보조 렌즈와 짧은 귀 두 개. `HeadshotPlateMesh`: 렌즈를 감싸는 사각 센서 하우징. 두 헤드 컴포넌트는 부모 클래스의 헤드 판정을 사용한다.
- 센서를 조준한 투사체가 같은 표적의 센서에 실제 명중하면 기존 공통 헤드샷 배율(현재 2배)을 적용한다. 단순히 센서에 우연히 맞은 경우는 부모 클래스 규칙대로 처리한다.
- 체력 100, 최저 체력 1, 초당 50 회복(최대 회복 시간 2초), 화면 공간 체력바를 상속한다. 체력바는 짧은 귀와 겹치지 않게 Z=180cm에 배치했다.
- `RangeTarget/RangePracticeTarget.blend`와 `RangeTarget/Models/`에 편집 원본과 부위별 GLB를 보관한다. `RangeTarget/mesh_manifest.json`에 메시·부위별 재질·범위 정보를 기록했다. `Range/Assets/GLB/SM_RobotTarget.glb`는 수정 전 원본 보관용이며 레벨 표적에서는 사용하지 않는다.
- 센서의 전용 재질은 부모의 디버그 색상 파라미터를 사용하지 않으므로 PIE에서도 드론 외형을 유지한다.

## 디버깅용 무기고

`WeaponRack_001`은 `/Game/Blueprints/Debug/BP_DebugArmory` 인스턴스다. 원래 `SM_WeaponRack` 메시와 위치·회전·`ROOT_Range` 부모 관계를 유지한다. 개발 빌드에서는 200cm 안에서 **무기고** 상호작용으로 창을 열어 무기(근접 무기·부착물 포함), 탄약, 보호장비(머리·몸통·얼굴·귀)를 선택하고 1~999개씩 무료로 지급받을 수 있다. 재고나 비용은 없으며 가방·음식·재료·화폐·설계도는 목록에서 제외한다. 지급 아이템은 인벤토리에 들어가며 공간이 부족하면 요청 전체를 취소한다.

Shipping에서는 선반 메시와 충돌만 남는다. 상호작용 컴포넌트/마커가 제거되고, 상호작용 후보·무기고 창·카탈로그·직접 지급 함수도 각각 차단한다. BP에서 표시 설정을 바꿔 Shipping 차단을 해제할 수 없다. 창 상태는 저장하지 않으며 지급된 아이템은 기존 인벤토리 저장 규칙을 따른다.

`Tools/Basement/verify_armory.py`가 저장 BP·메시·상호작용 키·배치를 확인한다. 네이티브 `TunaSweeper.DebugArmory.InteractionAndSupply` 자동화는 실제 상호작용과 UI, 무료 지급, 수량/종류 제한, 공간 부족 시 원상 복구, 거리·사망·탑승·닫기 검증을 수행한다.

## 사격대와 안전 칸막이

방은 오른쪽(UE +Y)으로 **사로 210cm + 별도 이동 공간 140cm = 350cm** 확장했다. 내부 폭은 10.8m에서 14.3m이며 기존 입구 위치·폭과 방 깊이는 유지한다. 오른쪽 통로를 따라 사격대 옆으로 돌아 표적 앞까지 걸어갈 수 있다. 우측 벽·수납품·배관·조명도 외곽으로 옮겼고, 다섯 번째 표적·바닥 패드·후면 방탄판·조명과 `05` 바닥 표시를 추가했다. 확장 바닥은 기존 1.2m 타일 크기를 유지한다. 편집 원본과 변경 배치는 `RangeExpansion/RangeExpansion.blend`, `Models/`, `layout_manifest.json`에 있다.

`FiringBench_001`~`005`는 표적 01~05와 중심을 맞춘 5개 사수 자리다. 사로 간격은 210cm이며, `LanePartition_001`~`004`이 사수 사이를 구획한다. 칸막이는 높이 170cm, 본체 두께 10.5cm, 받침 포함 길이 110cm인 독립 메시다. 중심을 원본 Y=-4.09m에 두어 서 있는 사수 자리만 보호하고, 앞쪽 받침도 테이블 뒤끝보다 8cm 뒤에서 끝나므로 테이블 사이를 가리지 않는다. 모서리는 3단 베벨로 마감했고 기존 `MI_Range_Props` 아틀라스를 사용한다. 탄환과 캐릭터 충돌을 모두 유지한다.

사격대 위 장식 탄약 상자 `AmmoBox_001`~`003`은 렌더링을 유지하되 인스턴스 컴포넌트를 `NoCollision`으로 설정하고 overlap도 끈다. 탄환·조준 및 캐릭터 충돌에 관여하지 않는다. 공유 `SM_AmmoBox` 메시와 다른 위치의 상자, 사격대·칸막이 충돌은 유지한다.

모니터 `ControlConsole_001`은 왼쪽 전용 책상 `RangeControlDesk_001`에 연결되어 책상을 옮기면 함께 이동한다. 책상은 기존 사격대와 같은 높이 93.5cm, 폭 120cm이며 공유 재질을 사용한다. 사로 뒤쪽에는 116cm 깊이의 진입 통로가 남는다. 칸막이·사격대는 각각 이동할 수 있고 모두 `ROOT_Range`에 속한다.

`RangeStations/RangeStations.blend`, `Models/`, 메시·배치 manifest가 편집 원본이다. 원래 `Range/Scene/placement_manifest.json`은 최초 사격장 자료이며 최신 사격대 배치는 `RangeStations/placement_manifest.json`을 참고한다. `Tools/Basement/verify_stations.py`를 별도 렌더 에디터의 `-ExecutePythonScript`로 실행하면 저장된 레벨의 10개 사로 진입 경로와 7개 표적 접근 경로, 5개 사격 시야, 칸막이 충돌, 테이블 구간 24개 투시선과 8cm 이격, 표적 정렬과 모니터 받침을 검사한다.

## 이동과 확장

- Outliner `Basement/` 아래의 `ROOT_Range`, `ROOT_LadderHub`, 각 통로 `ROOT_*`를 이동하면 연결된 자식 액터가 함께 이동한다. 공유 Static Mesh를 사용하므로 개별 부품도 교체할 수 있다.
- `ROOT_Connector_East`, `ROOT_Connector_West`, `ROOT_Connector_South`는 끝단의 바닥 중앙 스냅 기준이다. 새 시설 바닥의 Z=0과 출입구 중심을 이 기준에 맞춘다.
- 연결 기준점과 임시 끝벽 묶음은 해당 통로 루트의 자식이고, 시설 조명도 각 시설 루트에 연결되어 함께 이동한다. 통로 루트 피벗은 사다리실 쪽 연결부 중앙이다.
- 연결부는 240cm 유효 폭이며, 모듈 배치 기준은 40cm이다. 임시 막음벽은 `*_RemovableCap` 묶음으로 분리되어 있어 확장 시 해당 묶음만 제거한다. 문은 없다.
- 기본 벽 길이는 40/80/160/240cm, 높이는 280cm, 두께는 26cm이다. 바닥은 80cm 단위, 두께 22cm, 완성 상면 Z=0이다. 북쪽 통로의 마지막 바닥만 길이 40cm로 조정한다.
- `SM_Basement_CornerConvex`, `SM_Basement_CornerConcave`는 90도를 3개 평면으로 나누는 외측·내측 코너다. 안쪽 반경 40cm이며 평벽 접합점까지의 거리는 40cm이다. 미러링 없이 90도씩 회전해서 재사용한다.
- 원본 모델 좌표는 X=가로, Y=사격 방향, Z=높이(m)이다. 레벨에서는 원본 +Y → UE +X, 원본 +X → UE +Y, 1m → 100cm로 변환했다. 배치 정보는 `layout_manifest.json`에 있다.

## 카메라와 투명벽

사격장은 기존 `/Game/Camera/BP_LocationBlendCamera` 인스턴스 `Basement_RangeLocationBlend`가 `Basement_RangeCamera`를 참조한다. 플레이어가 UE (1700, 175, 0) 기준 반경 1400cm에 들어오면 기본 시점에서 부드럽게 전환하고, 1000cm 안에서는 사선 카메라를 완전히 적용한다. 사격장 바닥 전체가 완료 반경에 포함된다. 사다리실 중심에서는 가중치 0이며 퇴장하면 플레이어 카메라로 복귀한다. 우선순위는 10이다. 카메라는 UE (90, -1225, 2265)에서 (1490, 175, 65)를 향하며 FOV 55°, yaw 45°로 사격장 전체를 보여 준다.

촬영 카메라와 거리 판정 BP는 서로 독립적으로 조정할 수 있으며 둘 다 `ROOT_Range`의 자식이다. 구도만 바꿀 때는 `Basement_RangeCamera`를 Pilot하고, 전환 범위는 BP의 `Blend Start Distance`와 `Blend Complete Distance`에서 조정한다. `range_camera_manifest.json`에 최종 배치와 추가로 숨긴 벽·부착물 목록을 기록한다.

기본 UE -X 방향의 앞쪽 벽에 더해 사선 카메라를 가리는 UE -Y 쪽 사격장 측벽과 입구 복도 서쪽 벽의 Visibility를 끄고 Hidden In Game을 켰다. 그림자도 끄되 BlockAll과 QueryAndPhysics 충돌은 유지한다. `BasementCollisionOnly` 태그로 찾을 수 있다. 해당 측벽의 배관·등기구·환기 패널도 숨겼으며, 광원과 독립 가구는 유지한다. 에디터에서도 숨김 상태로 저장된다.

`Tools/Basement/verify_range_camera.py`는 저장된 범위·구도·투명벽 충돌과 실제 PIE 진입/접근/5개 사로/표적 측면/퇴장 위치의 카메라 가중치·시점·복귀를 확인한다. `range_camera_validation.json`과 `Previews/UE_Basement_RangeGameplay.png`가 검증 결과다.

방 전체를 다른 방향으로 회전·재배치할 때는 새 카메라 방향에 맞게 앞쪽 벽의 표시 상태를 다시 지정한다. 자동 카메라 추적 숨김 로직은 사용하지 않는다. 충돌까지 제거하거나 액터를 삭제하면 투명벽 기능도 사라지므로 표시 설정만 변경한다.

## 원본과 텍스처

- `Basement_Modules.blend`: 개별 모듈을 분리해 보관한 편집 원본.
- `Models/`: 독립적으로 재사용할 수 있는 GLB 9종. 오목 코너의 둥근 부분 아래까지 바닥을 채우는 `FloorCornerFill`을 포함한다.
- `Range/`: `TempFormChat/TunaSweeper_Range`에서 복사한 사격장 개별 모델, 공유 텍스처, 원래 배치 정보. 임시 입력 폴더 없이도 원본 에셋을 사용할 수 있다.
- `Textures/T_Basement_Concrete.png`: 내장 image generation으로 생성한 콘크리트 BaseColor. 벽/바닥은 같은 표면을 공유하고 재질 인스턴스의 Tint와 Roughness로 구분한다. 바닥에는 조금 더 어두운 청회색을 적용했다.

생성 프롬프트: “Use case: stylized-concept. Asset type: seamless base color texture for modular underground corridor walls and floors in an Unreal Engine stylized game. Generate a square, completely flat orthographic albedo texture of pale desaturated blue-grey smooth cast concrete matching a clean softly stylized shooting range. Target average color approximately sRGB #A8BCC9. Very subtle broad cloudy tonal variation, sparse shallow tiny concrete pores, quiet low-contrast surface. Smooth matte poured concrete. No panels, no seams, no edge borders, no bolts, no stripes, no cracks, no text, no lighting, no shadows, no perspective, no objects. Seamless tile on all edges, restrained clean readable game material. Surface should look almost flat and quiet, not gritty or photorealistically busy. Opaque square texture.”

## 검증

`source_validation.json`은 메시 연결·체적·UV·코너 분할 검증 결과다. `Tools/Basement/verify_unreal.py`는 저장된 레벨을 다시 열어 재질·메시·부모 관계·캐릭터 크기 통로·투명벽 충돌·바닥 연결을 검사한다. `Tools/Basement/capture_unreal.py`는 실제 에디터 뷰포트를 캡처한다. 임포트와 생성 스크립트는 일회성 작업 후 제거하며, 시작 시 자동 생성하는 코드는 없다.

`Tools/Basement/verify_target.py`는 저장된 BP의 상속·부위·재질·충돌 기본값과 절반 높이 귀를, `verify_target_runtime.py`는 5개 배치의 센서·몸체·양쪽 짧은 귀 충돌, 원래 긴 귀 끝 위치의 빈 공간, PIE 피해·체력바·최저 체력·회복을 검증한다. 결과는 `target_asset_validation.json`, `target_runtime_validation.json`에 저장한다. 헤드샷 조준 의도 규칙은 기존 네이티브 자동화 테스트 `TunaSweeper.Combat.PracticeDummy.HitDamage`로 함께 검증한다.
