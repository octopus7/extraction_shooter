param([switch]$Map)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64'
$taskMode=if($Map){'MapReload'}else{'Reload'}
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),
 '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting',
 '-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',
 "-abslog=$taskRoot/TunaSweeper/Saved/Logs/ModularInterior_$taskMode.log")
$taskStarted=Get-Date
if($Map){
 $taskArgs+='-RenderOffscreen'
 $taskArgs+='-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry'
 $taskArgs+="-ExecCmds=py $PSScriptRoot/verify_map_driver.py,t.MaxFPS 15"
 $taskQuotedArgs=$taskArgs | ForEach-Object { '"'+$_+'"' }
 $taskProcess=Start-Process -FilePath (Join-Path $taskEngine 'UnrealEditor.exe') -ArgumentList $taskQuotedArgs -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/unreal_$taskMode.log" -RedirectStandardError "$PSScriptRoot/unreal_$taskMode.stderr.log"
 $taskExit=$taskProcess.ExitCode
}else{
 $taskArgs+='-nullrhi'
 $taskArgs+='-run=pythonscript'
 $taskArgs+="-script=$PSScriptRoot/verify_unreal_assets.py"
 & (Join-Path $taskEngine 'UnrealEditor-Cmd.exe') @taskArgs *> "$PSScriptRoot/unreal_$taskMode.log"
 $taskExit=$LASTEXITCODE
}
Write-Output "Unreal $taskMode exit: $taskExit"
$taskReport=if($Map){'unreal_map_reload_validation.json'}else{'unreal_reload_validation.json'}
$taskReportPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ModularInteriorPreview/$taskReport"
if(!(Test-Path -LiteralPath $taskReportPath)){throw 'No validation report was produced'}
if((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted){throw 'Validation report is stale'}
$taskResult=Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
if(!$taskResult.passed){throw 'Validation assertions did not pass'}
Write-Output "Asset assertions passed: $taskReportPath"
if($taskExit -ne 0){throw "Unreal exited with $taskExit; inspect log independently of validation reports."}
