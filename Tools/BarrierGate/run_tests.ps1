$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskStarted = Get-Date
& 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
    "$taskRoot/TunaSweeper/TunaSweeper.uproject" '-unattended' '-nullrhi' '-nosound' '-nosplash' `
    '-ExecCmds=Automation RunTests TunaSweeper.Interaction.BarrierGate' '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$taskRoot/TunaSweeper/Saved/Automation/BarrierGate/Runtime" `
    "-abslog=$taskRoot/TunaSweeper/Saved/Logs/BarrierGate_Runtime.log" '-stdout' '-FullStdOutLogOutput' `
    *> "$taskRoot/TunaSweeper/Saved/BarrierGate_Runtime.stdout.log"
$taskExit = $LASTEXITCODE
if ($taskExit -ne 0) { exit $taskExit }
$taskReportPath = "$taskRoot/TunaSweeper/Saved/Automation/BarrierGate/Runtime/index.json"
if (-not (Test-Path -LiteralPath $taskReportPath)) { throw 'No automation report was produced.' }
if ((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted) { throw 'Automation report is stale.' }
$taskReport = Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
if ($taskReport.failed -gt 0 -or $taskReport.succeeded -ne 1 -or $taskReport.notRun -gt 0) {
    $taskReport.tests.entries | Where-Object { $_.event.type -eq 'Error' } | ForEach-Object { Write-Output $_.event.message }
    throw 'Barrier gate automation failed. See the JSON report.'
}
Write-Output 'Barrier gate automation passed: 1 test, 0 failures.'
