param([string]$Script='verify_assets_driver.py',[switch]$Render=$true)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskArgs=@((Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,GeometryScripting','-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput',"-abslog=$taskRoot/TunaSweeper/Saved/Logs/Loot_$Script.log",'-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0',"-ExecCmds=py $PSScriptRoot/$Script,t.MaxFPS 20")
if($Render){$taskArgs+='-RenderOffscreen'}else{$taskArgs+='-nullrhi'}
$taskQuoted=$taskArgs | ForEach-Object {'"'+$_+'"'}
$taskProcess=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput "$PSScriptRoot/unreal_$Script.log" -RedirectStandardError "$PSScriptRoot/unreal_$Script.stderr.log"
Write-Output "Unreal process exit: $($taskProcess.ExitCode)"
exit $taskProcess.ExitCode
