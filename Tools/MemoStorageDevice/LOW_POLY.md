# 메모 저장장치 저폴리 추가 버전

`SM_MemoStorageDevice_LowPoly`는 원본의 실루엣·색·크기·피벗을 유지하는 별도 메시다.
**936삼각형**, 528정점. UCX 충돌 상자 12삼각형까지 포함해 **948삼각형**이다.
기존 6,012삼각형 대비 약 **84.4% 감소**했다. 모든 렌더 면을 삼각형으로 고정했으므로
쿼드 기준이 아니라 FBX와 UE에서 실제 사용하는 삼각형 기준으로 1,000 미만이다.

본체와 뚜껑을 합치고, 측면 그립과 이중 패널 테두리를 제거했다. 커넥터 접점은
3개로 줄이고 실루엣에 영향을 주는 본체·보호 모서리에만 1단 베벨을 남겼다.
단순 면은 평면 노멀로 처리했다. 기존 재질 4개를 공유하며 텍스처는 추가하지 않는다.

- UE: `/Game/Meshes/Props/MemoStorageDevice/SM_MemoStorageDevice_LowPoly`
- 원본 폴더: `TunaSweeper/SourceArt/Memo/StorageDevice/LowPoly/`
- Blender: `SM_MemoStorageDevice_LowPoly.blend`
- FBX: `Models/SM_MemoStorageDevice_LowPoly.fbx`
- 프리뷰: `Previews/`의 5방향 PNG.
- 검증: `model_manifest.json`, `fbx_validation.json`, UE 임포트·재로드 JSON.

크기는 12×7×2.0705cm이며 본체 바닥 중앙 피벗, +X 커넥터 방향이다.
기존 모델/재질/BP/맵은 유지한다. 연결은 기존 안내와 동일하게 새 메시만 선택하고,
`Visual Scale=(1,1,1)` 및 슬롯 0의 `M_MemoDevice_Shell`을 명시한다.

저장소 루트에서:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/MemoStorageDevice/build_memo.py -- --low-poly
& 'C:/Program Files/Blender Foundation/Blender 4.5/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/MemoStorageDevice/verify_fbx.py -- --low-poly
& Tools/MemoStorageDevice/run_unreal.ps1 -Mode Import -LowPoly
& Tools/MemoStorageDevice/run_unreal.ps1 -Mode Verify -LowPoly
```

저폴리 모드 임포터는 기존 재질을 읽기만 하며 저폴리 메시 이외에는 저장하지 않는다.
기존 버전의 생성 명령은 그대로 유지되며 `--low-poly`가 없으면 기존 버전이 생성된다.
UE 시작 시 기존 Niagara ensure가 있으면 Python 검증 성공과 별개로 종료 코드 1이
발생할 수 있다. 실행 결과와 애셋 검증 결과를 각각의 JSON에서 확인한다.

최종 검증: Blender/FBX 및 UE 5.7.4 별도 프로세스 재로드에서 936삼각형을 확인했다.
UV·바닥 피벗·치수·재질 슬롯·충돌 검사를 통과했으며, 기존 애셋 77개 해시가 동일했다.
UE 정점 수는 노멀/UV 경계 분리 후 1,296개다. 다방향 최종 렌더를 시각 확인했다.
임포트와 재로드 Python은 오류 없이 완료했지만 기존 Niagara 시작 ensure 때문에
두 commandlet 모두 종료 코드는 1이었다. 이번 변경에는 C++ 재빌드가 필요하지 않다.
