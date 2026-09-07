# 연구소·보급 창고 가구 12종

기존 회색 콘크리트·도장 금속 실내에 조합하는 정적 저폴리 소품 세트다. 문·서랍·계기판·라벨은 공유 아틀라스로 표현하며 수납, 연구, 루팅, 이동 기능은 추가하지 않는다. 기존 실내 기본 6종은 `c2b70edf`의 필요한 공개 경로를 변경 없이 재사용했다. 건축 확장 18종, 천장, 천장 보·기둥은 제작하지 않는다.

## 산출물과 위치

- 원본: `TunaSweeper/SourceArt/Environment/LabSupplyProps/LabSupplyProps.blend`
- 재현 스크립트: `Tools/LabSupplyProps/build_props.py`, `parts/*.py`
- 모델별 UE FBX: `TunaSweeper/SourceArt/Environment/LabSupplyProps/Models/SM_LSP_<Key>.fbx`
- **ImageGen 컨셉**: `References/ImageGen_Concept12.png`, 실제 프롬프트 `References/concept.prompt.txt`
- **ImageGen 텍스처 원본**: `Textures/ImageGen_Atlas_v1.png`, `ImageGen_Atlas_Final.png`; 실제 생성·편집 프롬프트 `atlas.prompt.txt`, `atlas-edit.prompt.txt`; 사용본 `T_LSP_Atlas.png`
- **실제 모델 렌더**: `Previews/Models_12_Sheet.png`, `Model_<Key>.png`, `Lab_Oblique.png`, `Lab_PlayCamera.png`, `Warehouse_Oblique.png`, `Warehouse_PlayCamera.png`
- 낡음 비교: `Previews/Wear_BaseOnly.png`, `Wear_AdditionalDirt.png`; 작업대 확대 비교 `CloseWear_Base.png`, `CloseWear_Dust.png`
- UE: `/Game/Environment/LabSupplyProps/Meshes`, `/Materials`, `/Textures`; 전용 맵 `/Game/Environment/LabSupplyProps/Maps/L_LabSupplyProps`

위의 References·Textures·Previews 경로는 원본 폴더 기준이다. 컨셉 이미지는 모델 검증 렌더와 구분한다. 수치의 원본은 `model_manifest.json`이며 검증 상태는 [VALIDATION.md](VALIDATION.md)에 기록한다.

## 크기와 조합

원본 단위는 m, UE는 cm다. +X 북쪽, +Y 동쪽, +Z 위쪽, 정면은 -Y다. 벽걸이 제어함만 후면 하단 중앙 피벗이며 몸체가 -Y로 돌출한다. 나머지는 XY 중앙·바닥 Z=0이고 오브젝트 변환은 단위 변환이다. FBX는 UE 좌표계에 맞춰 Y 반전과 면 방향 보정을 적용하며 독립 재로드 검증에서 이를 되돌린다.

| Key | 종류 | X×Y×Z (m) | 삼각형 | 단순 충돌 박스 |
|---|---|---|---:|---:|
| Workbench | 빈 금속 작업대 | 1.60×0.70×0.90 | 204 | 17 |
| Shelf | 개방형 철제 선반 | 1.40×0.60×2.00 | 288 | 24 |
| Cabinet | 낮은 수납장 | 1.20×0.60×0.90 | 36 | 3 |
| Cart | 이동식 카트 | 0.80×0.50×0.85 | 272 | 16 |
| Chair | 낮은 등받이 의자 | 0.48×0.52×0.80 | 156 | 16 |
| ControlBox | 벽걸이 제어함 | 0.60×0.20×0.80 | 24 | 2 |
| Analyzer | 대형 분석 장비 | 1.20×0.80×1.50 | 144 | 13 |
| Sink | 실험 싱크대 | 1.60×0.70×1.20 | 272 | 18 |
| SampleTray | 시료 용기·트레이 묶음 | 0.45×0.30×0.219 | 564 | 7 |
| Pallet | 빈 팔레트 | 1.20×1.00×0.15 | 240 | 20 |
| SupplyCrate | 밀폐형 보급 상자 | 0.60×0.40×0.40 | 132 | 1 |
| BoxBundle | 박스 적재 묶음 | 1.00×0.70×0.70 | 36 | 3 |
| 합계 | 12종 | | **2,368** | **140** |

모델마다 재질 슬롯은 1개다. 싱크대 전체 높이 1.20m는 수도꼭지 포함이며 상판은 작업대와 같은 Z=.90m다. 두 가구의 폭·깊이가 같아 옆으로 연결할 수 있다. 카트 상판 Z=.735m, 하단 선반 Z=.175m, 손잡이 상단 Z=.85m; 의자 좌면 Z=.45m다.

선반의 선반면 높이는 .12/.72/1.32/1.92m, 내부 폭×깊이 1.30×.50m, 단 사이 유효 높이 .545m다. 보급 상자 두 개가 한 단에 나란히 들어간다. 높은 선반은 두 샘플 모두 벽면에 둔다. 시료 트레이는 작업대·수납장·카트에 별도로 올리고, 싱크대에서는 수조를 피해 배치한다. 팔레트 상면 Z=.15m에 박스 묶음을 얹거나 보급 상자를 가로로 두 개 배치하고 .40m씩 적층할 수 있다. 빈 공간을 가로지르는 단일 충돌 헐 대신 부재별 박스를 사용한다.

## 공유 표면과 오염

`M_LSP_Surface` 하나를 사용하는 Opaque·단면 재질이다. ImageGen 기본 낡음이 포함된 2048² 아틀라스와 기존 `ModularInteriorPreview/Textures/T_MI_DirtMask` 1024² 마스크를 사용한다. 연결된 실제 텍스처 샘플 노드는 2개이며 일반 가구와 시료 병 모두 투명 셰이딩을 쓰지 않는다. 작은 병은 탑다운에서 투명 내부보다 실루엣·뚜껑·라벨이 중요하므로 불투명 도장 표현으로 비용을 줄였다.

UV0는 4×4 아틀라스와 타일 내부 .025–.975 여백, UV1 `DirtUV`는 평면 미터/2 좌표다. UE 생성 라이트맵 UV는 UV2다. 활성 버텍스 색상 `SurfaceParams`는 R=금속성, G=거칠기, B=0, A=1이다. 추가 오염은 마스크로 색과 거칠기를 혼합하며 넓은 데칼이나 투명도 마스크를 사용하지 않는다.

| UE Custom Primitive Data 인덱스 | 값 | 기본값 |
|---:|---|---:|
| 0 | DirtStrength, 추가 오염 강도 | .35 |
| 1 | DirtScale, 반복 크기 | 1 |
| 2 | DirtOffsetU, 분포 위치 U | 0 |
| 3 | DirtOffsetV, 분포 위치 V | 0 |

## 재현과 읽기 전용 UE 검증

저장소 루트에서 실행한다. Blender 작업은 원본·FBX·프리뷰를 재생성하며 기존 UE 패키지를 자동 변경하지 않는다.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --python Tools/LabSupplyProps/build_props.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --python Tools/LabSupplyProps/validate_source.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --python Tools/LabSupplyProps/render_details.py
./Tools/LabSupplyProps/run_unreal.ps1 -Mode Assets
./Tools/LabSupplyProps/run_unreal.ps1 -Mode Map
```

UE 에셋은 일회성 에디터 생성 후 저장된 패키지를 사용한다. 생성 애셋과 생성기를 함께 검증·커밋한 다음, 생성기와 진입점을 제거하는 별도 커밋으로 정리한다. 최종 유지 명령은 위의 읽기 전용 `Assets`/`Map` 검증이며 시작 시 재생성을 하지 않는다. 모델 원본 커밋과 UE 임포트 커밋은 분리한다.
