# TunaSweeper 빌드 구조

패키징 산출물은 플랫폼과 배포 단위별로 아래 경로를 사용한다.

```text
Builds/
├─ Steam/Demo/    # SteamPipe가 업로드할 Steam 데모 빌드
└─ Stove/Demo/    # 향후 STOVE Uploader가 사용할 데모 빌드
```

- 현재 활성 빌드는 `Steam/Demo`뿐이며, `BatchScripts/PackageTunaSweeperWin64.bat Shipping`으로 생성한다.
- `Stove/Demo`는 향후 사용 경로를 문서로만 예약했으며 폴더, 패키징 스크립트, 런타임 연동은 아직 만들지 않는다.
- Steam Online Subsystem은 활성화되어 있다. 현재 `SteamDevAppId=480`은 로컬 개발용 테스트 ID이므로 SteamPipe 업로드 전 Steamworks에서 발급된 TunaSweeper Demo App ID로 교체해야 한다.
