param([ValidateSet('Import','Assets','CreateMap','Map')][string]$Mode='Assets')
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64'
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',"-abslog=$taskRoot/TunaSweeper/Saved/Logs/Expansion_$Mode.log")
$taskStart=Get-Date
if($Mode -in @('Import','CreateMap','Map')){
 $taskScript=if($Mode -eq 'Import'){'import_once_driver.py'}elseif($Mode -eq 'CreateMap'){'map_once_driver.py'}else{'verify_map_driver.py'}
 $taskArgs+='-RenderOffscreen'
 $taskArgs+='-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry'
 $taskArgs+="-ExecCmds=py $PSScriptRoot/$taskScript,t.MaxFPS 15"
 $taskQuoted=$taskArgs | ForEach-Object {'"'+$_+'"'}
 $taskProcess=Start-Process -FilePath (Join-Path $taskEngine 'UnrealEditor.exe') -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/unreal_$Mode.log" -RedirectStandardError "$PSScriptRoot/unreal_$Mode.stderr.log"
 $taskExit=$taskProcess.ExitCode
}else{
 $taskScript=if($Mode -eq 'Import'){'import_unreal_once.py'}else{'verify_unreal_assets.py'}
 $taskArgs+=@('-nullrhi','-run=pythonscript',"-script=$PSScriptRoot/$taskScript")
 & (Join-Path $taskEngine 'UnrealEditor-Cmd.exe') @taskArgs *> "$PSScriptRoot/unreal_$Mode.log"
 $taskExit=$LASTEXITCODE
}
Write-Output "Unreal $Mode process exit: $taskExit"
$taskReport=@{Import='unreal_import_validation.json';Assets='unreal_reload_validation.json';CreateMap='unreal_map_validation.json';Map='unreal_map_reload_validation.json'}[$Mode]
$taskPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/$taskReport"
if(!(Test-Path -LiteralPath $taskPath)){throw 'No report produced'}
if((Get-Item -LiteralPath $taskPath).LastWriteTime -lt $taskStart){throw 'Stale report'}
$taskResult=Get-Content -LiteralPath $taskPath -Raw | ConvertFrom-Json
if(!$taskResult.passed){throw 'Validation failed'}
Write-Output "Assertions passed: $taskReport (process exit $taskExit)"
exit $taskExit
