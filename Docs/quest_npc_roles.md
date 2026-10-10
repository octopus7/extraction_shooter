# 퀘스트 NPC 제공·완료와 등장 역할

## 제공 NPC와 완료 NPC

퀘스트 정의의 `provider_id`는 수락을 담당한다. 선택 필드 `completion_provider_id`를 생략하거나 빈 문자열로 두면 제공 NPC가 보상 수령도 담당한다. 값을 지정하면 보상 수령 및 보상 가능 표시만 해당 NPC로 연결된다.

인벤토리 퀘스트 화면에서는 보상을 수령할 수 없다. NPC를 통해 진입한 화면의 수락·모두 수락은 제공 NPC 기준, 보상 수령은 완료 NPC 기준으로 검사한다. 아이템 제출 목표의 `target_provider_id`는 별개로 유지된다.

## 수동 배치

콘텐츠 브라우저에서 `/Game/Characters/BabyBoar/BP_BabyBoar`를 원하는 두 위치에 직접 배치한다. 이 작업에서는 맵에 배치하지 않았다.

각 인스턴스의 디테일 패널 `Quest > Presence`에서 다음 값을 지정한다.

- `Presence Quest Id`: 등장 여부를 판단할 퀘스트의 정수 ID.
- `Presence Role = FieldOffer`: 해당 퀘스트를 수락할 수 있을 때만 등장. 수락 즉시 사라진다.
- `Presence Role = BaseResident`: 해당 퀘스트 보상을 받아 `RewardCompleted`가 된 후 등장. 목표 달성만으로는 등장하지 않는다.

정의가 없거나 실행 가능한 목표가 아직 연결되지 않은 퀘스트는 필드용 NPC를 등장시키지 않는다. 에디터에서는 배치 편의를 위해 메시를 표시하며, 게임 시작·퀘스트 진행 변경·저장 로드 시 런타임 조건을 적용한다. 숨김은 충돌·상호작용·표시도 함께 비활성화한다.

등장 역할은 서로 다른 위치에 배치한 인스턴스의 표시를 전환한다. 자동 경로 이동 AI는 포함하지 않는다.

## 메시

원본은 `TunaSweeper/SourceArt/Characters/BabyBoar`에 있다. 최대 32³ 격자 안의 2cm 복셀 메시이며 팔레트 텍스처 UV와 단일 머터리얼을 사용한다. UE 애셋은 `BP_BabyBoar`, `SM_BabyBoar`, `M_BabyBoar`, `T_BabyBoarPalette`이다.
