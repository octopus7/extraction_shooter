# 관제실 V2 — Stone Vault

첨부 이미지의 석조 아치·목재 리브·빈티지 지도 콘솔을 반영한 추가 버전이다. 기존 목조 다락 관제실과 지하 시설은 그대로 유지한다. 인물과 두더지 캐릭터는 환경 모델링 범위에서 제외했다.

## 열기

- Unreal 5.7: `/Game/Environment/MoleControlV2/Maps/L_MoleControlV2_StoneVault`
- Blender 조립 장면: `L_MoleControlV2_StoneVault.blend`
- UE 실내 카메라: `MoleV2_InteriorCamera`. `MoleV2_ReviewCamera`는 외부 구조 확인용이다.
- 실제 UE 화면: [UE interior](Previews/UE_MoleControlV2_StoneVault_Interior.png)
- Blender 구조 확인: [cutaway](Previews/L_MoleControlV2_StoneVault.png), [top](Previews/L_MoleControlV2_StoneVault_Top.png), [interior](Previews/L_MoleControlV2_StoneVault_Interior.png)

6×6m 방, 바닥 높이 320cm, 벽 높이 240cm에 분리된 배럴 볼트 천장을 얹었다. UE 레벨에는 모든 벽과 천장이 실제로 존재한다. Blender cutaway/top 프리뷰만 검토 편의를 위해 일부 벽/천장을 숨긴다. Blender 프리뷰 조명은 검토용이며 UE 최종 조명과 동일한 렌더는 아니다.

## 모델 및 머터리얼

신규 19종: 셸 8종(돌벽·창벽·아치 문틀·목재 문짝·나무 바닥·해치 바닥·볼트 천장·끝벽), 콘솔/통신 3종, 생활 소품 8종(수납장·꽃무늬 갓 램프·회전 의자·화분·책·머그컵·커튼·꽃무늬 러그).

41개 모델 배치 중 사다리 1개는 기존 `FacilityRooms/Meshes/SM_FacilityRooms_Ladder300`을 참조한다. 나머지는 신규 모델이며 숲 카드 1개를 별도로 배치했다. 사다리 메시와 90×90cm 열린 해치는 포함하지만 등반·문 열기·레벨 이동 게임플레이는 추가하지 않았다. 문짝은 독립 메시로 닫힌 상태다.

`/Game/Environment/MoleControlV2/Materials`의 17개 의미별 머터리얼 인스턴스에서 `Tint` 색을 조절할 수 있다. 나무·돌·황동·철·분홍 천·크림 천·책 표지·지도·계기·램프 표시등·러그 등이 분리되어 있다. `Tint`는 원본 텍스처와 곱해지며 `Metallic`, `Roughness`, `EmissionStrength`, `Atlas`도 조절 가능하다. 러그는 독립 텍스처 override를 사용한다. 같은 재질을 공유한 물체는 함께 변경되므로 개별 변경이 필요하면 인스턴스를 복제해 해당 슬롯에 지정한다.

신규 FBX는 바닥 중심 피벗, cm 크기, UV0 아틀라스/UV1 라이트맵과 수작업 UCX 박스 충돌을 갖는다. 원본 합계 208,764삼각형/134 UCX, UE LOD0 합계 207,996삼각형/134 충돌체다. 모든 배치 모델은 `BlockAll`, 숲 카드만 `NoCollision`/그림자 제외다.

## 이미지 생성 및 빛

내장 ImageGen을 사용했다. [사용자 원본](References/User_Reference.png)을 바탕으로 인물 없는 [환경 레퍼런스](References/Environment_Reference.png)를 생성하고 [16칸 재질 아틀라스](Textures/T_MoleControlV2_Atlas.png)와 [원형 러그 이미지](Textures/T_MoleControlV2_Rug.png)를 별도로 제작했다. 정확한 프롬프트는 [prompts.md](References/prompts.md)와 [Rug.prompt.txt](References/Rug.prompt.txt)에 보존했다. 숲 이미지는 기존 FacilityRooms의 내장 ImageGen 결과를 재사용했다.

100×110cm 실제 창 개구부 뒤에 양면 Unlit 숲 임포스터를 설치했다. 창 밖 Movable SpotLight + ExponentialHeightFog/Volumetric Fog가 창틀 그림자와 입사광을 만든다. 광원·안개 액터는 `Lighting_WindowSunBeam`, `Lighting_WindowVolumetricFog`다. 따뜻한 램프와 부드러운 보조광을 함께 사용하며, 램프와 간접광 근사용 CeilingBounce는 그림자를 끈 확산광으로 처리했다. 창문 입사광의 볼륨 그림자는 켜져 있다. 볼륨 안개가 비활성화되는 낮은 그래픽 설정에서는 빛줄기가 보이지 않을 수 있다.

## 검증 및 유지 관리

- `fbx_validation.json`: 19 FBX 재임포트, 삼각형/퇴화 면/UV/재질/치수/134 UCX 및 문·해치·창 실제 레이 검사.
- `layout_validation.json`: 창/해치 개구부와 주요 동선 2개 검사.
- `Manifests/Shell_validation.json`, `Console_validation.json`: 각 제작 에이전트의 독립 재로드 검증.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: UE 저장·새 프로세스 재로드, 재질 연결·러그 override·실제 배치·카메라·빛/숲 검사. 기존 모든 맵 및 Agit/FacilityRooms 에셋 203개 해시 보존 확인.
- `Previews/unreal_viewport_capture.json`: 실제 1600×900 UE 캡처와 입력 패키지 해시 불변 증거. 시각 검토에서 커튼/라디오 간섭을 해소하고 최초의 어두운 실내 조명을 보정했다.
- `content_copy_validation.json`: 빌드 가능한 원본 프로젝트에서 작업 폴더로 옮긴 신규 콘텐츠 42파일의 SHA-256 일치 증거. 신규 MoleControlV2 폴더만 복사했다.

검증 Python은 통과했지만 UE 명령렛 프로세스는 기존 프로젝트 Niagara 초기화의 typed-element `Registry` ensure로 종료 코드 1을 반환한다. 이는 신규 자산 검증 실패와 구분해서 기록한다. 초기 FBX 임포트의 near-zero tangent 경고 이후 UE 빌드 설정에서 tangent 재계산·비 MikkTSpace·full precision UV를 지정했다.

반복 제작용 Blender 스크립트와 읽기 전용 검증기는 `Tools/MoleControlV2`에 있다. `build_shell.py`, `build_console.py`, `build_decor.py` → `merge_manifest.py`, `build_layout.py` → `build_scenes.py` 순서로 소스를 재생성한다. `verify_fbx.py`는 Blender background에서, `verify_layout.py`는 일반 Python에서 실행한다. `verify_unreal.py`는 UE Python 명령렛으로 실행한다. `capture_unreal.py`는 주석의 전용 unattended 렌더 에디터 프로세스에서만 실행하며 해당 프로세스만 종료한다.

일회성 UE 임포터는 생성 에셋과 함께 첫 커밋한 뒤 다음 커밋에서 즉시 제거한다. 자동 시작·재생성 경로는 추가하지 않았다. 원격 푸시는 하지 않는다.
