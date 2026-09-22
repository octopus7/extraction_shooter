# EOS Steam 코옵 설정

이 기능은 Steam 플랫폼 인증을 OnlineSubsystemEOS의 EOS Connect 경로로 넘기고, EOS 세션 검색을 사용합니다. 별도 매칭 서버나 전용 서버는 사용하지 않습니다.

실행 환경에서는 추적되지 않는 환경별 Engine.ini에 다음 EOS artifact 필드를 공급해야 합니다.

- ClientId
- ClientSecret
- ProductId
- SandboxId
- DeploymentId
- ClientEncryptionKey

공개 저장소의 `DefaultEngine.ini`에는 비밀값을 넣지 않습니다. Steam은 `GetAuthTicketForWebApi` 경로를 사용하도록 `SteamTokenType=WebApi:epiconlineservices`로 설정하며, EOS Developer Portal에서 같은 remote service identity를 Steam Identity Provider에 등록해야 합니다. Demo와 Main은 서로 다른 EOS sandbox/deployment와 Steam 앱 설정을 사용합니다.

수동 검증은 EOS 자격증명을 주입한 Win64 패키지 두 개로 진행합니다. 한 프로세스가 호스트 세션을 만들고 표시된 8자리 코드를 두 번째 프로세스에 입력하면 두 번째 프로세스가 세션을 검색해 호스트 리슨 서버로 이동해야 합니다. 자격증명이 없는 환경에서는 온라인 코옵이 비활성화되고 싱글플레이 진입은 유지되어야 합니다.
