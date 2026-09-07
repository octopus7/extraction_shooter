param([ValidateSet('Audit','Import','Verify')][string]$Mode='Import', [switch]$LowPoly)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$taskEngine = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$taskScript = if ($Mode -eq 'Audit') { 'audit_existing.py' } else { 'import_unreal.py' }
$taskPreviousVerify = $env:MEMO_VERIFY_ONLY
$taskPreviousLow = $env:MEMO_LOW_POLY
$taskSource = "$taskRoot\TunaSweeper\SourceArt\Memo\StorageDevice"
$taskLogLabel = 'MemoDevice'
if ($LowPoly) {
    if ($Mode -eq 'Audit') { throw 'Audit checks existing memo defaults; omit LowPoly for Audit' }
    $taskSource += '\LowPoly'
    $taskLogLabel += '_LowPoly'
}
$taskPreviousCache = [Environment]::GetEnvironmentVariable('UE-LocalDataCachePath')
$taskStarted = Get-Date
try {
    $env:MEMO_VERIFY_ONLY = if ($Mode -eq 'Verify') { '1' } else { '0' }
    $env:MEMO_LOW_POLY = if ($LowPoly) { '1' } else { '0' }
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', "$taskRoot\TunaSweeper\DerivedDataCache")
    & $taskEngine "$taskRoot\TunaSweeper\TunaSweeper.uproject" `
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
        '-run=pythonscript' "-script=$PSScriptRoot\$taskScript" `
        '-ddc=InstalledNoZenLocalFallback' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
        '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' `
        *> "$taskRoot\TunaSweeper\Saved\${taskLogLabel}_$Mode.log"
    $taskExit = $LASTEXITCODE
    $taskLogLines = Get-Content -LiteralPath "$taskRoot\TunaSweeper\Saved\${taskLogLabel}_$Mode.log"
    $taskExecution = [ordered]@{
        mode = $Mode
        engine_exit_code = $taskExit
        python_completed = [bool]($taskLogLines | Select-String 'Python script executed successfully')
        startup_typed_element_ensure = [bool]($taskLogLines | Select-String 'Typed element was requested.*NE_PostProcess')
        python_errors = @($taskLogLines | Select-String 'LogPython: Error:' | ForEach-Object { $_.Line })
        log = "TunaSweeper/Saved/${taskLogLabel}_$Mode.log"
    }
    $taskExecution | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 `
        -LiteralPath "$taskSource\unreal_execution_$Mode.json"
    if ($taskExit -ne 0) { throw "UE $Mode exited with $taskExit; see Saved/${taskLogLabel}_$Mode.log" }
    $taskReportName = switch ($Mode) {
        'Audit' { 'existing_state_audit.json' }
        'Import' { 'unreal_import_validation.json' }
        'Verify' { 'unreal_reload_validation.json' }
    }
    $taskReport = Get-Item -LiteralPath "$taskSource\$taskReportName"
    if ($taskReport.LastWriteTime -lt $taskStarted) { throw 'Report is stale' }
    if ($Mode -ne 'Audit' -and -not (Get-Content $taskReport.FullName -Raw | ConvertFrom-Json).passed) {
        throw 'Asset verification did not pass'
    }
    Write-Output "Memo $Mode completed with exit 0: $($taskReport.FullName)"
} finally {
    $env:MEMO_VERIFY_ONLY = $taskPreviousVerify
    $env:MEMO_LOW_POLY = $taskPreviousLow
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $taskPreviousCache)
}
