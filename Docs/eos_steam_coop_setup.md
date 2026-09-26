# EOS Steam 코옵 설정과 검증 범위

이번 구현 범위는 Steam 인증 → EOS Connect → 8자리 숫자 코드로 로비 검색 → 2인 리슨 서버 접속과 퇴장이다. 자체 매칭 서버와 전용 게임 서버는 없다. Epic 계정 로그인은 요구하지 않으며 Steam은 기존 기본 온라인 서비스로 유지한다.

접속 확인은 `/Game/Maps/CoopStaging`에서 수행한다. 기존 레이드의 전투·인벤토리·퀘스트·세이브 동기화는 이 단계의 구현 범위가 아니다. 호스트·헬퍼가 연결됐다는 표시와 실제 게임플레이 코옵 완성은 구분한다. 코옵 로비/코드/인증 상태는 저장하지 않는다. 기존 싱글플레이의 실시간 연구 타이머는 타이틀과 마찬가지로 계속 진행되며, 연구 완료로 기존 세이브가 갱신될 수 있다. 코옵 진행 미저장은 파일 전체의 불변을 뜻하지 않는다.

## 서비스 설정

EOS Developer Portal에서 해당 제품과 Sandbox/Deployment를 만들고 Steam Identity Provider를 실제 Steam App ID에 연결한다. Web API 티켓 identity는 `epiconlineservices`로 맞춘다. Steam용 파트너 서버 키를 게임에 넣지 않는다.

엔진 설정은 `bUseEAS=false`, `bUseEOSConnect=true`, `SteamTokenType=WebApi:epiconlineservices`를 사용한다. 설치된 UE 5.7 OSS EOS가 Steam 티켓 획득·Connect 로그인·유효한 EOS_InvalidUser에 대한 계정 생성·인증 갱신을 처리한다. 게임은 그 위에서 로그인 상태와 세션 수명만 관리한다.

배포 환경별 EOS artifact 설정에는 ArtifactName, ClientId, ClientSecret, ProductId, SandboxId, DeploymentId, ClientEncryptionKey가 필요하다. DefaultArtifactName은 실제 artifact와 일치해야 한다. 실제 값은 버전 관리되지 않는 환경 설정으로 공급하고 추적 소스·로그·요청 기록에 넣지 않는다. 게임 클라이언트용 EOS 자격증명은 패키지에서 추출될 수 있으므로 반드시 클라이언트 전용 최소 권한 정책을 사용한다. 서버 관리용 키를 사용하지 않는다.

Demo와 Main 간 접속은 허용하지 않는다. 제품/배포 환경 분리와 게임의 호환성 검사로 구분한다. 설정만으로 모든 계정·배포 환경이 자동 생성되지는 않는다.

## 코드의 의미

초대 코드는 선행 0을 포함하는 ASCII 숫자 8자리다. 사용자가 공유하기 쉬운 방 검색 키이며 신원 인증이나 암호화 비밀번호가 아니다. 실제 접속 허용은 EOS 신원 및 로비 참가 상태와 인원 제한으로 판정한다. 같은 코드가 둘 이상의 방에 대응하면 임의 방으로 접속하지 않고 실패 처리한다.

## 검증의 한계

실제 서비스 검증에는 올바른 EOS 설정과 서로 다른 Steam 계정을 사용하는 두 PC가 필요하다. 같은 PC에서 두 창을 켜는 것만으로 두 Steam 신원이 생기지 않는다. 자격증명이 준비되지 않은 상태에서 가능한 검증은 빌드, 로컬 자동화, WBP/맵 검사 및 서비스 미설정 시 실패 처리다. 이것을 실제 인터넷 P2P/릴레이 접속 성공으로 보고하지 않는다.

참고: [Epic OSS EOS 문서](https://dev.epicgames.com/documentation/en-us/unreal-engine/online-subsystem-eos-plugin-in-unreal-engine?application_version=5.7). 구체 API와 Steam 토큰 경로는 로컬 UE 5.7 엔진 소스로 확인했다.
