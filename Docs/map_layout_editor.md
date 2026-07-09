# 맵 배치 에디터

`Tools/MapLayoutEditor`는 UE5 레벨을 만들기 전에 필드, 구역, 동선, 스폰 위치, 랜드마크, 퀘스트 위치를 빠르게 배치하기 위한 웹 기반 MVP 툴이다.

## 기준

- 좌표 단위는 meter다.
- 프로젝트 기준과 동일하게 `+X`는 북쪽, `+Y`는 북쪽 기준 오른쪽, `+Z`는 위쪽이다.
- UE 변환은 `1m = 100 Unreal units` 기준이다.
- Export JSON은 `mapData`와 `editorState`를 분리해 저장한다.
- UE import에는 `mapData` 중심으로 사용할 수 있게 구성했다.

## 실행

```bash
cd Tools/MapLayoutEditor
npm install
npm run dev
```

기본 Vite 포트는 `5178`이다. 포트가 사용 중이면 Vite가 다음 포트를 안내한다.

## 주요 기능

- 마우스 휠 포인터 기준 zoom
- Select 상태의 빈 공간 드래그 또는 마우스 가운데 버튼 드래그 pan
- point / rect / path 추가
- 오브젝트 선택, 드래그 이동, Delete/Backspace 삭제
- name, type, position, size, rotation, tags, note, color, visible, locked 편집
- zoom에 영향받지 않는 고정 px 라벨
- LocalStorage 자동 저장과 시작 시 복원
- JSON Export/Import
- Reset View, Reset Project
- 오른쪽 오브젝트 목록을 통한 invisible 오브젝트 재선택

## 검증

```bash
cd Tools/MapLayoutEditor
npm run build
```

## 현재 한계

- path는 MVP 범위에 맞춰 기본 2점 또는 샘플 포인트를 생성하며, 개별 path point 편집 UI는 아직 없다.
- background image, snap, undo/redo, multi-select, layer system, UE import script는 후속 확장 항목이다.
