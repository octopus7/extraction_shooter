# 지하 시설 모듈과 사격장

레벨: `/Game/Environment/Basement/Maps/L_Basement`

사다리실을 중심으로 북쪽(+X)에 사격장, 동·서·남쪽에 시설 확장용 통로를 배치했다. 기존 벙커 레벨과 별개인 편집 가능한 레벨이며, 통로와 방 입구에 문·문틀·문턱·천장이 없다. 사다리는 기존 FacilityRooms 메시를 재사용한 시각 모델이다. 다른 레벨로 이동하는 사다리 상호작용과 실제 표적 게임 로직은 포함하지 않는다.

## 이동과 확장

- Outliner `Basement/` 아래의 `ROOT_Range`, `ROOT_LadderHub`, 각 통로 `ROOT_*`를 이동하면 연결된 자식 액터가 함께 이동한다. 공유 Static Mesh를 사용하므로 개별 부품도 교체할 수 있다.
- `ROOT_Connector_East`, `ROOT_Connector_West`, `ROOT_Connector_South`는 끝단의 바닥 중앙 스냅 기준이다. 새 시설 바닥의 Z=0과 출입구 중심을 이 기준에 맞춘다.
- 연결 기준점과 임시 끝벽 묶음은 해당 통로 루트의 자식이고, 시설 조명도 각 시설 루트에 연결되어 함께 이동한다. 통로 루트 피벗은 사다리실 쪽 연결부 중앙이다.
- 연결부는 240cm 유효 폭이며, 모듈 배치 기준은 40cm이다. 임시 막음벽은 `*_RemovableCap` 묶음으로 분리되어 있어 확장 시 해당 묶음만 제거한다. 문은 없다.
- 기본 벽 길이는 40/80/160/240cm, 높이는 280cm, 두께는 26cm이다. 바닥은 80cm 단위, 두께 22cm, 완성 상면 Z=0이다. 북쪽 통로의 마지막 바닥만 길이 40cm로 조정한다.
- `SM_Basement_CornerConvex`, `SM_Basement_CornerConcave`는 90도를 3개 평면으로 나누는 외측·내측 코너다. 안쪽 반경 40cm이며 평벽 접합점까지의 거리는 40cm이다. 미러링 없이 90도씩 회전해서 재사용한다.
- 원본 모델 좌표는 X=가로, Y=사격 방향, Z=높이(m)이다. 레벨에서는 원본 +Y → UE +X, 원본 +X → UE +Y, 1m → 100cm로 변환했다. 배치 정보는 `layout_manifest.json`에 있다.

## 카메라와 투명벽

프로젝트 기본 카메라는 UE -X에서 +X 방향을 본다. 이 방향에서 캐릭터를 가리는 앞쪽 벽은 컴포넌트 Visibility를 끄고 Hidden In Game을 켰다. 그림자도 끄되 BlockAll과 QueryAndPhysics 충돌은 유지한다. `BasementCollisionOnly` 태그로 찾을 수 있다. 에디터에서도 해당 벽은 숨김 상태로 저장된다.

방 전체를 다른 방향으로 회전·재배치할 때는 새 카메라 방향에 맞게 앞쪽 벽의 표시 상태를 다시 지정한다. 자동 카메라 추적 숨김 로직은 사용하지 않는다. 충돌까지 제거하거나 액터를 삭제하면 투명벽 기능도 사라지므로 표시 설정만 변경한다.

## 원본과 텍스처

- `Basement_Modules.blend`: 개별 모듈을 분리해 보관한 편집 원본.
- `Models/`: 독립적으로 재사용할 수 있는 GLB 9종. 오목 코너의 둥근 부분 아래까지 바닥을 채우는 `FloorCornerFill`을 포함한다.
- `Range/`: `TempFormChat/TunaSweeper_Range`에서 복사한 사격장 개별 모델, 공유 텍스처, 원래 배치 정보. 임시 입력 폴더 없이도 원본 에셋을 사용할 수 있다.
- `Textures/T_Basement_Concrete.png`: 내장 image generation으로 생성한 콘크리트 BaseColor. 벽/바닥은 같은 표면을 공유하고 재질 인스턴스의 Tint와 Roughness로 구분한다. 바닥에는 조금 더 어두운 청회색을 적용했다.

생성 프롬프트: “Use case: stylized-concept. Asset type: seamless base color texture for modular underground corridor walls and floors in an Unreal Engine stylized game. Generate a square, completely flat orthographic albedo texture of pale desaturated blue-grey smooth cast concrete matching a clean softly stylized shooting range. Target average color approximately sRGB #A8BCC9. Very subtle broad cloudy tonal variation, sparse shallow tiny concrete pores, quiet low-contrast surface. Smooth matte poured concrete. No panels, no seams, no edge borders, no bolts, no stripes, no cracks, no text, no lighting, no shadows, no perspective, no objects. Seamless tile on all edges, restrained clean readable game material. Surface should look almost flat and quiet, not gritty or photorealistically busy. Opaque square texture.”

## 검증

`source_validation.json`은 메시 연결·체적·UV·코너 분할 검증 결과다. `Tools/Basement/verify_unreal.py`는 저장된 레벨을 다시 열어 재질·메시·부모 관계·캐릭터 크기 통로·투명벽 충돌·바닥 연결을 검사한다. `Tools/Basement/capture_unreal.py`는 실제 에디터 뷰포트를 캡처한다. 임포트와 생성 스크립트는 일회성 작업 후 제거하며, 시작 시 자동 생성하는 코드는 없다.
