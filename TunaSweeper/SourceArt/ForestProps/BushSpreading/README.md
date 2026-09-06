# BushSpreading — 낮은 덤불 넓게 퍼진형

길 가장자리와 바위 아래에 쓰는 비대칭 공용 덤불 1종. 기존 `Blender/` 및 `/Game/Nature/{Bush,GrassLow,Flower,SimpleTree,Wood,RockBasic}`의 원본, UE 메시 치수와 재질 연결, 텍스처를 조사했다. Reference 프리뷰는 원본 메시와 base color를 재구성한 비교 렌더이며 실제 게임 조명 스크린샷이 아니다. 꽃은 형태 비교에만 사용했으며 새 덤불에는 꽃이 없다.

큰 잎, 낮게 뻗는 가지, 절제된 올리브 녹색을 사용한다. 목질은 기존 Wood의 따뜻한 갈색에 맞췄다. 잎맥·잔점·노멀 텍스처 없이 면의 기울기로 큰 명암을 만든다. 중앙 잎 무리를 보강한 최종본의 hero/front/back/side/top 및 9개 반복 배치 렌더를 직접 확인했다.

| 항목 | 값 |
|---|---|
| Blender 공간 크기 X × Y × Z | 2.0895 × 1.3410 × 0.6613 m |
| 실제 폭 × 깊이 × 높이 | 208.95 × 134.10 × 66.13 cm |
| 삼각형 / 원본 정점 | 2,746 / 1,741 |
| 잎 / 가지 | 157 / 27 |
| 메시 / 재질 슬롯 / 텍스처 | 1 / 1 / 128×128 RGB 팔레트 1장 |
| UV0 / UV1 | 색상 팔레트 / 별도 lightmap UV |
| 피벗 | 지면 Z=0, 루트 근처 (0,0,0) |
| 잎 뒷면 | 두께가 있는 닫힌 메시, 불투명 단면 재질 |
| 충돌 | 없음. UE NoCollision, 내비게이션 데이터 없음 |

UE 5.7.4 실제 재로드 결과: X/Y/Z = 208.9521 / 134.1016 / 66.1343cm, Blender→UE 축은 `(X, -Y, Z)`, 최대 bounds 오차 0.000006cm 미만이다. 프로젝트 기준 UE +X는 북쪽, +Y는 동쪽이다. UV와 flat normal 경계에서 분리된 UE LOD0 정점 수는 8,036개다.

## 산출물 및 재현

- `SM_BushSpreading.blend`: 텍스처 내장, 미터 단위, 원본 메시 및 PREVIEW_ONLY 스튜디오 컬렉션.
- `SM_BushSpreading.fbx`: 원본 메시만 포함, `-Y` forward / `Z` up, FBX 단위 정보로 UE에서 ×100 cm 변환. Import Uniform Scale=1. UE 변환 축은 실제 로드한 bounds를 비교해 `unreal_*_validation.json`에 기록한다.
- `Textures/T_BushSpreading_Palette.png`: sRGB 팔레트, RGB만 사용. 모든 UV는 색상 셀 안쪽을 사용한다. UE에서는 128px의 작은 팔레트 색이 먼 거리에서 섞이지 않게 mip을 생성하지 않는다.
- `Previews/`: 다방향, 반복 배치, 기존 자연물 비교 렌더.
- `Previews/gameplay_distance.png`: C++ 기본 카메라 거리 12m, 하향 60°, 수평 FOV 70°, 1920×1080에서의 9개 반복 배치. 실제 게임 스크린샷은 아니며 BP override, 조준/대체 카메라 모드는 포함하지 않는다.
- `model_validation.json`: 원본 및 FBX 재로드의 치수·위상·노멀·UV·재질 검사.
- `blend_reload_validation.json`: 새 프로세스에서 저장된 blend의 UV 면적, UV 범위, 내장 PNG 일치 검사.

저장소 루트 PowerShell에서:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 3 --python-exit-code 1 --python Tools/ForestProps/BushSpreading/build_bush.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 2 --python-exit-code 1 --python Tools/ForestProps/BushSpreading/validate_source.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 2 --python-exit-code 1 --python Tools/ForestProps/BushSpreading/render_gameplay_readability.py
& Tools/ForestProps/BushSpreading/run_unreal.ps1 -InspectReferences
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 2 --python-exit-code 1 --python Tools/ForestProps/BushSpreading/inspect_references.py
```

Blender 4.5.12 LTS, 고정 seed 90637. FBX 재로드에서 원본과 삼각형 수·치수가 일치하며, 퇴화 면·비정상 노멀·열린 경계·비유한 UV가 0개다. UV 삼각형의 면적도 0보다 크다.

## 배치 안내

긴 방향을 길 가장자리에 맞추고 yaw를 섞어 반복감을 낮춘다. 기본 균일 스케일 0.85–1.15, 중심 간격 약 150–230cm를 출발점으로 삼는다. 연속 가장자리는 간격을 줄여 외곽 잎끼리 조금 겹치게 한다. 바위 아래에는 루트 중심을 바위 쪽으로 붙이고 잎이 길 쪽으로 나오게 회전한다. 지면 기울기는 약 10° 이내, 필요한 경우 1–3cm만 묻는다. 비균일 스케일과 과도한 밀집은 잎 겹침을 늘린다.

런타임 바람과 전용 LOD는 추가하지 않았다. 다량 배치는 인스턴싱과 배치 거리별 culling을 사용하고 실제 레벨의 밀도로 성능을 측정해야 한다. 불투명 입체 잎이므로 알파 카드 오버드로는 없지만 겹친 잎의 일반적인 래스터 비용은 남는다. 기본 LOD0을 유지하며 원거리 비용은 배치 측에서 조절한다. 레벨·BP·퀘스트·기능 변경은 없다.

## UE 검증 환경

이 작업 사본에는 컴파일된 TunaSweeper 모듈이 없어 UE 5.7의 콘텐츠 전용 검증 프로젝트를 `TunaSweeper/Saved/BushSpreadingValidation`에 생성한다. 기존 자연물은 비교용으로 복사하고 새 애셋만 `/Game/Nature/ForestProps/BushSpreading`에 생성한다. 최종 새 폴더만 본 프로젝트 Content로 복사한다. 다른 공용 애셋, BP, 맵은 저장하지 않는다.

```powershell
& Tools/ForestProps/BushSpreading/run_unreal.ps1
& Tools/ForestProps/BushSpreading/run_unreal.ps1 -VerifyOnly
& Tools/ForestProps/BushSpreading/run_unreal.ps1 -Render
& Tools/ForestProps/BushSpreading/copy_verified_assets.ps1
```

`unreal_import_validation.json`, `unreal_reload_validation.json`, `Previews/unreal_hero.png`은 임포트 단계의 별도 커밋에 포함된다. 실제 게임 레벨에서의 최종 조명·배치 성능 확인은 이 독립 에셋 검증 범위에 포함되지 않는다.

최종 UE 임포트·새 프로세스 재로드·DirectX 12 에디터 렌더 모두 종료 코드 0을 확인했다. `unreal_render_validation.json`은 새 이미지 생성과 정상 종료를 기록하며 최종 이미지를 직접 시각 확인했다. 전경의 잎·목질 색, 닫힌 잎의 뒷면, 비대칭 외곽과 지면 기준을 확인했다. 검증 장면의 조명은 게임 조명과 별개다. `project_copy_validation.json`의 SHA-256으로 본 프로젝트의 3개 uasset이 재로드한 파일과 동일함을 확인했다. 전체 게임 빌드·PIE·레벨 배치는 수행하지 않았다.
