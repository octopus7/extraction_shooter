# LunaMk2 의상 썸네일

각 의상 모델의 캡처를 참조해 내장 `image_gen.imagegen`으로 새 전신 일러스트를 생성했습니다. 6종 모두 **1024 × 1536, 세로 2:3, RGBA 투명 PNG**입니다.

생성 도구가 반환한 PNG와 알파 채널을 그대로 보존했습니다. 로컬 이미지 보정이나 알파 추출은 적용하지 않았습니다. UI에서는 이미지 전체 캔버스를 동일한 2:3 영역에 표시하세요.

| 의상 | 최종 썸네일 | 모델 캡처 |
| --- | --- | --- |
| 메이드복 | [PNG](Images/LunaMk2_Maid_UI.png) | [캡처](Captures/LunaMk2_Maid_Capture.png) |
| 교복 | [PNG](Images/LunaMk2_SchoolUniform_UI.png) | [캡처](Captures/LunaMk2_SchoolUniform_Capture.png) |
| 정비복 | [PNG](Images/LunaMk2_MechanicOutfit_UI.png) | [캡처](Captures/LunaMk2_MechanicOutfit_Capture.png) |
| 운동복 | [PNG](Images/LunaMk2_Sportswear_UI.png) | [캡처](Captures/LunaMk2_Sportswear_Capture.png) |
| 토끼 잠옷 | [PNG](Images/LunaMk2_BunnyPajamas_UI.png) | [캡처](Captures/LunaMk2_BunnyPajamas_Capture.png) |
| 모험가복 | [PNG](Images/LunaMk2_AdventurerOutfit_UI.png) | [캡처](Captures/LunaMk2_AdventurerOutfit_Capture.png) |

## 제작 및 검증 자료

- `prompts.json`: 최종 이미지에 사용한 프롬프트와 캡처 참조 경로. 모든 호출에서 `transparent_background: true`를 사용했습니다.
- `capture_manifest.json`: 같은 카메라로 촬영한 6종 캡처 설정, 표시 오브젝트와 SHA-256. 원본 Blender 파일은 수정되지 않았습니다.
- `image_validation.json`: PNG 무결성, 공통 해상도·비율, 실제 알파 채널과 캐릭터 영역 검사.

의상별로 새 일러스트를 생성했으므로 자연스러운 자세와 발 위치에는 작은 차이가 있습니다. 공통 캔버스의 비율은 동일합니다. 이번 작업에는 Unreal 텍스처 임포트나 의상 교체 UI 연결을 포함하지 않습니다.
