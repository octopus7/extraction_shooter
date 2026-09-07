param([string]$Script='verify_unreal.py',[switch]$FullEditor)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64'
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',"-abslog=$taskRoot/TunaSweeper/Saved/Logs/ExtractionMarkers_$Script.log",'-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry')
if($FullEditor){
 $taskArgs+=@('-RenderOffscreen',"-ExecCmds=py $PSScriptRoot/$Script,t.MaxFPS 20")
 $taskQuoted=$taskArgs | ForEach-Object {'"'+$_+'"'}
 $taskProcess=Start-Process -FilePath (Join-Path $taskEngine 'UnrealEditor.exe') -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/$Script.log" -RedirectStandardError "$PSScriptRoot/$Script.stderr"
 Write-Output "UE process exit: $($taskProcess.ExitCode)"
 exit $taskProcess.ExitCode
}else{
 $taskArgs+=@('-nullrhi','-run=pythonscript',"-script=$PSScriptRoot/$Script")
 & (Join-Path $taskEngine 'UnrealEditor-Cmd.exe') @taskArgs *> "$PSScriptRoot/$Script.log"
 Write-Output "UE process exit: $LASTEXITCODE"
 exit $LASTEXITCODE
}
