# BushRound — 낮은 둥근 덤불

담당 모델 1종. 꽃·게임 로직·퀘스트·BP·맵 배치 변경 없음.

원본 관리 관례에 따라 `TunaSweeper/SourceArt/Environment/ForestProps/BushRound/` 사용.
`BushRound.blend`에 모델과 128×128 팔레트 텍스처가 내장되어 있으며 `Models/SM_BushRound.fbx`, `Textures/T_BushRound_Palette.png`도 제공한다.

## 형태와 배치

- 비대칭 7개 잎 덩어리, 둥근 8각 잎 133장, 큰 녹색 면과 갈색 가지. 잔점·꽃·미세 텍스처 없음.
- Blender 치수 X/Y/Z = **152.82 × 129.04 × 72.73cm**. 지면 접점 피벗 `(0,0,0)`.
- 2,408 삼각형 / 1,498 지오메트리 정점. 닫힌 잎 입체로 뒷면도 표시; 불투명 1개 재질 슬롯. 알파 마스크·투명 카드·충돌·Nanite 없음.
- UV0은 색 팔레트용 의도적 중첩, UV1은 개별 아일랜드의 lightmap UV. UV0의 중첩을 잘못된 lightmap UV로 사용하지 않는다.
- 독립 배치 중심 간격 140–180cm, 군락은 110–140cm. 균등 스케일 0.85–1.15와 무작위 yaw 권장. 피벗을 지면에 두고 경사에서는 1–3cm만 묻어 접지한다.
- 다량 배치 시 Foliage/HISM과 프로젝트의 거리 컬링 정책 적용. 별도 LOD는 포함하지 않으며 실제 레벨의 밀도·그림자 비용은 배치 후 프로파일링한다. 반복 프리뷰는 16개를 132cm 기준으로 배치한 예시다.
- UE 규칙: +X 북쪽, +Y 오른쪽, +Z 위, 1m=100cm. FBX는 -Y forward/+Z up, 단위 변환 활성화, UE import scale=1. 실제 축 대응과 cm 오차는 `unreal_reload_validation.json`에서 확인한다.
- UE 5.7.4 재로드 측정: **152.8166 × 129.0353 × 72.7254cm**, Blender `(X,Y,Z)` → UE `(X,-Y,Z)`, 최대 경계 오차 0.0000043cm. UV seam과 flat normal 분리 후 UE 정점 수는 7,124개다.

## 재현

프로젝트 루트 PowerShell에서:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b -t 4 --factory-startup --python-exit-code 1 --python Tools/ForestProps/BushRound/build_bush.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b -t 4 --python-exit-code 1 --python Tools/ForestProps/BushRound/verify_source.py
./Tools/ForestProps/BushRound/run_unreal.ps1 -Mode Import
./Tools/ForestProps/BushRound/run_unreal.ps1 -Mode Reload
./Tools/ForestProps/BushRound/run_unreal.ps1 -Mode Preview
```

Blender 4.5 LTS 스크립트는 seed 90631로 동일한 지오메트리를 생성한다. 원본 저장 후에만 프리뷰 바닥·조명·복제본을 추가하므로 FBX와 원본에 검수 오브젝트가 섞이지 않는다.

UE 5.7 검증은 `TunaSweeper/Saved/BushRoundReview/`의 임시 콘텐츠 전용 프로젝트를 사용한다. Content junction은 현재 체크아웃의 Content를 가리키며 저장은 `/Game/Nature/ForestProps/BushRound/` 안으로 제한한다. 게임 모듈 빌드나 기존 맵 변경 없이 실제 `.uasset`를 만들고 별도 프로세스에서 재로드한다. Unreal 캐시 접근 때문에 샌드박스 밖 실행이 필요할 수 있다.

기존 6개 Nature 그룹은 `existing_assets.json`, Blender 원본과 Bush FBX 비교는 `existing_blender.json` 및 `Previews/ExistingStyle.png`에 기록했다. 비교용 원본은 각자 정규화했으므로 그 그림은 크기 비교 자료가 아니다. `SimpleTree`와 `RockBasic`은 원본의 대표 mesh object, Bush는 UE에서 내보낸 기존 mesh이다.

## 검증 산출물

- `model_manifest.json`: 치수, 폴리곤, UV 범위·퇴화, 노멀, manifold, FBX 재로드 비교.
- `source_reload_validation.json`: 저장된 .blend 재오픈, 텍스처 내장·알파, 개별 닫힌 입체의 바깥쪽 노멀, SHA-256.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: 실제 UE cm 치수, 축, UV, 재질 연결, 충돌, 재로드.
- `unreal_display_validation.json`, `Previews/BushRound_UE_Reload.png`: 저장하지 않은 검수 월드에서 RHI로 실제 재로드 애셋 렌더.
- `Previews/BushRound_Hero.png`, `Back`, `Front`, `Top`, `Repeated`: 다방향·반복 배치 렌더.

2026-09-06 검수: Blender 다방향 4장·반복 배치 1장과 UE RHI 최종 렌더를 직접 확인했다. UE import/reload 프로세스는 종료 코드 0으로 통과했고, 경고는 검수 프로젝트 Content junction의 실제 경로 차이였다. UE 프리뷰는 새로 로드한 재질의 셰이더 컴파일을 완료한 뒤 촬영하며, 실제 사용 텍스처가 `T_BushRound_Palette`임을 보고서에 기록한다. 첫 실행에는 셰이더 컴파일로 수 분 걸릴 수 있다. 검수 월드와 조명은 저장하지 않는다.

API 참고: [Blender FBX export](https://docs.blender.org/api/main/bpy.ops.export_scene.html), [UE 5.7 StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.7).
