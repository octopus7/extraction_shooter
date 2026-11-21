$ErrorActionPreference = 'Stop'
$selector = Join-Path $PSScriptRoot 'StopTunaSweeperBuildProcesses.ps1'
if (-not (Test-Path -LiteralPath $selector -PathType Leaf)) {
    throw 'The scoped build and packaging process selector is missing.'
}
. $selector

function New-TestProcess($id, $parent, $name, $command) {
    [pscustomobject]@{
        ProcessId = $id; ParentProcessId = $parent; Name = $name; CommandLine = $command
        CreationDate = [datetime]'2026-11-21' + [timespan]::FromSeconds($id)
    }
}

$project = 'D:\Work\TunaSweeper'
$file = "$project\TunaSweeper.uproject"
$processes = @(
    (New-TestProcess 10 0 'cmd.exe' "cmd /c $project\BatchScripts\BuildAndRunTunaSweeper.bat")
    (New-TestProcess 11 10 'dotnet.exe' "dotnet UnrealBuildTool.dll -Project=$file")
    (New-TestProcess 20 0 'cmd.exe' "cmd /k $project\BatchScripts\PackageTunaSweeperSteamFullWin64.bat")
    (New-TestProcess 21 20 'dotnet.exe' "dotnet AutomationTool.dll BuildCookRun -project=$file")
    (New-TestProcess 22 21 'cmd.exe' 'cmd /c launch-compiler')
    (New-TestProcess 23 22 'dotnet.exe' "dotnet UnrealBuildTool.dll -Project=$file")
    (New-TestProcess 30 0 'dotnet.exe' "dotnet AutomationTool.dll BuildCookRun -project=$file")
    (New-TestProcess 31 30 'dotnet.exe' "dotnet UnrealBuildTool.dll -Project=$file")
    (New-TestProcess 40 0 'dotnet.exe' "dotnet UnrealBuildTool.dll -Project=$file")
    (New-TestProcess 50 0 'dotnet.exe' 'dotnet AutomationTool.dll BuildCookRun -project=D:\Other\TunaSweeper\TunaSweeper.uproject')
    (New-TestProcess 51 0 'dotnet.exe' "dotnet AutomationTool.dll Turnkey -project=$file")
    (New-TestProcess 60 0 'cmd.exe' "cmd /c $project\BatchScripts\KillTunaSweeperBuild.bat Packaging")
    (New-TestProcess 70 0 'cmd.exe' "cmd /k $project\BatchScripts\BuildAndRunTunaSweeper.bat")
    (New-TestProcess 71 70 'dotnet.exe' "dotnet UnrealBuildTool.dll -Project=$file")
    (New-TestProcess 72 70 'dotnet.exe' "dotnet AutomationTool.dll BuildCookRun -project=$file")
)

foreach ($case in @('Build:10,40,71', 'Packaging:20,30,72', 'All:10,20,30,40,70')) {
    $mode, $expected = $case.Split(':')
    $actual = (Get-TunaSweeperStopTargets -ProjectDirectory $project -Mode $mode -Processes $processes |
        ForEach-Object ProcessId | Sort-Object) -join ','
    if ($actual -ne $expected) {
        throw ("{0}: expected target PIDs {1}, got {2}" -f $mode, $expected, $actual)
    }
}

Write-Host 'Build and packaging process selection tests passed.'
