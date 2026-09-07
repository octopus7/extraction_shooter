# FacilityRooms — 지하 설비실과 목조 다락 관제실

기존 기지·맵을 교체하지 않는 독립 환경 세트입니다. cm 단위, 바닥 중앙 피벗, 200cm 벽/바닥 모듈을 사용합니다.

최종37종(건축18·설비9·관제10), 원본171,948삼각형/UE LOD0 171,468삼각형, UCX160개입니다. 지하75개·다락43개 모델 배치와 숲 카드1개를 두 레벨로 저장했습니다. FBX/배치/독립 UE 재로드 모두 통과했고, `Previews/UE_ControlFloor02_Interior.png`에서 숲과 창틀 그림자·볼륨광을 실제 UE 렌더로 확인했습니다.

## Unreal 레벨

- `/Game/Environment/FacilityRooms/Maps/L_Facility_Basement`: 12×10m 지하 시설, 보일러·발전기·수도펌프·압력탱크·전기함·자재창고·분리된 사다리실.
- `/Game/Environment/FacilityRooms/Maps/L_Facility_ControlFloor02`: 6×6m 목조 다락, 바닥 높이320cm, 상황 모니터·책상·통신장비·책·머그컵·따뜻한 등불.

다락의 `AtticRoof` 폴더는 분리된 목조 지붕입니다. 내부는 `Facility_InteriorCamera`를 Pilot하여 확인합니다. Blender 개요 프리뷰만 앞쪽 외벽과 지붕을 임시 숨겨 구조를 보여주며 저장된 UE 레벨에서는 모두 표시됩니다.

## 창문과 빛

- 실제95×95cm 열린 목조 창, 바닥에서 sill85cm, 중앙3cm 십자살. 벽/충돌 박스가 개구부 전체를 막지 않습니다.
- `Exterior_ForestImpostor`: ImageGen 숲 텍스처를 사용하는 양면 Unlit 카드. 충돌/그림자 없음. 실제 숲 지형이나 다방향 3D 수목이 아닌 창밖 배경용 평면 임포스터입니다.
- `Lighting_WindowSunBeam`: 창밖의 따뜻한 Movable SpotLight, Volumetric Scattering Intensity와 Cast Volumetric Shadow 설정.
- `Lighting_WindowVolumetricFog`: ExponentialHeightFog의 Volumetric Fog 사용. 창틀을 통과한 빛은 실제 조명/안개로 구현하며 반투명 빛줄기 평면으로 대체하지 않습니다.
- `Lighting_StableExposure`: 노출 고정용 PostProcessVolume. 조명 연출을 바꿀 때 창빛 강도·안개 밀도·노출을 함께 조절합니다. 볼륨 안개를 지원하는 렌더/그림자 품질이 필요합니다.

## 색상과 충돌

`/Game/Environment/FacilityRooms/Materials`의16개 재질 인스턴스는 의미별 슬롯으로 나뉩니다. `Tint`, `Metallic`, `Roughness`, `EmissionStrength`를 조정할 수 있습니다. 색상은 공유 아틀라스와 Tint의 곱입니다. 개별 오브젝트만 색을 바꾸려면 인스턴스를 복제해 해당 슬롯에 지정합니다.

모델별 UCX 박스 충돌과 레벨의 BlockAll 설정을 포함합니다. 문120×230cm, 해치90×90cm와 주요 통행 경로를 검증합니다. 사다리·해치는 모델 및 개구부만 제공하며, 오르내리기 상호작용·레벨 이동·저장 로직은 추가하지 않습니다.

## 소스와 검증

- `FacilityRooms_Architecture.blend`, `FacilityRooms_Utilities.blend`, `FacilityRooms_Control.blend`: 개별 모델 카탈로그 원본.
- `L_Facility_Basement.blend`, `L_Facility_ControlFloor02.blend`: UE와 동일 배치의 편집 가능한 조립 씬.
- `Models/`: FBX. `Manifests/` 및 `model_manifest.json`: 치수·슬롯·충돌 규격.
- `References/`: 내장 ImageGen 레퍼런스와 정확한 프롬프트. 최초 콘크리트 관제실은 방향 변경 전 시안이며 최종 기준은 `ControlRoom_Attic_Reference.png`입니다.
- `Textures/`: 내장 ImageGen 공유 아틀라스와 숲 이미지. 로컬 필터로 이미지 결과를 대체하지 않았습니다.
- `Previews/`: Blender 렌더 및 실제 UE 화면 증거. 파일명 `UE_`만 Unreal 렌더입니다.
- `../../../..` 위 프로젝트 루트의 `Tools/FacilityRooms/`: 반복 사용 Blender 제작/배치 스크립트와 읽기 전용 검증기. 실행 시 Blender `--background --factory-startup`을 사용해 개인 애드온과 분리합니다.
- `fbx_validation.json`, `layout_validation.json`, `unreal_import_validation.json`, `unreal_reload_validation.json`: 검증 결과. UE 명령렛 종료 코드와 자산 검증 결과는 구별합니다. 기존 프로젝트 Niagara typed-element Registry ensure가 발생할 수 있습니다.

완료된 일회성 UE 임포터는 생성 에셋과 함께 첫 커밋한 후 제거했습니다. 최종 트리는 시작 시 자동 재생성하지 않습니다. 반복 사용 Blender 제작·배치 스크립트와 읽기 전용 검증·캡처 도구는 유지합니다.
