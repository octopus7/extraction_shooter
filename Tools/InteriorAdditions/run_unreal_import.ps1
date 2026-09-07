param([switch]$VerifyOnly, [switch]$SkipShowcase, [switch]$ReuseImported)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$taskEngine = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$taskMode = if ($VerifyOnly) { 'Reload' } else { 'Import' }
$taskScript = if ($VerifyOnly) { 'verify_unreal.py' } else { 'import_unreal.py' }
$taskPreviousShowcase = $env:INTERIOR_SKIP_SHOWCASE
$taskPreviousReuse = $env:INTERIOR_REUSE_IMPORTED
$taskStarted = Get-Date
try {
    $env:INTERIOR_SKIP_SHOWCASE = if ($SkipShowcase) { '1' } else { '0' }
    $env:INTERIOR_REUSE_IMPORTED = if ($ReuseImported) { '1' } else { '0' }
    & $taskEngine (Join-Path $taskRoot 'TunaSweeper\TunaSweeper.uproject') `
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
        '-run=pythonscript' "-script=$PSScriptRoot\$taskScript" `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
        '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' `
        "-abslog=$taskRoot\TunaSweeper\Saved\Logs\InteriorAdditions_$taskMode.log" `
        *> "$taskRoot\TunaSweeper\Saved\InteriorAdditions_$taskMode.stdout.log"
    $taskEngineExit = $LASTEXITCODE
    $taskReportName = if ($VerifyOnly) { 'unreal_reload_validation.json' } else { 'unreal_import_validation.json' }
    $taskReportPath = Join-Path $taskRoot "TunaSweeper\SourceArt\Environment\InteriorAdditions\$taskReportName"
    if (-not (Test-Path -LiteralPath $taskReportPath)) { throw "Interior $taskMode produced no report; engine exit $taskEngineExit" }
    if ((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted) { throw 'Interior report is stale' }
    $taskReport = Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
    if (-not $taskReport.passed) { throw 'Interior asset validation did not pass' }
    Write-Output "Interior asset validation passed: $taskReportPath (engine exit $taskEngineExit)"
    if ($taskEngineExit -ne 0) {
        throw "Assets passed validation, but Unreal exited with $taskEngineExit. Review InteriorAdditions_$taskMode.stdout.log for engine/startup errors."
    }
} finally {
    $env:INTERIOR_SKIP_SHOWCASE = $taskPreviousShowcase
    $env:INTERIOR_REUSE_IMPORTED = $taskPreviousReuse
}
