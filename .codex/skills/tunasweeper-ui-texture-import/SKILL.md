---
name: tunasweeper-ui-texture-import
description: Import local image files into the TunaSweeper UE project as UI textures through the TunaSweeperEditor command-line import path. Use when Codex needs to put a processed PNG/JPG into /Game UI texture assets, including title backgrounds, logos, UI panels, or future reusable UI images.
---

# TunaSweeper UI Texture Import

Use this project-local skill when a local image should become a TunaSweeper UI texture asset.

## Rules

- Do image cropping, resizing, alpha cleanup, or other processing outside Unreal first, preferably with Python.
- Use the Unreal editor module only for importing the already-prepared local file into a `.uasset`.
- Reuse the same editor-module path with different arguments:
  - `-TunaSweeperImportUiTextureSource=<local image>`
  - `-TunaSweeperImportUiTextureDest=<long package path>`
  - `-TunaSweeperImportUiTextureName=<asset name>`
- Default destination is `/Game/UI/Title`.
- Default overwrite behavior is replace existing.

## Script

Run:

```powershell
powershell -ExecutionPolicy Bypass -File ".codex/skills/tunasweeper-ui-texture-import/scripts/import-ui-texture.ps1" `
  -SourceFile "D:\github\extraction_shooter\chatgpt\Title_C1.png" `
  -DestinationPath "/Game/UI/Title" `
  -AssetName "Title_C1"
```

Optional:

- `-NoReplace` to preserve an existing asset.
- `-UnrealEditorExe "C:\Path\To\UnrealEditor.exe"` if the editor executable is not on PATH or `UNREAL_EDITOR_EXE`.
- `-ProjectPath "D:\github\extraction_shooter\TunaSweeper\TunaSweeper.uproject"` to override the project path.

## Current Title Assets

- Background source: `chatgpt/Title_C1.png`
- Logo source: `Docs/Story/tuna_sweeper_logo_transparent.png`
- Expected assets:
  - `/Game/UI/Title/Title_C1`
  - `/Game/UI/Title/tuna_sweeper_logo_transparent`
