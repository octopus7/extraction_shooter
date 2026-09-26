# STOVE 클라우드 저장

STOVE 패키지는 런처가 관리하는 폴더 동기화를 사용한다. 게임은 일반 로컬 저장 파일을 쓰고, STOVE 런처가 Studio에 등록된 경로와 클라우드를 동기화한다. 게임에서 업로드 서버를 직접 호출하지 않는다.

## 게임 연동

- STOVE PC SDK 3.4.2의 `Base_GetCloudSavingPath`로 Studio에 설정된 실제 클라우드 루트 경로를 조회한다.
- `TunaSweeperStove::Startup()`의 초기화 및 소유권 확인이 완료된 직후 경로와 `Base_GetUser`의 회원 번호를 캐시한다. 이 작업은 게임 모듈 시작 시 완료되므로 GameInstance의 저장 파일 로드보다 먼저 실행된다.
- 경로에는 `Demo` 또는 `FullGame`을 덧붙인다. 예를 들어 SDK가 `C:/Users/Player/AppData/Local/TunaSweeper/Saved/Cloud/Stove/12345`를 반환하면 정식판 저장 위치는 그 아래 `FullGame`이다.
- SDK가 실패하거나 빈 경로, 상대 경로, 미해결 변수, `..` 등을 반환하면 클라우드 루트를 사용하지 않는다. 호출자는 STOVE 전용 로컬 경로를 선택한다. 경로는 실행 도중 재조회하지 않아 플레이 중 저장 위치가 바뀌지 않는다.
- STOVE 이외의 패키지와 에디터는 이 SDK 경로를 사용하지 않는다. 조회 함수의 비-STOVE 구현은 빈 결과를 반환한다.
- SDK 경로와 회원 번호는 진단 로그에 출력하지 않는다. 클라우드 경로의 존재는 실제 업로드 성공을 뜻하지 않는다.

## Studio 설정

코드 배포만으로 런처 클라우드 기능이 활성화되지는 않는다. 각 배포 상품의 Studio 클라우드 저장 설정 또는 STOVE 운영 담당자를 통해 아래 설정을 적용해야 한다.

1. 클라우드 루트로 `($APPDATA_LOCAL)\TunaSweeper\Saved\Cloud\Stove\($MEMBER_NO)`를 등록한다. 이는 프로젝트 권장 구성이다. 공식 가이드에서 지원하는 `($APPDATA_LOCAL)`과 `($MEMBER_NO)`를 사용해 PC 사용자 및 STOVE 회원별로 분리한다.
2. SDK 반환 경로가 위 루트로 해석되는지 확인한다. 등록 경로 자체에 `Demo`나 `FullGame`을 추가하지 않는다. 게임이 해당 하위 폴더를 덧붙인다.
3. 상품에 해당하는 `Demo` 또는 `FullGame` 하위 폴더의 `.sav`, `.sav.previous` 및 `Backups` 안의 유효한 복구용 `.sav` 파일을 동기화 대상에 포함한다. 파일 필터가 지원되면 임시 `.candidate`, 손상 격리 `.corrupt`, 감사 로그 파일은 제외한다. Studio에서 하위 폴더나 파일 필터를 지정할 수 없는 경우 운영 담당자와 지원 구성을 확인한다.
4. 런처의 해당 게임에서 클라우드 저장 사용 항목이 활성화되고 사용자가 사용하도록 설정했는지 확인한다.
5. 출시 전 서로 다른 PC에서 같은 계정으로 저장·종료·다시 실행하여 진행도, 슬롯 삭제, 설정, 계정 분리와 충돌 처리를 검증한다. 클라우드를 끈 경우와 경로 조회 실패 시 로컬 저장 동작도 확인한다.

검색 색인에서 확인한 공식 가이드(문서 갱신일 2023-12-24)는 회원당 1 GB, 최대 200개 파일을 안내한다. 구 URL의 현재 직접 접속은 새 개발자센터로 리디렉션되므로 이 수치를 현재 운영 한도의 보증으로 사용하지 말고 출시 전 Studio 또는 운영 담당자에게 재확인한다. 게임은 `MaxSaveGameBackupCount = 30`으로 백업 수를 제한한다. 슬롯·설정·자동 복구·유효 백업까지 합산한 전체 파일 수와 용량이 적용 한도 이내인지 검증한다.

## 검증

- `Tools/StoveTests/RunTests.bat`: 소유권 정책 13개, 클라우드 경로 정책 19개 사례를 실행한다. 정상 드라이브·UNC·유니코드 경로와 상대 경로, 변수, 경로 이탈, 파일 패턴을 검사한다.
- 에디터 자동화 `TunaSweeper.Save.StoveCloudChannelGate`: 비-STOVE 빌드가 STOVE 클라우드 경로와 계정을 선택하지 않고 이전 출력값을 비우는지 확인한다.
- 오프라인 검사는 SDK 경로 검증과 채널 분기를 검증한다. 실제 STOVE 런처 업로드·다운로드와 Studio 설정 검증은 별도로 필요하다.

## 근거

- [STOVE 공식 클라우드 세이빙 가이드](https://studio-docs.onstove.com/pc/References/cloudsaving.html): 2026-09-26 검색 색인에서 공식 페이지 본문의 런처 동기화, Studio 등록, `($APPDATA_LOCAL)`·`($MEMBER_NO)` 변수, 1 GB·200개 제한을 확인했다. 구 URL의 직접 접속은 새 개발자센터로 이동한다. 변수 지원과 한도는 실제 Studio 설정 시 재확인한다.
- [공식 PC SDK 3.4.x 레퍼런스](https://developers.onstove.com/en/docs/stove/reference/pc-sdk/pc-sdk-reference-native-old-pc): `Base_GetCloudSavingPath`, 스토어인디 전용 API.
- 로컬 배포 SDK: `store/stove/StovePCSDK_Studio_Cpp_3.4.2/Include/BaseSDK.h:462,468,472`. 실제 선언은 `Base_GetCloudSavingPath(wchar_t* cloudSavingPath, uint32_t length)`이다. SDK가 `length`를 한국어로 "배열의 길이", 영어로 "Length of the array"라고 설명하므로 구현은 `wchar_t` 배열의 요소 개수를 전달한다. 바이트 크기를 전달하라는 설명은 없다.
