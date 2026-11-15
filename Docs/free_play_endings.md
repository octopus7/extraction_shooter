# 반복 가능한 엔딩 인터랙션

## 배치

- 실제 BP: `/Game/Interactables/Ending/BP_EndingInteraction`
- 부모 클래스: `ATunaSweeperEndingActor`
- 임시 외형은 철제 단말기다. BP에서 본체·화면·받침·버튼 메시를 교체할 수 있다.
- `/Game/MainRaid/RaidPlains`의 SSTO 실내에 `TS_Plains_SSTO_Ending`으로 하나 배치되어 있다. 선체 기준 로컬 위치는 `(365, 185, 181)`cm이며 중앙 통로를 향한다. 맞은편에 공용 참치 보관함이 있다.

기본 상호작용 표시명은 **엔딩?!**이다. 현재 세이브 슬롯의 공용 레이드 보관함 10칸이 모두 일반 참치(아이템 3004) 10개씩이면 **엔딩!?**로 바뀐다. 총 100개이며, 미식 참치·빈칸·덜 찬 칸은 조건을 만족하지 않는다. 보관함 변경 시 표시명을 갱신하고 상호작용 순간 다시 판정한다. 진엔딩에서 참치를 소모하지 않는다.

## 연출과 이어하기

월드를 일시정지하고 암전한 뒤 기존 타이핑 대사 UI로 제목 카드와 네 문단을 표시한다. 입력하면 현재 문장을 끝까지 표시하고, 다음 입력으로 넘어간다. 마지막 문단 뒤 저장 성공 시 `/Game/Maps/IntroMap`으로 돌아간다. 저장 실패 시 같은 화면에서 재시도하며, 성공하기 전에는 타이틀로 이동하지 않는다.

소지품과 공용 보관함, 기존 진행도를 유지한다. 레이드에서 얻은 대기 경험치는 한 번만 정산한다. 같은 슬롯을 선택하면 기존 이어하기 흐름에 따라 기지에서 계속 플레이하며 엔딩도 다시 볼 수 있다. 일반 엔딩을 먼저 본 뒤 참치를 채워 진엔딩을 볼 수 있다. 엔딩 완료 시 세이브를 삭제하지 않는다.

## 임시 이야기 교체

| 데이터 애셋 | 임시 제목 | 내용 |
| --- | --- | --- |
| `DA_Ending_Normal` | 잠깐의 평온 | 무사히 돌아온 루나가 잠깐 쉬고 다시 내일을 준비한다. |
| `DA_Ending_True` | 백 개의 내일 | 비축한 참치가 함께 살아갈 약속이 되고 기지가 생존자들의 귀환점이 된다. |

두 애셋은 BP와 같은 폴더에 있다. BP의 `NormalEnding` / `TrueEnding`에 다른 `TunaSweeperEndingPresentation` 데이터 애셋을 지정할 수 있다. `TitleStringKey`와 순서 있는 `LineStringKeys`가 제목과 문단을 결정한다. 문단 수는 자유롭게 바꿀 수 있다.

실제 문구는 `Content/Data/UITextStrings.csv`의 `ui.freeplay_ending.normal.*` / `ui.freeplay_ending.true.*`에 있으며 기존 6개 언어 흐름을 사용한다. 요청에 따른 임시 공개 데모 저작 텍스트이며, 비공개 내러티브 팩을 복사하거나 변경하지 않았다. 내용만 교체할 때는 해당 문자열을, 구성까지 바꿀 때는 데이터 애셋의 키 배열을 수정한다. 새 위젯 클래스나 런타임 위젯 트리는 추가하지 않았다.

완료 기록은 기존 시나리오 플래그 배열의 `ending.freeplay.normal_seen`, `ending.freeplay.true_seen`이다. 별도 저장 버전 변경과 도전과제 추가는 없다.

## 검증

`TunaSweeper.FreePlayEnding` 자동화는 native 클래스·실제 BP/스토리 애셋·참치 조건·세이브 거부와 재시도·경험치 중복 방지·슬롯 재로드·반복 엔딩을 확인한다. 디스크 시험은 `-UserDir=<project>/Saved/RaidStorageAutomation/`에서만 수행한다. `RenderStories`는 렌더링 가능한 실행에서 실제 대사 UI 이미지를 생성하고 문장 진행을 검증한다.

`Tools/SSTOLander/verify_ending.ps1`은 저장된 RaidPlains를 PIE로 열어 사다리 입장, 중앙 통로 캡슐 충돌, 두 액터의 상호작용 거리, 보관함 열기·아이템 이동·즉시 파일 변경·LED, 두 엔딩의 대사 입력과 타이틀 복귀, 실제 시작 버튼을 통한 같은 슬롯 이어하기를 검사한다. 테스트 세이브는 `Saved/SSTOEndingAutomation/` 아래에 격리하며 이 경로가 아니면 실행을 거부한다. 사용자 세이브를 테스트에 사용하지 않는다. 실행 전 `Docs/unreal_editor_coordination.md`에 따라 에디터를 조율하고 최신 에디터 모듈을 빌드한다.

PIE 테스트는 레이드에 도달한 슬롯을 재현하기 위해 테스트 슬롯에만 오프닝 완료 기록을 넣는다. 기존 `IntroMap`의 `BP_Morph` Construction Script 오류는 [기존 점검 기록](demo_build_implementation_audit_2026-09-02.md)의 알려진 문제로 한정해 허용한다. 이 테스트가 해당 표정 제어 문제까지 수정하거나 무오류를 보증하는 것은 아니다. 나머지 테스트 오류는 실패 처리한다.
