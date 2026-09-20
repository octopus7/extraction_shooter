# 타이틀 캐릭터와 스튜디오

`/Game/Maps/IntroMap`에서 두 액터를 각각 편집한다.

- `TS_TitlePresentation` → `/Game/UI/Title/BP_TitlePresentationActor`: 캐릭터, 얼굴, 스커트, 시선 추적, 메뉴 전환 카메라.
- `TS_TitleStudio` → `/Game/UI/Title/BP_TitleStudio`: 호수 배경판, 벽·바닥, 캐릭터 조명 및 환경광. `Presentation Actor`는 위 캐릭터 액터를 참조한다.

배경 원본은 `chatgpt/title_matte_lake.png`, 텍스처는 `/Game/UI/Title/T_TitleMatteLake`, 머티리얼은 `/Game/UI/Title/M_TitleMatteLake`이다. `MatteBackdrop`은 캐릭터 뒤에서 카메라 방향과 화면 비율에 맞춰 배치되며 이미지 비율을 유지하고 가장자리를 잘라 화면을 채운다. 배경은 조명 음영을 받지 않는 머티리얼을 사용한다.

`Show Studio Geometry` 기본값은 꺼짐이다. 켜면 기존 벽·바닥을 표시하고 호수 배경판을 숨겨 세트장을 별도로 확인할 수 있다. 조명은 두 모드에서 스튜디오 BP가 관리한다.

회귀 검증: `TunaSweeper.Title.Studio.SeparationAndBackdrop`. 실제 타이틀 카메라의 렌더 캡처는 `TunaSweeper/Saved/Screenshots/TitleMatteLake.png`에 저장된다.
