# SSOT·구현·애셋 대조 검토

- 검토 시작: 2026-09-06 21:46:09
- 기준: `Docs/SSOT/README.md`의 우선순위, SSOT 폴더의 설계 문서, `Docs/game_conventions.md`의 공통 규칙.
- 대상: 공개 UE C++ 소스, `Content/Data` 최상위 JSON 30개와 문자열 CSV, 이 데이터가 참조하는 애셋, 벙커 및 데모 레이드 맵.
- 방법: 소스의 실제 로더·소비 경로 대조, JSON/문자열/아이템 참조 검사, UE 5.7 Python commandlet을 통한 애셋·맵 읽기. 게임 코드, 데이터, 애셋은 수정하지 않았다.
- 범위 제한: Main 전용 내러티브 팩 내용, 온라인 QuestStudio 최신 작성본, 전체 BP 그래프와 모든 애셋의 시각적 외양, PIE·패키지 플레이는 검증하지 않았다. 공개 데모에 M01~M20이 없다는 이유로 Main에도 없다고 판정하지 않는다.

## 결론

**최초 불일치 8종 중 1종을 문서 정정으로 해결했고, 7종이 남아 있다.** 2026-09-06 사용자 확인에 따라 미구현 NFT 영구 수납 확장은 잘못 남은 기획으로 폐기했다. 남은 항목은 로더 문제 2종, 설계와 다른 구현 또는 필요한 기능이 빠진 부분 3종, AI SSOT 갱신 누락 1종, 아이콘 규격 불일치 1종이다.

P1은 데이터 작성 시 확률·배율 또는 해금이 의도와 크게 달라지는 항목, P2는 설계 정합성 및 기능 차이, P3는 낮은 우선순위의 애셋 규격 차이로 사용했다. 현재 데모에서 진행 불능을 재현했다는 뜻은 아니다.

| 번호 | 우선도 | 항목 | 판정 |
| --- | --- | --- | --- |
| 1 | P1 | 확률·배율 원본의 단위 | 10000분율 SSOT와 float/혼합 단위 로더가 충돌 |
| 2 | P1 | 조제법의 해금 필드 생략 | SSOT는 잠김, 실제 기본값은 해금 |
| 3 | 정정 완료 | 폐기된 NFT 확장 기획 | 미구현 기획·관련 퀘스트와 대화를 제거하고 장착 가방 기준으로 통일 |
| 4 | P2 | M09 병렬 선행조건 | 3개 중 2개 완료 조건을 현행 일반 퀘스트 스키마가 표현하지 못함 |
| 5 | P2 | 자판기 상품별 해금 | 상품 정의·구매 처리에 해금 조건이 없음 |
| 6 | P2 | 헤드샷 Core 판정 | 사격 더미가 별도 탄도 프록시 대신 실제 충돌 컴포넌트를 사용 |
| 7 | P2 | 원거리 AI SSOT | 현재 전투 FSM과 상태·이동 규칙 설명이 다르고 문서 내부 거리도 충돌 |
| 8 | P3 | 야구방망이 아이콘 | 최종 아이콘 256×256 규칙에 대해 실제 Texture2D는 1024×1024 |

## 1. 확률·배율 단위가 로더마다 다르다

**SSOT:** `Docs/SSOT/data_authoring_conventions.md:7`부터 모든 게임플레이 확률·비율·배율 원본을 10000분율 정수로 기록한다. `1`은 0.01%다.

**실제:** 아래 값들은 현재 JSON과 코드가 float 단위를 사용한다.

| 입력 경로 | 현재 값 예 | 실제 해석 | SSOT대로 작성할 때의 문제 |
| --- | --- | --- | --- |
| `EnemySpawns.json:6` → `RaidPlacementSubsystem.cpp:51` | `spawn_chance: 1.0` | 0~1로 clamp | 65%를 `6500`으로 적으면 100%로 바뀜 |
| `EnemySpawnProfiles.json:14` → `RaidPlacementSubsystem.cpp:347` | `loot_loaded_ammo_deduction_ratio: 0.4` | 0~1로 clamp | 40%를 `4000`으로 적으면 비율 부분이 100% 차감으로 바뀜 |
| `EnemyCombatProfiles.json:31` → `EnemySpawnSubsystem.cpp:1090` | `cross_reposition_chance: 0.15` | 0~1로 clamp | 15%를 `1500`으로 적으면 100%가 됨 |
| `EnemyCombatProfiles.json:49` → `EnemySpawnSubsystem.cpp:1058` | `weapon_spread_multiplier: 2.0` | float 배율 그대로 사용 | 2배를 `20000`으로 적으면 로더는 20000배 값을 전달함 |
| `LootContainerContents.json:103` → `ItemDataSubsystem.cpp:45` | `drop_chance: 0.65` | 1 초과만 10000으로 나누고 0~1은 그대로 사용 | `1`을 0.01%가 아닌 100%로 해석 |

위 cpp 경로의 공통 접두사는 `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeper`, JSON은 `TunaSweeper/Content/Data/`다.

현재 공개 JSON에서도 소수 확률·배율 값이 사용되고 있어 규칙 위반이 실제 데이터에 존재한다. 다만 표의 `6500`, `4000`, `1500`, `20000`, `1` 오해석 사례는 **코드의 입력 해석 결과**이며, 이번 검토에서 운영 데이터를 바꿔 플레이 재현한 결과는 아니다.

**권장:** JSON 값과 해당 로더를 동시에 10000분율 계약으로 맞춘다. `1`의 의미가 충돌하므로 크기에 따른 자동 단위 추론을 계속 사용하면 안 된다. 데이터만 정수로 바꾸면 확률·전리품·적 정확도가 바뀐다.

## 2. `auto_unlocked` 생략 시 동작이 반대다

- SSOT `Docs/SSOT/workbench_recipe_blueprint_unlocking.md:35`: 퀘스트/설계도로 열 조제법은 `auto_unlocked`를 false 또는 생략 상태로 둔다.
- 실제 `TunaSweeper/Source/TunaSweeper/Public/Subsystem/TunaSweeperItemDataSubsystem.h:348`: `bAutoUnlocked = true`가 기본값이다.
- 로더 `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp:1557`: `auto_unlocked`와 별칭 `unlocked`가 둘 다 없으면 true가 유지된다.
- 소비부 `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceWorkbench.cpp:494`: 기본 해금 값이 true면 저장된 해금 ID 없이 제조 가능 여부를 통과한다.

**영향:** SSOT를 따라 해금 필드를 생략한 설계도 전용 조제법이 처음부터 열린다. 현재 등록된 큰생수/왕생수 2개는 각각 true/false를 명시하므로 현재 왕생수에 이 문제가 발생한다고 판정하지 않았다.

**권장:** 생략을 잠김으로 처리하도록 기본값을 바꾸거나, 현행 동작을 유지할 의도라면 SSOT에서 생략 허용을 제거하고 명시적 false를 요구한다.

## 3. 정정 완료: NFT 영구 수납 확장은 잘못 남은 기획이다

- `TunaSweeper/Content/Data/ItemTable.json`과 아이템 문자열에 NFT 아이템이 없고, 공개 C++에도 NFT 소비·영구 수납 증가 기능이 없다. NFT 이름의 콘텐츠 애셋 파일도 확인되지 않았다.
- 실제 데이터 `TunaSweeper/Content/Data/ItemTable.json:479`부터는 ID 5002~5005의 장착형 1~4단계 가방이며 용량은 50/60/80/100칸이다.
- 실제 용량 계산 `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceEquipmentWeapon.cpp:417`은 현재 장착 가방의 `InventorySlotCapacity`를 사용한다.

**판정 정정:** 이전 보고서에서 NFT 영구 확장을 구현 선택지로 둔 것은 잘못이었다. 사용자 지시에 따라 이 항목은 구현 누락을 보충할 과제가 아니라 SSOT에서 제거할 잘못된 기획으로 확정했다.

**완료한 조치:** 퀘스트/아이템 SSOT에서 NFT와 전용 재료 5종, S01~S03 퀘스트, 선행조건 그래프, 영구 수납 증가 설명을 제거했다. 대화집의 S01~S03 대화를 제거하고 지역 개방 기준·패키지 README를 장착 가방 기준으로 정정했다. 나머지 퀘스트 ID는 유지했다. 게임 데이터·코드·애셋과 저장 형식은 변경하지 않았다.

## 4. M09의 ‘3개 중 2개’ 조건을 표현할 수 없다

- SSOT `Docs/SSOT/TunaSweeper_SSOT_Quest_Item_v0.6.md` 5절 M09: M09는 M06~M08 중 2개 이상 완료하면 열린다.
- 실제 `TunaSweeper/Source/TunaSweeper/Public/Quest/TunaSweeperQuestTypes.h:187`: 선행조건은 `RequiredCompletedQuestIds` 배열이다.
- 실제 `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperQuestSubsystem.cpp:1389`: 배열을 순회하며 하나라도 `RewardCompleted`가 아니면 false를 반환한다.

**영향:** 세 ID를 배열에 넣으면 셋 모두 완료해야 한다. 두 ID만 넣으면 그 두 개가 필수가 되므로 어느 두 개든 허용하는 조건과 다르다. 일반 퀘스트 스키마에 완료 개수 또는 조건 그룹 기능이 필요하다.

**판정 범위:** SSOT를 구현하는 데 필요한 공통 기능의 누락이다. 현행 공개 데모 퀘스트가 이 조건 때문에 막힌다는 주장은 아니다.

## 5. 자판기 상품별 퀘스트 해금 조건이 없다

- SSOT `Docs/SSOT/TunaSweeper_SSOT_Quest_Item_v0.6.md:46`부터: 폭발물은 퀘스트로 해금되는 상품이며 상품별 잠금 조건 UI가 필요하다.
- 실제 `TunaSweeper/Source/TunaSweeper/Public/Subsystem/TunaSweeperItemDataSubsystem.h:259`: 상품 정의는 ItemId, StockQuantity, PriceOverride만 가진다. 상품 표시용 구조체에도 잠금 상태·조건이 없다.
- 실제 `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceShop.cpp:98`: 구매 처리에서 아이템·재고·코인·인벤토리를 검사하지만 퀘스트 해금은 검사하지 않는다.

**영향:** 폭발물 상품 행을 추가하는 것만으로 M11 이후 판매를 구현할 수 없다. 현재 상점에 SSOT의 ‘자판기 폭발물’이 이미 등록되어 처음부터 팔린다는 뜻은 아니다.

**권장:** 상품별 조건 데이터, 표시 상태, 실제 구매 허용 검사를 함께 구현한다. UI에서만 숨기는 처리로 끝내면 구매 함수와 조건이 분리된다.

## 6. 더미의 Head Core가 탄도 중심선 검사가 아니다

- SSOT `Docs/SSOT/headshot_hit_zone_design.md:22`, `:25`, `:50`: 최종 탄도선이 작은 머리 중심부 프록시를 지나야 하며, 탄환 반지름이 판정을 크게 넓히면 안 된다. Intent/Core는 실제 명중을 가로채지 않는 보조 판정이다.
- 실제 `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperShootingPracticeDummyActor.cpp:159`: Body, CriticalPlate, HeadshotPlate, Head를 모두 투사체를 차단하는 실제 충돌 컴포넌트로 설정한다.
- 실제 같은 파일 `:184`: Head Intent와 Head Core의 AND 조건은 있지만, Core는 `IsHeadshotComponent(HitComponent)`로 결정한다. 대상 내부 탄도 구간과 작은 Core 프록시의 교차를 별도로 계산하지 않는다.
- 투사체 기본 반지름은 `TunaSweeper/Source/TunaSweeper/Private/Weapon/TunaSweeperProjectile.cpp:101`에서 12cm다.

**영향:** 조준 의도가 Head인 경우, 탄도 중심선이 작은 머리 중심부를 통과했는지보다 구형 투사체가 Head 컴포넌트에 충돌했는지가 판정을 결정한다. SSOT의 엄격한 Core 조건을 검증하는 더미로 사용하기에는 차이가 있다.

**판정 범위:** 현재 사격 더미의 공통 C++ 경로다. 머리가 없는 모든 적에게 헤드샷을 추가해야 한다는 뜻은 아니다. SSOT도 대상별 헤드샷 제외를 허용한다.

## 7. 원거리 AI SSOT가 현행 FSM을 설명하지 않는다

- SSOT `Docs/SSOT/ranged_enemy_combat_pattern.md:33`부터: EvaluateCombat, AdvanceBurst, HoldFire, SeekLineOfFire, KeepDistance를 기본 구조로 둔다. `:62`의 전진 거리·정지 시간 표를 제시한다.
- 실제 `TunaSweeper/Source/TunaSweeper/Public/AI/TunaSweeperEnemyAIController.h:14`: Idle, Aim, Firing, Recover, Observe, Reposition, Reload, HitEvade, SeekLineOfFire로 구성한다.
- 현행 프로필 `TunaSweeper/Content/Data/EnemyCombatProfiles.json`: 사격 횟수·현 위치 사격 예산·재배치 거리·회복/관찰 시간을 제어한다. 일반 소총의 재배치는 180~300cm로 작성되어 있다.
- `Docs/enemy_ai_state_flow.md:3`부터의 구현 설명은 현재 상태 구조와 맞는다. SSOT 폴더의 우선 문서와 외부 구현 문서가 서로 다른 기본 구조를 가리킨다.
- SSOT 자체도 `:52`에서는 근접 위험권 430cm, `:113`에서는 600cm를 사용한다. 현행 일반 소총 프로필은 430cm다.

**권장:** 현재 FSM을 유지한다면 원거리 AI SSOT를 현행 구조와 프로필 기준으로 갱신한다. 단순 enum 명칭 차이만 지적한 것이 아니라 정지 교전·이동을 결정하는 규칙과 데이터가 달라진 상태다. 이 차이만으로 AI 구현을 이전 구조로 되돌려야 한다고 판정하지 않는다.

## 8. 야구방망이 아이콘은 1024×1024다

- 공통 기준 `Docs/game_conventions.md:31`: 최종 게임용 개별 아이템 아이콘은 256×256이다.
- 연결 `TunaSweeper/Content/Data/ItemTable.json:149`: 실제 아이템이 `T_UIIcon_BaseballBat.uasset`를 참조한다.
- UE에서 읽은 애셋: `TunaSweeper/Content/UI/Icons/T_UIIcon_BaseballBat.uasset`.
- 확인값: Texture2D 1024×1024, `MaxTextureSize=0`, `LODBias=0`, `TEXTUREGROUP_UI`, `TMGS_NO_MIPMAPS`.
- 검사한 아이콘 Texture2D 43개 중 나머지 42개는 256×256이었다.

**영향:** 최종 아이콘 규격을 벗어나며 같은 256×256 규격에 비해 픽셀 수가 16배다. 화면에서 16배 크게 표시된다는 뜻은 아니며, 패키지 용량·VRAM 차이를 실제 측정한 결과도 아니다.

**권장:** 원본은 별도로 유지하고 런타임 아이콘을 256×256으로 맞춘다. 고해상도 예외가 의도된 것이라면 예외를 공통 기준에 명시한다.

## 콘텐츠 범위와 애셋 연결의 현황

- 공개 `QuestDefinitions.json`에는 `demo_q1_water_intake_check` 1개가 있다. SSOT 본편 M01~M20/S04~S20 및 대화집을 현재 데모 구현 완료 목록으로 해석하면 안 된다.
- 최초 점검 시 SSOT 8절의 외양 묘사 아이템명은 82개였고, NFT 및 전용 재료 5종 제거 후 76개다. 공개 ItemTable은 48개다. 표시명 완전 일치로는 직접 대응하지 않으며, 예를 들어 현재 일반 참치캔을 SSOT의 ‘찌그러진 참치캔’과 같은 항목으로 볼지 명시적 매핑이 필요하다.
- 이는 **SSOT에 적힌 애셋이 모두 삭제되었거나 파일이 없다**는 판정이 아니다. 이름 변경·데모 범위·아직 데이터에 연결되지 않은 소스 아트는 별도로 확인해야 한다.
- `TunaSweeper/Source/TunaSweeper/Private/Settings/TunaSweeperBuildFlavor.cpp:104`의 경로 선택상 공개 QuestDefinitions는 Demo용이고 Main 퀘스트는 별도 팩에서 읽는다. 이 보고서는 Main 팩 내용을 공개 문서에 옮기거나 전체 본편의 콘텐츠 누락을 추정하지 않는다.

## 일치한 부분과 검증 결과

- 공개 최상위 JSON 30개 파싱 성공. 검사한 아이템 ID 참조와 문자열 키 참조에 누락이 없었다.
- JSON의 런타임 `/Game` 경로와 규칙으로 조합한 아이콘 경로는 중복 제외 51개이며 파일이 모두 존재했다. FM 사운드의 `export_asset_path`는 출력 디렉터리이므로 애셋 누락 판정에서 제외했다.
- UE 직접 로드: 51개 모두 객체 로드 성공. Texture2D 43개, BlueprintGeneratedClass 3개, Material 5개.
- UE 맵 로드: BunkerMap 100개 액터, DemoRaidMap 210개, DemoBoxRaidMap 12개. 맵에 상속된 BP의 모든 동작을 실행 검증한 것은 아니다.
- DemoRaidMap의 `BP_RaidPlacementAnchor_C`는 `PlacementId=1`로 현재 EnemySpawns의 행과 일치했다.
- 큰생수의 기본 해금·재료·출력, 왕생수의 명시적 잠금·설계도 연결·제조/분해 재료는 작업대 SSOT와 맞는다. 설계도 등록 코드는 1개 소비, recipe_id 해금, 중복 등록 방지, 저장을 처리한다. 위 2번의 생략 기본값 문제는 별개다.
- 중량 디버프 JSON의 7000/10000/5000 값과 동일 디버프 재적용 시 기존 상태를 덮어쓰는 구현은 해당 SSOT와 맞는다.
- 문자열 `ui.common.confirm` 중복 1건은 별도로 재확인했다(`UITextStrings.csv:7`, `:194`). 이는 SSOT 직접 충돌 항목 8종에는 포함하지 않았다.

### UE 실행 결과의 제한

첫 commandlet 시도는 쓰기 가능한 DDC 노드가 없어 종료 코드 3으로 실패했다. 메모리 DDC 시도 후 프로젝트 내부의 로컬 DDC를 쓰는 `InstalledNoZenLocalFallback`으로 실행해 모든 검사 결과를 얻었다.

애셋·맵 검사 스크립트는 끝까지 실행됐지만 **최종 commandlet 종료 코드는 1**이다. 사족보행 적 생성자에서 Niagara를 동기 로드할 때 발생하는 기존 ensure와 Niagara의 ChaosNiagara import 경고 등 초기화 로그가 남았다. 따라서 객체 로드 결과가 나온 것과 엔진 검증 명령이 오류 없이 통과한 것은 구분한다. 빌드·cook·PIE 통과를 주장하지 않는다.

읽기 전용 검사 산출물은 `TunaSweeper/Saved/SSOTAudit/`에 있다.

- `public_data_audit.json`: 최초 검토 시점의 공개 데이터 참조·이름 대조. NFT 관련 SSOT 정정 전 82개 이름을 기준으로 생성한 기록이다.
- `unreal_asset_audit.json`: UE 객체 클래스·텍스처 크기·맵 액터 대조.
- `baseball_bat_texture_audit.json`: 야구방망이 텍스처 제한 설정.
- `unreal_local_ddc.stdout.log`, `texture.stdout.log`: 최종 UE 실행 로그.

## 권장 처리 순서

1. 확률·배율 단위와 `auto_unlocked` 생략 계약을 먼저 정리한다. SSOT를 따르는 신규 데이터가 의도와 다르게 실행되는 문제를 막는다.
2. 가방 관련 SSOT 정정은 완료했다. 현행 AI FSM의 SSOT 갱신 또는 구현 변경 방향은 별도로 정한다.
3. 본편 퀘스트 적용 전에 N개 중 K개 선행조건과 상품별 해금 처리를 갖춘다.
4. 헤드샷 Core 판정과 아이콘 해상도를 기준에 맞춘다.
5. SSOT의 각 콘텐츠에 적용 flavor, 구현 상태, 실제 데이터 ID·애셋 경로를 연결해 설계안과 구현 완료 상태를 구분한다.
