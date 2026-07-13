# MP4 Sound Wave Extractor

An Unreal Engine 5.7, Windows editor plugin that extracts a selected MP4 audio range into a `SoundWave` asset. It uses Windows Media Foundation directly and does not include or invoke FFmpeg.

## Features

- Opens from `TunaSweeper > Audio > MP4 Sound Wave Extractor`.
- Reads the selected MP4 duration and displays an audio waveform.
- Selects the export range by dragging A and B handles.
- Previews the selected range without creating a content asset.
- Extracts 16-bit mono or stereo PCM WAV and imports it as a `SoundWave` asset.

## Requirements

- Unreal Engine 5.7
- Windows 10 or later
- An MP4 whose audio stream can be decoded by Windows Media Foundation

## Installation

1. Copy the `MP4SoundWaveExtractor` folder into `<YourProject>/Plugins/`.
2. Regenerate project files if necessary, then build and open the editor.
3. Enable **MP4 Sound Wave Extractor** in the Plugins window if it is not already enabled.
4. Open **TunaSweeper > Audio > MP4 Sound Wave Extractor**.

The editor generates `Binaries/` and `Intermediate/` on the first build. They are intentionally excluded from source control and from the release ZIP.

## Publishing

The plugin source does not include a license. Choose and add an appropriate `LICENSE` file before publishing a public repository.
