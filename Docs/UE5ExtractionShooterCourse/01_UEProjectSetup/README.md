# 01. UE 프로젝트 설정

## 학습 목표

- UE5.7 C++ 프로젝트의 기본 구성을 파악한다.
- 런타임 모듈과 에디터 모듈의 책임을 분리한다.
- 강의용 프로젝트를 만들 때 필요한 최소 설정과 실제 프로젝트 설정의 차이를 이해한다.

## 강의 흐름

1. `TunaSweeper.uproject`에서 엔진 버전과 활성 플러그인을 확인한다.
2. `TunaSweeper.Target.cs`, `TunaSweeperEditor.Target.cs`에서 빌드 타깃을 확인한다.
3. `TunaSweeper.Build.cs`에서 모듈 의존성을 읽는다.
4. `Content/Data`와 `Source`가 함께 게임 규칙을 구성하는 방식을 설명한다.

## 핵심 개념

- 게임 실행에 필요한 코드는 런타임 모듈에 둔다.
- 에디터 전용 생성, 캡처, 자동화 도구는 에디터 모듈에 둔다.
- 강의 실습에서는 기능을 작게 만들되, 폴더와 클래스 책임은 처음부터 분리한다.
- 데이터는 코드에 하드코딩하지 않고 가능한 한 JSON, DataAsset, Blueprint 설정으로 분리한다.

## 기준 프로젝트에서 볼 지점

- 프로젝트 파일: `TunaSweeper/TunaSweeper.uproject`
- 런타임 모듈: `TunaSweeper/Source/TunaSweeper`
- 에디터 모듈: `TunaSweeper/Source/TunaSweeperEditor`
- 데이터 파일: `TunaSweeper/Content/Data`

## 실습

- 새 C++ UE 프로젝트를 만든다고 가정하고 `Game`, `Editor`, `Data`, `Docs` 폴더를 설계한다.
- 런타임에 필요한 기능과 에디터에서만 필요한 기능을 표로 분류한다.
- 강의용 샘플에서 처음 만들 클래스 3개를 고른다. 예: 플레이어 캐릭터, 상호작용 컴포넌트, 게임 인스턴스.

## 확인 포인트

- 에디터 전용 코드가 패키징 대상 런타임 코드에 섞이면 안 되는 이유를 설명할 수 있다.
- C++ 클래스, Blueprint 파생 클래스, JSON 데이터의 관계를 설명할 수 있다.
- 강의 프로젝트도 처음부터 저장/데이터/입력 기준을 문서화해야 하는 이유를 이해한다.
