# 옷장과 의상

## 동작

벙커의 옷장 액터와 상호작용하면 의상 목록이 열린다. 의상을 선택해 전신 이미지를 확인한 뒤 착용한다. 기존 메이드, 교복, 정비사, 운동복, 토끼 잠옷, 탐험가, 우의의 7개 의상을 지원한다. 외형 변경은 능력치나 인벤토리에 영향을 주지 않는다.

상호작용 거리 밖으로 이동하거나 옷장이 제거되거나 캐릭터가 사망·탑승하면 창을 닫는다. 착용 요청에서도 같은 조건을 재검사한다. ESC와 닫기 버튼으로 게임 조작에 복귀한다.

## 데이터 경계

- 착용 의상 ID는 각 게임 세이브에 저장한다. 예전 세이브의 빈 ID 또는 삭제된 ID는 기본 메이드로 복원한다.
- 해금 목록은 세이브 슬롯과 별개인 전역 저장 데이터다. 새 게임, 슬롯 전환, 슬롯 삭제로 해금 기록을 지우지 않는다.
- 현재는 전체 해금 예외를 켠다. 이 예외는 영구 해금 목록에 모든 의상을 기록하지 않는다. 예외를 끄면 실제 해금 기록과 기본 의상만 사용할 수 있다.
- 알 수 없는 ID, 잠긴 의상, 에셋 로드 실패는 착용 상태를 변경하지 않는다. 저장 실패 시 성공으로 표시하지 않는다.

## 에셋과 표시

`/Game/Characters/Player/LunaMk2/Outfits/DA_LunaMk2_Outfits`가 의상 ID, 표시 이름 키, 썸네일, 몸체 메시, 의상 메시를 연결한다. UI 이미지는 `/Game/UI/Wardrobe/T_UIOutfit_<ID>`이며 원본 투명 PNG의 2:3 비율을 유지한다. 목록은 3열 그리드와 세로 스크롤을 사용하며 일곱 번째 우의 카드는 세 번째 행에 표시된다. 키보드로 화면 밖 카드를 선택하면 해당 카드가 보이도록 스크롤한다.

새 의상은 기존 130본 skeleton을 사용한다. 본체는 해당 의상의 몸체 가림을 실제 메시로 반영하고, 의상 컴포넌트는 본체 포즈를 따른다. 토끼 잠옷의 본체만 양갈래를 숨긴다. 별도 얼굴 메시·표정, 애니메이션, 기존 물리는 보존한다. 메이드로 돌아오면 원래 본체·재질·치마를 복원한다. 우의는 내린 후드의 노란 우비, 빨간 리본의 아이보리 원피스, 분홍 장화로 구성하며 기존 얼굴과 체형을 유지한다.

표시 문구는 `Content/Data/UITextStrings.csv`의 `ui.wardrobe.*`, `ui.interaction.wardrobe_open` 키로 한국어·영어·일본어를 제공한다.

## 배치와 해금 연결

1. 벙커 레벨에 `/Game/Interaction/BP_Wardrobe`를 배치한다. 두 문 수납장 메시와 충돌, 상호작용 마커가 포함되어 있다. 피벗은 바닥이며 기본 크기는 약 106 × 72 × 189 cm다.
2. 캐릭터가 200 cm 안에서 기존 상호작용 입력을 사용하면 옷장이 열린다. 액터의 Interactable 컴포넌트에서 거리를 조정할 수 있다. 이 변경은 맵에 액터를 자동 배치하지 않는다.
3. 보상 등에서 GameInstance의 `TryUnlockOutfit(OutfitId)`를 호출한다. `true`는 이미 해금됐거나 전역 저장이 성공했음을 뜻한다. `false`면 획득 성공으로 처리하지 않는다.
4. 실제 해금 조건을 사용할 때 `Config/DefaultGame.ini`의 아래 옵션을 끈다. 현재 기본값은 요청에 따라 `True`다.

```ini
[/Script/TunaSweeper.TunaSweeperGameInstance]
bUnlockAllOutfitsOverride=False
```

| 의상 | OutfitId |
|---|---|
| 메이드 | `Maid` |
| 교복 | `SchoolUniform` |
| 정비사 | `MechanicOutfit` |
| 운동복 | `Sportswear` |
| 토끼 잠옷 | `BunnyPajamas` |
| 탐험가 | `AdventurerOutfit` |
| 우의 | `Raincoat` |

`IsOutfitUnlocked`는 전체 해금 예외를 포함한 현재 사용 가능 여부를 반환한다. `TryUnlockOutfit`는 예외가 켜져 있어도 영구 해금 기록을 저장한다. 메이드는 항상 사용할 수 있다. 추가 의상은 Catalog와 `TunaSweeperOutfits::IsSupportedOutfitId` 양쪽에 등록한다.

전역 파일은 기존 계정·배포·Demo/Main 저장 경계를 따르는 `CosmeticUnlocks_<DistributionNamespace>.sav`다. 착용 저장은 버전 22이며 버전 20·21 세이브도 계속 읽는다. 우의 추가에도 저장 버전과 전역 해금 파일 형식은 바뀌지 않는다. 자세한 복구 정책은 `Docs/save_persistence.md`를 따른다.

## 구현 검증

전역 해금 저장과 슬롯별 착용 저장, 전체 해금 예외 해제, 잘못된 ID, 저장 실패, 메이드 복귀, 얼굴 보존, 의상 skeleton 호환성, UI 닫힘 조건을 자동화 테스트로 확인한다. 신규 메시와 UI는 실제 에디터 렌더로도 확인한다. 기존 Blender 파일과 기존 UE 캐릭터 에셋은 덮어쓰지 않는다.

우의 추가 전 검증 기록 (2026-09-26): UE 5.7 에디터 빌드 성공, `TunaSweeper.Wardrobe+TunaSweeper.Outfits+TunaSweeper.Save`의 14개 테스트 성공. 한국어·영어·일본어 패널을 1280 × 760과 960 × 570으로 렌더해 확인했다. `SourceArt/UI/Wardrobe/validation.json`과 `Previews/`에 결과를 보관한다. 의상별 39개 애니메이션 샘플에서 몸체와 의상의 130본 포즈 전달도 검증했다. 모든 전투 동작의 옷 관통 검사를 수행한 것은 아니며, 관련 캡처와 범위는 `SourceArt/Characters/LunaMk2/RuntimeOutfits/README.md`에 기록한다.

우의 추가 검증에는 7개 ID의 저장 복원, 우의의 해금 저장·재시작·실패 복구, 실제 메시 착용과 메이드 복귀를 포함한다. `-WardrobeUIPreview` 렌더 테스트는 3개 언어와 위 두 크기에서 일곱 번째 카드가 전부 보이도록 스크롤한 뒤 우의 미리보기와 활성화된 착용 버튼을 `Saved/WardrobePreview/Panel_<언어>_<크기>_Raincoat.png`로 저장한다.

2026-09-26 우의 최종 검증: 목 장식 가림 수정 후 같은 14개 테스트가 모두 통과했다. `SourceArt/UI/Wardrobe/raincoat_validation.json`과 `Previews/Raincoat_*`에 결과와 화면을 보관한다. 화면 없는 테스트 실행에는 `-nocef`를 사용하며, 셰이더 작업 경로는 `-ShaderWorkingDir=<프로젝트>/Saved/CodexRaincoat/Shaders`로 지정한다.
