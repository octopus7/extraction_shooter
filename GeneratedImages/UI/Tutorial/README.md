# 데모 조작 팝업 이미지

내장 image_gen으로 생성한 그림 전용 에셋. UE 임포트와 팝업 구현 전 단계입니다.

| 파일 | 내용 |
| --- | --- |
| T_Tutorial_Frame.png | 약한 종이 질감의 공통 프레임. 안쪽은 불투명, 바깥은 투명 |
| T_Tutorial_Basics.png | 이동 / 상호작용 / 발사 |
| T_Tutorial_Combat.png | 재장전 / 구르기 / 달리기 |
| T_Tutorial_Items.png | 전리품 획득 / 인벤토리 / 회복 |

모두 1536×1024 RGBA PNG. 각 페이지는 세 장면을 포함하며 프레임과 별도 레이어로 배치합니다. 제목, 설명, 키 표시, 버튼은 이미지에 포함하지 않습니다. 설명은 프로젝트 문자열 키로 로컬라이징하고 키 표시는 실제 입력 바인딩에서 표시해야 합니다. 캐릭터는 조작 설명용 범용 탐험가 삽화입니다.

Source/에는 단색 배경 원본, Debug/에는 같은 원본으로부터 파생한 검정·흰색 배경 및 투명도 검수 이미지를 보관합니다. 알파는 프로젝트 .codex/skills/icon-alpha-from-solid-bg/scripts/extract-alpha-from-solid-bg.ps1을 Windows PowerShell에서 실행해 추출했습니다. 원본별 테두리 색 자동 추정, TransparentDistance=22, OpaqueDistance=40, 크기 유지.

## 생성 프롬프트

도구: 내장 image_gen. Combat/Items는 Basics 원본을 캐릭터 및 화풍 참조로 제공했습니다.

### Frame

Use case: ui-mockup. Create only a reusable blank game tutorial popup panel texture, front-facing orthographic, wide landscape 1536x1024. A large warm ivory parchment-paper rectangle occupying 92 percent of canvas with slightly rounded corners, subtle fine paper fibers and very restrained edge wear, thin double dark warm brown border, tiny understated corner ornaments. Broad light uniform empty interior suitable for overlaying three illustrations plus localized UI title, captions and buttons. No pre-drawn buttons, slots, divisions, characters, icons, text, letters, numbers, logos or watermarks. Texture must be faint, clean, calm and readable, not grungy. Outside the rectangle only a perfectly flat solid muted blue #5E7F86 backdrop, no shadows. Do not use backdrop blue within panel.

### Basics

Use case: illustration-story. Production game tutorial illustration asset for TunaSweeper, not a screenshot or complete popup. Wide landscape 1536x1024. Three separated, equally sized pictorial vignettes in a single horizontal row centered vertically, generous empty margins and gaps. Consistent charming hand-painted field-manual style, bold clean dark brown outlines, flat cream and warm orange clothing, simple anonymous compact explorer with backpack and covered face, legible at small sizes. No text, letters, numbers, labels, keycaps, logos, watermarks or panel borders. All subjects fully opaque. Perfectly uniform flat muted blue background #5E7F86, no gradients, texture, ground shadows or scenery. Never use this blue inside subjects. Clear silhouettes and restrained shading. Left vignette: explorer walking with four bold cream directional arrows around their feet indicating movement. Middle: same explorer reaching to open a small supply chest, lid partly raised. Right: same explorer aiming and firing a compact rifle toward the right, small stylized orange muzzle flash, no enemy. Each scene isolated and complete.

### Combat

Use case: illustration-story. Production game tutorial illustration asset for TunaSweeper, not a screenshot or complete popup. Wide landscape 1536x1024. Three separated, equally sized pictorial vignettes in a single horizontal row centered vertically, generous empty margins and gaps. Consistent charming hand-painted field-manual style, bold clean dark brown outlines, flat cream and warm orange clothing, simple anonymous compact explorer with backpack and covered face, legible at small sizes. No text, letters, numbers, labels, keycaps, logos, watermarks or panel borders. All subjects fully opaque. Perfectly uniform flat muted blue background #5E7F86, no gradients, texture, ground shadows or scenery. Never use this blue inside subjects. Clear silhouettes and restrained shading. Left vignette: explorer replacing the magazine of a compact rifle, detached magazine clearly visible and short cream insertion arrow. Middle: explorer tucked into a forward dodge roll, curved cream motion arrow above body, no duplicate body. Right: explorer sprinting, strong running pose and two short cream speed strokes. Each scene isolated and complete. Use supplied image solely as character and art style reference; replace all three actions as specified.

### Items

Use case: illustration-story. Production game tutorial illustration asset for TunaSweeper, not a screenshot or complete popup. Wide landscape 1536x1024. Three separated, equally sized pictorial vignettes in a single horizontal row centered vertically, generous empty margins and gaps. Consistent charming hand-painted field-manual style, bold clean dark brown outlines, flat cream and warm orange clothing, simple anonymous compact explorer with backpack and covered face, legible at small sizes. No text, letters, numbers, labels, keycaps, logos, watermarks or panel borders. All subjects fully opaque. Perfectly uniform flat muted blue background #5E7F86, no gradients, texture, ground shadows or scenery. Never use this blue inside subjects. Clear silhouettes and restrained shading. Left vignette: open supply chest with small can and bandage bundle, curved cream arrow pointing toward a small backpack beside it to convey collecting loot. Middle: open backpack viewed three-quarter, neat visible can, bandage and spare magazine to convey inventory. Right: seated explorer wrapping a clean cream bandage around forearm, small cream medical plus symbol nearby, no blood. Each scene isolated and complete. Use supplied image solely as character and art style reference; replace all three actions as specified.
