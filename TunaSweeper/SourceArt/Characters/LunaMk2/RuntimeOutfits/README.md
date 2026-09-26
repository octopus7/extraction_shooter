# LunaMk2 런타임 의상 에셋

기존 메이드 캐릭터를 보존하면서 교복·정비사·운동복·토끼 잠옷·탐험가 의상을 교체하기 위한 파생 에셋이다.

## 구성

각 의상 폴더에 두 FBX를 보관한다.

- `SKM_LunaMk2_<ID>_Base.fbx`: 해당 Blender 프리뷰의 Mask를 적용한 몸체와 Eye·Head·SideTail. 토끼 잠옷에서는 SideTail을 제외한다. **Face는 포함하지 않는다.**
- `SKM_LunaMk2_<ID>_Clothing.fbx`: 두께 등 메시 수정자를 적용한 새 의상.

ID는 `SchoolUniform`, `MechanicOutfit`, `Sportswear`, `BunnyPajamas`, `AdventurerOutfit`이다. UE 경로는 `/Game/Characters/Player/LunaMk2/Outfits/<ID>/`이며 메시 이름은 FBX와 같다. 새 재질 29개와 교복 체크 텍스처 1개도 각 의상 경로에 있다.

원래 Blender 파일, 의상 전용 FBX, UE 본체·얼굴·스켈레톤·재질·애니메이션은 변경하지 않았다. 원래 몸체에서 발견한 중복·퇴화 면은 런타임 사본에서 정리했고, 본 영향의 상대 비율을 유지하여 가중치 합을 정규화했다. 몸체와 정비사 일부 파트는 원래의 최대 7개 영향을 유지한다.

## 연결 규칙

- 두 메시는 기존 `/Game/Characters/Player/LunaMk2/SKM_LunaMk2_Skeleton`의 130개 본을 사용한다. 새 본과 새 스켈레톤을 만들지 않는다.
- Base는 기존 `M_Luna`·`M_Luna_001`, `SKM_LunaMk2_PhysicsAsset`, `ABP_LunaMk2_WristPostProcess`를 사용한다.
- Clothing은 Base의 최종 포즈를 Leader Pose로 따른다. 별도 PhysicsAsset과 후처리 AnimBP는 없다. 토끼 귀가 본체보다 높으므로 의상의 자체 bounds를 사용한다.
- 기존 얼굴 컴포넌트와 얼굴 표정 6개는 유지한다. 기존 Face는 구 Luna의 178본 스켈레톤을 사용하므로 의상 메시와 합치거나 의상용 Leader Pose로 바꾸지 않는다.
- 새 의상 선택 시 기존 메이드 치마 컴포넌트를 숨기고, 메이드 복귀 시 원래 표시 상태를 복원한다.

## 내보내기와 임포트

FBX 좌표는 센티미터다. Blender 원본의 미터 좌표를 런타임 사본에서 100배로 변환하고 scene unit scale을 0.01로 설정하여 UE 루트 본에 100배 스케일이 남지 않도록 했다. 축은 `-Y Forward / Z Up`, leaf bone과 애니메이션 출력은 끈다. `Armature` 래퍼는 UE의 Blender FBX 처리에서 제거된다.

기존 스켈레톤을 지정해 임포트하며 `Update Skeleton Reference Pose`와 `Use T0 As Ref Pose`는 끈다. 재질 자동 생성과 PhysicsAsset 자동 생성도 끈다. 재질의 단색 PBR 수치는 `export_manifest.json`에 기록했다. 모든 새 재질은 `Used with Skeletal Mesh`를 켜 저장했다.

UE의 FBX importer가 Blender bind pose의 상대 행렬을 재구성한다는 경고를 출력했다. 재구성된 실제 UE 메시의 130개 local reference transform을 기존 본체와 비교하여 위치 오차 최대 0.00205cm, 회전 오차 0.00750도, scale 오차 0.00001 이내임을 확인했다. Base와 Clothing의 기준 변환은 서로 같다.

## 검증 결과

- `preimport_reference_comparison.json`: 기존 UE 본체를 FBX로 추출하여 Blender 원본과 임포트 전에 비교.
- `fbx_validation.json`: FBX 10개 재읽기, 130본·UV·정규화 가중치·삼각형 수·범위 검사. 퇴화 삼각형 0개.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: UE 5.7.4 임포트와 새 프로세스 재읽기. 기존 에셋/원본 해시 보존 및 재질 수치·텍스처·사용 플래그 검사.
- `material_usage_validation.json`: 새 재질 29개의 스켈레탈 메시 사용 플래그 저장.
- `unreal_runtime_visual_validation.json`: 각 의상에 타이틀 A/B/C의 39개 샘플을 요청하고 130개 본의 Leader Pose 전달을 비교. 위치·회전 차이 0. 실제 UE 에셋으로 임시 월드에서 전신 5장과 옷장 액터 1장을 캡처.
- `cleanup_validation.json`: 일회성 스크립트 제거 후 원본과 최종 산출물 해시 확인.

`Previews/`의 이미지는 게임용 UI 일러스트와 별개인 실제 UE 검증 캡처다. 천 물리, 새 LOD, 모든 전투 동작의 옷 관통 검사는 이 에셋 검증에 포함하지 않는다. 후속 모델 수정은 각 의상의 기존 Blender 원본에서 진행한다.

## Raincoat 추가

일곱 번째 착용 의상인 우비는 `Raincoat/` 아래에 별도 기록한다. 기존 다섯 교체 의상의 검증 기록은 유지한다.

- `/Game/Characters/Player/LunaMk2/Outfits/Raincoat/SKM_LunaMk2_Raincoat_Base`: Mask 적용 몸체·Eye·Head·SideTail, 4,290정점·8,099삼각형. Face 제외, 기존 재질·물리·손목 후처리 사용.
- 같은 폴더의 `SKM_LunaMk2_Raincoat_Clothing`: 의상·원피스·분홍 장화, 56,298정점·112,340삼각형, 새 재질 10개. Base의 Leader Pose 사용.
- [편집 원본 안내](../Raincoat/README.md), [FBX 기록](Raincoat/export_manifest.json), [재임포트 검사](Raincoat/fbx_validation.json), [UE 재읽기 검사](Raincoat/unreal_reload_validation.json), [실제 포즈·캡처 검사](Raincoat/unreal_runtime_visual_validation.json), [정리·해시 검사](Raincoat/cleanup_validation.json).

Raincoat는 동일한 130본 스켈레톤을 사용하며, 원래 목 장식은 Raincoat 사본의 Mask로 가린다. A/B/C 39개 UE 샘플은 손 위치가 실제로 변하는지도 검사해 정지된 포즈를 반복한 결과와 구분한다. `Raincoat/Previews/`의 A/B/C PNG는 실제 UE 메시로 만든 전신 캡처다.
