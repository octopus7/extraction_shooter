# GrassDenseShort — 짧고 촘촘한 풀 군락

- 원본: `TunaSweeper/SourceArt/Environment/GrassDenseShort/` (프로젝트의 기존 SourceArt 관리 규칙 사용).
- UE: `/Game/Nature/ForestProps/GrassDenseShort/`.
- 77.21 × 68.79 × 18.23cm, 지면 피벗 `(0,0,0)`. 비대칭 외곽 때문에 XY 바운드 중심과 피벗은 다르다.
- 43개 잎 묶음, 잎 215장, 정점 1,505개, 삼각형 1,290개, 재질 슬롯 1개.
- 불투명 양면 재질, 64×64 RGB 색상 텍스처 1장. 꽃·알파 카드·노이즈·바닥 판·충돌 없음.
- 1 UV 채널. 팔레트 재사용 UV 겹침은 의도적이다. 동적 조명용이며 베이크용 라이트맵 UV는 생성하지 않는다.

## 재현

프로젝트 루트 PowerShell에서:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 6 --python-exit-code 1 --python Tools/ForestProps/GrassDenseShort/build_grass.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 4 --python-exit-code 1 --python Tools/ForestProps/GrassDenseShort/verify_fbx.py
& Tools/ForestProps/GrassDenseShort/run_unreal.ps1 -Mode import
& Tools/ForestProps/GrassDenseShort/run_unreal.ps1 -Mode reload
```

`SM_GrassDenseShort.blend`는 텍스처를 pack하고 메시·조명·카메라·바닥·숨긴 25개 반복 배치 프리뷰를 포함한다. FBX에는 원점의 실제 메시 하나만 export한다. 재현 seed는 906413.

Blender 1 단위 = 1m, FBX unit scale 보존, Unreal 임포트 배율 1. `-Y forward / Z up` export와 UE scene conversion 조합은 Blender `(X,Y,Z)`를 UE `(X,-Y,Z) × 100cm`로 변환한다. 재로드에서 부호 있는 바운드로 검사한다.

워크트리의 게임 DLL 빌드에 의존하지 않도록 Saved 내부의 임시 Content-only UE 5.7 프로젝트를 사용한다. Content junction은 이 워크트리의 실제 TunaSweeper Content를 가리킨다. 새 폴더의 메시·재질·텍스처만 저장하며 기존 Nature 6개 폴더의 파일 해시를 검사한다. 기존 BP·맵을 로드하거나 저장하지 않는다. 게임 실행·레벨 배치는 본 작업 범위에 포함하지 않는다.

## 배치

- 기본 간격 45–60cm, yaw 0–360°, 균일 scale 0.88–1.12를 출발점으로 사용한다. 넓은 빈틈 없는 피복은 45–50cm 쪽을 사용하고 일부 간격을 흔든다.
- 지형과 맞닿도록 피벗을 지면에 놓는다. 경사에는 표면 법선을 따르게 하되 심한 비균일 스케일을 피한다.
- foliage / ISM / HISM용 재질 사용 플래그와 NoCollision 프로필을 저장한다. 잎 뒷면은 양면 렌더링으로 유지한다.
- 마스크 카드의 빈 영역 픽셀 비용은 없다. 실제 잎 겹침·양면 셰이딩 비용은 남으므로 과도한 중첩을 피한다.
- 25개는 LOD0 기준 32,250 삼각형이다. 단일 LOD, Nanite 비활성. 원거리 cull과 밀도는 실제 게임 카메라·목표 하드웨어에서 결정한다. FPS 예산 검증을 수행했다는 의미는 아니다.
- 원본의 반복 배치 카메라는 검토용이며 실제 게임 카메라와 정확히 일치하지 않는다.

## 스타일 및 검증

`Reference/existing_nature_style.png`는 실제 Bush / GrassLow / Flower / SimpleTree / Wood / RockBasic 메시를 같은 표시 크기로 정규화한 Blender 비교 렌더다. 충돌 메시를 제외했다. 직접 연결된 UE BaseColor 텍스처를 사용하고, Bush는 원본 base color, GrassLow는 기존 원본의 단색 녹색을 사용했다. 두 재질의 UE 연산 그래프 전체를 재현한 화면은 아니다. 기존 원본 `.blend`와 텍스처도 조사했다. 비교용 Flower의 꽃은 새 풀에 포함되지 않는다.

`Previews/01_hero`부터 `06_game_distance`까지 정면 사선·반대 사선·상면·측면·25개 반복·거리 축소 뷰를 제공한다. 팔레트는 잔점 없이 낮은 채도의 녹색 덩어리로 구성했다.

`model_manifest.json`, `fbx_reload_validation.json`은 치수·UV·법선·퇴화 면·재질·포장 텍스처·지면 피벗 검증 결과다. UE 결과는 `unreal_import_validation.json`, 별도 실행 재로드 결과는 `unreal_reload_validation.json`에 저장된다.
