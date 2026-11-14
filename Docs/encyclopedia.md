# 도감 1차 구현

- 레벨에 배치한 `BP_EncyclopediaTerminal`과 기존 상호작용 방식으로 연다. 도감 전용 단축키와 HUD 진입 버튼은 없다. Esc/닫기로 닫는다. 단말기가 사라지거나 상호작용 거리를 벗어나면 닫힌다. 인벤토리·대화·배치·다른 모달 사용 중에는 진입하지 않는다. 독립 실행에서만 게임을 일시정지한다.
- 분류: 적 / 무기 / 탄종. 일반 아이템·부착물·세력·지역은 제외한다.
- 적은 로컬 플레이어가 종류별로 처음 처치하면 해금된다. 색상 변형은 같은 종류로 묶고 보스는 더 구체적인 클래스 정의로 구분한다. 실험실 처치는 제외한다.
- 무기·탄종은 기존 `EverAcquiredItemIds`를 참조한다. 최초 획득 이후 소비·분실해도 해금 상태는 유지한다. 기존 세이브의 획득 이력도 반영한다.
- 무기는 종류 → 등급 → 현재 언어의 이름 → ID 순이다. 탄종은 실제 무기 데이터의 호환 탄약 태그를 역참조한다. 권총/기관단총 공용 탄은 두 하위 분류에 표시하지만 진행도는 ID별로 한 번만 센다.
- 미해금 이름·설명·능력치·등급은 숨기며 투명 초상의 검은 실루엣만 표시한다. 적 상세는 이름·외형·설명·전투 특징만 표시하며 약점·지역은 없다.
- 획득 가능한 무기/탄약 목록과 기본 탄창·재장전·관통·피해 배율은 기존 아이템/무기 SSOT를 사용한다. 부착물 보정과 연구 보정은 적용하지 않은 기본 정보다.

## 에디터에서 수정할 에셋

- `/Game/UI/Encyclopedia/WBP_EncyclopediaPanel_GPT6Astra`: 전체 화면, 탭, 종류 필터, 목록, 상세 패널.
- `/Game/UI/Encyclopedia/WBP_EncyclopediaEntry_GPT6Astra`: 목록 한 행. `EntriesList`의 EntryWidgetClass로 연결된다.
- `/Game/Interaction/BP_EncyclopediaTerminal`: 배치 가능한 도감 단말기. 부모는 `AEncyclopediaTerminal`이며 VisualMesh, 상호작용 거리/마커, PanelWidgetClass를 편집할 수 있다. 원하는 레벨에 직접 배치한다. 이번 변경에서는 레벨에 배치하지 않았다.
- `/Game/UI/Encyclopedia/DA_EncyclopediaEnemies`: 안정적인 적 종류 ID, 판정 클래스, 문자열 키, 초상. 가장 구체적인 상속 클래스가 우선한다. 종류 ID는 저장 키이므로 기존 ID 변경 시 이관을 고려해야 한다.
- `/Game/UI/Encyclopedia/Portraits/`: 실제 적 모델에서 캡처한 투명 초상 10개. 색 틴트를 검정으로 바꾸어 실루엣으로 표시한다.

모든 위젯 트리는 WBP에 직렬화되어 있다. 런타임 클래스는 BindWidget, 이벤트, 텍스트/브러시와 ListView 데이터만 갱신한다. 구조·크기·간격은 Designer에서 수정하며 BindWidget 이름은 유지한다. 일회성 에셋 생성 코드는 검증 후 제거한다.

문자열은 `Content/Data/UITextStrings.csv`의 `codex.*` 키와 기존 아이템 문자열을 사용한다. 신규 키는 한국어·영어·일본어·중국어 간체/번체·러시아어를 제공한다. 런타임 언어 변경 시 다시 조회한다.

위젯 작성 모델은 실제 세션 메타데이터에서 확인한 `gpt-6-astra`, 추론 강도 `medium`이며 소스는 `UI/Astra/` 아래에 둔다.

## 검증

`TunaSweeper.Encyclopedia.Contract`는 저장 필드와 실제 WBP/컴파일 클래스/편집 트리를 검사한다. `TunaSweeper.Encyclopedia.Runtime`은 실제 아이템 데이터의 제외 규칙, 분류/등급 순서, 해금, 중복 처리, 적 변형/보스 구분, 실험실 격리, 메모리 세이브 왕복, 슬롯 초기화, 실루엣 알파, 언어 갱신 및 WBP 렌더링을 검사한다. 테스트는 사용자 세이브에 쓰지 않는 별도 GameInstance를 사용한다.

렌더 결과는 `Saved/Automation/Encyclopedia/`에 기록한다. 엔진 자동화는 `UnrealEditor-Cmd.exe -RenderOffscreen`으로 실행하며 컴퓨터 유즈를 사용하지 않는다.

2026-11-15 검증 결과: 생성기 제거 후 UE 5.7 Editor Development 빌드 성공. 도감 2개, 일시정지 메뉴 4개, 차륜형 적 3개 자동화 테스트 총 9개 통과. `Saved/Logs/Encyclopedia-final-tests.log` 참조. 렌더된 적/무기/탄종 화면도 확인했다. 수동 플레이 검증은 수행하지 않았다.

도감 단말기 전환 검증: `TunaSweeper.Encyclopedia.TerminalInteraction`은 실제 BP를 임시 PIE 월드에 생성하여 상호작용 디스패치, 중복/거리 제한, 일시정지와 입력 복구, 일시정지 틱에서 거리 이탈/단말기 제거 시 닫힘을 확인한다. 사용자 저장 파일은 테스트 전용 경로로 격리한다. 레벨 파일은 저장하지 않는다. 생성기 제거 후 UE 5.7 빌드 및 도감 자동화 3개 통과 (`Saved/Logs/EncyclopediaTerminal-verified-tests.log`).
