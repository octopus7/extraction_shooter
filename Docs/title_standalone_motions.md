# 루나 Mk2 타이틀 전용 모션

2026-09-20 제작. 첨부 타이틀 이미지에서 생성한 전신 포즈 레퍼런스를 바탕으로, 기존 Luna Mk2 리그에 아래 애니메이션을 제작했다. 최초에는 미연결 에셋으로 제작했으며, 후속 작업에서 별도 `ABP_LunaMk2_Title`을 만들어 타이틀 액터와 IntroMap에 적용했다.

## 타이틀 적용

`ABP_LunaMk2_Title`의 부모는 `UTunaSweeperTitleAnimInstance`다. C를 시작 시 한 번 재생하고 A로 연결한다. A/B는 각각 2~4회(8~16초) 무작위로 반복한 뒤 AtoB/BtoA를 통해 교대로 전환한다. 완전한 루프 경계에서만 전환하며 프레임의 남은 시간을 다음 클립으로 넘긴다. 메뉴 표시 전환은 애니메이션을 재시작하지 않는다.

AnimGraph는 명시적 시간으로 Sequence Evaluator를 평가하고, 기존 Title Head Look 및 양갈래 Rigid Body 설정을 이어받는다. C의 마지막 0.5초 동안 머리 추적을 부드럽게 켜 뒷모습을 유지한다. 기존 안구 추적과 별도 타이틀 치마 ABP도 유지한다. 이 평가기는 노티파이와 루트 모션 추출을 사용하지 않는다.

`BP_TitlePresentationActor.BodyMesh`, IntroMap에 저장된 타이틀 메시, 네이티브 액터 기본값은 새 ABP를 사용한다. 플레이어 캐릭터는 기존 `ABP_LunaMk2`를 계속 사용한다.

`Tools/TitleMotions/verify_title_abp.py`는 저장된 양쪽 BP 및 IntroMap의 연결 분리를 검사한다. `TunaSweeper.Title.Animation.Timing`은 진입·랜덤 체류·프레임 시간 처리를, `EvaluatedPose`는 5개 구간의 실제 ABP 출력과 원본 클립 포즈 일치를 검사한다. `Presentation`은 IntroMap PIE의 C/A/B 재생과 메뉴 복귀 시 재시작 방지를 검사하고 Saved/Screenshots에 화면을 기록한다. 타이틀 시선·머리카락·치마 회귀 검사도 통과했다.

## 저장된 UE 에셋

폴더: `/Game/Characters/Player/LunaMk2/Animations/Title/`

| 에셋 | 길이 | 동작 |
| --- | --- | --- |
| `AS_LunaMk2_Title_A` | 4초 | 상체를 살짝 앞으로 숙이고 뒷짐을 진 기본 포즈. 발을 고정하고 흉곽·어깨에 미세한 호흡을 준 루프. |
| `AS_LunaMk2_Title_B` | 4초 | 왼다리로 체중을 옮기고 오른발을 약간 앞으로·바깥으로 내민 호흡 루프. |
| `AS_LunaMk2_Title_C` | 3초 | 약 140도 돌아선 뒷모습에서 짧게 멈춘 뒤, 발을 들어 디디며 정면 A 포즈로 전환. |
| `AS_LunaMk2_Title_AtoB` | 1.6초 | 오른발을 들어 B 자세로 옮기는 연결 모션. |
| `AS_LunaMk2_Title_BtoA` | 1.6초 | 오른발을 들어 A 자세로 되돌리는 연결 모션. |

모두 30fps, `SKM_LunaMk2_Skeleton` 사용. 루트 이동은 없고 루트 모션 추출은 비활성화했다. C의 방향 전환은 root 본 회전으로 베이크되어 있으므로, 향후 타이틀 연결 시 액터를 같은 각도로 중복 회전시키지 않는다. A/B는 루프 재생, C와 연결 모션은 한 번 재생한다.

A/B 첫 프레임은 호흡의 같은 위상이다. 연결 모션 시작·끝은 해당 루프의 0초 포즈와 일치한다. 랜덤 전환 시 루프 경계에서 연결 모션을 시작하면 포즈가 정확히 맞으며, 임의 시점 전환은 별도 블렌딩이 필요하다.

## 원본과 미리보기

`TunaSweeper/SourceArt/Characters/LunaMk2/TitleMotions/`:

- `LunaMk2_TitleMotions.blend`: 편집 가능한 5개 Action과 미리보기용 치마·카메라·조명. 기본 선택은 A.
- `AS_LunaMk2_Title_*.fbx`: 원본 Blender 리그의 애니메이션 전용 FBX. UE 에셋은 FBX 재타기팅 대신 실제 UE 바인드 회전을 보존한 JSON 트랙으로 생성했다.
- `AS_LunaMk2_Title_*.json`: UE 로컬 좌표의 위치·쿼터니언 트랙. cm 단위, 30fps, 양끝 키 포함.
- `References/`: 사용자 승인 레퍼런스 시트와 내장 이미지 생성 프롬프트.
- `Previews/`: 실제 리그의 포즈 렌더, 연속 재생 영상과 구간 시간표.
- `unreal_validation.json`: 저장된 UE 에셋을 별도 프로세스에서 평가한 검증 결과.

미리보기는 Blender에서 원본 모델과 타이틀 치마로 렌더링했다. 실제 타이틀 조명·커서 시선·머리카락/치마 물리 시뮬레이션을 포함한 게임 화면은 아니다. 표정·깜빡임·커서 시선은 이번 클립에 추가하지 않았다.

## 검증과 생성 코드 정리

`Tools/TitleMotions/verify_unreal.py`는 실제 UE 스켈레톤의 평가 포즈로 A/B 루프 경계, 발 고정, 호흡 변화, C→A 및 양방향 연결 경계, C의 방향 차이와 발 들기, 유효한 위치·단위 회전·스케일, 루트 이동 비활성화를 검사한다. 실행은 UE 5.7 Python commandlet이며 기존 프로젝트 에셋을 저장하지 않는다.

에셋·원본·일회성 생성기는 `f2ffbeaa`에 함께 검증·커밋했다. 생성기와 조회/렌더 보조 스크립트는 바로 다음 정리 커밋에서 제거했으며, 제거 후 같은 UE 검증을 다시 통과했다. 검증 스크립트는 유지한다. 실행 시 자동 생성 경로는 없다.
