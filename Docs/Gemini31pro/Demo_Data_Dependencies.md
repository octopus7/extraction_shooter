# 데모 빌드 리소스 및 데이터 의존성 (Dependencies)

데모 빌드 완성을 위해 필수적으로 맞물려 돌아가는 데이터와 리소스 간의 의존 관계를 정리한 문서입니다. 특정 기능(예: 퀘스트 진행, 적 스폰)이 작동하려면 연결된 하위 데이터가 모두 정상적으로 동기화(Sync)되어 있어야 합니다.

## 데이터 의존성 관계도 (Dependency Graph)

```mermaid
graph TD
    %% 레벨 및 앵커
    Level["데모 맵 (Demo Level)"]
    Level -->|위치/ID 지정| Anchor["Raid Placement Anchor (에디터 배치)"]
    Level -->|기믹 배치| BomberSpawner["Rolling Bomber Spawner"]

    %% 퀘스트 시스템
    QuestStudio["QuestStudio (demo.json)"] -->|퀘스트 내보내기| QuestDef["QuestDefinitions.json"]
    QuestDef -->|다국어 텍스트 참조| QuestText["QuestTextStrings.csv"]
    QuestDef -->|필요/보상 아이템 참조| ItemTable["ItemTable.json"]
    
    %% 스폰 시스템 (데이터 주도)
    Anchor -->|스폰 매칭 (Placement ID)| EnemyPlace["EnemyPlacements.json"]
    Anchor -->|스폰 매칭 (Placement ID)| LootPlace["LootContainerSpawns.json"]
    
    EnemyPlace -->|적 속성 참조| EnemyProf["EnemyProfiles.json"]
    EnemyProf -->|전투 AI 패턴 참조| CombatProf["EnemyCombatProfiles.json"]
    EnemyProf -->|장착 무기 및 드랍 참조| ItemTable
    
    LootPlace -->|루팅 내용 참조| ItemTable
    
    %% 아이템 시스템
    ItemTable -->|다국어 텍스트 참조| ItemText["ItemNameStrings.csv"]
    ItemTable -->|UI 리소스 참조| UAsset["UI Icons (T_UIIcon_...uasset)"]
```

## 주요 의존성 분석 요약

### 1. 퀘스트 데이터 의존성 (`QuestStudio` → `UE5 JSON`)
* **핵심 흐름**: 웹 기반 또는 외부 툴인 `QuestStudio`에서 작성된 노드 기반 퀘스트(`demo.json`)가 언리얼 프로젝트의 `QuestDefinitions.json`으로 완전히 덮어써져야만 인게임에서 동작합니다.
* **상태**: 현재 `ItemTable.json`과 `QuestTextStrings.csv`의 번역 데이터는 존재하지만, 중간 다리 역할인 `QuestDefinitions.json`에 Q2~Q4가 없어서 의존성이 끊어져 있습니다.

### 2. 적/전리품 스폰 의존성 (`Level Anchor` → `JSON Profiles`)
* **핵심 흐름**: 레벨 디자이너가 맵에 배치한 `Raid Placement Anchor`는 게임 로드 시 `EnemyPlacements.json`과 매칭됩니다. 
* **주의점**: 맵 파일(`.umap`)의 앵커 ID와 JSON 내의 `placement_id`가 단 하나라도 어긋나면 해당 적/전리품은 생성되지 않는 고아(Dangling) 상태가 됩니다. 또한 적이 들고 나오는 무기와 전리품 상자의 내용물은 최종적으로 `ItemTable.json`의 아이템 ID에 의존합니다.

### 3. 아이템 및 현지화(Localization) 의존성
* **핵심 흐름**: 모든 아이템, 퀘스트, UI는 하드코딩된 텍스트 대신 `StringKey`를 가지고 CSV 파일(`ItemNameStrings.csv`, `QuestTextStrings.csv`)을 참조합니다.
* **상태**: 데모에 필요한 4종 핵심 아이템(크로우바, 밸브 손잡이, 방수 테이프, 참치 통조림)의 번역 및 아이콘 참조 무결성은 완벽히 유지되고 있습니다.
