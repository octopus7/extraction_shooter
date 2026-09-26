# LunaMk2 SF 슈트

[사용자 레퍼런스](References/SciFiSuit_Reference.png)를 바탕으로 만든 여덟 번째 의상이다. 불투명한 흰 분절 외장, 차콜색 관절 실링, 청록 발광선, 작은 분홍 장식, 높은 칼라, 장갑, 부츠와 기계식 양갈래 장식으로 구성했다. 원래 캐릭터의 키·체형·얼굴·머리카락 좌표를 유지한다.

## 편집 원본과 출력

- `LunaMk2_SciFiSuit.blend`: Blender 5.2 편집 원본. 122개 의상 파트, 63,140정점, 125,864삼각형, 원래 130본 리그를 포함한다.
- `Models/SKM_LunaMk2_SciFiSuit.fbx`: 의상만 합친 센티미터 단위 FBX. 재질 5개, 추가 본과 애니메이션은 없다.
- `materials.json`: 선형 PBR 색상·거칠기·금속성·발광 값. 별도 새 이미지 텍스처는 사용하지 않는다.
- `Previews/`: 정면·사선·후면 및 타이틀 A/B/C 포즈의 Blender 렌더.
- `blender_validation.json`: 편집 메시, 원본 좌표·본 보존, 실제 39개 포즈 검사.
- `Validation/`: 원래 장식의 가시성 설정과 기존 손가락 변형 비교 근거.
- [런타임 FBX·UE 검증·캡처](../RuntimeOutfits/SciFiSuit/).

## 컬렉션과 가시성

| 컬렉션 | 내용 |
| --- | --- |
| `00_Original_Maid_Preserved` | 원래 메이드 캐릭터와 타이틀 스커트. 숨김 상태로 보존. |
| `01_Character_Preview` | 파생 몸체·눈·머리·양갈래·얼굴과 `SK_LunaMk2_SciFiSuit` 리그. |
| `02_SciFiSuit_Export` | 편집 가능한 새 의상 122개 파트. |
| `03_Preview_Studio` | 검토용 카메라·조명·바닥. |

몸체의 `SciFiSuit_BodyCoverage_Reversible` Mask는 외장 안의 몸체·기존 옷·손발을 가리고 목 피부만 표시한다. 머리는 `SciFi_HideMaidAccessories_Reversible` Geometry Nodes가 FACE 속성 `SciFi_HideMaidAccessory`에 표시된 메이드 프릴·리본 장식 면만 숨긴다. 양갈래의 `SciFiSuit_HideMaidRibbons_Reversible` Mask는 작은 리본 8개 연결 성분만 숨기고 716정점짜리 머리카락 두 성분을 유지한다. 이 수정자들을 끄면 기존 형상이 복원된다.

프릴 아래의 원래 두피 안쪽 면은 고개를 숙일 때 어둡게 보일 수 있다. 이는 남은 메이드 장식으로 확정된 면이 아니므로 머리카락·텍스처를 더 삭제하거나 새 머리띠를 추가하지 않았다. 원래 얼굴 메시와 표정은 유지하며 런타임 Base에는 Face를 합치지 않는다.

## 가중치와 검증

의상은 최대 4개 본의 정규화된 가중치를 사용한다. 몸통은 spine 높이, 팔다리는 twist 본 사이를 연속 보간한다. 부츠는 calf에서 foot로 연결하고, 머리 기계 장식은 head 본을 따른다. 두께와 베벨은 편집 메시로 적용했고 UV를 포함한다.

122개 파트의 비다양체 모서리·느슨한 정점·퇴화 삼각형·면적 0 UV 삼각형은 모두 0개다. `calc_loop_triangles` 면적과 고정된 기준 삼각형을 사용해 타이틀 A/B/C 각각 13개, 총 39개 포즈를 검사했다. 실제 왼손 위치 변화는 20.762cm이며 기하 오류는 없다.

손가락 모서리 늘어남 경고 6건은 원래 손의 같은 모서리와 같은 본 가중치에서 재현되는 특성이다. 왼쪽 약지 끝 관절은 원본 약 2.664배·장갑 약 2.619배, 오른쪽 새끼손가락 관절은 원본 약 2.884배·장갑 약 3.201배다. 장갑의 0.65mm 표면 여유가 비율 차이를 만든다. 손가락 가중치와 원래 접힘을 보존했으며 새 팔 외장의 가중치 경계 문제는 수정했다. 구체적인 정점·가중치·프레임은 `Validation/glove_source_comparison.json`에 기록했다.

FBX 재읽기와 UE 새 프로세스 재읽기 검사를 통과했다. UE에서 원래 130본 기준 변환과의 최대 차이는 위치 0.002043cm, 회전 0.007494도, scale 0.00000936이다. Base와 Clothing은 같은 기존 Skeleton을 사용하며, 실제 UE A/B/C 39개 포즈에서 130개 Leader Pose 변환 차이가 0이다. 기존 얼굴 컴포넌트도 유지된다.

천 물리·추가 LOD는 만들지 않았다. 타이틀 포즈와 정지 자세를 확인했으며 모든 전투·이동 모션을 검증한 것은 아니다. 기존 Blender·UE 에셋의 해시를 보호하고, 일회성 생성기는 검증 후 제거한다. 이후 편집은 이 Blender 원본에서 진행한다.
