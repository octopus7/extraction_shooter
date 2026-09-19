# Gasoline ATV audio source

Original procedural sound design for a single-cylinder four-stroke gasoline ATV.
These are synthesized assets, not vehicle field recordings. No external samples,
speech, music, or third-party media were used. All WAV files are lossless 48 kHz,
24-bit PCM, mono for positional vehicle playback.

| Source | Duration | Playback |
| --- | ---: | --- |
| `SW_ATV_Mount_Start.wav` | 3.80 s | Mounting = ignition: starter crank, catch, brief rev, idle settle. Play once. |
| `SW_ATV_Idle_Loop.wav` | 8.00 s | Continuous idle, nominal 1,560 RPM. |
| `SW_ATV_Drive_Loop.wav` | 8.00 s | Continuous driving, nominal 4,200 RPM. |
| `SW_ATV_Boost_Loop.wav` | 8.00 s | Continuous sprint/high throttle, nominal 6,840 RPM. |
| `SW_ATV_Dismount_Stop.wav` | 2.40 s | Dismount when engine shuts down: idle, cutoff, decaying rotation. Play once. |
| `SW_ATV_Dismount_Mechanical.wav` | 0.95 s | Optional engine-on exit: footrest/seat/suspension cue without ignition/shutdown. |
| `ATV_Audition_Sequence.wav` | 22.12 s | Preview: start, idle, driving, boost, driving, idle, stop. Do not import as gameplay SFX. |

## Playback

- Mounting sound is the engine start, not a seat/footstep cue. Begin Idle at
  3.50 seconds into Mount_Start and crossfade over 0.30 seconds.
- Fade between running states over 0.25–0.40 seconds. Their levels intentionally
  rise with load: idle -21, drive -19, boost -17 dBFS RMS. Keep a shared gain and
  reserve at least 3 dB of mixer headroom for overlaps. Avoid stacking all three
  loops at full volume. Modest pitch variation can fill RPM gaps; these are three
  anchor recordings rather than a complete continuous RPM system.
- Dismount_Stop is an engine-off transition. Fade the current loop out over
  0.18 seconds while starting it. If dismounting leaves the engine on, retain
  Idle and optionally play Dismount_Mechanical instead. Never layer both exits.
- Engine loops have full-file forward `smpl` loop metadata, inclusive end sample
  383,999. Their underlying signal and noise are periodic; do not add fades at
  the loop boundary. Explicitly enable `bLooping` in Unreal; WAV metadata alone
  should not be treated as an import-setting guarantee.
- Import production cues to `/Game/Audio/ATV` with the WAV basenames. The existing
  native importer supports `-TunaSweeperImportAudioSource=...`,
  `-TunaSweeperImportAudioDest=/Game/Audio/ATV`,
  `-TunaSweeperImportAudioName=...`, and `-TunaSweeperImportAudioQuit`.
  Add `-TunaSweeperImportAudioNoLoop` for Start, Stop, and Mechanical.
  Use `-unattended -nullrhi -nosplash`; omit `-nosound` during import, because
  the startup importer needs the BINKA decoder registered by audio initialization.
  Game-state routing and attenuation are separate integration work.

## Reproduction and validation

Run from the repository with Python 3.10+ and NumPy:

```text
python Tools/ATVAudio/render_atv_audio.py
python Tools/ATVAudio/validate_atv_audio.py
```

The renderer uses deterministic seeds, combustion harmonics, exhaust resonances,
load-dependent intake noise, valve ticks, and crank/starter mechanics. It creates
source WAVs and the machine-readable `audio_manifest.json`; it does not launch
Unreal or modify project code. `audio_qa.json` records file hashes, format,
duration, peak, RMS, DC, clipping counts, and loop-boundary checks. RMS figures
are signal measurements, not LUFS loudness certification.

Audible QA remains unverified: no audio listening/understanding tool was
available to the authoring agent. The preview is provided for human audition.
Objective QA verifies technical usability, not recorded-engine realism.

The six SoundWaves are imported under `TunaSweeper/Content/Audio/ATV` using the
existing native importer, with no new editor generator or startup code.
`unreal_audio_validation.json` records a fresh UE 5.7 process reloading and
checking their duration, channel count, sample rate, and saved looping flag.
`Tools/ATVAudio/verify_unreal_audio.py` is a read-only verification script.
