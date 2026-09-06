param([switch]$VerifyOnly)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$taskEngine = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$taskMode = if ($VerifyOnly) { 'Reload' } else { 'Import' }
$taskPreviousVerify = $env:GARAGE_VERIFY_ONLY
$taskStarted = Get-Date
try {
    $env:GARAGE_VERIFY_ONLY = if ($VerifyOnly) { '1' } else { '0' }
    & $taskEngine (Join-Path $taskRoot 'TunaSweeper\TunaSweeper.uproject') `
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
        '-run=pythonscript' "-script=$PSScriptRoot\import_unreal.py" `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
        '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' `
        "-abslog=$taskRoot\TunaSweeper\Saved\Logs\GarageDoorConcrete_$taskMode.log" `
        *> "$taskRoot\TunaSweeper\Saved\GarageDoorConcrete_$taskMode.stdout.log"
    $taskEngineExit = $LASTEXITCODE
    $taskReportName = if ($VerifyOnly) { 'unreal_reload_validation.json' } else { 'unreal_import_validation.json' }
    $taskReportPath = Join-Path $taskRoot "TunaSweeper\SourceArt\Environment\GarageDoorConcrete\$taskReportName"
    if (-not (Test-Path -LiteralPath $taskReportPath)) { throw "Garage $taskMode produced no report; engine exit $taskEngineExit" }
    if ((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted) { throw 'Garage report is stale' }
    $taskReport = Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
    if (-not $taskReport.passed) { throw 'Garage validation did not pass' }
    Write-Output "Garage asset validation passed: $taskReportPath (engine exit $taskEngineExit)"
    if ($taskEngineExit -ne 0) {
        throw "Asset validation passed, but Unreal exited with $taskEngineExit. Review startup/engine errors in GarageDoorConcrete_$taskMode.stdout.log."
    }
} finally {
    $env:GARAGE_VERIFY_ONLY = $taskPreviousVerify
}
