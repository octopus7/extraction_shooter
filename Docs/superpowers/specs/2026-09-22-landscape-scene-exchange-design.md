# Landscape Scene Exchange 플러그인 설계

## 1. 목적과 범위

`LandscapeSceneExchange`는 Unreal Engine 5.7 에디터에서 현재 월드의 랜드스케이프와 사용자가 명시적으로 선택한 액터를 외부 도구 또는 AI가 읽을 수 있는 패키지로 내보내고, AI 또는 사용자가 만든 추가 배치 델타를 다시 Unreal 월드에 미리보기·적용하는 에디터 플러그인이다.

1차 버전의 입력 범위는 다음과 같다.

- 현재 에디터 월드
- 하나 이상의 선택된 Landscape 프록시
- 사용자가 선택한 정적 메시 액터 또는 컴포넌트
- 선택적으로 `SceneExchangeSource` 태그가 붙은 액터

선택되지 않은 일반 액터, 스켈레탈 메시, 폴리지 인스턴스, 스플라인, 볼륨은 1차 버전에서 자동 추출하지 않는다. 데이터 계약에는 이후 확장을 위한 `kind`와 provenance 필드를 둔다.

플러그인은 런타임 게임 로직이나 조리된 패키지에 의존하지 않는다. 캡처·추출·미리보기·적용은 에디터 전용이며, 최종 게임 액터가 플러그인 런타임 모듈에 의존하지 않도록 한다.

## 2. 플러그인 모듈과 프로젝트 통합

플러그인 디렉터리는 `TunaSweeper/Plugins/LandscapeSceneExchange`로 둔다.

```text
LandscapeSceneExchange/
  LandscapeSceneExchange.uplugin
  Source/
    LandscapeSceneExchange/
      ...
    LandscapeSceneExchangeEditor/
      ...
```

`LandscapeSceneExchange` 모듈은 UE 오브젝트를 최소화한 스키마·해시·좌표·연산 타입과 직렬화 규칙을 제공한다. `LandscapeSceneExchangeEditor` 모듈은 에디터 월드 접근, SceneCapture, Landscape 샘플링, 메시 추출, Slate/ToolMenus, 트랜잭션 적용을 담당한다.

플러그인은 `CanContainContent=false`, 기본 활성화, `PostEngineInit` 에디터 모듈로 구성한다. 현재 프로젝트의 `TunaSweeperMapCaptureActor`, `TunaSweeperMapDefinition`, `TunaSweeperEditor`의 디테일 커스터마이즈·콘솔 명령 패턴을 참고하되, 교환 패키지의 좌표와 파일 포맷은 플러그인이 소유한다. 기존 지도 캡처 메타데이터와의 변환은 에디터 어댑터로 제공한다.

에디터 UI는 별도 도킹 탭 `Landscape Scene Exchange`와 레벨 에디터 메뉴를 제공한다.

- `Export Snapshot`
- `Validate Package`
- `Preview Delta`
- `Apply Delta`
- `Re-export Applied Delta`
- `Merge / Rebase`

모든 버튼, 메뉴, 경고, 검증 메시지는 프로젝트의 기존 localization/string-key workflow를 사용한다. 코드에 표시 문자열을 하드코딩하지 않는다.

## 3. 패키지 구조

패키지는 소스 관리와 외부 도구 사용을 위해 디렉터리 형태를 기본으로 한다. 필요하면 동일 파일 집합을 zip으로 묶을 수 있지만 zip은 교환 포맷의 정본이 아니다.

```text
<map>.sceneexchange/
  manifest.json
  snapshot/
    snapshot.json
    entities.json
    landscape/
      basecolor_linear.exr
      basecolor_preview.png
      terrain.glb
    meshes/
      mesh_<meshHash>.glb
  deltas/
    delta_<deltaId>.jsonl
  reports/
    validation.json
    conflicts.json
```

`manifest.json`은 패키지 버전, 현재 snapshot ID/hash, 포함된 델타 ID, 파일별 SHA-256, writer/minReader 버전을 기록한다. JSON은 UTF-8, 정렬된 키, 유한한 숫자, 명시된 소수점 정밀도 정책으로 canonicalize한다.

## 4. 좌표계와 안정성 규칙

프로젝트 기준을 그대로 사용한다.

- Unreal 단위: cm
- 왼손 좌표계
- +X: 북쪽/forward
- +Y: 북쪽 기준 오른쪽
- +Z: 위쪽

모든 snapshot은 `coordinateFrame`을 저장한다. 여기에는 축, 단위, 월드 원점, World Partition origin/rebase 정보, source map package path와 source revision이 포함된다.

캡처 범위는 단순한 중심·크기만 저장하지 않고 다음을 모두 저장한다.

- `boundsWorldCm`
- 캡처 카메라 위치·회전·orthographic width/height
- `worldToRaster` 4x4 행렬
- `rasterToWorld` 4x4 행렬
- 픽셀 중심 규칙
- 이미지 행 방향과 UV 방향
- 타일 크기와 overlap

그래서 후속 도구가 범위나 축 방향을 추론하지 않아도 된다.

소스 엔티티의 ID는 액터 이름이나 배열 인덱스를 사용하지 않는다.

- Landscape: Landscape GUID
- 일반 액터: Actor GUID + 컴포넌트 상대 경로
- GUID를 사용할 수 없는 선택 항목: 월드 경로·컴포넌트 경로·asset path의 canonical hash를 사용하고 `stability=derived`를 기록

메시, transform, entity 순서의 정렬 기준도 명시하여 같은 입력을 두 번 export하면 동일한 snapshot hash를 얻도록 한다.

## 5. Snapshot 데이터

`snapshot/snapshot.json`은 다음 정보를 가진다.

```json
{
  "schema": "landscape-scene-exchange/snapshot/1.0",
  "snapshotId": "snapshot_...",
  "contentHash": "sha256:...",
  "sourceMapPath": "/Game/Maps/RaidMap",
  "sourceRevision": "...",
  "engineVersion": "5.7",
  "pluginVersion": "...",
  "coordinateFrame": {},
  "capture": {},
  "landscapeMesh": {},
  "entityFile": "entities.json"
}
```

`entities.json`의 각 source entity는 다음을 포함한다.

- `entityId`
- `kind` (`Landscape`, `StaticMeshActor`, `StaticMeshComponent`)
- `assetRef` (`softObjectPath`, `classPath`)
- 월드 transform과 capture/landscape local transform
- bounds
- source GUID, actor path, component path
- tags
- `provenance.kind=source`

월드 transform은 사람이 확인하기 위한 값으로도 남기지만, 배치 연산은 반드시 명시된 local space를 사용한다. Quaternion은 normalize하고 `w >= 0`으로 canonicalize한다.

## 6. BaseColor 탑뷰 캡처

기존 `ATunaSweeperMapCaptureActor`와 별개로 `ASceneExchangeCaptureActor`를 제공한다. 캡처 액터는 수동 Box 범위와 선택된 Landscape를 기준으로 동작하며, 기존 지도 캡처 액터의 월드↔UV 규칙과 일치하는 변환 어댑터를 제공한다.

캡처는 다음 두 레이어를 분리한다.

1. `landscapeBaseColor`: Landscape만 보이는 BaseColor 패스
2. `occupancy` 또는 `entityIdMask`: 선택 액터의 footprint를 선택적으로 굽는 보조 레이어

Landscape BaseColor 캡처는 조명, 그림자, fog, AO, post process를 끄고 `SCS_BaseColor` 또는 동등한 BaseColor 전용 패스를 사용한다. 선택 액터를 BaseColor 이미지에 섞지 않아 AI가 지형 색과 기존 배치를 혼동하지 않도록 한다.

정본은 linear EXR이고, 사람이 확인하거나 간단한 AI 입력에 사용하는 sRGB PNG는 파생본이다. 모든 레이어는 다음을 기록한다.

- 해상도와 타일 크기
- 색 공간과 인코딩
- alpha/no-data 규칙
- visibility filter
- raw SHA-256
- 플랫폼 차이를 위한 optional normalized/perceptual hash

이미지는 타일 기반으로 설계한다. 기본 tile size는 1024px, 기본 overlap은 8px이며, 2048px 이하 범위는 한 장으로 저장할 수 있다. 픽셀 중심과 타일 가장자리 규칙은 manifest에 기록한다.

## 7. 간략화 메시 추출

### Landscape

Landscape는 임의의 렌더 삼각형을 복사하지 않고 heightfield로 추출한다.

- 선택된 Landscape bounds를 기준으로 규칙적인 XY grid를 만든다.
- Landscape 높이를 결정적인 샘플링 규칙으로 읽는다.
- 정점과 삼각형 winding은 고정한다.
- 법선은 같은 샘플 grid의 유한 차분으로 계산한다.
- 목표 grid 간격 또는 triangle budget을 설정할 수 있다.
- 동일한 입력·설정·플러그인 버전은 동일한 GLB와 mesh hash를 만든다.

`terrain.glb`는 `LandscapeLocal` 좌표를 기본으로 하고, snapshot에는 landscape GUID와 local-to-world transform을 기록한다. BaseColor 텍스처와 메시 좌표는 같은 capture frame을 사용한다.

### 선택 정적 메시

같은 source mesh를 여러 액터가 공유하면 메시 라이브러리에는 한 번만 저장하고 entity가 `meshHash`를 참조한다. 각 entity는 자신의 transform과 bounds를 별도로 가진다.

1차 버전은 source asset path를 항상 저장하며, 외부 도구가 형상을 필요로 할 때만 결정적 간략화 GLB를 생성한다. 원본 mesh asset을 찾지 못하거나 render data를 읽지 못하면 조용히 생략하지 않고 validation report에 오류를 남긴다.

간략화 설정과 도구 버전은 메시 메타데이터에 포함한다.

## 8. Delta 데이터 계약

델타는 append-only JSONL 파일이다. Snapshot을 수정하지 않는다.

```json
{
  "schema": "landscape-scene-exchange/delta/1.0",
  "deltaId": "delta_...",
  "parentSnapshotId": "snapshot_...",
  "parentSnapshotHash": "sha256:...",
  "parentDeltaIds": [],
  "author": "...",
  "tool": { "id": "...", "version": "..." },
  "generator": {
    "modelHash": "...",
    "promptHash": "...",
    "seed": 123
  },
  "status": "Proposed",
  "ops": []
}
```

각 operation은 다음 필드를 가진다.

- `opId`: 안정적인 UUID 또는 canonical content hash 기반 ID
- `sequence`와 `deps`
- `action`: `Add`, `Update`, `Delete`, `Patch`, `ResolveConflict`
- `targetId`
- `expectedTargetHash`
- `payload`
- `provenance.kind`: `ai`, `user`, `plugin`

추가 배치는 `Add`로만 시작한다. transform에는 `space`를 반드시 기록하며 기본값은 `LandscapeLocal`이다. 필요하면 resolved world transform을 함께 저장하지만 source 값으로 취급하지 않는다.

배치 제약에는 다음을 포함할 수 있다.

- Landscape surface snap
- normal align
- yaw-only 또는 자유 회전
- scale 범위
- 금지 영역/충돌체 검사
- confidence와 rationale

델타에는 `inputHashes`, `randomSeed`, 모델·프롬프트·생성기 버전을 기록하여 동일 입력의 재생성을 추적한다.

## 9. Preview, Apply, Re-export

1. 사용자가 delta 파일을 선택한다.
2. 플러그인이 schema, parent hash, asset path, coordinate frame을 검증한다.
3. 통과한 Add/Update 작업을 ghost actor로 미리 표시한다.
4. 사용자가 적용하면 단일 UE transaction으로 실제 액터를 생성·수정한다.
5. 적용 결과의 materialized hash와 적용 tool version을 delta에 기록한다.
6. 재실행 시 `opId` ledger를 사용해 이미 적용된 작업은 no-op 처리한다.

적용된 추가 배치는 actor tag와 sidecar ledger 양쪽에 provenance를 남긴다. 예를 들어 entity ID와 delta ID를 기록하되, 최종 게임 액터가 플러그인 런타임 모듈을 요구하지 않도록 plain actor metadata를 우선 사용한다.

사용자가 적용 후 위치를 수동 조정하면 기존 AI operation을 덮어쓰지 않고 새 `Update` operation을 append한다. Re-export도 source entity와 delta-owned entity를 분리한다.

## 10. 병합과 충돌

공통 `parentSnapshotHash`가 있는 델타만 자동 3-way merge 대상이다. 부모 hash가 다르면 먼저 stale 상태로 표시하고, stable source ID를 이용한 rebase를 명시적으로 시도한다.

자동 병합 규칙:

- 서로 다른 target: 병합
- 같은 target의 서로 다른 field: 병합
- 같은 field 동시 수정: conflict
- delete/update 충돌: conflict
- expected hash 불일치: conflict
- 공간 겹침, 금지 볼륨, 충돌체 위반: semantic conflict

충돌 양쪽 operation은 삭제하지 않고 `reports/conflicts.json`에 보존한다. 해결은 기존 operation 변경이 아니라 `ResolveConflict` operation으로 기록한다. 자동 우선순위는 표시 순서에만 사용하고 의미상의 승자를 임의로 선택하지 않는다.

## 11. 버전과 마이그레이션

외부 schema 버전과 UE custom version은 분리한다.

- `snapshotSchemaVersion`
- `deltaSchemaVersion`
- `writerVersion`
- `minReaderVersion`
- `minWriterVersion`
- engine/plugin version

Reader는 알 수 없는 필드를 보존하고 destructive downgrade를 수행하지 않는다. 마이그레이션은 원본 패키지를 수정하지 않고 새 패키지와 mapping report를 만든다.

## 12. 명령과 검증

에디터 UI와 함께 다음 콘솔 명령을 제공한다.

```text
LandscapeSceneExchange.Export <packageDir>
LandscapeSceneExchange.Validate <packageDir>
LandscapeSceneExchange.PreviewDelta <deltaFile>
LandscapeSceneExchange.ApplyDelta <deltaFile>
LandscapeSceneExchange.Merge <base> <left> <right>
```

검증 기준은 다음과 같다.

- 같은 월드를 같은 설정으로 두 번 export했을 때 canonical snapshot hash가 동일함
- world↔raster 왕복 좌표 오차가 허용 오차 이내임
- Landscape BaseColor 캡처에 선택 액터 색이 섞이지 않음
- 메시 정점·삼각형 순서와 hash가 결정적임
- 같은 delta를 두 번 적용해도 중복 액터가 생기지 않음
- source snapshot이 delta 적용 전후에 변경되지 않음
- parent hash가 다른 delta는 자동 적용되지 않음
- field conflict와 semantic conflict가 모두 보존됨
- re-export 시 source/delta-owned entity가 분리됨
- 에셋 누락·잘못된 좌표계·깨진 JSON이 부분 적용 없이 `Blocked`가 됨

초기 자동화 검증은 기존 procedural terrain 테스트 맵을 golden fixture로 활용한다. 이후 선택 액터, landscape heightfield, Add/Update/Delete 델타, stale rebase, idempotent apply를 각각 fixture로 추가한다.

## 13. 구현 순서

구현은 다음 순서로 나눈다.

1. 스키마·좌표·canonical JSON·hash·fixture validator
2. Landscape BaseColor exporter와 deterministic terrain GLB exporter
3. 선택 액터 entity exporter와 mesh library
4. delta parser/validator와 ghost preview
5. transaction apply와 applied-op ledger
6. source/delta 분리 re-export
7. 3-way merge, rebase, conflict UI

각 단계는 이전 단계의 golden fixture와 hash를 유지해야 하며, 전체 구현이 끝날 때까지 자동으로 델타를 적용하지 않는다.
