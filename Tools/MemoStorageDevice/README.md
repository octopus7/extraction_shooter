# 공용 메모 저장장치

손바닥 크기의 청색 데이터 카트리지 1종. 기존 메모 텍스처의 청색·흑연색·청록 팔레트를 이어가며, 보호 모서리·돌출 커넥터·상태 표시등으로 읽힌다. 메모 본문이나 ID가 없는 공용 프랍이다.

추가된 **936삼각형 저폴리 버전**은 [LOW_POLY.md](LOW_POLY.md)를 참고한다. 아래 규격은 기존 6,012삼각형 원본 기준이다.

## 산출물

원본 루트: `TunaSweeper/SourceArt/Memo/StorageDevice/`

- `SM_MemoStorageDevice.blend`: Blender 4.5 LTS 원본, 완성 메시와 별도 프리뷰 컬렉션.
- `Models/SM_MemoStorageDevice.fbx`: UE용 메시 1개 + UCX 충돌 1개.
- `Previews/`: Hero, Rear, Top, Bottom, Connector 및 12m 카메라 비교 렌더.
- `model_manifest.json`, `fbx_validation.json`: 치수·슬롯·토폴로지 및 재로드 검증.
- `existing_state_audit.json`: 현재 UE CDO, 파생 BP, 기존 재질·텍스처와 직렬화 참조 조사.
- `camera_readability.json`: 기본 네이티브 카메라를 근사한 화면상 크기.

실제 UE 애셋 경로:

`/Game/Meshes/Props/MemoStorageDevice/SM_MemoStorageDevice`

새 재질은 같은 폴더의 `Materials/` 아래에 있다. 기존 `/Game/Interaction/M_MemoStorageDevice`와 `T_MemoStorageDevice`를 수정하거나 덮어쓰지 않는다. 네 가지 상수 PBR 재질만 사용하므로 새 텍스처 의존성은 없다. 기존 이미지의 미세 스크래치를 작은 모델에 다시 입히지 않았다.

## 제작 규격

| 항목 | 값 |
| --- | --- |
| 실제 크기 | 12 × 7 × 2.0705 cm |
| 축 | +X 커넥터 방향, Z 위, Y 좌우 대칭 |
| 피벗 | 본체 바닥 중앙 (0,0,0); 커넥터 때문에 바운드 중심 X는 +0.5cm |
| 바운드 cm | X -5.5~6.5, Y -3.5~3.5, Z 0~2.0705 |
| 원본 단위 | Blender meter, 제작 함수 입력 cm, FBX 단위 변환 포함 |
| 메시 | 6,012 삼각형, 3,088 정점, 41개 닫힌 분리 부품을 한 메시로 결합 |
| UV | 0~1 범위 Smart UV, 통합 패킹; UE lightmap UV1 생성, 해상도 64 |
| 슬롯 0 | M_MemoDevice_Shell — 청색 외장 |
| 슬롯 1 | M_MemoDevice_Guard — 짙은 보호재 |
| 슬롯 2 | M_MemoDevice_Metal — 커넥터·금속 테두리 |
| 슬롯 3 | M_MemoDevice_Status — 청록, emissive 2 |
| 충돌 | 전체 바운드를 감싼 보수적인 UCX box 1개 |

부품 접합부 내부에는 의도적인 교차 면이 있다. 각 부품은 닫힌 솔리드이며, 3D 출력용으로 불리언 결합된 단일 수밀체는 아니다. 애니메이션 없는 소품이므로 LOD0 하나와 일반 Static Mesh 렌더링을 사용한다. Nanite는 끈다.

## 연결 안내

현재 네이티브 메모 CDO는 엔진 Cube와 기존 메모 재질을 사용한다. UE 레지스트리와 콘텐츠 직렬화 조사에서 이 클래스의 파생 BP 및 직접 저장된 메모 액터 참조가 없었고, 공개 데모 `MemoSpawns.json`은 빈 배열이었다. 조사 시점의 기존 텍스처는 1254×1254, 기존 저장 재질은 단면이었다. C++ 재생성 루틴의 two-sided 설정과 저장 애셋 상태가 다르므로 실제 조사 보고서를 기준으로 했다.

사용자가 메모 BP 또는 스폰 설정을 연결할 때:

1. `Visual Mesh Asset`에 새 Static Mesh를 지정한다.
2. `Visual Material Asset`에 `/Game/Meshes/Props/MemoStorageDevice/Materials/M_MemoDevice_Shell`을 명시한다. 생성자가 컴포넌트 슬롯 0에 기존 재질을 설정하므로 이 항목을 비우는 것만으로 기존 오버라이드가 지워지지 않는다.
3. `Visual Scale`은 **(1,1,1)**, 액터 스케일도 (1,1,1)을 기준으로 한다. 기존 **(0.85,0.55,0.08)** 값은 100cm 큐브용이어서 새 메시를 찌그러뜨린다.
4. `Visual Relative Location`은 (0,0,0)으로 시작한다. 바닥 피벗이므로 액터 위치를 실제 지지면에 놓는다. 기존 큐브는 중심 피벗이므로 교체 배치 시 Z를 확인한다.
5. 슬롯 1~3은 메시 기본 재질을 유지한다. 슬롯 0에만 새 Shell 재질을 적용한다.

JSON 스폰에서는 `visual_mesh`, `visual_material`, `visual_scale`, `visual_relative_location`에 위 값을 명시한다. `visual_material` 생략/빈 문자열은 서브시스템이 기존 메모 재질로 대체한다. 이 작업에서는 BP·맵·메모/퀘스트 작성 데이터를 변경하지 않는다.

기본 카메라(거리 12m, pitch -60°, 수평 FOV 70°, 1920×1080)를 Blender에서 근사하면 스케일 1은 약 **8×13픽셀**, 스케일 3은 약 **24×38픽셀**이다. 커넥터 접점 같은 디테일은 근접 관찰용이며 멀리서는 외장 색·테두리·표시등만 남는다. 실물 크기 배치는 기존 상호작용 마커와 함께 사용한다. 필요하면 `Visual Scale=(3,3,3)`을 검토할 수 있지만 이 경우 36cm의 의도적 과장 크기다. 비교 이미지는 UI 마커가 없는 Blender 렌더이며 실제 게임 화면 검증을 대신하지 않는다.

## 재생성

저장소 루트 PowerShell에서 실행한다. 경로가 달라도 스크립트가 저장소 루트를 계산한다.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/MemoStorageDevice/build_memo.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/MemoStorageDevice/verify_fbx.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/MemoStorageDevice/render_camera_check.py
& Tools/MemoStorageDevice/run_unreal.ps1 -Mode Audit
& Tools/MemoStorageDevice/run_unreal.ps1 -Mode Import
& Tools/MemoStorageDevice/run_unreal.ps1 -Mode Verify
```

UE 실행에는 빌드된 TunaSweeperEditor 모듈과 UE 5.7이 필요하다. 명령행에서만 Python/EditorScripting 플러그인을 켜며 `.uproject`는 바꾸지 않는다. FBX는 legacy importer를 명시하고 크기 보정 없이 1.0으로 임포트한다. DDC는 작업트리 내 폴더와 엔진의 `InstalledNoZenLocalFallback`을 사용한다.

UE 시작 시 기존 Niagara `NE_PostProcess`의 Typed Element Registry ensure가 관찰되었다. 이 오류는 모델 생성 전 읽기 전용 조사에서도 발생했으며 명령행 종료 코드를 1로 만든다. 래퍼는 이를 성공으로 숨기지 않는다. Python 완료 표시·새 검증 JSON과 프로세스 종료 코드는 구분해서 확인한다. 실행 로그는 `TunaSweeper/Saved/MemoDevice_*.log`에 있다.

## 완료 검증

- 원본 커밋: `e602d949`. Blender 4.5.12 LTS로 생성·렌더·독립 FBX 재로드 완료. Hero/Rear/Top/Bottom/Connector와 카메라 거리 비교를 실제 시각 확인했다. 면 겹침과 과다 노출을 수정한 최종 렌더다.
- UE 5.7.4 `TunaSweeperEditor Win64 Development` 빌드: 성공, 종료 코드 0.
- Static Mesh 1개, 새 재질 4개 임포트 완료. 별도 UE 프로세스에서 저장 애셋 재로드 검증 통과. `unreal_import_validation.json`, `unreal_reload_validation.json` 참고.
- UE 바운드 최대 오차 **0.000000462cm**, 슬롯 4개 및 PBR 값·발광값, UCX 충돌 1개, UV0와 lightmap UV1 생성 설정 확인. UE 정점 4,897개는 노멀/UV 경계에서 분리된 값이다.
- 임포트·재로드 과정에서 보호한 기존 맵·BP·메모 파일 72개 해시가 동일했다. 게임 내 연결과 최종 플레이 화면 검증은 수행하지 않았다.
- 두 UE 명령은 Python 오류 없이 완료됐지만, 기존 Niagara 시작 ensure로 프로세스 종료 코드가 **1**이었다. `unreal_execution_Import.json`, `unreal_execution_Verify.json`에서 구분해 기록했다.
- `open_in_editor.py`는 이미 임포트된 메시를 Content Browser와 Static Mesh Editor에서 열기만 한다. 에디터 Python 콘솔에서 `exec(open(r'전체경로/Tools/MemoStorageDevice/open_in_editor.py', encoding='utf-8').read())`로 재사용할 수 있다.
