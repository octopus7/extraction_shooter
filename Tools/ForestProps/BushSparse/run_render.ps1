$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskAudit = Join-Path $taskRoot 'TunaSweeper/Saved/BushSparseAudit'
$taskArgs = @(
    '"' + "$taskAudit/BushSparseAudit.uproject" + '"',
    '-RenderOffscreen', '-unattended', '-nosplash', '-nosound', '-windowed', '-ResX=1400', '-ResY=1000',
    '-ExecCmds="py ' + "$PSScriptRoot/render_unreal.py" + '"',
    '-abslog="' + "$taskAudit/Render.log" + '"'
)
$taskProcess = Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $taskArgs -WindowStyle Hidden -PassThru
Write-Output "BushSparse offscreen review process: $($taskProcess.Id)"
