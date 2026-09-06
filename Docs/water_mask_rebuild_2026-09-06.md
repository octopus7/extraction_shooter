# 물 마스크 재구현 — 2026-09-06

작업 시작: 2026-09-06 22:17:39. 작업 브랜치: codex/water-mask-rebuild.

## 범위와 판단

사용자의 실패 판정을 기준으로 기존 StylizedWater를 새로 작성한다. 이전 자동화 통과 기록은 시각 품질의 증거로 사용하지 않는다. 기존 요구사항은 requests.md의 2026-08-19 20:46:11, 22:44:17, 2026-08-20 00:18:36, 00:37:08, 01:04:00과 questions.md 물 경계 기록에서 확인했다. 2026-07-06의 웅덩이 반사는 시간 스크롤 없는 가상 하늘 평면 반사 방식의 참고 자료다.

구현 표식: WATER_MASK_REBUILD. 하늘 실험 표식: WATER_SKY_PARALLAX_EXPERIMENT. 플러그인 전체는 Experimental로 표시한다. 디자이너 설정에는 코드 표식을 노출하지 않는다.

## 구조

- 기존 Masked / Single Layer Water 및 삼각형 선택 쇼어를 폐기한다.
- R 거리 / G 수심 마스크를 픽셀마다 읽는 Unlit Translucent 수면으로 변경한다. 전체 삼각형을 유지하고, 경계 feather와 foam/film은 연속 알파로 처리한다.
- 지형 맞춤은 명시적 편집 동작이다. 저장된 높이·깊이 배열로 수면을 재구축하며 게임 세이브 스키마를 바꾸지 않는다.
- 기본 물 재질은 하늘 애셋·하늘 shader include를 참조하지 않는다. 하늘 반사 전용 재질은 별도 폴더에 있다.
- 하늘 UV에는 Time이 없다. 반사 시선과 가상 평면 교차를 사용하며, 낮은 각도에서 부드러운 유한 거리 보정을 한다. 실제 스카이·Lumen/SSR 반사를 재현하는 기능은 아니다.
- 반투명 경계·DepthFade의 UE 5.7 동작 기준: [Epic transparency documentation](https://dev.epicgames.com/documentation/unreal-engine/using-transparency-in-unreal-engine-materials?application_version=5.7), [Epic depth expressions](https://dev.epicgames.com/documentation/unreal-engine/depth-material-expressions-in-unreal-engine?application_version=5.7).

## 전용 파일·참조 목록

모든 새 물 구현은 TunaSweeper/Plugins/StylizedWater 아래에 둔다.

| 위치 | 역할 |
|---|---|
| Source/StylizedWater/Public/StylizedWaterBodyActor.h | 배치 액터, 마스크·색상·지형·하늘 옵션 |
| Source/StylizedWater/Private/StylizedWaterBodyActor.cpp | 전체 메시, 지형 맞춤, 저장 재질 로드, MID 파라미터 |
| Source/StylizedWater/Private/StylizedWaterModule.cpp | 플러그인 shader 경로 등록 |
| Source/StylizedWater/StylizedWater.Build.cs | 런타임 모듈 의존성 |
| Source/StylizedWaterEditor/Private/StylizedWaterEditorModule.cpp | 기존 상단 메뉴를 통한 네이티브 액터 배치 |
| Source/StylizedWaterEditor/Private/Tests | 저장 애셋·재질·토폴로지 검사 및 실제 SceneCapture 렌더 |
| Source/StylizedWaterEditor/StylizedWaterEditor.Build.cs | 에디터/검증 의존성 |
| Shaders/Private/MaskWater.ush | 기본 물 픽셀 셰이딩 및 마스크 UV |
| Content/MaskWater/M_WaterMask | 하늘 의존성 없는 기본 재질 |
| Content/MaskWater/Masks/T_MaskLake, T_MaskBeach, T_MaskRiver | 새 1024² 거리·수심 마스크 |
| Content/MaskWater/Masks/T_MaskDemo0, T_MaskRaid0 | 두 기존 맵 지형을 샘플링한 1024² 거리·수심 마스크 |
| Shaders/SkyParallax/PaintedSky.ush | 시간 입력 없는 시차 반사 |
| Content/SkyParallax/M_WaterMaskSky, T_AnimeSky | 선택적 실험 재질·이미지 |
| Resources/SkyParallax | imagegen 원본·프롬프트 |
| Content/Review/M_ReviewGround, WaterMaskReview | 시각 검토용 재질·호수/해변/강 비교 맵 |
| Tools/verify_saved_water.py | 저장 애셋·맵 읽기 전용 검사, 결과는 Saved에만 기록 |
| StylizedWater.uplugin | 기존 프로젝트 플러그인 연결, Experimental 표기 |

프로젝트 외부 연결은 TunaSweeper.uproject의 기존 StylizedWater 활성 항목과 배치 맵 참조다. 기본 브랜치에 남은 다른 생성기가 검증 중 콘텐츠를 수정하지 않도록 TunaSweeperEditor.cpp에 SkipLegacyEditorAssetSetup 명령행 가드를 둔다. 이 가드는 애셋 생성기나 자동 재생성 경로가 아니다.

## 다른 작업과 통합

원본 저장 프로젝트의 미커밋 생성기 정리 결과는 복사하지 않았다. 이 브랜치에서 물 모듈의 과거 EnsurePluginAssets/OnEditorInitialized/재생성 플래그를 직접 제거했다. 원본 생성기 정리와 병합할 때 저장된 옛 6개 물 애셋을 로드하는 코드, 내부 BP 배치, 과거 palette 생성 연결을 되살리지 않는다. 원본 정리가 통합되면 SkipLegacyEditorAssetSetup 가드는 필요 없어져 제거할 수 있다.

과거 TunaSweeperPuddleSkyReflectionMaterial.cpp/.h 생성기와 공유 헤더 include, 일회성 시작 호출·재생성 플래그도 제거했다. 별도로 배치된 기존 웅덩이 반사 평면과 저장 재질, 취수 설비 및 /Game/Shader_Water 실험은 보존했다.

기존 벽 코핑 치수 불일치는 이 작업 범위가 아니며 관련 메시/테스트를 변경하지 않는다. 퀘스트/서사 저작 데이터와 접근 제한 ProductionPayload 저장소는 작업하지 않는다.

## 검증 및 커밋

### 생성기 포함 상태 검증

- UE 5.7 TunaSweeperEditor Win64 Development 빌드 성공(436.28초).
- 실제 DX12/SM6 에디터 자동화: AssetsAndTopology, RenderMatrix, SavedMapRenders 모두 Success. 2개 경고 없음, 1개 기존 맵 경고 동반, 실패 0; 프로세스 종료 상태 0.
- 저장한 새 패키지 10개를 다시 로드했다. 기본 재질의 애셋 의존성에는 하늘 폴더가 없다. 두 맵은 각각 네이티브 물 액터 1개, 수면 섹션 1개이며 ShoreOverlay 컴포넌트·옛 물 패키지 참조가 없다.
- 두 배치 맵의 지형 마스크 샘플은 각각 1,048,576/1,048,576 hit, 물 내부 샘플 66,164개. 저장 지형 맞춤은 각각 4,225/4,225개 정점이다. 기존 액터 위치·회전·크기(6000 × 4200 cm)를 유지했다.
- 기존 추적 콘텐츠 925개 해시 감사: 요청 대상 맵 2개 변경, 실패한 물 애셋 6개 제거, 나머지 917개 동일.
- 삭제한 전용 패키지: BP_StylizedWaterBody_Internal, MI_StylizedWater_CalmAnime, MI_StylizedWater_ShoreOverlay, M_StylizedWaterSurface, M_StylizedWaterShoreOverlay, T_WaterDepthGradient. 기존 Resources/SourceArt의 palette 원본·파생 PNG 3개도 제거했다.

RenderMatrix는 호수·해변·강의 근거리/원거리, 하늘 off/on, 카메라 이동, 지형 위 얇은 막과 낮은 시선각을 20장으로 검사한다. SavedMapRenders는 실제 두 맵을 2장으로 기록한다. 고정 카메라 하늘 T0/T3의 RGBA8 평균 절댓값 차이는 0.000000, 강 물결 T0/T2는 0.287244, 평평한 지형의 2×2와 64×64 수면 차이는 0.000886이다. 후자는 경계가 메시 셀 해상도에 종속되지 않는지 확인한다. 재질 컴파일 오류 0 및 전체 토폴로지도 별도로 검사했다.

### 렌더 관찰과 한계

확인한 이미지에서 청록색 얕은 물에서 짙은 수심색까지 이어지고, 해안 경계와 지형 위 막에 과거 삼각형 조각·불투명 흰 파편이 보이지 않았다. 하늘을 켜면 구름 무늬가 나타나고 카메라 이동 시 바뀐다. 시간만 흐를 때 하늘 무늬는 고정된다. 가는 물결 선은 별도로 흐른다.

검사는 1600 × 1000 SceneCapture, 고정 수동 노출이며 제어된 RenderMatrix 장면에서는 AA/모션 블러를 껐다. 화면 위 검정은 검토 장면의 빈 배경이다. 해변·강의 사각 도메인 끝이 보이므로 실제 배치에서 메시/마스크 범위를 충분히 확장하거나 연결해야 한다. 지형 맞춤은 수동 동작이며, 낮은 격자는 굴곡을 충분히 따라가지 못한다. 원본 하늘은 완벽한 반복 타일이 아니므로 mirror 주소 모드를 쓴다. 실제 맵 이미지는 기존 조명 아래 어둡게 보이며 게임플레이 전체 조명, 패키징, 다른 GPU와 플랫폼까지 검증했다는 의미가 아니다. 최종 미술 품질은 사용자가 아래 이미지와 에디터에서 판단할 수 있다.

| 비교 | 렌더 증거 |
|---|---|
| 근거리 경계 | [호수](Images/WaterMask20260906/Lake_Near_Off.png), [해변](Images/WaterMask20260906/Beach_Near_Off.png) |
| 선택적 하늘/카메라 이동 | [off](Images/WaterMask20260906/Lake_Far_Off.png), [on](Images/WaterMask20260906/Lake_Far_On.png), [이동](Images/WaterMask20260906/Lake_Far_Moved.png) |
| 강 | [하늘 on](Images/WaterMask20260906/River_Far_On.png) |
| 지형 위 얇은 막 | [근거리](Images/WaterMask20260906/Beach_TerrainFilm_Close.png), [낮은 각도](Images/WaterMask20260906/Beach_TerrainFilm_Grazing.png) |
| 실제 저장 맵 | [DemoRaidMap](Images/WaterMask20260906/DemoRaid_Migrated.png), [RaidMap](Images/WaterMask20260906/Raid_Migrated.png) |

전체 22장과 원본 자동화 보고서는 실행 후 TunaSweeper/Saved/WaterRebuild/Renders, Automation에 생성된다. 추적한 요약은 [verification.json](Images/WaterMask20260906/verification.json)이다.

기존 맵의 M_GarageDoorLED/M_GarageDoorMetal Nanite 사용 플래그 및 RecastNavMesh 경고는 그대로다. Python commandlet은 읽기 검사를 통과했지만, 기존 Niagara CameraShakeSourceComponent의 조기 TypedElement Registry ensure 때문에 프로세스 종료 코드는 1이다. RegionalGroundFog 기존 재질의 sampler 경고도 남아 있다. 이 항목을 물 검증 성공으로 덮어 기록하지 않았으며 관련 애셋을 저장하지 않았다.

### 두 연속 로컬 커밋

첫 커밋 **4c58b165**는 위 검증을 거친 저장 애셋·구현과 GenerateMaskWaterAssetsCommandlet.cpp/.h, Tools/one_shot_migrate_and_gallery.py를 함께 기록했다. 생성은 명시적 commandlet/스크립트 호출만으로 수행했으며 시작 자동 생성 연결은 없다. 바로 다음 커밋에서 이 세 파일과 전용 AssetRegistry/AssetTools/BlueprintGraph/Kismet/MaterialEditor/Projects 에디터 의존성을 제거했다. 런타임의 Projects/RenderCore는 shader 경로 연결에, 에디터의 ImageCore/RenderCore 등은 유지하는 실제 렌더 검사에 필요하다. 푸시는 하지 않았다.

생성기 제거 후 빌드 성공(14.28초), 2026-09-06 23:30:45 렌더 자동화 3개 Success(경고 동반 1개, 실패 0), 프로세스 종료 상태 0. 저장 애셋 10개와 맵 2개를 다시 로드했고 제거한 생성기 클래스가 더 이상 등록되지 않았음을 검사했다. 첫 커밋 이후 콘텐츠 변경은 없으며 917개 관련 없는 기존 패키지의 해시도 그대로다. [제거 후 검사 결과](Images/WaterMask20260906/verification_after_cleanup.json)를 별도로 보관한다. Python commandlet의 기존 ensure/종료 코드 1은 제거 전과 동일하다.

읽기 전용 재검사: UE Python commandlet로 Plugins/StylizedWater/Tools/verify_saved_water.py를 실행한다. 렌더 재검사: 일반 에디터에 `-dx12 -sm6 -SkipLegacyEditorAssetSetup -ExecCmds="Automation RunTests StylizedWater.MaskWater" -TestExit="Automation Test Queue Empty"`를 전달한다. 자동화 탭에서도 같은 접두사의 세 검사를 실행할 수 있다. 마지막에는 현재 워크트리의 일반 에디터에서 /StylizedWater/Review/WaterMaskReview를 열고 호수 검토 카메라로 이동했다(23:31:40 WATER_REVIEW_VIEW_READY 로그). 시작 후에도 저장 콘텐츠 해시는 동일했다.
