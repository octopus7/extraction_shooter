# 타이틀 스커트 리그

타이틀의 `Skirt`는 `BodyMesh`의 본 소켓이 아닌 컴포넌트에 상대 변환 Identity로 연결한다. 메시와 리그를 몸체 기준 좌표로 변환했으므로 pelvis 소켓 오프셋을 다시 적용하면 안 된다.

전용 에셋은 `/Game/Characters/Player/LunaMk2/Skirt/`의 `SKM_LunaMk2_TitleSkirt`, `SK_LunaMk2_TitleSkirt`, `PA_LunaMk2_TitleSkirt`, `ABP_LunaMk2_TitleSkirt`다. 원본 Luna 스커트, 몸체 스켈레톤과 Blender 원본은 수정하지 않는다.

- 몸체의 본 계층과 레퍼런스 포즈를 그대로 포함하고 기존 스커트 본 계층을 pelvis 아래에 보존한다.
- AnimBP는 `Copy Pose From Mesh`로 부모 BodyMesh를 읽은 뒤 기존 `RigidBody` 노드를 실행한다. 몸체를 먼저 갱신하도록 tick prerequisite을 둔다. 별도 스커트 본과 물리를 유지하기 위해 Leader Pose는 사용하지 않는다.
- 몸체 기준 Z 88–96 cm 구간에서 기존 스커트 웨이트를 몸체 표면의 보간 웨이트로 부드럽게 전환한다. 상단은 pelvis/spine 계열을 따르고 하단은 기존 스커트 물리 웨이트를 유지한다.
- 허리 표면 여유는 몸체의 torso 삼각형을 기준으로 반복 보정한다. 앞치마와 검은 치마를 같은 표면으로 밀면 두 겹이 겹치므로, 앞치마 상단에는 추가 1cm 간격을 주고 아래로 갈수록 줄인다. 이는 에셋의 기준 포즈 보정이며 모든 애니메이션에 대한 실시간 충돌 해결을 의미하지 않는다.
- 타이틀 Blueprint와 IntroMap의 저장된 인스턴스에도 새 에셋과 상대 변환을 적용한다.

앞치마 반원 엣지와 프릴은 84개 논리 정점을 공유한다. 서로 다른 정점 수에 맞춰 양쪽 엣지를 분할하므로 중간에서 연결이 끊기는 T자 접합을 두지 않는다. 좌우 프릴 중앙의 자유 단면 정점 3쌍도 용접한다. 접합부는 앞치마 엣지의 보간 웨이트를 공통으로 사용하고, 프릴 바깥으로 갈수록 기존 물리 웨이트를 섞는다. UV·법선 등의 코너 속성은 별도 vertex instance에 유지하므로 텍스처 경계 때문에 렌더 정점이 나뉘더라도 위치와 스킨 웨이트는 동일하다.

메시 소스의 `TitleSkirtPart` 삼각형 속성과 `TitleApronJoin` 정점 속성은 접합부 회귀 검사에 사용한다. 파트가 연결된 이후에는 연결 요소만으로 앞치마와 프릴을 구분할 수 없으므로 이 속성을 유지한다. `WeldedApronSeam` 검사는 공유 정점 수, 중앙 용접 및 미연결 접합 엣지를 검사한다. `title_seam_*.png`는 재생 중 접합부 확대 캡처다.

`TunaSweeper.Title.Skirt` 자동화 검사는 본 계층, 웨이트 정규화, 하단 물리 유지, 상단 기준 포즈 간격, 앞치마와 검은 치마의 삼각형 관통 및 타이틀 애니메이션의 몸체 포즈 추종을 검사한다. 렌더링 가능한 에디터 실행에서 `Saved/TitleSkirt/title_*.png`, 조명 없는 `title_basecolor_*.png`, 몸체를 숨긴 `title_garment_*.png`를 저장해 관통과 그림자를 구분한다.

에셋 생성 코드는 프로젝트 규칙에 따라 생성 에셋과 함께 커밋한 뒤 다음 커밋에서 제거한다. 다시 생성해야 할 때는 해당 이력의 생성기를 참고하며 시작 시 자동 재생성 경로는 두지 않는다.

## 타이틀 전용 탄성 복원

2026-09-21: `PA_LunaMk2_TitleSkirt`의 관절에 TwistAndSwing 위치·속도 구동을 켜고 기준 자세를 목표로 사용한다. 가속도 구동의 spring 800, damping 50, force limit 0(무제한)을 적용했다. 관절 swing은 양축 18도, twist는 8도로 제한하고 바디 linear/angular damping을 2/6으로 설정한다. 자유롭게 접힌 상태로 남던 치마를 기본 실루엣으로 복원하면서 작은 움직임을 유지하기 위한 타이틀 전용 설정이다.

`ABP_LunaMk2_TitleSkirt`의 RigidBody는 world inertia 비중 0.2, damping world 비중 0, 중력 override Z=-300cm/s²를 사용한다. CopyPose·본 계층·메시·충돌 형상은 유지하며 플레이어용 치마 에셋에는 적용하지 않는다.

`TunaSweeper.Title.Skirt.ElasticRecovery`는 C를 3초 재생한 후 A 첫 포즈로 고정하여 8초간 실제 본 물리를 평가한다. 유효한 변환, 움직임 유지, 최종 1초 잔떨림과 기준 로컬 회전으로의 복원을 검사한다. 변경 전 최대 잔류 회전은 49.294도로 실패했고, 적용 후 9.086도로 감소했다. 최종 프레임 간 이동은 약 0.000003cm이며 진입 중 움직임은 유지된다. 실제 타이틀 C/A/B 재생과 조명 없는 치마 실루엣 캡처로도 확인했다.
