param([ValidateSet('Import','Reload','References','Preview')][string]$Mode='Import')
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskHost=Join-Path $taskRoot 'TunaSweeper/Saved/ExposedRootsUE'
$taskOut=Join-Path $taskRoot 'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
New-Item -ItemType Directory -Force "$taskHost/Content/Nature" | Out-Null
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content "$taskHost/ExposedRootsUE.uproject"
if ($Mode -eq 'References') {
    foreach ($taskFolder in @('Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic')) { Copy-Item -LiteralPath "$taskRoot/TunaSweeper/Content/Nature/$taskFolder" -Destination "$taskHost/Content/Nature" -Recurse -Force }
}
if ($Mode -eq 'Reload' -or $Mode -eq 'Preview') {
    New-Item -ItemType Directory -Force "$taskHost/Content/Nature/ForestProps" | Out-Null
    Copy-Item -LiteralPath "$taskRoot/TunaSweeper/Content/Nature/ForestProps/ExposedRoots" -Destination "$taskHost/Content/Nature/ForestProps" -Recurse -Force
}
$taskScript=if ($Mode -eq 'References') {'export_ue_reference.py'} elseif ($Mode -eq 'Preview') {'preview_unreal.py'} else {'import_unreal.py'}
$taskPrevious=$env:EXPOSED_ROOTS_VERIFY
$taskCachePrevious=[Environment]::GetEnvironmentVariable('UE-LocalDataCachePath','Process')
try {
    $taskStarted=Get-Date
    New-Item -ItemType Directory -Force "$taskHost/DerivedDataCache" | Out-Null
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',"$taskHost/DerivedDataCache",'Process')
    $env:EXPOSED_ROOTS_VERIFY=if ($Mode -eq 'Reload') {'1'} else {'0'}
    $taskArgs=@("$taskHost/ExposedRootsUE.uproject",'-ddc=InstalledNoZenLocalFallback','-DDC-ForceMemoryCache','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput','-ini:Engine:[Zen]:AutoLaunch=false','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0,Interchange.FeatureFlags.Import.FBX=0')
    if ($Mode -eq 'Preview') {$taskArgs+=@('-RenderOffscreen',"-ExecutePythonScript=$PSScriptRoot/$taskScript")}
    else {$taskArgs+=@('-nullrhi','-run=pythonscript',"-script=$PSScriptRoot/$taskScript")}
    $taskLog=Join-Path $taskHost "$Mode.$(Get-Date -Format 'HHmmss').stdout.log"
    Write-Output "UE log: $taskLog"
    & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' @taskArgs *> $taskLog
    if ($LASTEXITCODE -ne 0) {throw "UE $Mode exited $LASTEXITCODE. See $taskLog"}
    $taskReportName=switch ($Mode) {'Import' {'unreal_import_validation.json'} 'Reload' {'unreal_reload_validation.json'} 'Preview' {'unreal_render_validation.json'} 'References' {'ue_reference_inventory.json'}}
    $taskReportFile=Join-Path $taskOut $taskReportName
    if (-not (Test-Path -LiteralPath $taskReportFile) -or (Get-Item -LiteralPath $taskReportFile).LastWriteTime -lt $taskStarted) {throw "Missing or stale report: $taskReportFile"}
    if ($Mode -eq 'Import') {
        $taskReport=Get-Content "$taskOut/unreal_import_validation.json" -Raw | ConvertFrom-Json
        if (-not $taskReport.passed) {throw 'Import validation failed'}
        New-Item -ItemType Directory -Force "$taskRoot/TunaSweeper/Content/Nature/ForestProps" | Out-Null
        Copy-Item -LiteralPath "$taskHost/Content/Nature/ForestProps/ExposedRoots" -Destination "$taskRoot/TunaSweeper/Content/Nature/ForestProps" -Recurse -Force
    }
    Write-Output "UE $Mode completed successfully."
} finally {$env:EXPOSED_ROOTS_VERIFY=$taskPrevious;[Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskCachePrevious,'Process')}
