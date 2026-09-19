param([string]$Script = 'verify_unreal.py')
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskLog = Join-Path $taskRoot 'TunaSweeper/Saved/Logs/BarrierGate_Python.log'
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
    "$taskRoot/TunaSweeper/TunaSweeper.uproject" `
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-run=pythonscript' "-script=$PSScriptRoot/$Script" `
    '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
    '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
    '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' "-abslog=$taskLog" `
    *> "$taskRoot/TunaSweeper/Saved/BarrierGate_Python.stdout.log"
exit $LASTEXITCODE
