# Exposed Roots / 노출 뿌리

재사용 가능한 노출 뿌리 **1종**. 기존 나무/그루터기 아래에 겹치는 소품이며 나무 몸통은 포함하지 않는다.

- 원본: `TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots/ExposedRoots.blend`
- FBX: 동일 원본 폴더의 `Models/SM_ExposedRoots.fbx`
- 텍스처: `Textures/T_ExposedRoots_Palette.png` (64×64, 불투명 4색)
- UE: `/Game/Nature/ForestProps/ExposedRoots/`의 `SM_ExposedRoots`, `M_ExposedRoots`, `T_ExposedRoots_Palette`
- 304 위치 정점 / 580삼각형 / UE 렌더 정점 1,740개(면 노멀·UV seam 분리) / 1재질 / 2 UV / 충돌 없음 / Nanite 미사용.

## 배치

Blender 1m = UE 100cm. FBX는 -Y forward, Z up, unit scale 적용, UE import scale 1. 원점 `(0,0,0)`은 **나무 중심의 지면 높이**다. 바닥 최저점은 원점 아래 8.7cm이며, 끝과 밑면을 흙 속에 묻도록 의도했다. 지상 최고 높이 42.5cm. Blender XY 전체 폭은 약 238.54×296.28cm, 전체 높이 51.2cm. UE 축의 실제 변환/치수는 `unreal_reload_validation.json`에 기록한다.

UE 5.7.4 실측 변환은 `(X, -Y, Z) × 100`이며 최대 경계 오차는 0.000011cm 미만이다. 열린 부분은 UE에서도 -X 방향이다. UE 경계는 X -92.18~146.36cm, Y -148.24~148.04cm, Z -8.70~42.50cm이다.

중앙 구멍 약 지름 45–55cm에 몸통을 겹치고 뿌리 안쪽의 닫힌 단면을 숨긴다. Blender -X 방향에 열린 부채꼴 공간이 있다. 받침목 지름 약 60–90cm를 출발점으로 조합하며, 가는 나무는 전체 배율 0.6–0.85를 권장한다. 반복 배치는 yaw를 달리하고 uniform scale 0.8–1.1 정도를 사용한다. 프리뷰는 동일 모델 6회 반복이며 새 변형 모델이 아니다. 경사지에서는 Z를 약간 내리거나 지면 기울기에 맞춰 회전한다. 단독으로 드러난 중앙 단면은 배치용 접합부다.

7개 닫힌 root shell을 하나의 Static Mesh로 묶었다. 보조 갈래의 부모 뿌리 내부 겹침은 의도된 조립 방식이다. UV0는 팔레트 샘플링을 위해 겹쳐 사용하며, UV1은 smart unwrap으로 분리한 lightmap chart다. 알파·양면 재질·잎·꽃·추가 충돌은 없다. 기존 나무 충돌을 사용한다. 큰 평면과 절제된 황갈색으로 원거리 식별을 우선했다.

## 기존 스타일 확인

`Blender/Wood.blend`, `SM_GrassLow.blend`, `SM_Flower.blend`, `SM_SimpleTree.blend`, `RockBasic.blend` 및 Content/Nature의 6개 대응 폴더를 조사했다. 기존 Wood의 황갈색, RockBasic의 넓은 면, GrassLow의 간결한 실루엣을 기준으로 제작했다. Bush는 UE 원본을 읽기 전용 FBX/TGA로 내보내 확인했다. 일부 Blender 텍스처 경로가 끊겨 비교 렌더에서 저장소 텍스처를 재연결했으며 SimpleTree는 UE base color를 사용했다. 기존 원본/공용 uasset은 수정하지 않았다. 비교 렌더는 동일 크기로 정규화한 색상·형태 참고이며 원래 애셋의 치수 비교가 아니다.

`PREVIEW_ONLY_*` 그루터기, 흙, 조명은 Blender 조합 프리뷰용이다. FBX에는 새 뿌리만 들어간다. 원본 blend는 텍스처를 pack했다. 낙엽이나 꽃은 새 모델에 포함하지 않았다.

## 재현

프로젝트 루트에서 Blender 4.5 LTS로 실행:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/ExposedRoots/build_exposed_roots.py
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/ExposedRoots/verify_saved_blend.py
& Tools/ForestProps/ExposedRoots/run_unreal.ps1 -Mode References
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' -b --factory-startup --python-exit-code 1 --python Tools/ForestProps/ExposedRoots/inspect_references.py
```

검증된 원본을 먼저 커밋한 뒤:

```powershell
& Tools/ForestProps/ExposedRoots/run_unreal.ps1 -Mode Import
& Tools/ForestProps/ExposedRoots/run_unreal.ps1 -Mode Reload
& Tools/ForestProps/ExposedRoots/run_unreal.ps1 -Mode Preview
```

UE 5.7 임시 검사 프로젝트는 `TunaSweeper/Saved/ExposedRootsUE`에 생성한다. 현재 worktree에 게임 모듈 바이너리가 없으므로 엔진 기본 Python/EditorScriptingUtilities만 사용하는 프로젝트에서 동일 `/Game` 경로로 임포트한다. Import 성공 후 전용 새 uasset 폴더만 본 프로젝트에 복사한다. Reload/Preview는 본 프로젝트의 최종 uasset을 다시 검사 호스트에 복사하여 새로운 UE 프로세스로 연다. 작업별 로컬 DDC와 메모리 fallback, Zen 자동실행 비활성화로 AppData 캐시 의존을 피한다. Preview는 실제 UE 재질과 메시를 임시 월드에서 렌더하며 레벨을 저장하지 않는다. 게임 빌드/실제 데모 레벨 배치는 이 소품 제작 검증에 포함하지 않는다.

Blender 보고서는 퇴화 면, 비정상 노멀, nonmanifold edge, UV 삼각형 면적, FBX 재로드와 치수를 검증한다. UE 보고서는 엔진 버전, cm 치수/축, UV, 재질 연결/불투명/단면, 충돌 없음과 새 프로세스 재로드를 검증한다. 프리뷰는 Hero/Back/Top/Side/WithExistingStump/Repetition/UE를 제공한다.

최종 검증: Blender 생성/FBX·blend 재로드, UE Import/Reload는 통과 및 종료 코드 0. 실제 UE BaseColor/조명 캡처를 저장하고 직접 확인했다. UE 렌더 호스트는 이미지/보고서 저장과 shutdown 로그 이후 종료 코드 `-1073741819 (0xC0000005)`를 반환했다. 따라서 렌더 명령 전체의 정상 종료를 주장하지 않는다. 별도 캡처 호스트의 종료 문제이며 원인은 확정하지 않았다. Import/Reload는 이 오류 없이 완료됐다. UE 조명 프리뷰는 방향광의 강한 그림자를 포함하는 기술 확인용이며, 아트 조합/반복 배치는 Blender 프리뷰를 참고한다.
