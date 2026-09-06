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
try {
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',(Join-Path $taskAudit 'DerivedDataCache'),'Process')
    $env:GRASS_SPARSE_VERIFY_ONLY=if ($VerifyOnly) {'1'} else {'0'}
    $taskScript=if ($InspectReferences) {'inspect_unreal.py'} elseif ($Render) {'render_unreal.py'} else {'import_unreal.py'}
    $taskMode=if ($InspectReferences) {'Reference'} elseif ($Render) {'Render'} elseif ($VerifyOnly) {'Reload'} else {'Import'}
    $taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $taskArgs=@($taskProject,'-ddc=InstalledNoZenLocalFallback','-unattended','-nosplash','-nosound','-stdout','-NoZenAutoLaunch','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0')
    if ($Render) {
        $taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
        $taskArgs+=@("-ExecutePythonScript=$PSScriptRoot/$taskScript",'-windowed','-ResX=1200','-ResY=900')
    } else {$taskArgs+=@('-run=pythonscript',"-script=$PSScriptRoot/$taskScript",'-nullrhi')}
    & $taskEngine @taskArgs *> (Join-Path $taskAudit "$taskMode.log")
    if ($LASTEXITCODE -ne 0) {throw "UE $taskMode failed: exit $LASTEXITCODE; see $taskAudit/$taskMode.log"}
    Write-Output "UE $taskMode completed successfully"
} finally {
    $env:GRASS_SPARSE_VERIFY_ONLY=$taskOldVerify
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskOldDDC,'Process')
}
