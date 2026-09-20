# 설명용 팝업 초벌

## 에셋과 편집

- `/Game/UI/Tutorial/WBP_TutorialPopup`: UMG 디자이너에서 계층을 직접 편집하는 설명용 팝업.
- `CommonFrame`: 모든 페이지가 공유하는 종이 질감 프레임.
- `PageSwitcher`: Active Widget Index를 0/1/2로 바꾸어 기본 조작, 전투, 아이템 페이지를 편집.
- 각 페이지의 `Illustration`, `Heading`, `Description` 위젯에서 이미지, 배치, 크기, 글꼴, 색상을 수정.
- `PopupSize`: 기준 크기 1440×960. 바깥 `PopupScale`이 화면 크기에 맞게 비율을 유지.
- `ContinueButton`: 표시만 있는 버튼. 클릭 이벤트를 연결하지 않음.
- `F_TutorialText` 및 `FF_Tutorial*`: 기존 프로젝트 글꼴과 CJK 대체 글꼴의 데이터를 포함하는 폰트 에셋.

## 로컬라이징

표시 문구는 `TunaSweeper/Content/Data/UITextStrings.csv`의 `ui.tutorial.*`와 기존 `ui.common.continue` 키를 사용한다. WBP Class Defaults의 `LocalizedTextKeys`는 디자이너 위젯 이름과 문자열 키를 연결한다. 텍스트 변경은 CSV에서, 배치와 스타일 변경은 디자이너에서 한다.

Class Defaults의 `PreviewLanguage`로 게임 인스턴스가 없는 디자이너에서 한국어·영어·일본어를 확인한다. 최소 네이티브 기반 클래스는 저장된 TextBlock의 내용만 갱신하며 위젯을 생성하거나 레이아웃을 덮어쓰지 않는다. 실제 게임 인스턴스가 있으면 기존 게임 언어 설정을 읽는다.

## 현재 범위

게임/HUD/맵 참조, 팝업 표시, 일시 정지, 입력 차단, 페이지 이동 이벤트, 확인 버튼 동작, 다시 보지 않기 및 저장 데이터는 연결하지 않았다. 현재 설명은 확인한 기본 키 매핑을 기준으로 작성한 초벌이다. 향후 실제 사용 연결 시 입력 재매핑 및 언어 변경 이벤트 반영을 함께 검토한다.

## 생성·검증

에디터 전용 생성기로 한 번 만들어 일반 WBP 에셋으로 저장한다. 검증한 에셋과 생성기를 함께 커밋한 직후 다음 커밋에서 생성기를 제거한다. 최종 소스에는 자동 재생성 경로를 남기지 않는다.

`TunaSweeper.UI.Tutorial.AuthoredAsset` 검사는 새 프로세스에서 저장된 디자이너 트리, 3페이지, 25개 문자열 키, 언어별 텍스트, 비연결 버튼, UI 텍스처 설정을 확인하고 실제 UMG를 렌더링한다. 미리보기는 `GeneratedImages/UI/Tutorial/WidgetPreviews/`에 저장된다.
