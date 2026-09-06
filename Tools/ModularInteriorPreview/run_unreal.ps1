param([switch]$VerifyOnly, [switch]$Map)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64'
$taskMode=if($VerifyOnly){'Reload'}else{'Import'}
if($Map){$taskMode='Map'+$taskMode}
$env:MI_VERIFY_ONLY=if($VerifyOnly){'1'}else{'0'}
$taskScript=if($Map){'build_preview_map.py'}else{'import_unreal.py'}
$taskExe=if($Map){'UnrealEditor.exe'}else{'UnrealEditor-Cmd.exe'}
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),
 '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting',
 '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0',
 '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0',
 '-unattended','-nullrhi','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',
 "-abslog=$taskRoot/TunaSweeper/Saved/Logs/ModularInterior_$taskMode.log")
if(!$Map){$taskArgs+='-run=pythonscript';$taskArgs+="-script=$PSScriptRoot/$taskScript"}
if($Map){
 $taskArgs=@($taskArgs | Where-Object { $_ -ne '-nullrhi' })
 $taskArgs+='-RenderOffscreen'
 $taskArgs+='-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry'
 $taskArgs+="-ExecCmds=py $PSScriptRoot/map_driver.py,t.MaxFPS 15"
}
$taskStarted=Get-Date
if($Map){
 $taskQuotedArgs=$taskArgs | ForEach-Object { '"'+$_+'"' }
 $taskProcess=Start-Process -FilePath (Join-Path $taskEngine $taskExe) -ArgumentList $taskQuotedArgs -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/unreal_$taskMode.log" -RedirectStandardError "$PSScriptRoot/unreal_$taskMode.stderr.log"
 $taskExit=$taskProcess.ExitCode
}else{
 & (Join-Path $taskEngine $taskExe) @taskArgs *> "$PSScriptRoot/unreal_$taskMode.log"
 $taskExit=$LASTEXITCODE
}
Write-Output "Unreal $taskMode exit: $taskExit"
$taskReport=if($Map){if($VerifyOnly){'unreal_map_reload_validation.json'}else{'unreal_map_validation.json'}}else{if($VerifyOnly){'unreal_reload_validation.json'}else{'unreal_import_validation.json'}}
$taskReportPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ModularInteriorPreview/$taskReport"
if(!(Test-Path -LiteralPath $taskReportPath)){throw 'No validation report was produced'}
if((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted){throw 'Validation report is stale'}
$taskResult=Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
if(!$taskResult.passed){throw 'Validation assertions did not pass'}
Write-Output "Asset assertions passed: $taskReportPath"
if($taskExit -ne 0){throw "Unreal exited with $taskExit; inspect log independently of validation reports."}
