# 용암 착탄과 지면 이펙트

UE 5.7 자산 경로: `/Game/Effects/LavaGround`.

| 자산 | 역할 |
| --- | --- |
| `NS_LavaImpactBurst` | 착탄 순간의 암석 파편, 먼지, 화염 충격파, 굴절 충격파, 용융 불티 |
| `NS_LavaGround` | 약 10초 유지되는 용암 표면, 입체 암석판, 열 왜곡, 불티, 낮은 화염 |

두 NS는 서로를 생성하거나 참조하지 않는다. 필요한 효과만 단독 생성하거나 같은 위치와 회전에서 함께 생성한다. 기본 지면은 XY 평면, 위쪽은 +Z이며 지면 지름은 약 480cm다. 평평한 바닥 기준으로 제작했고 경사면 밀착이나 지형 투영, 피해 판정, 투사체 충돌 이벤트 연결은 포함하지 않는다.

`L_LavaImpactGround_Preview`에는 두 효과의 `_PreviewLoop` 버전을 별도 NiagaraActor로 겹쳐 배치했다. 약 12초 주기로 반복하는 프리뷰용 복사본이며 실제 게임에서는 접미사가 없는 두 NS를 사용한다. 아웃라이너에서 각 액터를 숨겨 단독 표현을 확인할 수 있다.

공간 왜곡은 `M_Lava_HeatRefraction`, `M_Lava_ShockRefraction`의 2D Offset 굴절로 표현한다. Niagara 경량 이미터의 Dynamic Material Parameter 0의 X 채널은 입자 정규화 수명 0→1을 전달하며, 등장·냉각·소멸 마스크에 사용한다. 이 연결을 유지해야 한다.

비주얼 기준 원본과 생성 프롬프트는 `chatgpt/lava_ground_visual_target_v1.png`, `chatgpt/lava_ground_visual_target_v1_prompt.md`에 보존했다. 실제 UE 렌더는 `chatgpt/lava_ground_ue_preview_v1.png`에서 확인한다. 기준 이미지를 바탕으로 만든 첫 구현이며 후속 미술 조정 대상으로 사용한다.
