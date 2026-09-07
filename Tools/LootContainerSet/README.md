# 루팅 상자 3종 · 분리형 몸통/뚜껑

목재 상자, 금속 케이스, 보급 상자를 각각 Body/Lid로 분리한 6개 정적 메시다. 기존 `ATunaSweeperLootContainerActor`의 뚜껑 동작에 맞춘 자산과 전용 검수 BP/맵을 제공한다. 기존 게임 맵·`BP_LootContainer`·`LootContainerTable.json`·루팅 규칙·저장 데이터는 변경하지 않는다.

## 산출물

원본 루트는 `TunaSweeper/SourceArt/Environment/LootContainerSet`이다.

- `LootContainerSet.blend`: 6개 원본 메시와 밝은 낮 검수 장면.
- `Models/SM_LC_{Wood,Metal,Supply}_{Body,Lid}.fbx`: 모델별 FBX와 UCX 충돌.
- `Textures/T_LSP_Atlas.png`, `T_MI_DirtMask.png`: 재사용 공유 텍스처.
- `References/ImageGen_Concept.png`, `concept.prompt.txt`: 이번 세트의 ImageGen 컨셉과 실제 프롬프트. **컨셉 이미지이며 실제 모델 렌더가 아니다.**
- `References/ReusedAtlas_ImageGen.png`, `atlas.prompt.txt`, `atlas-edit.prompt.txt`: 재사용 아틀라스의 ImageGen 원본과 실제 생성·편집 프롬프트.
- `References/ReusedSupplyCrate_Model.png`: 이전 보급 상자의 실제 모델 렌더로, 재사용 외형 비교 자료다.
- `Previews/Actual_Closed_Comparison.png`, `Actual_Open_Comparison.png`, `Actual_Closed_Topdown.png`, `Actual_Open_Topdown.png`, `Actual_Separated_Parts.png`: 이번 Blender 모델의 실제 비교·수직 탑다운·분리 부품 렌더.
- `Previews/Actual_<Wood|Metal|Supply>_<Closed|Open|Rear>.png`: 종류별 실제 모델 렌더.
- `Previews/UE_Actual_<Closed|Open>_<Oblique|Topdown>.png`, `UE_Actual_Open_Rear.png`: 저장 UE 자산을 다시 로드하여 촬영하는 검수 이미지.
- `model_manifest.json`: 메시별 삼각형·경계·충돌 및 조립 Transform의 기준 자료.

UE 메시 경로는 `/Game/Interaction/LootContainerSet/Meshes/SM_LC_<종류>_<Body|Lid>`다. 오브젝트 경로가 필요한 필드에서는 마지막 이름을 반복하여 `.SM_LC_<종류>_<Body|Lid>`를 붙인다.

## 정확한 연결 Transform

Blender는 m, UE는 cm다. +Z 위쪽, 전면 +Y, 후면 힌지 -Y, 힌지 회전축 X다. 프로젝트 월드 북쪽은 +X이며 상자의 전면 방향과는 별개다. Body 원점은 바닥 중앙, Lid 원점은 후면 힌지의 뚜껑 하단이다. Lid 지오메트리는 로컬 Y=0에서 +Y로 뻗는다.

| 정의 ID / 종류 | 닫힌 전체 X×Y×Z (cm) | Body / Lid 삼각형 | `LidPivotRelativeLocation` (cm) |
|---|---|---:|---|
| 7001 / Wood | 110×80×55 | 156 / 36 | `(0, -40, 46)` |
| 7002 / Metal | 135×95×60 | 160 / 52 | `(0, -47.5, 50)` |
| 7003 / Supply | 165×110×70 | 168 / 60 | `(0, -55, 61.25)` |

연결할 액터에서 `BodyMeshOverride`와 `LidMeshOverride`를 해당 종류의 메시 쌍으로 지정하고, `LidPivotRelativeLocation`을 위 표대로 설정한다. `VisualMesh`(Body)의 부모 기준 상대 Location/Rotation은 모두 0, Scale은 `(1,1,1)`이다. `LidMesh`는 `LidPivot`의 자식으로 유지하고 **상대 Location/Rotation 모두 0, Scale `(1,1,1)`**로 설정한다. 액터 원점을 원하는 바닥 위치에 배치한다.

닫힘 회전은 `FRotator(Pitch=0, Yaw=0, Roll=0)`, 열림은 `FRotator(Pitch=0, Yaw=0, Roll=-105)`다. 원본 메시와 FBX에 별도 오브젝트 회전·스케일을 더하지 않는다. 기본 열림 시간은 0.35초·EaseOut이다. `bUseSeparateCloseTiming=false`이므로 기본 닫힘도 열림의 0.35초·EaseOut을 사용한다. 별도 닫힘 시간을 켰을 때만 저장된 `CloseAnimationDuration=0.25`초·EaseIn이 적용된다.

### 기존 BP와 데이터에 적용할 때

`existing_bp_audit.json`의 실제 기존 BP 조사에서는 두 MeshOverride가 비어 있고 Body/Lid가 기본 Cube를 사용했다. Body는 Location `(0,0,25)`, Scale `(1.1,.8,.5)`, Lid는 Location `(0,40,0)`, Scale `(1.1,.8,.08)`, 기존 힌지는 `(0,-40,55)`였다. 새 메시에는 위의 원점과 Transform 계약을 적용한다.

현재 `RefreshContainerPresentation()`은 MeshOverride가 있어도 정의 행의 `mesh_scale`을 Body에 적용하고, `material_path`를 Body/Lid 양쪽에 적용한다. 따라서 **7001~7003의 실제 연결은 별도 승인된 적용 작업에서 각 행의 `mesh_scale`을 `[1,1,1]`, `material_path`를 `/Game/Environment/LabSupplyProps/Materials/M_LSP_Surface.M_LSP_Surface`로 함께 설정해야 한다.** Body를 정의 행으로 선택하는 방식이라면 `static_mesh_path`에도 해당 Body 경로를 사용한다. Lid 선택과 힌지 설정은 액터의 MeshOverride/피벗 속성에서 지정한다. 이 세트 제작에서는 기존 BP와 정의 행을 덮어쓰지 않았다.

기존 상호작용 거리·내용물·정의 ID·저장 정책은 연결 대상의 기존 설정을 따른다. 검수 BP의 DefinitionId 0을 실제 게임 상자 설정으로 복사하지 않는다.

## 전용 검수 장면

`/Game/Interaction/LootContainerSet/Review/L_LootContainerSet`에 세 종류를 한 개씩 배치한다. 클래스는 같은 폴더의 `BP_LC_Review_Wood`, `BP_LC_Review_Metal`, `BP_LC_Review_Supply`이며 실제 네이티브 루팅 액터를 직접 상속한다. `ContainerDefinitionId=0`, `ContentsId=0`으로 게임 정의 데이터와 분리하여 메시 조립과 기존 열림·닫힘 동작을 검사한다. 새 루팅 기능이나 테스트용 게임 규칙을 추가하지 않는다.

검증기는 저장 BP/맵 재로드 후 메시·Transform·재질·충돌을 확인하고, PIE 네이티브 Tick에서 열림/닫힘의 중간 프레임과 끝각을 검사한다. 각 종류의 0~105° 구간을 1° 간격, 106개 각도로 나누어 뚜껑 충돌 박스와 몸통 충돌 박스의 관통을 검사한다. 이는 샘플 각도에서의 충돌 검증이며 연속 충돌 검출을 뜻하지 않는다. Blender와 UE 수직 탑다운 이미지는 상판 판독용 검수 구도이며 게임 플레이 카메라 성능 측정 자료가 아니다.

## 재질·UV·비용

6개 메시 합계는 **632삼각형**, 메시마다 **1개 재질 슬롯**이다. Body는 바닥과 네 벽의 충돌 박스 5개, Lid는 박스 1개로 종류당 6개·총 18개다. 몸통의 열린 내부를 가로지르는 단일 충돌 헐을 사용하지 않는다. Nanite는 사용하지 않는다.

공유 재질 `/Game/Environment/LabSupplyProps/Materials/M_LSP_Surface`는 Opaque·단면이며 아틀라스 **2048²**와 공용 오염 마스크 **1024²**, 연결된 텍스처 샘플 **2개**를 사용한다. 기본 낡음과 홈·래치·라벨은 아틀라스에 포함되어 있다. UV0 `UVMap`은 4×4 아틀라스, UV1 `DirtUV`는 닫힌 조립 좌표 기반 공용 오염 좌표, 생성 라이트맵은 UV2다. 뚜껑 피벗 이동 전 조립 좌표를 UV1에 유지하여 닫힌 몸통/뚜껑 사이 오염 분포가 이어진다. Vertex `SurfaceParams`의 R은 금속성, G는 거칠기, B=0, A=1이다.

| Custom Primitive Data | 항목 | 공유 마스터 기본값 | 검수 인스턴스 |
|---:|---|---:|---:|
| 0 | DirtStrength | .35 | .18 |
| 1 | DirtScale | 1 | 1 |
| 2 | DirtOffsetU | 0 | 0 |
| 3 | DirtOffsetV | 0 | 0 |

몸통과 뚜껑에 같은 CPD 값을 적용한다. DirtUV는 조립 좌표(m)에서 수평면 `(x/2,y/2)`, 수직·경사 측면 `((x+.731*y)/2,z/2)`를 사용하여 금속 케이스의 45° 모따기에서도 UV 면적을 유지한다. Blender 프리뷰는 추가 오염 강도 .18을 사용하며, UE 공유 재질은 오염에 따라 색·금속성·거칠기를 혼합한다. 위 수치는 실제 지오메트리와 재질 구성 비용이며 드로콜·GPU 시간·FPS 측정값이 아니다.

## 재사용 출처

보급 상자는 기존 LabSupplyProps의 밀폐형 `SM_LSP_SupplyCrate` 외형·보호 모서리·아틀라스 0/1/2/11/12 타일을 재사용하여 큰 루팅 상자 치수에 맞추고 몸통/뚜껑과 내부를 분리했다. 이전 원형은 60×40×40cm·132삼각형이며 새 165×110×70cm 버전은 가로·깊이 확대와 높이 조정이 들어간다. 독립된 새 보급 상자 디자인을 중복 제작한 것이 아니다.

원본 모델·ImageGen 아틀라스 커밋은 `3933ce90`, UE 임포트는 `cc27d019`, 이전 일회성 생성기 제거는 `535e12a9`다. 필요한 공유 UE 파일 `M_LSP_Surface.uasset`, `T_LSP_Atlas.uasset`, `T_MI_DirtMask.uasset` 세 개만 해당 기존 경로로 선별 재사용한다. 오염 마스크는 ModularInteriorPreview의 기존 ImageGen 자산이다. 관련 원본·프롬프트는 이 세트의 `References` 및 출처 기록으로 보존한다.

기존 `Blender/Wood.blend`와 `Blender/textures/T_WoodCommon.png`, `/Game/Nature/Wood/SM_CrateA`, `SM_CrateB`도 확인했다. 저장 UE 메타데이터상 CrateA는 6,590삼각형, CrateB는 14,296삼각형·각 1슬롯이다. 새 Wood는 기존 목재 표현을 참고하고 공용 아틀라스의 wood 타일을 사용하여 분리형 192삼각형으로 구성한다. 기존 목재 원본과 게임 자산은 수정하지 않는다.

## 재현과 검증

저장소 루트의 PowerShell에서 실행한다. Blender 빌더는 이 세트의 원본·FBX·프리뷰를 재생성한다. 아래 UE 명령은 저장된 애셋/검수 장면을 읽고 보고서와 프리뷰를 출력한다.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --python Tools/LootContainerSet/build_models.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --python Tools/LootContainerSet/validate_source.py
./Tools/LootContainerSet/run_unreal.ps1 -Script verify_assets_driver.py
./Tools/LootContainerSet/run_unreal.ps1 -Script verify_review_driver.py -Render
```

최종 판정 자료는 원본 폴더의 `source_validation.json`, `unreal_import_validation.json`, `unreal_reload_validation.json`, `unreal_review_validation.json`이다. 각 보고서의 `passed`와 상세 검사 결과를 확인한다. 소스 검증은 새로운 Blender 프로세스에서 .blend와 FBX를 재로드하여 치수·피벗·매니폴드·면 방향·퇴화 면·삼각형별 UV/색상 보존을 검사한다. UE 검증은 새 프로세스의 저장 메시에서 GeometryScript로 실제 삼각형·UV·노멀·색상과 충돌 경계를 확인하고 공유 재질·CPD·텍스처 크기도 검사한다.

재현용 Blender 빌더와 읽기 전용 검증기는 유지한다. 일회성 UE importer는 생성 자산과 함께 검증·커밋한 직후 제거하는 다음 커밋으로 정리한다. 재임포트가 필요한 경우 해당 자산 생성 커밋의 `Tools/LootContainerSet/import_once.py`를 git 이력에서 확인한다. 최종 소스 트리와 에디터 시작 경로에 자동 재생성 기능을 남기지 않는다. 검증된 원본, UE 임포트, 생성기 제거를 구분하여 로컬 커밋하며 원격 push는 수행하지 않는다.
