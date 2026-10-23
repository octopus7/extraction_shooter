# Shoreline Visual Iteration Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement in this session. No commits until the user's explicit later request.

**Goal:** 잔잔한 해안 물을 실제 렌더 비교로 개선하고 모든 단계의 날짜 기반 스크린샷을 보존한다.

**Architecture:** 독립 ShorelineReference의 저장된 Custom HLSL 재질에 표면 레이어를 추가한다. 읽기 전용 렌더 도구는 카메라/시각/토글을 고정하고 실행별 스크린샷과 manifest를 남긴다.

**Tech Stack:** UE 5.7, HLSL, Unreal Python, Pillow/NumPy.

**Spec:** [설계](../specs/2026-10-23-shoreline-visual-iteration-design.md)

## Global Constraints

- UE 5.7 D3D12/SM6, 기존 수면 높이/비충돌 유지.
- `Resources/Iterations/<실행시각-단계>/<뷰>/yyyymmdd-hhmm.png`, 덮어쓰기 금지.
- 기존 게임플레이/물 자산 변경 금지. 커밋/스테이징 금지.
- 일회성 갱신기 제거 후 저장 자산을 다시 검증한다.

## Review Focus

- 고정 시각에서 프레임 누적으로 생기는 얼룩/깜빡임: 동일 시각 반복 캡처.
- 얕은 영역 밖의 거품/카스틱: 직교 마스크와 레이어별 A/B.
- 과도한 반짝임과 반복 줄무늬: 근접·전체·낮은 각도 및 움직임 검토.
- 기본 자산의 저장/재로드 누락: 별도 UE 프로세스에서 소스와 그래프 검사.
- 동일 분 스크린샷 덮어쓰기: 단계별 디렉터리 및 고유 실행 루트.

## Task 1: 기록 가능한 기준 화면

Files: `Plugins/ShorelineReference/Tools/capture_iteration.py`, `Tools/analyze_iteration.py`.

- [x] 실제 GPU 캡처를 날짜 파일명과 manifest로 보관한다.
- [x] 켜기/끄기에서 표면 차이가 없으면 실패하는 검사로 기존 구현의 미구현 상태를 확인한다.

## Task 2: 표면 레이어와 반복 조정

Files: `Resources/Shaders/CalmShoreline.hlsl`, `Content/Materials/M_ShorelineReference.uasset`.

- [x] 기존 거품과 독립된 표면 반사/잔물결/얕은 빛무늬 입력을 추가한다.
- [x] 실제 렌더를 보고 적어도 첫 구현과 후속 조정을 비교·기록한다.
- [x] 반복 선/강한 반짝임을 줄이고 기본 파라미터를 저장한다.

## Task 3: 검증과 정리

Files: `Tools/verify_assets.py`, `README.md`, `Resources/implementation-ledger.md`, 작업 기록.

- [x] 새 레이어 A/B와 기존 해안 마스크/20초 움직임 검증을 실행한다.
- [x] 최종 이미지·검증 보고서·판단을 기록한다.
- [x] 일회성 자산 갱신기를 제거하고 새 UE 프로세스로 저장 자산을 검증한다.
- [x] 변경 범위를 확인하고 요청/질문 기록을 남긴다. 모든 변경은 미커밋 상태로 둔다.
