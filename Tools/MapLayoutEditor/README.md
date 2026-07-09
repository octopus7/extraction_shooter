# TunaSweeper Map Layout Editor

웹 기반 2D 맵 배치 MVP입니다. `+X`는 북쪽, `+Y`는 북쪽 기준 오른쪽, `1m = 100 Unreal units` 기준으로 저장합니다.

## 실행

```bash
cd Tools/MapLayoutEditor
npm install
npm run dev
```

Vite가 출력하는 로컬 URL을 브라우저에서 열면 됩니다. 기본 포트는 `5178`입니다.

## 빌드 검증

```bash
cd Tools/MapLayoutEditor
npm run build
```

## 테스트 체크리스트

- 샘플 오브젝트가 보이는지 확인한다.
- 마우스 휠 확대/축소가 포인터 기준으로 동작하는지 확인한다.
- 빈 공간 드래그 또는 마우스 가운데 버튼 드래그로 팬 이동이 되는지 확인한다.
- Point, Rect, Path를 추가하고 선택, 이동, 삭제할 수 있는지 확인한다.
- 속성 패널에서 이름, 태그, 메모, 색상, 위치, 크기, 잠금, 표시 여부가 반영되는지 확인한다.
- 라벨 토글과 zoom 무관 고정 크기 표시를 확인한다.
- 새로고침 후 LocalStorage 자동 저장 데이터가 복원되는지 확인한다.
- Export JSON으로 저장한 파일을 Import JSON으로 다시 불러올 수 있는지 확인한다.
- Reset View와 Reset Project가 동작하는지 확인한다.
