param([ValidateSet('Import','MapBuild','Assets','Map','Capture')][string]$Mode='Assets')
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64'
$taskScript=@{Import='import_once_driver.py';MapBuild='build_map_once.py';Assets='verify_assets_driver.py';Map='verify_map_driver.py';Capture='capture_once.py'}[$Mode]
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',"-abslog=$taskRoot/TunaSweeper/Saved/Logs/LSP_$Mode.log")
$taskStart=Get-Date
 $taskArgs+=@('-RenderOffscreen','-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',"-ExecCmds=py $PSScriptRoot/$taskScript,t.MaxFPS 15")
 $taskQuoted=$taskArgs | ForEach-Object {'"'+$_+'"'}
 $taskProcess=Start-Process -FilePath (Join-Path $taskEngine 'UnrealEditor.exe') -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/unreal_$Mode.log" -RedirectStandardError "$PSScriptRoot/unreal_$Mode.stderr.log"
 $taskExit=$taskProcess.ExitCode
Write-Output "Unreal $Mode exit: $taskExit"
$taskReportName=@{Import='unreal_import_validation.json';MapBuild='unreal_map_creation.json';Assets='unreal_reload_validation.json';Map='unreal_map_reload_validation.json';Capture='unreal_capture_validation.json'}[$Mode]
$taskReportPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/LabSupplyProps/$taskReportName"
if(!(Test-Path -LiteralPath $taskReportPath)){throw 'Report missing'}
if((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStart){throw 'Report stale'}
$taskReport=Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
if(!$taskReport.passed){throw 'Report assertions failed'}
$taskReport | Add-Member -MemberType NoteProperty -Name process_exit -Value $taskExit -Force
$taskReport | ConvertTo-Json -Depth 60 | Set-Content -LiteralPath $taskReportPath -Encoding utf8
exit $taskExit
