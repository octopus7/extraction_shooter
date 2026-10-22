param([string]$ReportName = 'PixelWorldBuilder')
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$project = Join-Path $repoRoot 'TunaSweeper/TunaSweeper.uproject'
$editor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$interactive = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
$matching = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe' OR Name = 'UnrealEditor-Cmd.exe'" | Where-Object {
    $_.ExecutablePath -and ($_.ExecutablePath.Replace('\','/') -ieq $interactive -or $_.ExecutablePath.Replace('\','/') -ieq $editor) -and
    $_.CommandLine -and $_.CommandLine.Replace('\','/').Contains($project.Replace('\','/'))
})
if ($matching.Count) { throw 'TunaSweeper editor is running. Follow Docs/unreal_editor_coordination.md before running map-changing tests.' }
$report = Join-Path $repoRoot ('TunaSweeper/Saved/Automation/' + $ReportName)
$log = Join-Path $repoRoot ('TunaSweeper/Saved/Logs/' + $ReportName + '.log')
& $editor $project /Engine/Maps/Entry -unattended -nop4 -nosplash -nosound -nullrhi -stdout -FullStdOutLogOutput '-ExecCmds=Automation RunTests PixelWorldBuilder' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report" "-abslog=$log"
if ($LASTEXITCODE -ne 0) { throw "UE tests failed with exit code $LASTEXITCODE; see $log" }
$results = Get-Content -LiteralPath (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
if ($results.failed -ne 0 -or $results.succeeded -lt 6) { throw 'PixelWorldBuilder automation report is incomplete or contains failures.' }
Write-Output "PixelWorldBuilder: $($results.succeeded) passed; $($results.failed) failed. $report"
