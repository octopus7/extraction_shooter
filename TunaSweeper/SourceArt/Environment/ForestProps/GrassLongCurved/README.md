# GrassLongCurved — 긴 곡선형 풀 군락

세 곳의 가까운 성장점에서 긴 잎 26장이 서로 다른 높이·방향으로 휘어지는 공용 환경 프랍이다. 꽃·줄기 씨앗·미세 얼룩 없이 넓은 녹색 면과 외곽으로 읽히도록 만들었다. 기존 BP, 맵, 기능, 퀘스트 및 공용 Nature 애셋은 수정하지 않는다.

## 파일과 재현

- `GrassLongCurved.blend`: 텍스처를 내부에 패킹한 미터 단위 원본. LOD0만 표시하며 LOD1/2는 숨겨져 있다. `PREVIEW_ONLY` 컬렉션은 FBX에 포함되지 않는다.
- `Models/SM_GrassLongCurved.fbx`: UE에 임포트할 본체. 나머지 두 FBX는 해당 메시의 LOD1/2용이다. 모두 텍스처가 내장되어 있다.
- `Textures/T_GrassLongCurved_BaseColor.png`: 128×128, 불투명 RGB. 네 개의 넓은 색 띠와 부드러운 뿌리–끝 변화만 사용한다.
- `Previews/`: Hero, Front, Back, Top, Repeated 렌더. 반복 프리뷰는 16개 인스턴스를 회전·크기 변주한 예시이며 게임 레벨 배치가 아니다.
- `model_manifest.json`, `fbx_validation.json`: 실제 치수, 폴리곤 수, 원본 및 별도 프로세스 FBX 재로드 검사.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: UE 임포트와 별도 프로세스 재로드 검사 결과. 임포트 완료 후 생성된다.

프로젝트 루트 PowerShell에서 Blender 4.5 LTS로 실행한다.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 4 --python Tools/ForestProps/GrassLongCurved/build_grass.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup -t 2 --python Tools/ForestProps/GrassLongCurved/verify_fbx.py
& Tools/ForestProps/GrassLongCurved/run_unreal.ps1
& Tools/ForestProps/GrassLongCurved/run_unreal.ps1 -VerifyOnly
```

고정 난수 시드 6072026. Blender 메시·UV·작은 팔레트 텍스처를 모두 스크립트로 생성한다. 원본 폴더는 기존 `TunaSweeper/SourceArt/Environment/` 규칙을 따른다. 원본을 먼저 커밋하고 UE `.uasset` 및 UE 검증 결과는 별도로 커밋한다.

## 크기와 배치

- 약 118×127×84cm. 정확한 수치는 manifest와 UE reload report를 따른다. 1 Blender m = 100 UE cm, UE import scale 1.0.
- FBX `-Y forward, +Z up`, UE `convert_scene / convert_scene_unit` 활성화. 실제 부호와 XY 변환은 UE 보고서의 `axis_mapping`에서 검증한다. 지면은 +Z, 월드 북쪽은 +X이다.
- 피벗은 세 뿌리 묶음의 중심 `(0,0,0)`이며 최저 메시 높이도 0이다. 경사면에서 필요하면 1~2cm만 묻힌다.
- 기본 간격 90~130cm, 균일 스케일 0.82~1.13, yaw 0~360°를 출발점으로 사용한다. 길 가장자리에는 60~85cm 스케일 높이로 혼합하면 좋다. 과도한 비균일 스케일은 피한다.
- 기존 `SM_GrassLow`는 UE에서 약 42cm 높이이며 직선 위주의 작은 날이다. `SM_GrassLowB`는 확대형 약 117cm이다. 새 프랍의 차별점은 높이만이 아니라 여러 방향으로 굽은 넓은 외곽이다.

## 렌더링과 비용

LOD0/1/2는 각각 988/374/140삼각형, 26/17/10장 잎이다. 모든 LOD는 외곽을 정의하는 주요 잎을 공유한다. UE 화면 크기 전환값은 1.0/0.20/0.075이며 실제 카메라에 따른 조정과 밀집 배치 성능 평가는 프로젝트에서 할 수 있다.

단일 `M_GrassLongCurved` 재질 슬롯, Opaque, Two Sided, Roughness 0.88, Specular 0.25. 열린 잎 표면의 경계는 의도된 것으로, 뒷면은 양면 재질로 표시한다. 실제 면만 있어 알파 카드의 빈 부분을 셰이딩하지 않지만 겹친 잎의 픽셀 비용과 양면 그림자 비용은 남는다. 노멀맵, 알파 마스크, 바람 WPO는 없다.

충돌 메시를 생성하지 않으며 BodySetup은 NoCollision, 각 LOD 섹션 충돌도 끈다. Nanite 및 메시 거리장 생성은 사용하지 않는다. UV0은 색상 띠를 의도적으로 공유하며 면적 0인 UV 삼각형은 없다. UE가 별도 UV1 라이트맵을 생성한다.

## 기존 스타일 조사

`References/`의 이미지는 기존 원본을 읽어 별도 렌더한 조사 자료이다. 원본 텍스처 경로만 메모리에서 재연결했다. Bush는 UE의 기존 메시·색상 텍스처를 Saved 아래로 읽기 전용 export한 후 충돌 형상을 제외하고 렌더했다. `unreal_reference_inventory.json`은 Bush, GrassLow, Flower, SimpleTree, Wood, RockBasic의 크기·UV·재질 경로를 기록한다.

게임 `store/screenshot/ScreenShot_battle.png`와 비교해 큰 잎 면, 나무의 절제된 올리브 녹색, 둥글게 단순화된 목재·바위 형태를 기준으로 잡았다. 기존 Wood의 갈색은 조사에만 사용했고 이 풀에는 목질 소품을 추가하지 않았다. Flower는 주변 잎의 면 구성만 참고했으며 새 프랍에는 꽃이 없다.
