# 마을 구획 항공뷰 미리보기

`/Game/MainRaid/RaidVillage`에서 세 구획을 독립 `DecalActor`로 표시한다. 초기 배치 판단을 위한 평면 이미지이며 실제 건물이나 충돌을 만들지 않는다. Farm, Workshop, Depot은 기존 블록아웃의 임시 구획 이름이다.

## 편집

- World Outliner의 `EditorPreviews/VillageLayout` 폴더에서 `TS_Village_LayoutPreview_Farm`, `Workshop`, `Depot`을 선택한다.
- 이동 도구로 위치를 조절하고, Z축 회전으로 이미지 방향을 바꾼다. 아래로 향한 투영 회전(Pitch -90°)을 유지한다.
- `Decal Component > Decal Size`의 Y/Z는 평면 크기의 **반값(cm)**이다. 기본값 3000/3000은 60×60m이다. X는 높이 방향 투영 범위의 반값이다.
- `MI_Layout_*`에서 `PreviewTexture`와 `PreviewOpacity`를 바꾼다. 기본 불투명도는 0.9다. 구획마다 별도 인스턴스를 사용한다.
- Outliner의 폴더 눈 아이콘으로 한 번에 숨기거나, 해당 데칼만 삭제한다. 에디터 Game View(G)를 끄면 보인다.

## 범위와 높이

| 구획 | 중심 X/Y(cm) | 크기 |
|---|---|---|
| Farm | -8500 / -8500 | 60×60m |
| Workshop | -3500 / 8500 | 60×60m |
| Depot | 8200 / -700 | 60×60m |

구역 사이 기존 이동 경로에는 투영하지 않는다. 투영 높이는 각 사각형 내부의 지형 높이를 측정해 맞췄다. 이동하거나 지형을 수정하면 위치 Z와 Decal Size X를 함께 조절한다. 투영 상자는 다른 데칼 수신 메시에도 영향을 줄 수 있으므로 높이 범위를 필요한 만큼만 사용한다.

데칼은 `Is Editor Only Actor`와 `Hidden in Game`을 켜 두었다. 에디터 작업에 사용하며 PIE에서 숨기고 쿠킹에서 액터를 제외한다. 랜드스케이프 머터리얼, 높이·레이어 데이터, 기존 프랍과 철도는 변경하지 않는다.

## 에셋과 검증

- 공용 머터리얼: `/Game/MainRaid/EditorPreviews/VillageLayout/M_LocalLayoutPreview` (Deferred Decal)
- 구획별 텍스처·인스턴스: 동일 폴더의 `T_Layout_*`, `MI_Layout_*`
- 원본 PNG·최종 프롬프트: `TunaSweeper/SourceArt/EditorPreviews/VillageLayout/` (`prompts.json`). 내장 imagegen으로 각각 생성했다.
- 재검증: `Tools/VillageLayoutPreview/run_verify.ps1`
- 검증 결과와 렌더: `TunaSweeper/Saved/VillageLayoutPreview/verification.json`, `Overview.png`, 구획별 PNG

검증은 저장한 맵을 새 에디터 프로세스에서 다시 열어 데칼·머터리얼 연결, 아래 방향 투영, 에디터 전용 설정, 1m 간격 경로 비중첩을 검사하고 렌더를 저장한다. 검증 중 콘텐츠 패키지를 저장하지 않는다. 최초 작업의 원본 액터 조사 파일이 있으면 기존 액터 위치와 개수도 대조한다.
