# 반복 가능한 엔딩 인터랙션

## 배치

- 실제 BP: `/Game/Interactables/Ending/BP_EndingInteraction`
- 부모 클래스: `ATunaSweeperEndingActor`
- 임시 외형은 철제 단말기다. BP에서 본체·화면·받침·버튼 메시를 교체할 수 있다.
- `/Game/MainRaid/RaidPlains`의 SSTO 실내에 `TS_Plains_SSTO_Ending`으로 하나 배치되어 있다. 선체 기준 로컬 위치는 `(155, 185, 181)`cm이며 단말기 화면이 중앙 통로를 향한다. 통로 건너편에는 공용 참치 보관함이 있다.

기본 상호작용 표시명은 **엔딩?!**이다. 현재 세이브 슬롯의 공용 레이드 보관함 10칸이 모두 일반 참치(아이템 3004) 10개씩이면 **엔딩!?**로 바뀐다. 총 100개이며, 미식 참치·빈칸·덜 찬 칸은 조건을 만족하지 않는다. 보관함 변경 시 표시명을 갱신하고 상호작용 순간 다시 판정한다. 진엔딩에서 참치를 소모하지 않는다.

## 연출과 이어하기

단말기를 사용하면 레이드를 일시정지한다. 일반 엔딩은 암전 뒤 기존 타이핑 대사 UI로 제목과 네 문단을 표시한다. 한 번 입력하면 현재 문장이 완성되고 다음 입력으로 넘어간다.

진 엔딩은 평원 단말기의 `LaunchLander`에 지정된 SSTO로 재생한다. 시네마틱은 같은 선체 메시와 별도로 여닫히는 해치를 사용한다.

| 시간 | 고정 카메라에 보이는 장면 |
| --- | --- |
| 0–5.6초 | 석양의 좌현 계단이 접히고 객실에서 내려온 해치가 밀착된다. |
| 5.6–10.9초 | 넓은 측면 구도에서 기체가 제자리에서 떠오르고 착륙발이 접힌다. |
| 10.9–16.6초 | 구름과 먼 평원 위로 수평을 잡은 기체가 상승하며 긴 연소 구름을 남긴다. |
| 16.6–21초 | 행성 가장자리의 노을 위로 작은 기체와 희미한 꼬리가 보인다. |
| 21초 이후 | 1.7초에 걸쳐 암전한 뒤 1.1초간 검은 화면을 유지한다. 중앙 독백이 0.8초 동안 나타나고 5초 동안 머문다. |

재생 시간은 첫 번째 고정 컷을 기준으로 한다. [연출 스토리보드](../chatgpt/SSTOEnding/ssto_ending_storyboard.png) · [평원 단말기에서 촬영한 진 엔딩 영상](../TunaSweeper/SourceArt/Environment/SSTO_TrueEnding/Previews/SSTO_TrueEnding.mp4)

연출이 끝나면 현재 슬롯에 저장하고 `/Game/Maps/IntroMap`으로 돌아간다. 저장에 실패하면 화면의 안내에 따라 같은 슬롯에 재시도한다.

소지품과 공용 보관함, 기존 진행도를 유지한다. 레이드에서 얻은 대기 경험치는 한 번만 정산한다. 같은 슬롯을 선택하면 기존 이어하기 흐름에 따라 기지에서 계속 플레이하며 엔딩도 다시 볼 수 있다. 일반 엔딩을 먼저 본 뒤 참치를 채워 진엔딩을 볼 수 있다. 엔딩 완료 시 세이브를 삭제하지 않는다.

## 임시 이야기 교체

| 데이터 애셋 | 임시 제목 | 내용 |
| --- | --- | --- |
| `DA_Ending_Normal` | 잠깐의 평온 | 무사히 돌아온 루나가 잠깐 쉬고 다시 내일을 준비한다. |
| `DA_Ending_True` | 백 개의 내일 | 비축한 참치가 함께 살아갈 약속이 되고 기지가 생존자들의 귀환점이 된다. |

두 애셋은 BP와 같은 폴더에 있다. BP의 `NormalEnding` / `TrueEnding`에 다른 `TunaSweeperEndingPresentation` 데이터 애셋을 지정할 수 있다. `TitleStringKey`와 순서 있는 `LineStringKeys`가 제목과 문단을 결정한다. 문단 수는 자유롭게 바꿀 수 있다.

실제 문구는 `Content/Data/UITextStrings.csv`의 `ui.freeplay_ending.normal.*` / `ui.freeplay_ending.true.*`에서 6개 언어로 관리한다. 평원 진 엔딩의 마지막 문장은 `DA_Ending_True.CinematicMonologueStringKey`가 선택한다. 독백의 서체·색·중앙 배치는 `/Game/UI/Story/Astra/WBP_EndingMonologue_GPT62Astra`의 디자이너에서 편집한다. 고고도 구름과 궤도 지평선의 배경 원화 및 편집용 해치는 `SourceArt/Environment/SSTO_TrueEnding`에 있다.

완료 기록은 기존 시나리오 플래그 배열의 `ending.freeplay.normal_seen`, `ending.freeplay.true_seen`이다. 별도 저장 버전 변경과 도전과제 추가는 없다.

## 검증

`TunaSweeper.FreePlayEnding` 자동화는 native 클래스·실제 BP/스토리 애셋·참치 조건·세이브 거부와 재시도·경험치 중복 방지·슬롯 재로드·반복 엔딩을 확인한다. 디스크 시험은 `-UserDir=<project>/Saved/RaidStorageAutomation/`에서만 수행한다. `RenderStories`는 렌더링 가능한 실행에서 실제 대사 UI 이미지를 생성하고 문장 진행을 검증한다.

`Tools/SSTOLander/verify_ending.ps1`은 저장된 평원맵에서 승하차, 보관함과 단말기 접근, 아이템 이동, 일반 엔딩 대사, 진 엔딩의 네 고정 카메라와 암전·중앙 독백, 타이틀에서 같은 슬롯 이어하기를 실행한다. `-RecordFilm`을 주면 진 엔딩 재생 중 실제 뷰포트 프레임을 경과 밀리초 이름으로 격리된 테스트 저장 폴더의 `SSTOEnding/Film`에 기록한다. 테스트 세이브는 `Saved/SSTOEndingAutomation/` 아래에 격리한다. 실행 전 `Docs/unreal_editor_coordination.md`에 따라 에디터를 조율하고 최신 에디터 모듈을 빌드한다.

PIE 테스트는 레이드에 도달한 슬롯을 재현하기 위해 테스트 슬롯에만 오프닝 완료 기록을 넣는다. 기존 `IntroMap`의 `BP_Morph` Construction Script 오류는 [기존 점검 기록](demo_build_implementation_audit_2026-09-02.md)의 알려진 문제로 한정해 허용한다. 이 테스트가 해당 표정 제어 문제까지 수정하거나 무오류를 보증하는 것은 아니다. 나머지 테스트 오류는 실패 처리한다.
