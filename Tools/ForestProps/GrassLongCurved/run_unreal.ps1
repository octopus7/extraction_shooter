param([switch]$VerifyOnly)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path "$PSScriptRoot/../../..").Path
$taskPrevious=$env:GRASS_LONG_VERIFY_ONLY
$taskMode=if($VerifyOnly){'Reload'}else{'Import'}
$taskStart=Get-Date
try {
    $env:GRASS_LONG_VERIFY_ONLY=if($VerifyOnly){'1'}else{'0'}
    & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$taskRoot/TunaSweeper/TunaSweeper.uproject" `
      '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-run=pythonscript' "-script=$PSScriptRoot/import_unreal.py" `
      '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-NoZenAutoLaunch' `
      '-DDC=InstalledNoZenLocalFallback' "-LocalDataCachePath=$taskRoot/TunaSweeper/DerivedDataCache/GrassLongCurved" `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
      "-abslog=$taskRoot/TunaSweeper/Saved/Logs/GrassLongCurved_$taskMode.log" `
      *> "$taskRoot/TunaSweeper/Saved/GrassLongCurved_$taskMode.stdout.log"
    $taskExit=$LASTEXITCODE
    $taskReportName=if($VerifyOnly){'unreal_reload_validation.json'}else{'unreal_import_validation.json'}
    $taskReport="$taskRoot/TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved/$taskReportName"
    if(!(Test-Path $taskReport)){throw "No validation report. UE exit $taskExit"}
    if((Get-Item $taskReport).LastWriteTime -lt $taskStart){throw 'Stale validation report'}
    if(!(Get-Content $taskReport -Raw | ConvertFrom-Json).passed){throw 'Validation failed'}
    if($taskExit -ne 0){throw "Validation passed but engine exit $taskExit; see log"}
    Write-Output "GrassLongCurved $taskMode passed; UE exit $taskExit"
} finally {$env:GRASS_LONG_VERIFY_ONLY=$taskPrevious}
