param([switch]$VerifyOnly, [switch]$InspectReferences, [switch]$Render)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskAudit=Join-Path $taskRoot 'TunaSweeper/Saved/GrassSparseAudit'
New-Item -ItemType Directory -Force $taskAudit | Out-Null
$taskProject=Join-Path $taskAudit 'GrassSparseAudit.uproject'
if (-not (Test-Path $taskProject)) {
    '{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content $taskProject
}
if (-not (Test-Path (Join-Path $taskAudit 'Content'))) {
    New-Item -ItemType Junction -Path (Join-Path $taskAudit 'Content') -Target (Join-Path $taskRoot 'TunaSweeper/Content') | Out-Null
}
$taskOldVerify=$env:GRASS_SPARSE_VERIFY_ONLY
$taskOldDDC=[Environment]::GetEnvironmentVariable('UE-LocalDataCachePath','Process')
$taskStarted=Get-Date
try {
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',(Join-Path $taskAudit 'DerivedDataCache'),'Process')
    $env:GRASS_SPARSE_VERIFY_ONLY=if ($VerifyOnly) {'1'} else {'0'}
    $taskScript=if ($InspectReferences) {'inspect_unreal.py'} elseif ($Render) {'render_unreal.py'} else {'import_unreal.py'}
    $taskMode=if ($InspectReferences) {'Reference'} elseif ($Render) {'Render'} elseif ($VerifyOnly) {'Reload'} else {'Import'}
    $taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $taskArgs=@($taskProject,'-ddc=InstalledNoZenLocalFallback','-unattended','-nosplash','-nosound','-stdout','-NoZenAutoLaunch','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0')
    if ($Render) {
        $taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
        $taskArgs+=@("-ExecutePythonScript=$PSScriptRoot/$taskScript",'-windowed','-ResX=1200','-ResY=900')
    } else {$taskArgs+=@('-run=pythonscript',"-script=$PSScriptRoot/$taskScript",'-nullrhi')}
    if ($Render) {
        $taskProcess=Start-Process -FilePath $taskEngine -ArgumentList $taskArgs -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput (Join-Path $taskAudit "$taskMode.log") -RedirectStandardError (Join-Path $taskAudit "$taskMode.stderr.log")
        $taskExit=$taskProcess.ExitCode
    } else {
        & $taskEngine @taskArgs *> (Join-Path $taskAudit "$taskMode.log")
        $taskExit=$LASTEXITCODE
    }
    if ($taskExit -ne 0) {throw "UE $taskMode failed: exit $taskExit; see $taskAudit/$taskMode.log"}
    $taskReportName=if ($InspectReferences) {'Reference/ue_existing_assets.json'} elseif ($Render) {'unreal_visual_validation.json'} elseif ($VerifyOnly) {'unreal_reload_validation.json'} else {'unreal_import_validation.json'}
    $taskReportPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse/$taskReportName"
    if (-not (Test-Path $taskReportPath) -or (Get-Item $taskReportPath).LastWriteTime -lt $taskStarted) {throw "UE $taskMode produced no fresh validation report"}
    Write-Output "UE $taskMode completed successfully"
} finally {
    $env:GRASS_SPARSE_VERIFY_ONLY=$taskOldVerify
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskOldDDC,'Process')
}
