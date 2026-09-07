# BushSparse — 낮은 덤불 성긴형

기존 `Content/Nature`의 Bush, GrassLow, Flower, SimpleTree, Wood, RockBasic을
UE에서 읽고 메시/텍스처를 추출해 `ExistingNature_Reference.png`로 확인했다.
참조 시트는 형태 비교를 위해 각 모델의 최대 치수를 1.6 m로 정규화했다.
실제 크기/재질 설정은 `References/existing_asset_audit.json`에 기록되어 있다.
기존 Blender의 풀 단색, WoodCommon 및 SimpleTree 텍스처도 확인했다.

새 덤불은 7개 비대칭 줄기, 14개 말단 잎 군집, 총 63개 잎으로 구성한다.
가지 사이와 중심에 빈 공간을 남겼다. 꽃/낙엽/별도 뿌리 프랍은 포함하지 않는다.

## 산출물

- 원본: `TunaSweeper/SourceArt/Environment/ForestProps/BushSparse/BushSparse.blend`
- UE FBX: 동일 폴더 `Models/SM_BushSparse.fbx`
- 자체 제작 단색 팔레트: `Textures/T_BushSparse_Palette.png` (192 × 32, .blend와 FBX에도 내장)
- 프리뷰: `Previews/`의 Hero, Front, Back, Top, Repeated, 기존 참조 시트
- UE 목적지: `/Game/Nature/ForestProps/BushSparse/`의 SM_BushSparse, M_BushSparse, T_BushSparse_Palette

## 치수와 배치

- Blender 크기: X 123.273 cm × Y 128.171 cm × Z 72.260 cm. 원본 단위는 m.
- FBX: -Y forward / Z up, unit scale 적용, UE import scale 1.0. 실제 UE 축 대응은
  `unreal_reload_validation.json`에서 측정값을 확인한다. UE +X 북쪽, +Y 오른쪽, +Z 위.
- 피벗은 줄기 밑동 (0,0,0), 최저점 Z=0. 지면에 배치하고 울퉁불퉁한 곳은 0~1 cm 내린다.
- 권장 균일 스케일 0.85~1.15, yaw 자유 회전, 중심 간격 1.2~1.6 m.
  Repeated는 12개, 간격 약 1.3 m, 위치/회전/균일 크기 변주를 보여준다.
- 1,190 triangles, 749 원본 정점, 재질 1개, UV 2개. UE 정점은 UV/flat normal 경계에서 분리된다.
- 잎은 닫힌 입체 메시이며 재질은 Opaque, 단면 렌더링이다. 알파 카드/마스크/투명도 없음.
  뒷면도 실제 기하로 보인다. 충돌 형상 0, NoCollision, navigation data 비활성.
- 색상 UV0은 6개 평면 색상 영역 안에 충분히 들어간다. 팔레트 샘플링은 nearest/no mip:
  면 내부가 일정한 색이며 작은 비정방형 팔레트의 먼 mip에서 타 색상 혼합을 막는다.
  UV1은 별도 펼침이다. 정적 프랍, 바람/WPO 없음. 대량 배치는 인스턴싱과 프로젝트에 맞는
  cull distance를 적용한다. 실제 레벨/성능 설정/foliage type은 변경하지 않는다.

## 재생성/검증

저장소 루트의 PowerShell에서 Blender 4.5 LTS로 실행한다.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/BushSparse/build_bush_sparse.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/BushSparse/verify_fbx.py
powershell -ExecutionPolicy Bypass -File Tools/ForestProps/BushSparse/run_unreal.ps1
powershell -ExecutionPolicy Bypass -File Tools/ForestProps/BushSparse/run_unreal.ps1 -VerifyOnly
powershell -ExecutionPolicy Bypass -File Tools/ForestProps/BushSparse/run_render.ps1
```

Blender 검증: 유한 좌표/단위 노멀, 퇴화 면 0, 닫힌 기하, 0~1 UV 및 삼각형 UV 면적,
두 UV 채널, 단일 재질, FBX 새 프로세스 재로드 치수 오차 0, 내장 텍스처 .blend 재로드.
UE 검증: cm 치수, 측정한 축 변환, Z=0 피벗, UV 2개, 재질/텍스처 연결,
불투명/단면 설정, 충돌 없음과 navigation 설정을 재로드해 확인한다.

현재 worktree에는 게임 모듈 바이너리가 없으므로 UE 5.7의 disposable 콘텐츠 전용 검증
프로젝트를 Saved/BushSparseAudit에 만든다. 성공한 3개 애셋만 동일 /Game 경로로 실제
TunaSweeper Content에 복사하고 SHA256 동일성을 확인한다. 재로드는 실제 목적지에서
동일 바이트를 다시 가져와 수행한다. 기존 공용 애셋, BP, 맵은 수정하지 않는다.
이는 새 Static Mesh/재질의 엔진 검증이며 게임 플레이 검증은 아니다.
UE 실행은 AppData 엔진 캐시 쓰기가 허용되는 환경이 필요하다.

원본을 먼저 커밋하고 UE 애셋/UE 검증 산출물은 별도 커밋한다. Push하지 않는다.

## UE 측정 결과

UE 5.7.4에서 크기 123.273216 × 128.170807 × 72.259918 cm, 원본과 최대 오차
0.000002623 cm. 실제 축 대응은 UE (X,Y,Z) = Blender (X,-Y,Z) × 100.
렌더 정점은 3,477개이며 UV 2개, 재질 슬롯 1개다. 저장된 3개 .uasset의 SHA256을
실제 Content 폴더와 대조했다. 임포트 프로세스는 신규 재질 생성 전 조회 경고 2개,
오류 0개/종료 0이었다. 새 프로세스 재로드는 경고 0개, 오류 0개/종료 0이었다.
GPU 프리뷰는 `render_unreal.py`의 임시 월드에서 저장된 애셋을 읽어 생성한다.
실제 게임 맵/광원에서 촬영한 화면은 아니며 작업을 위해 게임 맵을 저장하지 않는다.
