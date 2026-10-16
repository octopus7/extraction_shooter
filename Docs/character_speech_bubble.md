# 캐릭터 머리 위 말풍선 편집

- WBP: `/Game/UI/SpeechBubble/WBP_CharacterSpeechBubble`
- 배경 스타일: `/Game/UI/SpeechBubble/MI_CharacterSpeechBubble`
- 배경 원본 머티리얼: `/Game/UI/SpeechBubble/M_CharacterSpeechBubble`
- 기본 폰트: `/Game/UI/Fonts/F_TunaSweeperUI` (나눔스퀘어라운드 Regular/Bold 및 기존 CJK·라틴 폴백을 저장된 Font Face로 참조)

WBP 디자이너에서 `BubbleText`의 폰트·크기·색상·줄바꿈, `TextPadding` 안의 텍스트 슬롯 여백, `BodyBox`의 본체 최소/최대 크기를 편집한다. `TailBox`는 실제 이미지를 포함하지 않는 꼬리 크기 가이드이며 Width/Height Override로 꼬리 폭과 길이를 정한다. 런타임은 디자이너의 폰트와 브러시를 덮어쓰거나 위젯 트리를 재생성하지 않는다.

`BodyImage`는 본체와 꼬리의 합집합을 한 번에 그린다. 두 이미지의 테두리를 겹치지 않으므로 접합부에 삼각형 밑변이나 본체의 내부 테두리가 남지 않는다. 머티리얼 인스턴스의 `FillColor`, `OutlineColor`, `BorderWidth`, `CornerRadius`로 배경색·테두리색·테두리 두께·둥근 모서리를 편집한다. `WidgetSize`, `BodyPadding`, `TailDirection`, `TailGeometry`는 런타임에서 계산하는 배치 값이다.

WBP Class Defaults의 `TailReserve`는 꼬리 방향에 확보하는 공간, `TailTipInset`은 크기 가이드 끝과 실제 꼬리 끝 사이의 여백이다. `PreviewTextStringKey`는 기존 로컬라이징 키로 디자이너 미리보기 문구를 선택하며, `PreviewTailDirection`에서 꼬리 없음과 8방향을 확인할 수 있다. 게임 중 문구는 호출자가 전달한 기존 번역 결과를 사용한다.

폰트는 `BubbleText`의 Font Object에서 교체할 수 있다. 메모리에만 존재하는 CompositeFont는 WBP 저장 후 유지되지 않으므로 저장 가능한 Font 에셋을 지정한다. 디자이너에서는 런타임 Tick 없이도 PreConstruct에서 자식 레이아웃을 계산하여 배경 크기를 초기화한다.

`BodyImage`, `BodyBox`, `TailBox`, `BubbleText`의 이름과 타입은 C++ 바인딩용이므로 유지한다. `BodyBox`와 `TailBox`는 루트 Overlay의 직계 자식으로 유지한다. 두 슬롯의 정렬과 본체 슬롯의 여백은 꼬리 방향에 따라 자동 조절되므로 본문 내부 여백은 `BubbleText` 슬롯에서 편집한다.

화면/월드/액터 기준 위치, 캐릭터 머리 추적, 꼬리 방향, 표시 시간, 동일 대상 교체, 핸들로 숨기기, 레벨 전환 시 정리는 기존 `TunaSweeperSpeechBubbleSubsystem` API를 유지한다. 표시 상태는 세이브 대상이 아니다. 기존 적 상태·저금통용 `/Game/UI/WBP_SpeechBubble`은 별도 에셋이다.
