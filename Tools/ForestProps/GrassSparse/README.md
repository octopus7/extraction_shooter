# GrassSparse — 성긴 풀 군락

독립 프랍 한 종. 기존 `Blender/SM_GrassLow.blend`의 폭 있는 뾰족한 잎을 기준으로, 작은 풀 5묶음을 비대칭으로 배치했다. 가운데와 묶음 사이에는 지형이 보인다. 꽃·바닥 흙 메시·돌은 포함하지 않는다.

## 산출물

기존 원본 관리 규칙에 맞춰 `TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse/`에 저장한다.

- `GrassSparse.blend`: 텍스처 내장 원본, 모델과 PREVIEW_ONLY 컬렉션 구분.
- `Models/SM_GrassSparse.fbx`: 잎 메시만 포함, 텍스처 내장.
- `Textures/T_GrassSparse_BaseColor.png`: 128² RGB, 4개의 넓은 색 띠와 완만한 밑동–잎끝 명암. 절차적 생성으로 점·노이즈 없음.
- `Previews/01_Hero.png`–`04_LowSide.png`: 앞·뒤·위·낮은 측면.
- `Previews/05_Repeated_16.png`: 임의 회전·크기의 16개 반복 배치.
- `model_manifest.json`, `fbx_validation.json`: 치수·형상 및 별도 Blender 프로세스 FBX 재로드 검사.
- `Reference/`: 기존 Bush, GrassLow, Flower, SimpleTree, Wood, RockBasic 조사 및 렌더. 참조용이며 새 모델로 배포하지 않는다.

## 치수·사용

- Blender 1단위 = 1m; UE 100단위 = 1m. 약 **130.87 × 96.34 × 34.31cm**.
- 피벗 `(0,0,0)`은 지면. 다섯 묶음의 밑동 모두 Z=0, 아래로 돌출하는 기하 없음.
- Blender FBX `forward=-Y`, `up=Z`, scale 1, 단위 변환 사용. UE 수치 매핑은 `(X,-Y,Z) × 100`; 게임 북쪽은 UE +X.
- 30장의 접힌 잎, 300 vertices / 300 triangles, 재질 슬롯 1개, 색상 UV 1개. 면의 경계는 얇은 잎에 의도된 열린 경계다.
- 불투명 양면 재질로 뒷면 표시. 알파 테스트·투명 카드 없음. 면 자체의 겹침 비용은 남지만 빈 사각 카드 픽셀을 그리지 않는다.
- 충돌 도형 0개, NoCollision, 내비게이션 데이터 없음, Nanite 미사용. 식생 인스턴스도 충돌과 내비게이션 영향을 끈 상태로 배치 권장.
- 평탄하거나 완만한 길 가장자리·돌 주변에 사용. 간격 1.1–1.5m, yaw 0–360°, 균일 scale 0.82–1.12부터 조절. 여러 밑동을 가진 단일 메시이므로 급경사에는 적합하지 않다. 지형에 0–1cm 정도만 묻어서 사용.
- 300삼각형이므로 별도 LOD는 제공하지 않는다. 먼 거리 cull은 실제 레벨에서 조절한다. 성능 벤치마크를 실행한 수치는 아니다.

## 재현

저장소 루트 PowerShell에서 실행:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/GrassSparse/build_grass_sparse.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/GrassSparse/verify_fbx.py
& Tools/ForestProps/GrassSparse/run_unreal.ps1
& Tools/ForestProps/GrassSparse/run_unreal.ps1 -VerifyOnly
& Tools/ForestProps/GrassSparse/run_unreal.ps1 -Render
```

UE 검사 프로젝트는 `TunaSweeper/Saved/GrassSparseAudit/`에 생성한다. 게임 DLL이 없는 독립 작업트리에서도 설치된 UE 5.7을 사용하며, Content junction으로 이 작업트리의 실제 Content를 마운트한다. 애셋은 `/Game/Nature/ForestProps/GrassSparse/`에만 저장한다. DDC도 검사 폴더 내부에 저장한다. 기존 자연물 해시를 임포트 전후 비교하며 기존 BP·맵·게임 모듈은 수정하지 않는다. 게임 전체 실행 검증을 대신하지 않는다.
