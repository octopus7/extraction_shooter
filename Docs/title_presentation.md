# 타이틀 캐릭터와 스튜디오

`/Game/Maps/IntroMap`에서 두 액터를 각각 편집한다.

- `TS_TitlePresentation` → `/Game/UI/Title/BP_TitlePresentationActor`: 캐릭터, 얼굴, 스커트, 시선 추적, 메뉴 전환 카메라.
- `TS_TitleStudio` → `/Game/UI/Title/BP_TitleStudio`: 호수 배경판, 벽·바닥, 캐릭터 조명 및 환경광. `Presentation Actor`는 위 캐릭터 액터를 참조한다.

배경 원본은 `chatgpt/title_matte_lake.png`, 상하 확장본은 `chatgpt/title_matte_lake_extended.png`, 적용 텍스처는 `/Game/UI/Title/T_TitleMatteLakeExtended`, 머티리얼은 `/Game/UI/Title/M_TitleMatteLake`이다. 확장본은 내장 imagegen으로 원본의 나무·호수·상자 구도를 유지하면서 하늘과 지면을 추가한 정사각 이미지다.

`MatteBackdrop`에는 `/Game/UI/Title/SM_TitleCurvedScreen`을 사용한다. 원본 메시는 `TunaSweeper/SourceArt/Title/SM_TitleCurvedScreen.obj`이며, 중앙보다 가장자리가 카메라 쪽으로 휘어진 2,048삼각형 곡면이다. UE 투영 행렬로 계산한 화면 영역에 8% 여유를 더해 배치한다. 메시 UV는 그림 매핑에 사용하지 않는다.

머티리얼은 `ScreenPosition.ViewportUV`와 `ViewSize`로 카메라 투영 좌표를 계산한다. 정사각 확장본의 비율을 유지하는 cover 매핑으로 화면을 채우며, 화면 중앙에서 가장자리로 갈수록 밉 필터 기반의 부드러운 흐림이 증가한다. `MatteTexture`와 `EdgeBlurPixels`로 텍스처와 흐림 강도를 조절한다. Unlit·Translucent·Opacity 1·깊이 검사 유지·안개 해제·그림자 해제를 사용하여 캐릭터 뒤에 그리되 곡률의 음영이나 불투명 발광 배경의 간접광 영향을 피한다. EyeAdaptation 역보정으로 배경 노출도 상쇄한다.

타이틀 카메라는 Manual 노출을 사용하며 물리 카메라 노출과 로컬 노출 대비를 해제한다. `Title Exposure Compensation`으로 고정 밝기를 조절한다. 게임 플레이 카메라나 전역 자동노출 설정은 바꾸지 않는다.

`Show Studio Geometry` 기본값은 꺼짐이다. 켜면 기존 벽·바닥을 표시하고 호수 배경판을 숨겨 세트장을 별도로 확인할 수 있다. 조명은 두 모드에서 스튜디오 BP가 관리한다.

회귀 검증: `TunaSweeper.Title.Studio.SeparationAndBackdrop`. 실제 타이틀 카메라의 렌더 캡처는 `TunaSweeper/Saved/Screenshots/TitleMatteLake.png`, 화면 비율별 캡처는 같은 폴더의 `TitleProjected_<가로>x<세로>.png`에 저장된다.
