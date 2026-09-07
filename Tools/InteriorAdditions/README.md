# Interior additions

사용자가 제공한 주방·침실·응접실·세탁실 화면과 기존 Agit 모델을 기준으로 제작한 추가 가구 29종이다. 기존 메시·머티리얼·맵을 교체하지 않는다.

## 결과 위치

- Unreal: `/Game/Environment/Bunker/InteriorAdditions/`
- 확인용 맵: `/Game/Environment/Bunker/InteriorAdditions/Maps/L_InteriorAdditions_Showcase`
- Blender·FBX·텍스처·프리뷰: `TunaSweeper/SourceArt/Environment/InteriorAdditions/`
- 전체 목록과 cm 치수: 위 경로의 `model_manifest.json`
- 프리뷰: `Previews/All_Catalog.png`, `Kitchen_Catalog.png`, `BathroomLaundry_Catalog.png`, `Furniture_Catalog.png`

카탈로그는 개별 모델을 알아보기 쉽도록 표시 크기를 정규화한다. 캡션은 실제 cm 치수이며, 카테고리별 `.blend` 및 Unreal 확인용 맵은 실제 크기를 유지한다. 런타임 상호작용이나 기존 레벨 배치는 추가하지 않는다. 모델은 문·서랍 등을 포함한 정적 메시다.

## 구성

| 구분 | 메시 |
| --- | --- |
| 주방 10종 | Refrigerator, SinkCabinet, StoveOven, PantryShelf, CanisterSet, ElectricKettle, CookingPot, FoldedNapkin, WindowCurtains, MilkCan |
| 세탁실·욕실 8종 | WashingMachine, LaundryShelf, LaundryHamper, Toilet, Bathtub, Washbasin, TowelStack, DetergentSet |
| 응접실·침실·식사 가구 11종 | DiningTable, Stool, Sofa, CoffeeTable, Bed, Nightstand, BedsideLamp, BedroomCabinet, RoundRug, OvalRug, FloorCushion |

모든 메시 이름은 `SM_InteriorAdditions_` 접두사를 사용한다. 주요 가구는 기존 메시의 실제 크기와 배치 배율을 반영했다. 소파는 기존 배율 1.216255, 티테이블은 3.1225, 욕조는 1.344926을 적용한 화면상의 크기에 맞췄다. 기존 X 방향을 향한 주방·좌변기 모델은 새 모델의 폭 X, 정면 -Y, 위 Z 기준으로 축을 정리했다. 별도 제작한 소품·러그·수납장 크기는 참고 화면의 비율에 맞춘 값이며 기존 메시와 일대일 치수 일치를 의미하지 않는다. 커튼은 폭·높이를 맞추고 벽에서 돌출되는 깊이를 줄였다.

측정 근거는 `References/existing_unreal_mesh_bounds.json`, `existing_bunker_component_scales.json`, `ExistingBlenderWorldBounds.json`, `BathroomScaleComparison.json` 및 기존 모델의 contact PNG에 보존한다.

## 이미지 생성과 모델링

레퍼런스 두 장과 4×4 Base Color 아틀라스는 내장 `image_gen`으로 생성했다. CLI/API 대체 경로는 사용하지 않았다. 최종 프롬프트는 `References/*.prompt.txt`와 `Textures/T_InteriorAdditions_Atlas.prompt.txt`에 있다. 원본 아틀라스는 도구가 반환한 1254×1254 PNG를 그대로 보존하며, UE에서는 월드 텍스처·sRGB·스트리밍·밉맵 및 power-of-two 빌드 설정을 적용한다.

16개 재질은 같은 이미지의 타일을 UV로 공유한다. 금속·거칠기 값은 각 재질의 상수로 정의했다. 기존 메시를 복사하지 않고 Blender Python으로 형상과 UV, FBX, UCX 충돌을 만들었다. 실제 속이 파인 싱크대·세면대·좌변기·욕조, 손잡이·수전, 버너 코일, 가구 다리·천 주름·봉제선 등이 포함된다. 욕조에는 참고 게임 화면에 맞춘 청록색 수면이 있다.

## 유지하는 제작·검증 도구

`common.py`와 `build_kitchen.py`, `build_bathroom.py`, `build_furniture.py`는 반복 제작용 Blender 소스다. `merge_manifest.py`는 카테고리별 목록을 병합한다. `render_catalog.py`는 저장된 Blender 모델에서 프리뷰만 렌더링한다. 원본 제작 파일과 기존 언리얼 에셋은 자동으로 재생성되지 않는다.

Blender 실행 예:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 4 --python-exit-code 1 --python 'Tools/InteriorAdditions/build_kitchen.py'
```

`verify_fbx.py`는 새 Blender 프로세스에서 FBX를 다시 열어 치수·피벗·UV·재질·삼각형·충돌을 확인한다. `verify_unreal.py`는 새 UE 5.7 프로세스에서 저장된 에셋과 기존 Agit/맵 해시를 읽기 전용으로 검사한다. 검증 JSON은 SourceArt 루트에 보존한다. UE 실행은 AppData DDC 쓰기 권한이 필요하다.

UE 에디터용 일회성 임포터와 실행 래퍼는 에셋과 함께 검증·커밋한 다음 즉시 제거하고 다음 커밋에 기록한다. 이후에는 저장된 `.uasset`/`.umap`을 사용한다. 해당 임포터는 Git 이력으로만 확인한다.

## 검증 결과

- 29개 FBX를 새 Blender 프로세스에서 재로드: 375,764 삼각형, 유한 좌표·비퇴화 삼각형·UV·재질·피벗·cm 치수·UCX 검사 통과.
- UE 5.7 가져오기 및 새 프로세스 재로드: 29개 메시, 16개 머티리얼, 공용 텍스처, 충돌·라이트맵 UV·치수·재질 연결 검사 통과.
- 별도 확인용 맵에 29종을 실제 배율 1로 전시하고 명칭·조명·카메라를 저장했다. 충돌체는 총 39개, 치수 최대 오차는 0.00002cm 미만이며 기존 Agit 에셋과 기존 맵 142개의 해시가 유지된다.
- 에셋 검사와 Python 실행은 성공했지만 UE 프로세스는 기존 `ATunaSweeperQuadrupedEnemyCharacter`의 Niagara 로딩 중 typed-element Registry ensure 때문에 종료 코드 1을 반환한다. 프로젝트 전체 C++ 빌드나 패키징 성공을 의미하지 않는다.
