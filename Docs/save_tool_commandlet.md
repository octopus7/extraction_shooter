# TunaSweeperSaveTool 커맨드렛 — 에이전트 직접 사용

이 문서는 **.NET 툴을 거치지 않는 자동화/에이전트용 인터페이스**입니다. 사람용 콘솔 사용법은 [Tools/SaveTool/README.md](../Tools/SaveTool/README.md)에 있습니다. 두 경로는 동일한 네이티브 저장 서비스를 호출합니다.

## 실행 계약

UE 5.7의 빌드된 TunaSweeperEditor 모듈과 프로젝트 데이터가 필요합니다. 일반 패키징 게임에 포함되는 기능은 아닙니다. 읽기는 게임 실행 상태와 무관하게 가능하지만, 수정은 게임/PIE 종료 상태에서 수행합니다. 커맨드렛은 gameplay GameInstance를 초기화하지 않습니다.

`-run=TunaSweeperEditor.TunaSweeperSaveTool`처럼 모듈명을 반드시 포함합니다. 이 프로젝트의 Editor 모듈은 PostEngineInit에 로드되므로 짧은 `-run=TunaSweeperSaveTool`만으로는 클래스를 찾지 못합니다.

1. UTF-8 JSON 요청 파일을 작성합니다(최대 4 MiB).
2. **새로운 절대 경로 `.json` 응답 파일**을 지정합니다. 부모 디렉터리가 있어야 합니다. 기존 파일에는 쓰지 않습니다.
3. 종료 코드와 응답 JSON의 `ok`를 함께 검사합니다. stdout은 Unreal 로그이므로 JSON으로 해석하지 않습니다.

```powershell
$engine = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = 'D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject'
& $engine $project -run=TunaSweeperEditor.TunaSweeperSaveTool `
  '-Request=D:/temp/save-request.json' '-Response=D:/temp/save-response-new.json' `
  -unattended -nop4 -nosplash -nullrhi '-abslog=D:/temp/save-commandlet.log'
$LASTEXITCODE
Get-Content D:/temp/save-response-new.json -Raw | ConvertFrom-Json
```

Exit `0`: `ok:true`. Exit `1`: 유효한 JSON 응답으로 작업 실패/거절. Exit `2`: 인수·JSON 입력·응답 파일 I/O 실패; 응답이 없거나 불완전할 수 있습니다. 엔진 자체 시작 실패/비정상 종료는 다른 종료 코드도 가능합니다. 취소·시간 초과·응답 유실 시 이미 커밋됐을 수 있으므로 재실행 전에 저장을 조회합니다.

## 공통 JSON

```json
{
  "version": 1,
  "operation": "inspect",
  "savePath": "D:/github/extraction_shooter/TunaSweeper/Saved/SaveGames/Demo/TunaSweeperSave_Slot01.sav",
  "flavor": "Demo",
  "slot": 1
}
```

- `version`은 정수 `1`. 알 수 없는 필드는 거부합니다.
- `savePath`는 절대 경로. `flavor`는 정확히 `Demo` 또는 `Main`. `slot`은 Demo 1, Main 1~3.
- 저장 내부 클래스·버전·flavor·slot을 검증합니다. 편집 파일명은 `TunaSweeperSave_SlotNN.sav`와 슬롯 번호가 일치해야 합니다.
- `catalog`, `list`를 제외한 모든 작업은 위 저장 식별자를 요구합니다. 조회 버전 20~22, 편집/프리셋 내보내기는 22.
- 응답 공통: `version:1`, `ok:boolean`, `code:string`. `ok`가 참이어도 미리보기는 저장하지 않습니다.

## 작업

| operation | 추가 입력 | 결과 |
|---|---|---|
| `catalog` | 없음. 저장 식별자도 생략 | `items`: 전체 아이템 ID·문자열 키·ko/en/ja 이름·분류·중첩·방어·탄창·추가 가능 여부 |
| `list` | `directory`: 절대 폴더 경로. 저장 식별자 생략 | `slots`: 해당 폴더의 슬롯 파일과 읽기 가능 여부·버전·flavor·slot·hash |
| `inspect` | 없음 | `save`: 전체 저장 스냅샷, `itemDefinitions`, `inventoryCapacity`, `hash`, `validationCode` |
| `validate` | 없음 | inspect와 같은 데이터. 아이템 규칙 위반 시 `ok:false` |
| `add` | `items` 배열, 선택 `commit`, `expectedHash` | 변경 후 스냅샷, `sourceHash`, `committed`, 커밋 시 `hash`·`backupPath` |
| `preset-export` | 없음 | `preset` 객체와 `hash`. 호출자가 preset 객체만 파일로 저장 |
| `preset-apply` | `preset` 객체, 선택 `commit`, `expectedHash` | add와 동일 |

`inspect`는 구조를 읽을 수 있으면 `ok:true`로 반환하고 규칙 문제를 `validationCode`에 표시합니다. `validate`와 변경은 규칙 문제를 실패로 처리합니다. 검증은 UID 중복·소유 참조·부착물·장비 슬롯·장전 탄약·중첩·소지품 용량에 초점을 둡니다. 퀘스트/스토리 데이터의 의미적 유효성을 보증하는 기능은 아닙니다.

`save`의 필드명은 Unreal JSON 변환의 lowerCamelCase입니다. 모든 int64(ticks·경험치 등)는 정밀도 보존을 위해 **십진 문자열**입니다. `LastSavedAtTicks`는 기존 게임과 같은 로컬 시각이며 연구 시각은 기존 UTC 규약을 유지합니다. 이 스냅샷 전체를 다시 입력해 임의 수정하는 기능은 없습니다.

카탈로그 요청 예:

```json
{"version":1,"operation":"catalog"}
```

## 소지품 추가 item 형식

`add`의 `items`는 1~1000개 항목입니다. 한 항목의 수량은 1~1,000,000이며 기본 1입니다. 무기·탄약·머리/몸통 방어구를 지원합니다. 장착용 가방·얼굴·귀 장비는 프리셋을 통해 지정할 수 있습니다.

```json
{
  "itemId": 1002,
  "quantity": 1,
  "loadedAmmoItemId": 2002,
  "loadedAmmoCount": 30,
  "attachments": [
    {"slotTag":"attachment.slot.magazine", "itemId":2004}
  ]
}
```

이는 형식 설명용 예시이며 배포 프리셋이 아닙니다. 반드시 `catalog`와 현재 프로젝트 데이터를 확인하세요.

- `itemId`: 양수 정수. 프로젝트에 존재해야 합니다.
- `quantity`: 양수 정수, 기본 1. 빈 무기/탄약은 중첩 한도에 따라 분할하며 기존 소지품 스택을 먼저 채웁니다.
- `loadedAmmoItemId`: 생략하면 비장전. 지정하면 호환 탄종이어야 합니다.
- `loadedAmmoCount`: 기본 0. 비음수 정수, 부착물 포함 탄창 용량 이하. 장전 탄약 ID 없이 양수 불가.
- `attachments`: 선택 배열, 슬롯별 `slotTag`와 `itemId`. 무기에 허용된 슬롯과 총종을 검증합니다. 부착물 중첩/재귀 구성은 지원하지 않습니다.
- 장전 탄약 ID 또는 부착물을 지정하면 수량은 1이어야 합니다. 새 장전 탄약은 기존 소지품에서 차감하지 않습니다.
- 새 인스턴스 UID를 만들고 `EverAcquiredItemIds`를 갱신합니다. 퀘스트 진행·업적 이벤트는 발생시키지 않습니다.

위 item을 `items` 배열에 넣고 `operation:add`로 요청합니다. `commit`을 생략하거나 `false`로 설정해 먼저 검토한 뒤 `true`로 실행합니다. 미리보기의 `sourceHash`를 커밋 요청의 `expectedHash`로 전달하는 흐름을 권장합니다. SHA-1 hash는 변경 감지용이며 인증 목적이 아닙니다.

## 프리셋 형식과 의미

프리셋 루트는 `{"version":1,"equipment":[...]}`입니다. 각 항목은 `{"slot":0,"item":{...}}` 형태이며 `item`은 위 형식을 사용합니다. 전체 장착 교체이며 중복 슬롯은 거부합니다.

| slot | 장착 위치 |
|---|---|
| 0, 1 | 총기 1, 2 |
| 2 | 근접 무기 |
| 3 | 머리 |
| 4 | 몸통 |
| 5 | 얼굴 |
| 6 | 귀 |
| 7 | 가방 |

`equipment:[]`는 모든 장착을 해제하여 소지품으로 옮긴다는 뜻입니다. 실제 프리셋은 제공하지 않습니다. `preset-export`가 반환한 `preset` 객체를 저장하여 사용하세요.

기존 장착은 부착물과 함께 소지품으로 이동하고, 프리셋 장비는 새 인스턴스로 지급합니다. 누락 슬롯은 비웁니다. 교체 가방으로 용량이 줄면 넘친 아이템도 남은 빈칸으로 옮깁니다. 잠금 빈칸은 사용하지 않습니다. 부족하면 전체 요청을 취소합니다. 반복 적용은 멱등이 아닙니다. 기존 장비가 소지품에 쌓입니다. 퀵 슬롯·창고·기타 진행 데이터는 그대로 둡니다.

## 저장과 복원

모든 수정은 메모리 복사에서 끝까지 검증합니다. 커밋 직전에 원본 바이트가 변하지 않았는지 다시 확인하고, `.tool-backup-<UTC시각>-<GUID>` 고유 파일에 원본 바이트를 기록·재검증합니다. 이어 기존 `TunaSweeperSafeSave::SaveGameFileFailClosed` 경로가 CRC candidate 작성·검증, `.previous` 보존, 승격 및 재로드를 수행합니다. 저장 메타데이터의 `LastSavedAtTicks`는 갱신됩니다.

커맨드렛끼리는 시스템 mutex로 직렬화합니다. 게임 저장/클라우드 동기화는 이 mutex에 참여하지 않으므로 실행 중 게임을 편집해도 안전하다는 보장은 없습니다. 조회는 손상 파일 복구나 `.previous` 승격을 하지 않습니다. 요청은 실제 플레이어 세이브의 생성·삭제를 지원하지 않습니다.

복원은 게임/PIE 종료 후 현 파일을 별도 보관하고 원본 `backupPath`를 대상 `.sav`로 복사한 다음 validate로 확인합니다. 백업 자동 삭제·자동 복구는 하지 않습니다. 도구 백업은 게임의 자동 백업 검색 대상이 아닙니다.

## 오류 코드

주요 코드: `invalid_request`, `unknown_operation`, `invalid_identity`, `invalid_directory`, `invalid_save_filename`, `save_unreadable`, `save_invalid`, `edit_version_unsupported`, `item_data_unavailable`, `settings_unavailable`, `stale_save`, `tool_busy`, `invalid_items`, `invalid_item_spec`, `unknown_item`, `unsupported_item_category`, `invalid_stack`, `invalid_ammo`, `invalid_attachment`, `invalid_equipment`, `invalid_preset`, `invalid_uid`, `invalid_reference`, `orphan_item`, `inventory_full`, `backup_failed`, `save_failed`, `post_write_validation_failed`.

`save_invalid`는 CRC/클래스/지원 버전/대상 식별자 불일치를 포함합니다. 저장 실패에 `backupPath`가 있으면 복구 근거로 보존합니다. 임의의 실패를 자동 재시도하지 말고 code와 파일 상태를 확인합니다.
