# 재사용 환경·날씨 플러그인 설계

- 작성일: 2026-11-09
- 상태: 사용자 요청에 따른 상세 계획용 설계안. 제품 코드·애셋·음원은 아직 제작하지 않았다.
- 사용자 확정 범위: 1차는 시각·음향 표현과 음원 자체 제작. AI·플레이어 영향은 이후 확장할 수 있게 한다. 재사용 가능한 UE 플러그인으로 만든다.
- 아래 이름·수치·제작 방식·1차 제외 항목은 구현을 구체화하기 위한 제안이며, 이미 구현되거나 사용자가 개별 승인한 사실로 취급하지 않는다.
- 구현 계획: [2026-11-09-environment-weather.md](../plans/2026-11-09-environment-weather.md)

## 1. 목표와 접근 방식

`EnvironmentWeather`라는 UE 5.7 플러그인을 만든다. 공용 날씨 상태와 전환 규칙을 C++로 관리하고 DataAsset으로 프리셋을 편집한다. 하늘·조명·안개·강수·머티리얼·오디오는 상태를 받아 표현하는 개별 컴포넌트로 분리한다. TunaSweeper 연동 코드는 게임 모듈에 두며 플러그인이 게임 클래스를 역으로 참조하지 않는다.

| 검토 방식 | 장점·대가 | 결정 |
|---|---|---|
| 단일 Blueprint 액터 | 빠르게 배치하지만 상태·렌더링·게임 종속성이 섞이기 쉽다 | 초기 논의에서 비교한 대안 |
| 프리셋 기반 C++ 코어와 교체 가능한 표현부 | 데이터로 연출을 조절하고 다른 프로젝트에 이식할 수 있다 | 채택할 설계안 |
| 기압·습도 기반 기상 시뮬레이션 | 복잡한 자연 변화를 만들지만 튜닝과 검증 범위가 크다 | 1차 범위 제외 |

완료 결과는 독립 플러그인, 작동하는 시연 맵, 새로 합성한 원본 WAV 10개와 SoundWave, 프로젝트 선택적 어댑터, 테스트·사용 문서다. 실제 배포 맵에 날씨를 강제로 활성화하는 것은 이번 기본 구현 범위에 포함하지 않는다. 먼저 독립 시연 맵과 프로젝트 연동 검증 맵에서 완성하고, 적용 대상 맵은 이후 명시적으로 선택한다.

## 2. 범위

### 포함

- 맑음·흐림·약한 비·강한 비·안개: 다섯 프리셋, 네 날씨 계열.
- 강수량, 구름량, 안개량, 풍향·풍속, 돌풍, 표면 젖음과 건조.
- 수동 변경, 선택적 자동 스케줄, 일시적 우선순위 override, 자연스러운 전환.
- 기존 하늘/조명/안개를 명시적으로 바인딩하는 Rig와 독립 샘플 Rig.
- 카메라 주변 Niagara 비, 작성한 차폐 볼륨 내부의 빗방울 제거, 실내외 오디오 보간.
- 원본 바람·비 루프와 돌풍 효과음 제작, 별도 환경음 믹서, 음량 입력 API.
- 에디터 Details 기반 미리보기·검증, 품질 단계, 깨끗한 다른 UE 5.7 프로젝트에서 이식 검증.
- 현재 상태 조회, 위치별 강수 노출도 조회, 상태 변경 이벤트, 명시적 snapshot/restore 계약.

### 1차 제외

- AI 시야·청각, 플레이어 체온·이동·피해·스탯, 퀘스트 로직 변경.
- 자동 낮밤·계절·달력, 태양 천문 계산, 눈·적설, 번개·천둥, 홍수·물리적 웅덩이 생성.
- 서로 다른 지역의 독립적인 날씨 스케줄. 기존 지역 안개는 선택적 연결만 제공한다.
- 임의 건물의 자동 실내 판별, 모든 표면의 물방울 충돌·튀김, 물 플러그인의 파도·반사 개조.
- 게임 전체 음량 설정 UI, 네트워크 복제, 기존 세이브 슬롯에 날씨 영구 저장.
- 외부 음원 다운로드·구매·유료 생성 서비스, 실제 자연 녹음과 동일한 품질 보장.

## 3. 공통 제약

- UE 5.7만 대상으로 한다. `EngineAssociation=5.7`, `EngineIncludeOrderVersion.Unreal5_7`을 유지한다.
- 명시적인 별도 커밋 요청 전까지 Git 커밋·푸시를 하지 않는다. 이전 자동 커밋 규칙보다 최신 금지 지시를 우선한다.
- 다른 대화의 변경, 접근 제한 ProductionPayload, 퀘스트 데이터는 건드리지 않는다.
- UE 확인·실행·종료·빌드 전에 `Docs/unreal_editor_coordination.md`를 따른다. 공유 에디터를 임의 종료/재실행하지 않는다.
- 모든 사용자 노출 문구는 문자열 키로 조회한다. 플러그인 자체 CSV/StringTable을 제공하고 코드에 표시 문구/대체 문구를 하드코딩하지 않는다. 한국어 플레이어 문구는 `기지`를 사용한다.
- 1차는 기존 Details 시스템을 확장하며 신규 UMG 위젯을 만들지 않는다. 위젯이 추가되는 별도 범위에서는 실제 모델명·reasoning을 확인하고 최신 위젯 폴더/이름/WBP 트리 규칙을 적용한다.
- 월드 단위는 cm, 속도는 cm/s, +Z는 위다. 기본 북쪽은 +X이며 플러그인 북쪽 축은 설정으로 교체 가능하다. 풍향은 바람이 이동하는 방향으로 정의한다.
- 한 게임 월드에는 활성 Rig 하나만 허용한다. 여러 PIE 월드의 상태·MPC 인스턴스·오디오를 공유하지 않는다.
- 일회성 애셋 생성 코드는 최종 검증 전에 진입점·의존성과 함께 제거한다. 저장된 애셋을 시작할 때 재생성하지 않는다.

## 4. 현재 프로젝트와 연결 경계

2026-11-09 공개 C++/헤더/플러그인 검색에서 통합 날씨·낮밤 관리자는 발견하지 못했다. 바이너리 Blueprint 내부와 현재 에디터 상태는 조사하지 않았다.

| 현재 기능·근거 | 구현 원칙 |
|---|---|
| `TunaSweeperAnimeTreeActor.cpp`: `SetTreeParameters()`가 `RefreshTree()`를 호출하며 그림자 프록시를 재생성 | 경량 `SetWeatherWindStrength(float)`를 추가해 기존 잎/그림자 MID의 바람 값만 바꾼다. 기존 나무의 풍향 지원은 1차에서 추가하지 않는다. |
| `TunaSweeperExtractionPointActor.cpp`: `SetSmokeSignalWind(FVector2D,float)` | 게임 어댑터가 정규화한 XY 방향과 cm/s를 전달한다. 무풍은 속도 0이다. |
| `RegionalGroundFogActor.cpp`: refresh 시 카드 재생성, 현재 카드 이동은 자체 난수 방향 | 선택적 경량 밀도·풍속 multiplier API를 추가한다. 원래 값과 카드/난수 진행을 유지하며 disable 시 복원한다. 날씨 플러그인의 필수 의존성으로 만들지 않는다. |
| `StylizedWaterBodyActor.cpp`: `RebuildSurface()`는 메시 재생성, 그림 하늘 반사 별도 | 1차에서 수정하지 않는다. 바람을 수류로 덮어쓰지 않는다. 향후 별도 머티리얼 어댑터로 확장한다. |
| `TunaSweeperBgmSubsystem.cpp`: GameInstance 단일 BGM 채널 | 날씨는 월드 소유 UAudioComponent로 독립 재생한다. BGM 교체/종료 콜백 루프를 재사용하지 않는다. |
| `DefaultEngine.ini`: 출력 48 kHz. 공개 소스 검색에 기존 전역 SFX 음량 API 없음 | 자체 WeatherVolume/HostVolume 입력과 지정 가능한 SoundClass/Submix 제공. 기존 설정이 있다고 가정하지 않는다. |
| `DefaultGame.ini`: `/Game/Audio` cook 규칙 | 플러그인 콘텐츠는 별도 참조·PrimaryAssetLabel 및 cook 검증이 필요하다. |

## 5. 구조와 데이터 계약

플러그인은 `EnvironmentWeather` Runtime 모듈과 `EnvironmentWeatherEditor` Editor 모듈을 가진다. 엔진 모듈 외 프로젝트/지역안개/물 플러그인을 의존하지 않는다. Runtime은 Core, CoreUObject, Engine, Niagara, DeveloperSettings를 필요한 가시성에 맞춰 사용한다. Editor는 UnrealEd, PropertyEditor, Slate/SlateCore, Projects를 사용한다. MetaSound·AudioGameplayVolumes는 필수 의존성이 아니다.

`UEnvironmentWeatherSubsystem : UTickableWorldSubsystem`은 Game/PIE에서만 생성하고 등록된 Rig가 있어야 진행한다. 에디터 미리보기는 Editor 모듈의 별도 evaluator를 사용한다. Rig 해제·월드 종료 시 모든 소유 컴포넌트와 delegate를 정리한다.

| 데이터 | 주요 필드·규칙 |
|---|---|
| `FEnvironmentWeatherState` | CloudCover01, RainIntensity01, FogAmount01, Wetness01, Gust01은 0..1. WindDirection은 정규화된 XY, WindSpeedCmPerSecond는 0..2000. 무풍 방향은 마지막 유효 방향 유지. |
| `UEnvironmentWeatherProfile` | 안정적 `FName ProfileId`, 위 상태의 목표값 중 Wetness/Gust를 제외한 값, 전환 시간·채널 지연. 출력별 조명/안개/구름 곡선은 표현 설정에서 매핑한다. |
| `UEnvironmentWeatherConfiguration` | 프리셋 목록·기본 ID·Seed·허용 전환표·180..420초 유지 구간·표현/오디오 설정 참조. `bAutoSchedule=false`가 초기값. |
| `FEnvironmentWeatherTransition` | 현재 표시값으로 시작한 Source, Target, ElapsedSeconds, DurationSeconds, 항목별 지연. 중간에 새 요청을 받아도 값이 튀지 않는다. |
| `FEnvironmentWeatherSample` | 기본 State와 `PrecipitationExposure01`, `AudioExposure01`. 강수 차폐는 이진값, 음향 노출도는 경계에서 연속값이며 전역 강수량과 구분한다. |
| `FEnvironmentWeatherSnapshot` | Version=1, config ID, base profile ID/source/target, elapsed/duration, wetness, schedule 남은 시간, schedule/gust 각각의 RandomStream current seed, 돌풍 대기/진행 시간·길이·세기·serial, auto 여부. UObject/Actor 포인터와 음원 재생 위치는 저장하지 않는다. |

날씨 코어는 게임 시간 DeltaSeconds를 사용한다. pause 시 멈추고 time dilation을 따른다. 상태/시간 계산은 프레임별 단순 산술이며 delegate와 비싼 바인딩 갱신은 최대 10 Hz로 제한한다. 표현 컴포넌트가 필요하면 프레임별로 가벼운 보간을 한다. 에디터 미리보기에는 미리보기 전용 시간을 전달한다.

표준 전환은 15초 SmoothStep이다. 맑음/흐림에서 비로 바뀔 때 구름 지연 0초, 바람 2초, 강수 5초로 하고 각 채널은 전체 15초 안에 목표에 도달한다. 비가 약해질 때 강수는 즉시 감소를 시작한다. 풍향 보간은 최단 각도이며 180도 동률은 양의 yaw 방향이다. 0초 전환은 즉시 적용한다. 음수·NaN·Inf 요청은 변경 없이 거절한다.

기본 프리셋 제안(풍향은 +X): Clear=(구름 .15, 비 0, 안개 .05, 풍속 100), Overcast=(.8,0,.15,300), LightRain=(.85,.3,.2,400), HeavyRain=(1,1,.35,900), Fog=(.65,0,.85,100). 자동 전환은 Clear→Overcast/Fog, Overcast→Clear/LightRain/Fog, LightRain→Overcast/HeavyRain, HeavyRain→LightRain, Fog→Clear/Overcast이며 동일 가중치다. 직접 설정은 모든 프리셋 간 가능하다. 유지 타이머는 전환 완료 후 시작하며 전환 중에는 다음 자동 전환을 예약하지 않는다. 수동 요청은 도착 시 새 유지 시간을 뽑는다. override 동안 base 수동 변경이 없었으면 보관한 남은 시간을 복귀 후 재개하고, base 수동 변경이 있었으면 새 목표 도착 후 유지 시간을 새로 뽑는다.

젖음과 돌풍은 프리셋 보간 대상에서 제외하고 코어가 최종 State에 합성한다. RainIntensity>0이면 `Wetness += RainIntensity*DeltaSeconds/90`, 비가 없으면 `Wetness -= DeltaSeconds/180`, 결과는 0..1이다. 월드 젖음 MPC는 실내에 자동 적용하지 않는다. 샘플 머티리얼 함수에 SurfaceExposure 입력을 두어 지붕 아래/실내 표면은 작성자가 0으로 지정한다. 임의 프로젝트 머티리얼의 자동 젖음 처리는 하지 않는다.

override는 owner weak reference, Priority, 증가하는 sequence, 토큰으로 관리한다. 높은 Priority, 동률이면 최신 sequence가 이긴다. override 동안 자동 스케줄 타이머를 멈추고 해제하면 보관한 base 목표로 보간 후 남은 타이머를 재개한다. owner 파괴 시 해제하며 기본 복귀 시간은15초다. 최상위 override가 바뀌면 현재 표시값에서 새 목표로 전환한다. override 중 수동 SetWeatherProfile은 표시값을 바꾸지 않고 보관된 base 목표/복귀 시간을 갱신한다. 자동 스케줄을 켜거나 끄는 요청은 base 설정에 반영하되 override 중에는 타이머를 진행하지 않는다. 사용 중인 transient override가 있으면 snapshot 생성과 restore를 모두 명시적으로 거절한다. restore는 버전/config/profile 검증 후 원자적으로 반영하고 실패 시 기존 상태와 override 토큰을 유지한다. 실제 세이브 슬롯·맵 전환 persistence는 연결하지 않으며 맵 진입 시 기본 프리셋으로 시작한다.

돌풍은 코어가 관리한다. schedule과 분리된 seeded random stream을 사용하고, 시작 간격은 이전 돌풍 종료 후8..20초, 길이는6/7/8/10초 중 선택, peak 세기는 .25..1이다. envelope는 길이의 앞30% SmoothStep 상승과 뒤70% SmoothStep 감쇠다. 기본 풍속0에서는 이벤트를 발생시키지 않고 다음 간격을 예약한다. 실제 전달 풍속은 `min(2000, WindSpeedCmPerSecond*(1+.35*Gust01))`이다. 코어가 `OnGustStarted(serial,strength,duration)`를 발생시키며 오디오는 같은 사건에 한 번만 반응한다. snapshot은 진행 중 돌풍과 두 난수 stream을 보존하지만 restore 자체는 GustStarted를 재발행하지 않고 one-shot 재생 위치도 복원하지 않는다. 같은 입력/시간/seed의 상태 재현은 보장하고 파형 sample 단위 재현은 요구하지 않는다.

공개 계약은 `GetCurrentState()`, `GetStateAtLocation()`, `SetWeatherProfile()`, `PushWeatherOverride()/PopWeatherOverride()`, `CaptureSnapshot()/RestoreSnapshot()`, `OnWeatherStateChanged`, `SetHostVolume()`다. AI 또는 플레이어 클래스 참조와 게임플레이 부작용은 포함하지 않는다.

## 6. 시각 표현과 차폐

- Rig는 태양 DirectionalLight, SkyLight, HeightFog, VolumetricCloud를 명시적으로 참조한다. 배치된 모든 액터를 검색해 소유하지 않는다. 바인딩 당시 값을 baseline으로 저장하고 intensity/density를 누적 곱하지 않는다. 외부 소유 액터의 태양 회전·전역 노출·프로젝트 렌더 설정은 변경하지 않는다.
- 구름은 플러그인이 제공하는 파라미터화된 머티리얼을 샘플에 사용한다. 기존 구름 머티리얼은 coverage/tint 파라미터 계약을 설정하고 검증한 경우에만 연결한다. 지원하지 않는 표현 채널은 건너뛰고 진단한다.
- Rig를 해제하거나 미리보기를 끝내면 플러그인이 변경한 속성만 복원한다. 같은 속성을 동시에 변경하는 다른 환경 시스템과 중복 소유하지 않는다.
- 비는 월드 공간 파티클을 카메라 주변 반경 3000cm, 위쪽 1800cm 영역에 집중한다. 카메라가 움직여도 살아 있는 빗방울 전체가 카메라에 붙어 움직이지 않는다. 6000cm 이상 순간 이동하면 재중심화하고 기존 파티클을 비운다.
- `AEnvironmentWeatherShelterVolume`은 회전 가능한 box, `bBlockRain=true`, `FeatherDistanceCm=150`, `InteriorAudioExposure01=0`을 가진다. 강수는 box 내부에서 완전 차폐하는 이진 방식이며 부분 투과 지붕은1차에 지원하지 않는다. 음향 노출도만 실외1에서 실내 설정값으로 feather하며 겹친 볼륨의 결과는 각각 최솟값을 사용한다. 가까운 최대16개 강수 차폐 box의 변환/크기를 Niagara에 전달한다. 한 강수 영역과 겹치는 강수 차폐 볼륨이16개를 넘으면 검증 실패로 표시하고 그 강수 영역의 렌더링을 비활성화한다. 조용히 일부 차폐를 무시하지 않는다.
- 실내의 카메라라는 이유로 비 시스템 전체를 끄지 않는다. 창밖 입자는 유지하고 box 내부 입자만 제거한다. 복잡한 실내는 여러 box로 작성한다. 파티클의 이전/현재 위치 선분이 차폐 box를 통과하는 경우도 제거해 높은 속도에서 얇은 지붕을 통과하지 않게 한다.
- 위치별 샘플은 이진 PrecipitationExposure01과 연속 AudioExposure01을 함께 제공한다. 오디오만 AudioExposure01을 시간 보간하며 강수의 이진 차폐를 부분 음량과 혼동하지 않는다. 전역 MPC의 날씨 값을 실내 진입 때문에 바꾸지 않는다. 뷰가 없을 때 카메라 효과/오디오는 중지하며 코어 정책은 유지한다.
- 1차 지원은 단일 로컬 플레이어 뷰다. 다중 PIE 월드는 지원하지만 분할 화면과 네트워크 동기화는 별도 확장이다.

## 7. 음원 자체 제작

전용 생성형 음향 도구를 전제로 하지 않고 오프라인 절차적 합성으로 오리지널 WAV를 만든다. 원본은 `Resources/SourceAudio`, 임포트 결과는 `Content/Audio`에 둔다. 샘플레이트는 프로젝트 출력과 같은 48 kHz, 최종 임포트 WAV는 16-bit PCM이다. 내부 합성·믹스는 float이며 최종 16-bit 변환 시 dither한다. 음원 manifest에 seed·알고리즘 버전·파라미터·채널·프레임 수·checksum·제작 출처를 기록한다.

| 파일 접두사 `EW_` | 수량 | 길이/채널 | 제작 방식 |
|---|---:|---|---|
| Wind_Low / Mid / High | 3 | 각 60초 stereo loop | 유색잡음, 넓은 필터, 불규칙한 느린 진폭/필터 변조. 좁은 공명·규칙적인 사이렌 같은 주기 회피 |
| Rain_Light / Medium / Heavy | 3 | 각 60초 stereo loop | 연속 강수층과 불규칙한 물방울 충격층. 세기는 음량뿐 아니라 충격 밀도와 주파수 분포로 구분 |
| Gust_01..04 | 4 | 6/7/8/10초 mono one-shot | 서로 다른 seed와 상승·감쇠 곡선, 루프 층과 중첩 시 저역 폭주 방지 |

필수 원본은 약 72MB이며 런타임 메모리와는 별도다. 금속 지붕·나뭇잎·물튀김·천둥 음원은 1차 필수 목록에 넣지 않는다. 실내는 기존 비/바람 레이어의 감쇠·필터로 표현한다.

제작 순서는 20초 약풍·약한 비 샘플 → 음색 점검/수정 → 6개 긴 루프와 4개 돌풍 → 수치 검사 → UE 압축 후 루프 검사 → 실제 게임 믹스 비교다. 샘플 청취 의견을 기다리는 동안 코어/시각 구현은 독립 진행할 수 있다. 합성 결과가 청취 기준에 미달하면 미완료로 표시하고 조정한다. 외부 소스 전환은 별도 사용자 요청이 있을 때만 한다.

루프는 주기적 합성 또는 0.5..2초 겹침 구간으로 연결한다. 시작/끝을 무조건 0으로 fade해 매 루프마다 음량이 꺼지는 방식은 사용하지 않는다. 합성용 임시 스크립트와 임포터는 최종 정리에서 제거하되 최종 WAV와 재제작 recipe/manifest는 남긴다.

## 8. 오디오 재생·믹스

- `UAudioComponent + looping SoundWave`가 필수 경로다. SoundWave/압축의 실제 무봉합 루프를 확인하며 `OnAudioFinished`에서 다시 Play하는 방식은 쓰지 않는다.
- 바람 2 + 비 2 + 돌풍 1, 최대 5개 voice로 제한한다. 인접 강도 두 단계만 equal-power crossfade하며 무강수/무풍에서 해당 레이어는 무음이다. 바람/비 루프는 비공간화 배경음, 돌풍도 1차에서는 mono 비공간화 accent다.
- 최초 Play 시 시작 위치를 분산한다. 프레임별 갱신은 gain/filter만 변경하며 음원을 재시작하지 않는다. 장시간 무음 voice는 release하고 재진입 시 새 offset에서 fade-in한다. 믹서가 강수/바람 자산을 시작 전에 미리 로드하며 Tick에서 동기 로드하지 않는다.
- 기본 listener는 로컬 플레이어 view 위치이며 `SetListenerLocationOverride`로 호스트가 실제 오디오 listener 위치를 지정할 수 있다. 비 visual camera 위치와 독립이다.
- 완전 실내 초기값: 비 -24dB, 바람 -16dB, LPF 1800Hz. 실외는 0dB/LPF 해제. Exposure 보간 0.75초. 값은 물리 법칙이 아니라 튜닝용 데이터다.
- 최종 gain은 profile curve × crossfade × exposure × WeatherVolume × HostVolume이다. 두 volume 입력은 0..1이며 기본값 1. BGM·효과음·전체 게임 믹서는 수정하지 않는다.
- pause 시 코어와 AudioComponent를 함께 pause, resume 시 기존 재생 위치에서 재개한다. 맵 이동·Rig 해제 시 audio를 정리하고 다음 월드에서 중복 재생하지 않는다. editor preview는 명시적으로 소리 재생을 켠 경우에만 들린다.
- 자체 SoundClass/Submix/Concurrency를 제공하되 호스트가 대체할 수 있다. 기존 게임에 전역 SFX 설정이 있다고 가정하지 않는다.

## 9. 에디터·콘텐츠·성능

Details에서 preset, blend preview, 자동 스케줄, seed, 바인딩, 품질, volume을 조절한다. Preview/Stop/Restore/Validate 동작을 제공한다. 미리보기는 원본 애셋을 변경하지 않고 시작 전 액터 속성을 복원한다. 에디터 월드의 미리보기와 PIE 상태를 분리한다. 표시명·도움말·오류는 플러그인 문자열 테이블 키로 조회한다.

독립 `/EnvironmentWeather/Maps/L_EnvironmentWeather_Showcase`에 열린 공간, 실내/창문, 처마, 차폐 겹침, 젖음 샘플 표면, 바람 샘플을 둔다. 프로젝트 어댑터 전용 `/Game/Maps/Tests/L_EnvironmentWeatherIntegration`은 기존 게임 자산을 참조할 수 있으나 플러그인 콘텐츠에서 이 맵을 역참조하지 않는다.

품질 Low/Medium/High의 강수 최대 live particle 제안은 1500/4000/8000이다. Niagara Effect Type과 고정 bounds, 이미 생성한 컴포넌트를 재사용한다. 기존 하늘 설정을 임의로 낮추지 않으며 샘플 구름만 품질별 비용을 조절한다.

측정 제안 목표: 1080p, 같은 장치/시연 맵/카메라의 clear 대비 heavy-rain 추가 비용 High에서 CPU p95 0.5ms 이하, GPU p95 2.0ms 이하. 이는 측정 전 성능 보장이 아니다. 장치·RHI·품질·기준 프레임 시간과 60초 warmup 후 120초 측정을 기록한다. 초과 시 파티클·구름 비용을 조정하고 사용자에게 결과를 알린다.

## 10. 검증과 완료 기준

1. **코어:** 모든 직접 전환·허용 자동 전환, 같은 seed 재현, 0초/중단 전환, 180도 풍향, 잘못된 숫자, pause, override owner 종료, snapshot 실패 원자성 검증.
2. **시각:** 맑음→흐림→비→맑음, 안개 전환, 실내에서 창밖 비, 문턱 보간, 얇은 차폐 통과, 카메라 순간 이동, 젖음 후 건조를 GPU 렌더로 확인. NullRHI만으로 시각 성공을 주장하지 않는다.
3. **오디오 파일:** 정확한 형식·길이·checksum, finite samples, clipping 0, 각 채널 DC 절댓값 <=0.001(-60dBFS), 원본 peak <=-6dBFS. 경계 sample jump가 일반 내부 jump 분포 p99를 초과하는지, 경계 50ms RMS가 인접 250ms보다 3dB 이상 낮아지는지 검사하고 경고 구간을 점검한다. stereo mono-downmix 에너지 감소 6dB 초과도 검토한다.
4. **오디오 믹스:** 압축된 SoundWave에서 루프 seam과 최대 5 voice 확인. 최댓값/교차 구간을 캡처해 weather submix true peak <=-6dBTP로 조정한다. 게임 전체 사운드와의 마스킹은 별도 청취 판단이다.
5. **청취:** 3..5분 반복, 비/바람 단독·중첩, 문턱, 헤드폰/스피커/mono를 비교한다. 클릭·반복감·기계음·고역 피로를 확인한다. 자동 수치 검사는 자연스러운 음색의 증거가 아니며 최종 음색 수용 여부는 사용자 청취로 남긴다.
6. **수명/독립성:** 두 PIE 월드, 중복 Rig, 미리보기 복원, 10회 맵 이동 후 잔여 voice/컴포넌트 없음, 누락 자산의 채널별 안전한 비활성화, 플러그인→게임 역의존성 없음.
7. **재사용/cook:** 기본 플러그인만 복사한 깨끗한 UE 5.7 호스트에서 샘플 실행과 cook 결과의 SoundWave/Niagara/머티리얼 참조를 확인한다. 저장된 애셋만으로 일회성 생성기 제거 후 다시 빌드·검증한다.

## 11. 확장 경계와 남겨두는 정책

AI는 향후 위치별 State/Exposure와 이벤트를 읽고 자체 감지 보정 규칙을 적용한다. 플레이어 영향도 별도 컴포넌트가 상태를 읽도록 한다. 플러그인 코어에 게임 Actor 타입, 데미지, AI perception, 세이브 슬롯을 넣지 않는다. 멀티플레이는 별도 서버 권한/복제 액터가 base state·전환 시각·seed를 전송하도록 확장하며 UWorldSubsystem 자체의 자동 복제를 가정하지 않는다.

낮밤은 외부 시간 공급자와 별도 조명 어댑터로, 지역 기후는 위치 샘플 evaluator로, 물은 경량 머티리얼 어댑터로 확장한다. 1차에서는 이 기능의 빈 실행 경로나 가짜 구현을 만들지 않는다.

## 12. 근거

별도 설계 검토에서 override 중 수동 변경/restore, 돌풍 snapshot, 부분 차폐의 의미 세 가지가 지적되었다. 본 문서 5·6절에 base 갱신·활성 override restore 거절·논리 돌풍 상태 보존·강수/음향 노출도 분리로 반영했다. 이는 구현 전 설계 검토 결과이며 런타임 검증 통과를 뜻하지 않는다.

- 프로젝트: `Docs/game_conventions.md`, `Docs/save_persistence.md`, `Docs/unreal_editor_coordination.md`.
- 코드: `TunaSweeper/Source/TunaSweeper/Private/Environment/TunaSweeperAnimeTreeActor.cpp`, `Private/Interaction/TunaSweeperExtractionPointActor.cpp`, `Private/Subsystem/TunaSweeperBgmSubsystem.cpp` (뒤 두 경로도 같은 게임 모듈 기준).
- 플러그인: `TunaSweeper/Plugins/RegionalGroundFog`, `TunaSweeper/Plugins/StylizedWater`, `TunaSweeper/Plugins/PixelWorldBuilder`의 모듈·문자열 테이블 구조.
- [UE 5.7 Plugins](https://dev.epicgames.com/documentation/en-us/unreal-engine/plugins-in-unreal-engine?application_version=5.7): 코드/콘텐츠 플러그인과 모듈 분리.
- [UE 5.7 Material Parameter Collections](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-material-parameter-collections-in-unreal-engine?application_version=5.7): 공용 재질 파라미터 전달.
- [UE 5.7 Niagara scalability](https://dev.epicgames.com/documentation/en-us/unreal-engine/scalability-and-best-practices-for-niagara?application_version=5.7): Effect Type·인스턴스 수·bounds·프로파일링.
- [UE 5.7 Audio import](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-audio-files?application_version=5.7): WAV 임포트와 변환/압축 설정. 48kHz·10개·길이·음량 기준은 본 설계의 선택이다.

