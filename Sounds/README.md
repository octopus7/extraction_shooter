# 총소리 합성 테스트

실제 총기 녹음이나 전용 AI 오디오 모델을 사용하지 않고 코드로 합성한 게임 효과음 시안입니다.
노이즈 파열음, 저음 공명, 기계음, 짧은 반사음을 조합했습니다. 총기별 실제 음향 재현을 보장하지 않습니다.

| 파일 | 내용 | 길이 |
| --- | --- | --- |
| Pistol.wav | 짧고 날카로운 권총 단발 | 1.10초 |
| Rifle.wav | 강한 파열음과 저음이 있는 소총 단발 | 1.35초 |
| SMG.wav | 가벼운 발사음 6발 연사, 800 RPM | 1.80초 |
| Shotgun.wav | 넓고 묵직한 샷건 단발과 후속 기계음 | 1.80초 |

모두 48 kHz / 24-bit PCM / 모노 WAV, 피크 약 -3 dBFS입니다.
UE 임포트나 프로젝트 애셋 설정은 하지 않았습니다.

재생성: 이 폴더의 `generate.ps1`을 PowerShell에서 실행합니다.

## 슬라이딩 자동문

- `SlidingDoor_Open.wav`: 0.6초, 부드러운 모터·레일 구동음.
- `SlidingDoor_Close.wav`: 0.75초, 조금 낮은 구동음과 작은 고무 패킹 접촉음.
- 48 kHz / 16-bit PCM / 모노. 큰 충격음 없이 낮은 원본 레벨로 합성.
- 재생성: `generate_sliding_door.ps1`. 원본은 이 폴더에 보관하며 UE SoundWave는 `/Game/Audio/Doors`에 임포트.
- `ATunaSweeperSlidingDoorActor`의 `DoorSoundVolume` 기본값은 `0.35f`. 실행 중 C++/Blueprint에서 `SetDoorSoundVolume(0.2f)`로 더 작게, `SetDoorSoundVolume(0.0f)`로 음소거 가능.
- 문마다 오디오 컴포넌트 하나를 사용하며 반복 요청은 재시작하지 않음. 방향 전환 시 사운드 교체, 이동 완료 시 페이드아웃, 즉시 배치·종료 시 정지. 기본 이동 시간에 맞춰 녹음했고 변경된 이동 시간에는 재생 속도를 맞춤.
