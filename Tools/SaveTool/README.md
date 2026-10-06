# TunaSweeper 세이브 도구 — 사람용 CLI

.NET 10 콘솔 프로그램이 프로젝트 전용 UE 5.7 커맨드렛을 실행합니다. `.sav` 파일과 이 프로그램만으로 실행할 수는 없습니다. **이 문서는 사람이 콘솔 툴을 사용하는 방법**입니다. 에이전트가 .NET 없이 직접 실행할 때는 [커맨드렛 문서](../../Docs/save_tool_commandlet.md)를 사용하세요.

## 준비

- .NET 10 SDK(소스 실행/빌드), UE 5.7, `TunaSweeper/TunaSweeper.uproject` 및 프로젝트 데이터가 필요합니다.
- 새 커맨드렛이 포함되도록 `TunaSweeperEditor Win64 Development`를 먼저 빌드합니다.
- 저장 편집 전 게임과 PIE를 종료합니다. 실행 중 게임은 메모리의 이전 상태로 파일을 다시 저장할 수 있습니다.
- Demo/Main, Steam/Stove 계정별 폴더는 서로 다릅니다. 도구는 계정이나 저장 파일을 임의로 선택하지 않습니다.

저장소 루트의 PowerShell에서:

```powershell
dotnet build Tools/SaveTool/SaveTool.csproj -c Release
dotnet run --project Tools/SaveTool -- help
```

이후 예시는 `dotnet run`으로 실행하지만, 빌드 결과 `Tools/SaveTool/bin/Release/net10.0/SaveTool.exe` 뒤에 같은 인수를 넣어도 됩니다.

프로젝트는 현재 디렉터리와 프로그램 위치의 상위 폴더에서 찾습니다. 다른 위치에서는 `--project D:/github/extraction_shooter/TunaSweeper/TunaSweeper.uproject`를 지정하세요. 엔진은 `--editor`, `TUNASWEEPER_EDITOR` 환경변수, 기본 UE 5.7 설치 경로 순으로 찾습니다. `--editor`에는 **UnrealEditor-Cmd.exe 전체 경로**를 넣습니다.

## 먼저 조회하기

```powershell
dotnet run --project Tools/SaveTool -- catalog
dotnet run --project Tools/SaveTool -- list --directory D:/github/extraction_shooter/TunaSweeper/Saved/SaveGames/Demo

$save = 'D:/github/extraction_shooter/TunaSweeper/Saved/SaveGames/Demo/TunaSweeperSave_Slot01.sav'
dotnet run --project Tools/SaveTool -- inspect --save $save --flavor Demo --slot 1
dotnet run --project Tools/SaveTool -- validate --save $save --flavor Demo --slot 1
```

- `catalog`: 현재 아이템 ID·번역 이름·분류·최대 중첩 등을 표시합니다. JSON의 `canAdd`가 소지품 추가 지원 여부입니다.
- `list`: 지정한 폴더 바로 아래의 게임 슬롯을 조회합니다. 하위 계정 폴더를 자동 탐색하지 않습니다.
- `inspect`: 메타데이터, 소지품·장착·창고·보조 가방·퀵 슬롯, 경험치·코인·퀘스트·연구·월드 상태 등 전체 저장 필드를 조회합니다. 규칙 위반이 있어도 읽을 수 있는 데이터는 표시합니다.
- `validate`: CRC/저장 클래스·버전·대상 슬롯 외에도 아이템과 참조 관계를 검사합니다. 문제가 있으면 실패로 반환합니다.

Main은 `--flavor Main`과 해당 파일의 `--slot 1`, `2`, `3`을 사용합니다. 로컬 Main 저장 폴더명은 `FullGame`입니다. 다른 저장 위치는 [저장 규약](../../Docs/save_persistence.md)을 참고하세요.

## 무기·탄약·방어구 추가

```powershell
# 미리보기: 소총탄 250발. 현재 최대 중첩 120이면 여러 스택으로 분할합니다.
dotnet run --project Tools/SaveTool -- add --save $save --flavor Demo --slot 1 --item 2002 --quantity 250

# 실제 저장: 소총 1개에 소총탄 30발을 장전한 상태로 소지품에 추가합니다.
dotnet run --project Tools/SaveTool -- add --save $save --flavor Demo --slot 1 --item 1002 --ammo 2002 --rounds 30 --commit

# 방어구 ID는 catalog에서 확인합니다.
dotnet run --project Tools/SaveTool -- add --save $save --flavor Demo --slot 1 --item 5006 --quantity 1 --commit
```

`--quantity`의 기본값은 1입니다. 무기를 추가해도 자동 장착하지 않습니다. `--ammo`를 생략하면 빈 무기이며, 지정한 장전 탄약은 새로 지급됩니다. 기존 탄약에서 차감하지 않습니다. 장전 탄약과 부착물이 있는 무기는 한 번에 1개씩 지정하세요.

여러 아이템을 한 번에 추가하려면 JSON 배열 파일을 만들고 `--items <파일>`을 사용합니다. 각 항목은 커맨드렛 문서의 item 형식을 따릅니다. 모든 항목이 유효하고 공간이 충분할 때만 전체 변경을 저장합니다. 일부만 지급하지 않습니다.

## 장착 장비 프리셋

실제 배포 프리셋은 아직 없습니다. 현재 장착을 내보내거나 별도로 작성한 JSON을 적용할 수 있습니다.

```powershell
dotnet run --project Tools/SaveTool -- preset-export --save $save --flavor Demo --slot 1 --output D:/temp/my-equipment.json
dotnet run --project Tools/SaveTool -- preset-apply --save $save --flavor Demo --slot 1 --preset D:/temp/my-equipment.json
dotnet run --project Tools/SaveTool -- preset-apply --save $save --flavor Demo --slot 1 --preset D:/temp/my-equipment.json --commit
```

내보내기 경로의 부모 폴더는 미리 만들고, 새 파일명을 사용하세요. 기존 파일은 덮어쓰지 않습니다.

프리셋 적용은 **전체 장착 슬롯 교체**입니다. 프리셋에 빠진 슬롯은 비웁니다. 기존 장착 아이템과 부착물은 버리지 않고 소지품으로 옮기며, 새 장비는 새 인스턴스로 생성합니다. 따라서 반복 적용하면 기존 장비가 소지품에 쌓입니다. 가방이 작아져 넘치는 소지품도 가능한 빈칸으로 옮기고, 공간이 부족하면 전체 적용을 취소합니다. 저장 전 미리보기로 확인하세요.

## 출력·백업·오류

- `--json`: stdout에 JSON 결과만 출력합니다. Unreal 로그는 실행별 `TunaSweeper/Saved/SaveTool/Runs/<ID>/`에 남습니다.
- `--lang ko|en|ja`: 사람용 문구와 카탈로그 이름 언어. 기본값 `ko`.
- `--timeout 180`: 엔진 실행 제한 시간(초), 기본 180. 첫 실행이 오래 걸리면 최대 3600까지 지정합니다.
- 변경은 기본 미리보기입니다. `--commit`으로 저장하며, 결과 `backupPath`에 원본 그대로의 고유 백업이 남습니다. 엔진의 `.previous`도 함께 갱신됩니다.
- 조회 결과의 `hash`를 `--expected-hash <값>`으로 전달하면 그 이후 바뀐 저장 파일에 덮어쓰지 않습니다. 미리보기에서는 `sourceHash`를 사용합니다.
- 종료 코드: `0` 성공, `1` 커맨드렛이 요청 거절, `2` CLI/실행/파일 오류. 실패 시 출력의 `code`와 `detail`, 실행 로그를 확인하세요.
- 취소/시간 초과가 발생하면 파일이 이미 저장됐을 수도 있습니다. **같은 지급 명령을 바로 재실행하지 말고 먼저 inspect로 확인**하세요.
- 현재 버전 22는 편집 가능하며, 20/21은 조회 후 게임에서 정상 저장해 최신화해야 합니다. 도구는 자동 복구·마이그레이션을 하지 않습니다.

복원하려면 게임/PIE를 종료하고 현재 파일도 별도로 복사한 뒤, 결과의 `backupPath` 파일을 원래 `.sav` 경로로 복사합니다. 이후 `validate`로 확인합니다. Steam/Stove 클라우드가 동작 중이면 동기화 충돌을 사용자가 확인해야 합니다. 도구 백업은 자동 삭제하지 않습니다.

## 개발 검증

```powershell
dotnet run --project Tools/SaveTool.Tests
```

네이티브 자동화 테스트는 `TunaSweeper.SaveTool`입니다. 테스트는 `Saved/Automation/SaveTool`의 합성 세이브만 사용합니다.

실제 CLI 왕복 검증은 Release 빌드 후 `Tools/SaveTool.Tests/Smoke.ps1 -Fixture <합성 .sav 또는 .previous>`로 실행합니다. 입력은 `Saved/Automation/SaveTool` 아래로 제한하고 별도 복사본에서 실행합니다. 테스트가 끝에 의도적으로 손상시키는 `.sav` 대신 정상 `.previous` 또는 별도 정상 fixture를 선택하세요. 소지품 여유 공간이 필요합니다.
