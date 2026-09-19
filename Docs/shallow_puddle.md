# 얕은 물웅덩이

## 구현 범위

2026-09-19 대화에서 승인한 수평 메시 + Single Layer Water + 젖은 가장자리 데칼 구성을 독립된 `BP_ShallowPuddle`로 구현한다. 기존 물 렌더링 구현에는 의존하지 않는다. Niagara 제작은 사용자의 요청으로 보류한다.

## 구현 계획

- [x] 저장 에셋 검증기를 먼저 실행해 물웅덩이 에셋이 없는 상태를 확인한다.
- [x] `ATunaSweeperShallowPuddleActor`에 수평 수면, 젖음 데칼, 조절 가능한 외형, 수면 영역 판정과 발걸음 이벤트를 구현한다.
- [x] 기존 플레이어의 발걸음 타이밍에서 물 영역을 확인하고 물소리와 이벤트를 연결한다. AI 청각 소음 규칙은 유지한다.
- [x] 일회성 Python 제작 코드로 머티리얼, 기본 Blueprint, 독립된 검토 맵을 생성한다.
- [x] UE 5.7 빌드, 영역 판정 자동화, 저장 에셋 새 프로세스 재로딩, 렌더 확인을 수행한다.
- [x] 생성 코드와 검증된 에셋을 함께 커밋한 뒤, 다음 커밋에서 생성 코드만 제거하고 재검증한다.
- [x] 배치법과 Niagara 보류 작업을 문서화하고 프로젝트 에디터를 연다.

## 설계

- 런타임 클래스: `TunaSweeper/Source/TunaSweeper/{Public,Private}/Environment/TunaSweeperShallowPuddleActor.*`.
- 에셋 루트: `/Game/Environment/ShallowPuddle`. 기본 Blueprint는 `BP_ShallowPuddle`, 검토 맵은 `Maps/L_ShallowPuddle_Review`.
- 엔진 기본 Plane을 재사용한다. 수면은 액터의 yaw와 XY 크기를 따르지만 pitch/roll에는 기울어지지 않는다.
- 동일한 방사형 윤곽 수식으로 머티리얼과 발걸음 판정을 맞춘다. 사각형 모서리, 젖은 테두리, 수면보다 높은 바닥과 깊이 범위 밖의 바닥은 물 발걸음에서 제외한다.
- 수면과 데칼은 이동 충돌이나 내비게이션을 만들지 않는다. 걷는 바닥은 원래 지면이다.
- 발걸음 이벤트는 수면 위 위치, 이동 속도, 질주 여부, 발걸음 주체를 전달한다. Niagara 시스템과 실시간 유체 시뮬레이션은 추가하지 않는다.
- UI 문자열과 저장 데이터는 추가하지 않는다. 레벨에 저장된 액터 설정만 사용한다.

## 검토할 조건

크기 변경과 yaw, 음수 스케일, pitch/roll이 있는 액터, 윤곽 밖·높이 밖의 발 위치, 겹친 웅덩이, 비어 있는 오디오/머티리얼 참조, 저장 후 새 프로세스 재로딩을 확인한다.

## 배치와 조절

1. Content Browser에서 `/Game/Environment/ShallowPuddle/BP_ShallowPuddle`를 레벨로 드래그한다. 기존 물 플러그인 설정은 필요 없다.
2. 액터의 Z를 **수면 높이**에 맞춘다. 바닥이 보이는 얕은 웅덩이는 지면보다 약 2~6cm 위에서 시작한다. 요철이 큰 곳은 낮은 지형에 배치하거나 지형을 먼저 파낸다. 이 액터가 지형을 자동으로 파지는 않는다.
3. `Half Extent Cm`으로 크기를 조절한다. 기본값 (150, 100)은 300×200cm 지지 평면이며 실제 물 윤곽은 그 안쪽에 있다. XY 스케일도 지원하지만 기본적으로 이 속성으로 크기를 맞춘다. Z 스케일은 물 깊이에 영향을 주지 않는다.
4. `Outline Irregularity`는 윤곽의 굴곡, `Water Roughness`는 반사 선명도, `Ripple Strength/Speed`는 배경 잔물결을 조절한다.
5. `Absorption/Scattering`은 빛의 흡수·산란 계수(1/cm)다. Absorption을 키우면 바닥이 더 빠르게 어두워지고, Scattering을 키우면 물이 탁해진다. 일반적인 페인트 색상과 의미가 다르다.
6. `Wet Edge Width/Wetness/Wet Ground Color`로 주변 젖은 지면의 폭·강도·색을 조절한다. 받는 바닥 머티리얼에 데칼 응답 Color/Roughness가 켜져 있어야 한다. 데칼은 바닥 텍스처를 대체하지 않고 색과 거칠기를 혼합한다.
7. `Max Water Depth Cm`은 발걸음 바닥 판정과 젖음 데칼 투영 깊이다. 수면과 실제 지면의 거리보다 크게 맞추되 다른 층의 바닥까지 포함하지 않도록 한다. 물의 시각적 깊이는 실제 지면과 수면 사이 거리로 결정된다.
8. 움직임·충돌은 원래 지면이 담당한다. 물 액터는 수영, 부력, 이동 감속을 추가하지 않는다. 런타임에 액터 위치/크기/파라미터를 변경하면 `Refresh Puddle`을 호출한다.

물소리는 새로 합성한 `/Game/Environment/ShallowPuddle/Audio/SW_ShallowPuddle_Footstep`을 사용한다. 원본은 `TunaSweeper/SourceArt/Audio/ShallowPuddle/SW_ShallowPuddle_Footstep.wav`이며 0.42초·48kHz·24비트 모노 one-shot이다. 액터 자체 또는 Blueprint의 Class Defaults에서 `Puddle > Footstep > Water Footstep Sound`로 변경할 수 있다. 사운드 참조가 비어 있으면 일반 발소리로 돌아간다. 발소리 발생 주기와 AI 소음 수치는 기존 규칙을 사용한다. 기본 Blueprint에는 Niagara 시스템이 없으며 후속 연결은 [보류 문서](shallow_puddle_niagara_deferred.md)를 따른다.

## 검토 맵과 검증 도구

- 맵: `/Game/Environment/ShallowPuddle/Maps/L_ShallowPuddle_Review`.
- 월드 X=-440에 맑은 물, X=0에 기본 물, X=440에 탁한 물 예제를 둔다. 바닥 격자는 물 아래 바닥과 굴절을 비교하기 위한 검토용이다.
- 자동화 필터: `TunaSweeper.Environment.ShallowPuddle`.
- 기존 AI 발걸음 소음 회귀 필터: `TunaSweeper.Combat.Noise.PlayerFootsteps`.
- 저장 에셋 검사: `Tools/ShallowPuddle/verify_unreal.py`를 Unreal Python commandlet으로 실행한다.
- 오프스크린 렌더: `Tools/ShallowPuddle/render_review.py`를 실제 에디터에서 실행한다. 저장된 원본 에셋을 수정하거나 저장하지 않으며 결과는 `TunaSweeper/Saved/Automation/ShallowPuddle/ShallowPuddle_Review.png`에 기록한다.
- 검증은 `TunaSweeper/TunaSweeper.uproject`, UE 5.7, PythonScriptPlugin/EditorScriptingUtilities를 사용한다. NullRHI 검사는 저장/참조/형상 검증용이고 실제 셰이더 검증은 오프스크린 렌더에서 수행한다.

## 알려진 범위

- 정적인 작은 물웅덩이용이다. 물이 차오르거나 흐르는 시뮬레이션은 없다.
- 윤곽은 절차적인 방사형 마스크다. 임의의 복잡한 강 모양이나 지형 높이에 따른 자동 윤곽 생성은 지원하지 않는다.
- 수면끼리 겹쳐 놓는 배치는 피한다. 발걸음은 가장 높은 유효 수면 한 곳에만 전달하지만, 렌더링 중첩 자체를 합치지는 않는다.
- 반사의 모습은 프로젝트의 반사 설정, 화면 안에 보이는 물체, 하늘 조명에 따라 달라진다. 별도 Planar Reflection이나 SceneCapture를 물웅덩이마다 추가하지 않는다.
- 발걸음으로 퍼지는 링과 물튀김은 Niagara 보류 작업에 포함되며 현재 제공되지 않는다.

## 검증 기록 (2026-09-19)

- `TunaSweeperEditor Win64 Development` 빌드 성공.
- `TunaSweeper.Environment.ShallowPuddle.Footprint`, `Selection` 두 테스트 성공. 크기/yaw/음수 스케일/수평 유지/수심/윤곽 밖/중첩/숨김 및 머티리얼이 없는 경우를 검사했다.
- 기존 `TunaSweeper.Combat.Noise.PlayerFootsteps` 회귀 테스트 1개 성공.
- 새 프로세스에서 머티리얼·Blueprint·검토 맵 로드, 물리 충돌 없음, 수면 수평 유지 검사 성공.
- 실제 `OnPuddleFootstep`에 Python listener를 바인딩해 수면 위치/속도/질주/주체 전달, 마른 위치/높은 위치/머티리얼이 없는 수면의 이벤트 차단을 확인했다. 비활성 수면 문제는 먼저 실패를 재현한 후 수정했다.
- D3D12 SM6 실제 렌더 캡처를 열어 세 가지 수면과 바닥 비침 및 젖은 테두리를 확인했다. 물웅덩이 셰이더 컴파일 오류는 없었다. 검토용 SceneCapture의 수동 노출에는 물리 카메라 노출 비활성화 override가 필요했다.
- 검증 이미지는 `TunaSweeper/Saved/Automation/ShallowPuddle/ShallowPuddle_Review.png`. 테스트 결과 JSON도 같은 폴더 아래에 있다.
- 플레이어 직접 조작에 따른 오디오 청취 및 Niagara 시각 수용 확인은 자동 테스트로 대체했다고 주장하지 않는다. Niagara 수용 확인은 보류 문서의 후속 작업이다.
- 생성기와 에셋은 `8007e773`에 함께 기록했다. 일회성 생성기를 제거한 뒤 새 프로세스 에셋/이벤트 검증도 오류 0건으로 통과했다. 최종 소스 트리에는 생성기나 시작 시 재생성 경로가 없다.

## 전용 발소리 추가 (2026-09-19)

- 사용자 요청에 따라 기존 오디오 참조를 새 합성 WAV와 SoundWave로 교체했다. C++ 기본값, `BP_ShallowPuddle` 기본값, 검토 맵 세 인스턴스가 모두 `SW_ShallowPuddle_Footstep`을 참조한다.
- WAV 검사: 0.42초, 48kHz/24-bit/mono, 피크 -4.00dBFS, RMS -23.91dBFS, 클리핑 0건, 시작/끝 무음 확인.
- UE 5.7 에디터 빌드 성공. 새 프로세스에서 정확한 사운드 경로, SoundWave 종류, 길이/모노/샘플레이트/비루프, 기존 물웅덩이 및 이벤트 검사 통과.
- 임포트는 오디오 디코더 등록을 위해 `-AllowCommandletAudio`를 사용한다. `-nosound`로 임포트하면 BINKA 디코더가 초기화되지 않는다.
- 연결 검증 시 별도 동시 작업 중인 ATV의 `PA_ATV` 누락을 포함한 로딩 경고 4건이 있었다. 물웅덩이 오디오 검사는 성공했으며 해당 차량 에셋은 이 작업에서 수정하지 않았다.
- 음색의 최종 청취 평가는 사용자에게 맡긴다. Niagara 보류 상태는 유지한다.
