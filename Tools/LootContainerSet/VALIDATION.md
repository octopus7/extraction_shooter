# LootContainerSet 검증 결과

원본 커밋: `464414c6`. Blender 4.5.12 LTS 및 UE 5.7.4로 검증했다. 기존 C++를 수정하지 않은 자산 작업이며, 로컬의 호환 에디터 바이너리를 사용했다.

- `source_validation.json`: 6개 .blend/FBX, 6,398개 검사 통과. 실제 632삼각형, 올바른 피벗과 m→cm 축 변환, 매니폴드 솔리드·면 방향·노멀·퇴화/고립 지오메트리, UV0/UV1·정점 색상 및 삼각형 단위 FBX 재로드 일치.
- `unreal_import_validation.json`, `unreal_reload_validation.json`: 임포트와 별도 프로세스 재로드 각각 20,456개 검사 통과. 실제 UE SourceModel 삼각형·분리 노멀·UV·RGBA, 충돌 경계, 1슬롯·Opaque·2개 텍스처 샘플, 2048²/1024² 원본 크기, CPD 및 감사 전후 패키지 해시를 확인.
- `unreal_review_validation.json`: 저장된 3개 검수 BP/맵 재로드, 네이티브 PIE Tick의 열림/닫힘 각 42개 샘플 프레임과 완료 상태 통과. 3종×106각도 충돌 SAT와 15개 실제 Raycast로 닫힌 뚜껑·바닥·전면 벽의 차단 및 열린 입구·빈 내부의 통과를 확인. 프레임 수는 성능/FPS 측정값이 아니다.
- `assembly_validation.json`: 측정된 메시 경계로 닫힌 전체 치수 및 전체 0~105° 구간의 연속적인 몸통/뚜껑 비관통 조건을 증명. 닫힘 간격은 목재/금속 약 2mm, 보급 상자 보호 모서리 약 1mm다.
- `protected_assets_validation.json`: 작업 시작 커밋 `c1b408f8` 대비 기존 게임 콘텐츠·BP·데이터·C++·설정에 수정/삭제가 없음을 확인.

UE 검수 캡처는 2048² 아틀라스와 1024² 마스크의 실제 로드를 확인한 TextureQuality=3 상태에서 촬영했다. PIE가 적용하는 프로젝트 기본 TextureQuality=0은 설정 파일에서 유지된다. 낮 환경광은 전용 검수 맵에만 추가했다. Blender 실제 프리뷰와 UE 실제 프리뷰를 직접 시각 검수했으며 컨셉 ImageGen 이미지와 별도 보관했다.

비용: Wood 156+36, Metal 160+52, Supply 168+60 삼각형. 6개 핵심 메시 모두 1슬롯이며 동일 공유 재질, 추가 오염도 동일 마스크를 사용한다. 단순 충돌 총 18박스. Nanite·투명 블렌드·넓은 오염 데칼을 사용하지 않는다. GPU 시간이나 FPS는 측정하지 않았다.

기존 7001~7003 행과 BP는 연결하지 않았다. 실제 연결에는 README의 정확한 Transform과 데이터의 스케일/재질 적용 조건이 필요하다. 검수 BP의 DefinitionId=0은 기구 검수용이며 실제 게임 데이터로 복사하지 않는다.
