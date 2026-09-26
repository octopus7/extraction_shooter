# SciFiSuit 런타임 에셋

이 폴더는 [편집 원본](../../SciFiSuit/)에서 평가한 가시성 설정을 적용한 FBX와 UE 검증 결과를 보관한다.

| 출력 | 구성 | 정점 / 삼각형 |
| --- | --- | --- |
| `SKM_LunaMk2_SciFiSuit_Base.fbx` | 가려진 몸체·Eye·Head·SideTail. Face 제외. | 2,980 / 5,674 |
| `SKM_LunaMk2_SciFiSuit_Clothing.fbx` | 새 의상 122개 파트 합본, 재질 5개. | 63,140 / 125,864 |

UE 경로는 `/Game/Characters/Player/LunaMk2/Outfits/SciFiSuit/`이며 이름은 FBX와 같다. 두 메시 모두 기존 `/Game/Characters/Player/LunaMk2/SKM_LunaMk2_Skeleton`을 참조한다. 본 130개의 이름·부모 관계·기준 변환을 실제 임포트 결과에서 비교했다. Base는 기존 PhysicsAsset과 `ABP_LunaMk2_WristPostProcess`를 사용하고, Clothing은 별도 물리·후처리 없이 Base의 Leader Pose를 따른다.

Base는 기존 `M_Luna_001`, `M_Luna`를 사용한다. 새 외장 재질은 `M_SciFi_White`, `M_SciFi_Joint`, `M_SciFi_Metal`, `M_SciFi_Cyan`, `M_SciFi_Pink`다. 모두 Skeletal Mesh 사용 플래그가 저장되어 있으며, Cyan 발광의 선형 RGB는 `[0.08, 1.4, 2.0]`이다.

FBX는 센티미터 좌표, `-Y Forward / Z Up`, 애니메이션·leaf bone 없이 출력했다. Base 출력 사본에서만 면적이 `1e-10 m²` 이하인 원본의 퇴화·극소 삼각형 24개를 정리했다. 편집 원본의 몸체·머리 좌표와 보존 컬렉션은 변경하지 않았다. Base의 원래 최대 7개 본 영향은 비율을 유지해 정규화했고 의상은 최대 4개다.

## 검증 기록

- `export_manifest.json`: 입력·FBX 해시, 구성·재질·범위.
- `fbx_validation.json`: 독립 재임포트, 같은 130본 변환, UV와 퇴화 삼각형 0개.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: 실제 UE 기준 변환, 범위, 재질 입력 노드, PhysicsAsset·후처리 참조 검사.
- `unreal_runtime_visual_validation.json`: A/B/C 39개 실제 포즈, 왼손 20.762cm 변화, 130본 Leader Pose 오차 0, 원래 얼굴 보존.
- `Previews/SciFiSuit_Runtime_A.png`, `_B.png`, `_C.png`: 실제 UE 전신 캡처. 원근 카메라를 사용해 직교 근평면 잘림을 피했다.
- `cleanup_validation.json`: 일회성 파일 정리와 보호 대상 원본 해시 확인.

손가락의 기존 변형 경고와 두피 안쪽 음영은 편집 원본 README에 기록했다. 기존 옷장 카탈로그·썸네일 등록은 별도 루트 작업에서 처리한다.
