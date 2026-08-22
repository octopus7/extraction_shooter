# TunaSweeper 빌드 구조

패키징 산출물은 플랫폼과 배포 단위별로 아래 경로를 사용한다.

```text
Builds/
├─ Steam/
│  ├─ Demo/       # SteamPipe가 업로드할 Steam 데모 빌드
│  └─ Full/       # SteamPipe가 업로드할 Steam 본편 빌드
└─ Stove/Demo/    # 향후 STOVE Uploader가 사용할 데모 빌드
```

- Steam 데모는 `BatchScripts/PackageTunaSweeperWin64.bat Demo Shipping`, 본편은 `BatchScripts/PackageTunaSweeperWin64.bat Full Shipping`으로 패키징한다.
- 패키징 시작 시 선택한 출력 경로 아래의 생성 폴더 `Windows`를 교체하므로, 이전 타겟의 실행 파일이 SteamPipe 업로드 대상에 남지 않는다.
- `Stove/Demo`는 향후 사용 경로를 문서로만 예약했으며 폴더, 패키징 스크립트, 런타임 연동은 아직 만들지 않는다.
- Steam Online Subsystem은 활성화되어 있다. 에디터와 로컬 개발은 테스트 ID `480`을 사용한다.

## Steam 타겟

| 타겟 | 용도 | Custom Config | Steam App ID | 빌드 정의 |
| --- | --- | --- | --- | --- |
| `TunaSweeperDemo` | Steam 데모 | `Demo` | `5158070` | `TUNASWEEPER_DEMO=1` |
| `TunaSweeper` | Steam 본편 | `Full` | `5137900` | `TUNASWEEPER_DEMO=0` |

타겟별 설정은 `Config/Custom/Demo`와 `Config/Custom/Full`에서 분기된다.
