# 숲 바닥 낙엽 더미

`SM_LeafLitter` 한 종. 원본은 기존 환경 원본 규칙에 맞춰
`TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter/`에 저장한다.
UE 경로는 `/Game/Nature/ForestProps/LeafLitter/`이다.

- 크기: Blender XYZ 175.92 × 126.06 × 5.19cm. 1 Blender unit = 1m = 100 UE cm.
- 지면 피벗 `(0,0,0)`, 최저점 Z=0. UE 축 변환의 실제 결과는 `unreal_reload_validation.json` 참조.
- 검증된 UE 축: Blender `(X,Y,Z)` → UE `(X,-Y,Z)`, 각 좌표 ×100. 최대 경계 오차 0.000004cm 미만.
- 60장, 960 원본 정점, 1,680 삼각형, 재질 1슬롯, 128×32 RGB 팔레트 1장.
- UE LOD0은 면 노멀/UV 분리 후 5,040 정점을 보고한다. 원본 정점 수와 혼동하지 않는다.
- 접힌 넓은 잎을 네 덩어리로 겹치고 가장자리 8장은 성기게 배치한다. 미세 잎맥·꽃·노이즈 없음.
- 잎마다 1.5mm 닫힌 두께. Opaque / 단면 재질, 알파 오버드로 없음. 겹치는 불투명 면의 비용은 남는다.
- 충돌 없음, 내비게이션 없음, Nanite 없음. UV0은 팔레트 안에 의도적으로 겹친 비퇴화 삼각형.
  UE에서 UV1 라이트맵 생성. 동적 조명과 인스턴싱 용도를 우선한다.

## 배치

평탄한 숲 바닥과 나무 밑에 지면 Z를 맞춘다. 고도 변화가 5cm를 넘는 지면은
한 더미가 지형을 따라 휘어지지 않으므로 작은 스케일을 쓰거나 구역별 높이를 조절한다.
스케일 0.75–1.15, yaw 0–360°, 간격 약 1.2–1.6m를 출발점으로 사용한다.
군락을 연결하려면 10–25%만 겹치고, 동일 yaw의 규칙적 격자는 피한다.
다량 배치는 ISM/HISM 또는 Foliage 인스턴싱을 사용하고 거리에 따른 컬링을 설정한다.
LOD 자동 축소로 1.5mm 면을 무작정 합치면 잎이 사라질 수 있으므로 본 원본은 LOD0 한 개다.
게임 성능 측정이나 실제 레벨 배치는 이 제작에 포함되지 않는다.

## 재현 / 검증

프로젝트 루트의 PowerShell에서:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python Tools/ForestProps/LeafLitter/build_leaf_litter.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python Tools/ForestProps/LeafLitter/verify_fbx.py
./Tools/ForestProps/LeafLitter/run_unreal.ps1
./Tools/ForestProps/LeafLitter/run_unreal.ps1 -VerifyOnly
./Tools/ForestProps/LeafLitter/run_unreal.ps1 -Script render_unreal.py -Render
```

Blender 원본에 팔레트가 packed 되어 있다. FBX에도 텍스처를 내장하며 UE 스크립트는
외부 PNG로 명시적 재질을 만들어 자동 임포트 재질 이름에 의존하지 않는다.
`model_manifest.json`, `fbx_validation.json`, UE import/reload JSON을 검증 근거로 보관한다.
원본의 조명·지면·카메라와 반복 배치 오브젝트는 FBX에 포함되지 않는다.

새 워크트리의 프로젝트 C++ 바이너리 대신 `Saved/LeafLitterHost`의 콘텐츠 전용 UE 5.7
호스트가 실제 `TunaSweeper/Content`를 /Game으로 마운트한다. DDC도 이 임시 폴더에 둔다.
기존 BP·맵·공용 자연물을 저장하지 않는다. 전체 게임 빌드/PIE 검증과 구분한다.
UE 표시 검증은 숨겨진 실제 에디터에서 임시 월드와 SceneCapture2D를 만들고
180프레임을 진행한 뒤 저장된 메시/재질을 캡처한다. 검토 이미지는
`Previews/LeafLitter_UE.png`이며 임시 월드는 저장하지 않고 에디터를 종료한다.
처음 NullRHI 임포트의 Content Browser 동기화 충돌은 SyncToBrowser=0으로 해결했다.
단순 렌더 commandlet의 검은 이미지는 폐기했고 실제 에디터 캡처로 대체했다.

## 기존 아트 확인

Bush는 `/Game/Nature/Bush/Bush_Combined`와 원본 텍스처를 읽기 전용 export하여 조사했다.
GrassLow, Flower, SimpleTree, Wood, RockBasic은 `Blender/`의 기존 .blend를 직접 읽었다.
`Previews/Reference_*.png`는 기존 형상/UV/색을 유지하고 공통 무광 조명으로 비교한 연구 렌더다.
GrassLow의 단순한 날, Flower의 넓은 접힌 잎, SimpleTree의 큰 수관 덩어리,
Wood의 따뜻한 갈색, RockBasic의 큰 면, Bush의 겹친 잎을 참고했다.
새 낙엽은 Wood보다 채도가 낮은 올리브/갈색 팔레트를 사용하며 더 미세한 표면 묘사는 생략했다.
참조 FBX/TGA는 재제작 모델이 아니라 기존 애셋의 임시 조사용 복사본이다.
