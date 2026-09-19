# 얕은 물웅덩이

## 구현 범위

수평 메시 + 젖은 가장자리 데칼 구성을 독립된 `BP_ShallowPuddle`로 구현했다. 기존 물 렌더링 구현에는 의존하지 않는다. Niagara 물방울·파문은 발걸음 이벤트에 연결되어 있다. 2026-09-20에는 얕은 깊이에서 반사가 약해지는 Single Layer Water 대신 Thin Translucent 수면을 추가했다.

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
- 발걸음 이벤트는 수면 위 위치, 이동 속도, 질주 여부, 발걸음 주체를 전달한다. 기본 Blueprint가 짧은 Niagara 물방울·파문을 생성한다. 실시간 유체 시뮬레이션은 없다.
- UI 문자열과 저장 데이터는 추가하지 않는다. 레벨에 저장된 액터 설정만 사용한다.

## 검토할 조건

크기 변경과 yaw, 음수 스케일, pitch/roll이 있는 액터, 윤곽 밖·높이 밖의 발 위치, 겹친 웅덩이, 비어 있는 오디오/머티리얼 참조, 저장 후 새 프로세스 재로딩을 확인한다.

## 배치와 조절

1. Content Browser에서 `/Game/Environment/ShallowPuddle/BP_ShallowPuddle`를 레벨로 드래그한다. 기존 물 플러그인 설정은 필요 없다.
2. 액터의 Z를 **수면 높이**에 맞춘다. 바닥이 보이는 얕은 웅덩이는 지면보다 약 2~6cm 위에서 시작한다. 요철이 큰 곳은 낮은 지형에 배치하거나 지형을 먼저 파낸다. 이 액터가 지형을 자동으로 파지는 않는다.
3. `Half Extent Cm`으로 크기를 조절한다. 기본값 (150, 100)은 300×200cm 지지 평면이며 실제 물 윤곽은 그 안쪽에 있다. XY 스케일도 지원하지만 기본적으로 이 속성으로 크기를 맞춘다. Z 스케일은 물 깊이에 영향을 주지 않는다.
4. `Outline Irregularity`는 윤곽의 굴곡, `Water Roughness`는 반사 선명도, `Ripple Strength/Speed`는 배경 잔물결을 조절한다.
5. 새 수면의 `Absorption`은 `Water Optical Depth Cm`과 함께 바닥 투과색을 조절한다. 실제 지형 깊이가 아닌 미술 조절용 광학 두께를 사용한다. `Scattering`은 보존된 기존 Single Layer Water 머티리얼용이며 새 수면에는 적용되지 않는다.
6. `Wet Edge Width/Wetness/Wet Ground Color`로 주변 젖은 지면의 폭·강도·색을 조절한다. 받는 바닥 머티리얼에 데칼 응답 Color/Roughness가 켜져 있어야 한다. 데칼은 바닥 텍스처를 대체하지 않고 색과 거칠기를 혼합한다.
7. `Max Water Depth Cm`은 발걸음 바닥 판정과 젖음 데칼 투영 깊이다. 수면과 실제 지면의 거리보다 크게 맞추되 다른 층의 바닥까지 포함하지 않도록 한다. 새 수면의 투과색은 별도의 광학 두께로 조절하므로 이 값을 바꿔도 반사 강도가 달라지지 않는다.
8. 움직임·충돌은 원래 지면이 담당한다. 물 액터는 수영, 부력, 이동 감속을 추가하지 않는다. 런타임에 액터 위치/크기/파라미터를 변경하면 `Refresh Puddle`을 호출한다.

물소리는 새로 합성한 `/Game/Environment/ShallowPuddle/Audio/SW_ShallowPuddle_Footstep`을 사용한다. 원본은 `TunaSweeper/SourceArt/Audio/ShallowPuddle/SW_ShallowPuddle_Footstep.wav`이며 0.42초·48kHz·24비트 모노 one-shot이다. 액터 자체 또는 Blueprint의 Class Defaults에서 `Puddle > Footstep > Water Footstep Sound`로 변경할 수 있다. 사운드 참조가 비어 있으면 일반 발소리로 돌아간다. 발소리 발생 주기와 AI 소음 수치는 기존 규칙을 사용한다. 기본 Blueprint의 Niagara 연결과 설정은 [물방울·파문 문서](shallow_puddle_niagara_deferred.md)를 따른다.

## 검토 맵과 검증 도구

- 새 수면 계약 검사: `Tools/ShallowPuddle/verify_surface.py`는 Thin Translucent, Surface ForwardShading, front-layer 허용, 노멀·Specular 입력과 조절 파라미터, BP 기본 연결을 새 프로세스에서 검사한다.
- 깊이 비교 렌더: `Tools/ShallowPuddle/render_surface.py`는 저장하지 않는 검토 월드에서 캐릭터·카메라·수면을 고정하고 바닥만 옮겨 2cm/40cm 깊이를 비교한다. 결과는 `Saved/Automation/ShallowPuddle/Surface/`에 저장한다. `render.json`의 `rendered`는 캡처 완료를 뜻하며 시각 수용 판정은 아니다.

- 맵: `/Game/Environment/ShallowPuddle/Maps/L_ShallowPuddle_Review`.
- 월드 X=-440에 맑은 물, X=0에 기본 물, X=440에 탁한 물 예제를 둔다. 바닥 격자는 물 아래 바닥과 굴절을 비교하기 위한 검토용이다.
- 자동화 필터: `TunaSweeper.Environment.ShallowPuddle`.
- 기존 AI 발걸음 소음 회귀 필터: `TunaSweeper.Combat.Noise.PlayerFootsteps`.
- 저장 에셋 검사: `Tools/ShallowPuddle/verify_unreal.py`를 Unreal Python commandlet으로 실행한다.
- 오프스크린 렌더: `Tools/ShallowPuddle/render_review.py`를 실제 에디터에서 실행한다. 저장된 원본 에셋을 수정하거나 저장하지 않으며 결과는 `TunaSweeper/Saved/Automation/ShallowPuddle/ShallowPuddle_Review.png`에 기록한다.
- 검증은 `TunaSweeper/TunaSweeper.uproject`, UE 5.7, PythonScriptPlugin/EditorScriptingUtilities를 사용한다. NullRHI 검사는 저장/참조/형상 검증용이고 실제 셰이더 검증은 오프스크린 렌더에서 수행한다.

## 알려진 범위

- Thin Translucent 수면은 실제 수심에 따른 물속 체적 산란·굴절 대신 얇은 투과층을 사용한다. 깊은 호수/강 용도로 사용하지 않는다.
- `r.Lumen.TranslucencyReflections.FrontLayer.EnableForProject=True`로 고품질 투명 반사를 기본 활성화한다. 프로젝트 전체 Forward Shading 렌더러를 켜는 설정이 아니다. 실행 중인 에디터에는 설정 재로드/재시작이 필요할 수 있다.
- 고품질 반사는 가장 앞쪽 투명층에 적용된다. 중첩 투명 표면과 Niagara를 실제 플레이 화면에서 확인해야 하며 GPU 비용은 아직 측정하지 않았다. 낮은 그래픽 품질과 HWRT 미지원 환경에서는 반사 표현이 달라질 수 있다.

- 정적인 작은 물웅덩이용이다. 물이 차오르거나 흐르는 시뮬레이션은 없다.
- 윤곽은 절차적인 방사형 마스크다. 임의의 복잡한 강 모양이나 지형 높이에 따른 자동 윤곽 생성은 지원하지 않는다.
- 수면끼리 겹쳐 놓는 배치는 피한다. 발걸음은 가장 높은 유효 수면 한 곳에만 전달하지만, 렌더링 중첩 자체를 합치지는 않는다.
- 반사의 모습은 프로젝트의 반사 설정, 화면 안에 보이는 물체, 하늘 조명에 따라 달라진다. 별도 Planar Reflection이나 SceneCapture를 물웅덩이마다 추가하지 않는다.
- 발걸음으로 퍼지는 링과 물튀김은 `BP_ShallowPuddle`에 연결되어 있다. 부모 C++ 액터만 직접 배치하면 Blueprint의 Niagara 연결은 실행되지 않는다.

## 검증 기록 (2026-09-19)

아래는 최초 Single Layer Water 구현 시점의 기록이다. 새 수면의 기록은 별도 절을 따른다.

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
- 음색의 최종 청취 평가는 사용자에게 맡긴다. 이 오디오 작업 당시 보류했던 Niagara는 후속 Computer Use 작업에서 연결했다.

## 얕은 수면 반사 교체 (2026-09-20)

기본 BP는 `/Game/Environment/ShallowPuddle/Materials/MI_ShallowPuddle_Surface`를 사용한다. 부모 `M_ShallowPuddle_Surface`는 기존 물 머티리얼의 외곽과 월드 노멀을 복제하고 Thin Translucent + Surface ForwardShading으로 변환했다. 기존 `M_ShallowPuddle_Water`와 사용자가 수정한 Niagara는 그대로 보존했다. 엔진 셰이더는 수정하지 않았다.

UE 5.7 `SingleLayerWaterComposite.usf`의 `saturate(DeltaDepth * 0.02)`에 의한 얕은 깊이 반사 감쇠를 새 경로에서는 거치지 않는다. 프레넬에 따른 각도 변화는 유지하므로 바닥이 강하게 비치면 어두운 반사는 여전히 약하게 읽힐 수 있다.

머티리얼 인스턴스의 `Shallow Water Surface` 그룹에서 조절한다. 같은 인스턴스를 공유하는 물웅덩이에 함께 적용되며, 개별 외형은 인스턴스를 복제해 액터의 `Water Material`에 지정한다.

| 파라미터 | 기본값 | 의미 |
| --- | ---: | --- |
| WaterSpecular | 1.0 | 반사율 조절. 범위 0~1이며 1은 100% 거울 반사를 뜻하지 않는다. |
| WaterTransmission | 0.78 | 바닥 투과 배율. 낮출수록 바닥이 어두워져 반사 대비가 커진다. |
| WaterOpticalDepthCm | 3.0 | Absorption과 결합하는 미술 조절용 광학 두께. 지형과 수면 간격에는 종속되지 않는다. |

`WaterRoughness`와 `RippleStrength/Speed`는 기존대로 액터 속성이 우선한다. Thin Translucent의 SurfaceCoverage에 외곽 마스크를 연결해 수면 밖 투과·반사 영역이 사각형으로 남지 않도록 했다. 불투명 코팅은 0이며 굴절 오프셋은 추가하지 않았다.

검증 결과:

- 새 에셋 누락 상태에서 계약 검사 실패를 확인한 뒤 생성했다. 새 프로세스에서 셰이딩 방식, 노멀/반사 입력, 파라미터, 머티리얼 인스턴스와 BP 기본값을 확인했다.
- 고정 카메라/수면/캐릭터 아래 바닥만 이동한 2cm·40cm 비교 렌더에서 새 수면은 2cm에서도 하늘과 캐릭터 반사를 표시했다. 셰이더 컴파일 오류는 없었다.
- 검토 맵 3개 물웅덩이와 DemoRaidMap 2개 물웅덩이가 새 BP 기본값을 상속했다. 해당 맵 파일은 이 작업에서 저장하지 않았다.
- 저장 에셋·0.42초 전용 발소리·발걸음 이벤트 검증 통과. 새 수면 위 작은 링과 사용자가 추가한 큰 링이 함께 보이는 오프스크린 캡처를 확인했다.
- 6개 걷기/달리기 조합과 60초 분량 180회 질주 알림 시뮬레이션 통과. 최대 동시 Niagara 컴포넌트 4개, 종료 후 0개.
- C++ 변경/빌드는 없으며 열린 사용자 에디터를 재시작하지 않았다. Computer Use 최종 색감·강도 조정과 실제 게임 화면 수용 확인은 사용자 지시로 보류한다. GPU 성능 수치는 측정하지 않았다.
