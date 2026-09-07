param([ValidateSet('reference','import','reload','render')][string]$Mode='import')
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
# Content-only host avoids depending on unrelated game DLLs in a fresh worktree.
# Its Content junction targets this worktree's real TunaSweeper Content directory.
$taskHost=Join-Path $taskRoot 'TunaSweeper/Saved/GrassDenseShortHost'
New-Item -ItemType Directory -Force $taskHost | Out-Null
$taskProject=Join-Path $taskHost 'GrassDenseShortHost.uproject'
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content -LiteralPath $taskProject
$taskContent=Join-Path $taskHost 'Content'
if (!(Test-Path -LiteralPath $taskContent)) { New-Item -ItemType Junction -Path $taskContent -Target (Join-Path $taskRoot 'TunaSweeper/Content') | Out-Null }
$env:GRASS_DENSE_MODE=$Mode
${env:UE-LocalDataCachePath}=Join-Path $taskHost 'DerivedDataCache'
$taskScript=if ($Mode -eq 'reference') {'reference_unreal.py'} elseif ($Mode -eq 'render') {'render_unreal.py'} else {'import_unreal.py'}
$taskArgs=@($taskProject,'-run=pythonscript',"-script=$PSScriptRoot/$taskScript",'-ddc=InstalledNoZenLocalFallback','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0',"-abslog=$taskHost/$Mode.log")
$taskArgs+='-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0'
if ($Mode -eq 'render') {
    $taskArgs=@($taskArgs | Where-Object { $_ -ne '-run=pythonscript' -and $_ -notlike '-script=*' })
    $taskArgs+=@('-d3d12','-RenderOffscreen',"-ExecCmds=py $PSScriptRoot/$taskScript")
} else { $taskArgs+='-nullrhi' }
$taskStarted=Get-Date
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' @taskArgs *> (Join-Path $taskHost "$Mode.stdout.log")
if ($LASTEXITCODE -ne 0) { Get-Content (Join-Path $taskHost "$Mode.stdout.log") -Tail 45; throw "UE $Mode failed: $LASTEXITCODE" }
if ($Mode -ne 'reference') {
    $taskReport=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/GrassDenseShort/unreal_${Mode}_validation.json"
    if (!(Test-Path -LiteralPath $taskReport) -or (Get-Item -LiteralPath $taskReport).LastWriteTime -lt $taskStarted) { throw "Missing or stale $Mode validation report" }
    $taskResult=Get-Content -LiteralPath $taskReport -Raw | ConvertFrom-Json
    if ($Mode -eq 'render') { if (!$taskResult.nonblank_pixel_checks) { throw 'Render pixel checks failed' } }
    elseif (!$taskResult.passed) { throw "$Mode validation failed" }
}
Get-Content (Join-Path $taskHost "$Mode.stdout.log") -Tail 8
