# Shallow puddle footstep

- `SW_ShallowPuddle_Footstep.wav`: 새로 합성한 얕은 물 발걸음 one-shot. 젖은 발바닥 충격, 짧은 물살 잡음, 감쇠하는 작은 기포 성분을 혼합했다.
- 0.42초, mono, 48kHz, PCM 24-bit, 샘플 피크 약 -4dBFS, 비루프.
- 녹음 샘플이나 외부 음원을 사용하지 않은 원본 합성이다. 실제 물 발걸음 녹음이라고 주장하지 않는다.
- 게임 에셋: `/Game/Environment/ShallowPuddle/Audio/SW_ShallowPuddle_Footstep`.
- 연결: `BP_ShallowPuddle`의 `Puddle > Footstep > Water Footstep Sound`와 기본 C++ 액터. 검토 맵의 세 인스턴스도 같은 사운드를 사용한다.
- 반복감은 기존 플레이어 발소리의 피치 변동(0.92~1.08)으로 줄인다. 기존 볼륨 배율과 질주 볼륨 보정은 유지한다.
- `Tools/ShallowPuddle/render_footstep_audio.py`는 원본 WAV를 재현하는 오프라인 음원 도구다. Unreal 에셋을 생성하거나 임포트하지 않는다. `--verify`는 WAV를 다시 읽어 포맷·헤드룸·DC·끝부분의 무음을 검사한다.
- `audio_qa.json`은 파일/신호 검증 결과다. 자동 검사는 청취에 따른 음색 수용 판단을 대체하지 않는다.
- Niagara 물튀김/파문 제작 보류에는 변화가 없다.
