# Editor asset generation cleanup

2026-09-06 사용자의 생성 코드 정리 요청에 따라 기존 one-shot/RunOnce 생성기를 제거했다.
조사·검증은 [애셋 생성 코드 정리 보고서](asset_generation_cleanup_2026-09-06.md)를 참조한다.

## 현재 규칙

- Git에 저장된 `.uasset`/`.umap`이 애셋 편집의 기준이다. 에디터 시작이나 로컬 설정 초기화로 재생성하지 않는다.
- 기존 BP/WBP/ABP, DataAsset, 메시, 머티리얼, Niagara, 입력 애셋과 맵 및 런타임 C++ 부모 클래스는 유지한다.
- `TunaSweeperEditorOneShot_ToCleanupOnExplicitRequest.cpp`, `FTunaSweeperEditorRunOnce`, TaskId/Ensure/Setup 함수와 전용 명령행·콘솔 연결은 제거되었다. 과거 `SetupQuit`/`RebuildAssets` 실행 예제를 사용하지 않는다.
- `Saved/Config`의 `TunaSweeperEditor.RunOnce` 완료 목록은 더 이상 읽지 않는다. 삭제할 필요는 없다.
- 새 애셋에 에디터용 일회성 생성 코드를 작성했다면 생성 결과와 코드를 검증해 먼저 커밋한다. 그 커밋 직후 생성 코드·호출부·전용 의존성을 제거하고 재검증한 결과를 바로 다음 커밋으로 남긴다. 완성된 애셋에 과거 생성기를 일괄 재실행하지 않는다.
- 과거 생성 로직은 Git 이력을 참고한다. 애셋 누락 시 우선 Git/LFS의 원본을 복구한다.

## 유지하는 도구

- `TunaSweeperEditorAssetImport`: 명시적 UI 텍스처·오디오 임포트. 기존 `TunaSweeperImportUiTexture*`/`TunaSweeperImportAudio*` 옵션 유지.
- 맵 캡처·이미지 임포트, FM 사운드 제작, 빌드 대상·레벨 열기·AI 디버그·GLB 도구.
- 물·스플라인 배치, 벽 코핑, 물 깊이 베이크, 안개 시각화 등 현재 레벨 편집 기능.
- 반복 제작용 메시 생성·재임포트·원본 이미지 처리, 검증 전용 Python 도구. 자동 시작과 연결된 완성 애셋 생성기와 구분한다.
