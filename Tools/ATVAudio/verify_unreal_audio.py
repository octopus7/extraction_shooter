"""Read-only fresh-process SoundWave verification; run in UE Python commandlet.

This script does not generate, modify, save or import any Unreal assets.
"""
from pathlib import Path
import json
import unreal

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Audio/ATV'
manifest = json.loads((OUT/'audio_manifest.json').read_text(encoding='utf-8'))
results = []
for source in manifest['assets']:
    name = Path(source['file']).stem
    path = '/Game/Audio/ATV/' + name
    sound = unreal.load_asset(path)
    assert isinstance(sound, unreal.SoundWave), f'Missing SoundWave: {path}'
    duration = float(sound.get_editor_property('duration'))
    channels = int(sound.get_editor_property('num_channels'))
    rate = int(sound.get_editor_property('imported_sample_rate'))
    looping = bool(sound.get_editor_property('looping'))
    assert abs(duration-source['duration_seconds']) < .001, (path, duration)
    assert channels == 1 and rate == 48000, (path, channels, rate)
    assert looping == source['loop'], (path, looping)
    results.append(dict(asset=path, duration_seconds=duration, num_channels=channels,
                        imported_sample_rate=rate, looping=looping))
report = dict(passed=True, engine_version=unreal.SystemLibrary.get_engine_version(),
              validation='Fresh-process load and stored SoundWave property verification; no audible playback test.',
              assets=results)
(OUT/'unreal_audio_validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
unreal.log('ATV_AUDIO_VALIDATION_PASS: all six SoundWaves reloaded with expected duration, mono/48kHz format and loop flags.')
