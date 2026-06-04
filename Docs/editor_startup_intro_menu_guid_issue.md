# 에디터 시작 지연과 WBP_IntroMenu GUID 문제

## 요약

에디터를 열거나 PIE를 시작할 때 화면이 멈춘 것처럼 보였던 주된 원인은 일반적인 DDC, AssetRegistry, 셰이더 컴파일 작업이 아니라 `WBP_IntroMenu` 위젯 블루프린트 내부의 오래된 widget variable GUID였다.

문제가 된 GUID는 더 이상 실제 위젯 트리에 존재하지 않는 `DeleteHoldGaugeBox`, `DeleteHoldGaugeOverlay`, `DeleteHoldGaugeRing`, `DeleteHoldGaugeFill` 이름을 `WidgetVariableNameToGuidMap`에 계속 보관하고 있었다. UE 5.7의 UMG 컴파일러는 블루프린트 컴파일 중 이 map을 실제 source widget 목록과 대조하며, map에만 남은 이름이 있으면 handled ensure를 발생시킨다.

## 관측된 증상

최신 로그 기준으로 에디터 초기화 자체는 약 18초에서 28초 범위에서 끝났다. 체감 지연은 주로 `WBP_IntroMenu`가 로드 또는 컴파일되는 구간에서 발생했다.

로그에는 다음 형태의 ensure가 반복됐다.

```text
Ensure condition failed: SeenVariableNames.Contains(It.Key())
Variable [DeleteHoldGaugeBox] was deleted but still has a GUID referenced by WidgetBlueprint [WBP_IntroMenu]
Variable [DeleteHoldGaugeOverlay] was deleted but still has a GUID referenced by WidgetBlueprint [WBP_IntroMenu]
Variable [DeleteHoldGaugeRing] was deleted but still has a GUID referenced by WidgetBlueprint [WBP_IntroMenu]
Variable [DeleteHoldGaugeFill] was deleted but still has a GUID referenced by WidgetBlueprint [WBP_IntroMenu]
```

각 ensure는 오류 리포트 전송 경로까지 지나가며 수 초씩 멈춤을 만들었다. 이전 로그에서는 4개 ensure의 `SendNewReport`가 대략 4.1초, 11.3초, 2.6초, 2.7초를 소비했다.

## 직접 원인

`TunaSweeperEditor.cpp`의 위젯 재빌드 경로는 `WidgetTree`를 비우고 새 위젯을 코드로 다시 구성한다. 기존 helper는 모든 위젯을 변수로 등록하고 `WidgetVariableNameToGuidMap`을 갱신했지만, 실제 source widget으로 잡히지 않는 orphan 위젯 이름이 map에 남는 상황을 완전히 막지 못했다.

특히 legacy 삭제 홀드 게이지용 `DeleteHoldGauge*` 위젯들은 생성만 되고 실제 UI 트리에 붙지 않는 상태였다. UE의 `ForEachSourceWidget()`은 `WidgetTree`에 남은 source object를 기준으로 순회하기 때문에, GUID map에 남은 orphan 이름은 "삭제된 변수의 GUID가 아직 참조됨"으로 판정된다.

## 수정 내용

`TunaSweeperEditor.cpp`에 `SyncWidgetVariableGuidsToSource()`를 추가했다.

이 helper는 블루프린트 컴파일 전에 다음을 보장한다.

- 실제 source widget 이름을 다시 수집한다.
- widget animation 이름도 source 변수 목록에 포함한다.
- source 목록에 없는 GUID map 항목을 제거한다.
- invalid GUID 또는 중복 GUID를 새 GUID로 교체한다.
- source에는 있지만 map에 없는 항목을 새 GUID로 추가한다.

또한 위젯 트리 재빌드 직전 기존 `WidgetVariableNameToGuidMap`을 명시적으로 비우고, 실제 트리에 붙지 않는 legacy `DeleteHoldGauge*` orphan 위젯 생성 코드를 제거했다.

`IntroMenuGraphicsSettingsTaskId`도 새 값으로 갱신해, 다음 에디터 실행에서 `WBP_IntroMenu.uasset`이 다시 컴파일되고 저장되게 했다.

## 검증

`TunaSweeperEditor Win64 Development` 빌드는 성공했다.

수정 후 에디터를 실행해 `WBP_IntroMenu`가 다시 컴파일 및 저장되는 것을 확인했다. 첫 실행 초반에는 저장 전 기존 에셋이 로드되며 stale GUID ensure가 한 번 더 기록될 수 있지만, 저장 이후 로그 구간에서는 같은 `deleted but still has a GUID` ensure가 다시 발생하지 않았다.

검증 기준:

```text
PostSaveGuidEnsureCount=0
```

## 재발 방지 기준

코드로 UMG 위젯 트리를 재생성할 때는 생성한 위젯이 실제 트리에 붙는지 확인해야 한다. 임시 호환용 또는 legacy용 위젯을 생성만 해두고 부모에 붙이지 않으면 source widget 목록과 GUID map이 엇갈릴 수 있다.

새 위젯 재빌드 helper를 추가하거나 기존 helper를 수정할 때는 컴파일 직전에 `WidgetVariableNameToGuidMap`이 실제 source widget 및 animation 목록과 일치해야 한다.
