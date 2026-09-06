param([switch]$VerifyOnly)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskOldVerify=$env:NOISE_VERIFY_ONLY
try {
    $env:NOISE_VERIFY_ONLY=if ($VerifyOnly) {'1'} else {'0'}
    & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$taskRoot/TunaSweeper/TunaSweeper.uproject" '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' '-run=pythonscript' "-script=$PSScriptRoot/import_unreal.py" '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' '-DDC-ForceMemoryCache' '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' "-abslog=$taskRoot/TunaSweeper/Saved/Logs/NoiseImport.log" *> "$taskRoot/TunaSweeper/Saved/noise_import.log"
    if ($LASTEXITCODE -ne 0) {throw "Noise import failed: $LASTEXITCODE"}
    Write-Output 'Noise import/validation completed.'
} finally {$env:NOISE_VERIFY_ONLY=$taskOldVerify}
