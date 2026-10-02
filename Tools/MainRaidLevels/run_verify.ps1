param([switch]$AllowMissingMaps)

$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskProject = Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'
$taskEditor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$taskReport = Join-Path $taskRoot 'TunaSweeper/Saved/MainRaidLevels/verification.json'
$taskLog = Join-Path $taskRoot 'TunaSweeper/Saved/Logs/MainRaidLevels_Verify.log'
$taskMaps = @('RaidForest', 'RaidVillage', 'RaidPlains') | ForEach-Object {
    Join-Path $taskRoot "TunaSweeper/Content/MainRaid/$_.umap"
}

$taskMissing = @($taskMaps | Where-Object { !(Test-Path -LiteralPath $_) })
if ($taskMissing.Count -gt 0) {
    if ($AllowMissingMaps) {
        Write-Error ("Expected red baseline; missing map packages: " + ($taskMissing -join ', '))
        exit 3
    }
    throw ("Missing map packages: " + ($taskMissing -join ', '))
}

$taskStarted = Get-Date
$taskArgs = @(
    $taskProject,
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-RenderOffscreen',
    '-NullRHI',
    '-unattended',
    '-nop4',
    '-nosplash',
    '-nosound',
    '-NoWebBrowser',
    '-DDC-ForceMemoryCache',
    '-stdout',
    '-FullStdOutLogOutput',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    "-ExecutePythonScript=$PSScriptRoot/verify_maps.py",
    "-abslog=$taskLog"
)
$taskQuotedArgs = $taskArgs | ForEach-Object { '"' + $_ + '"' }
$taskProcess = Start-Process -FilePath $taskEditor -ArgumentList $taskQuotedArgs -WindowStyle Hidden -Wait -PassThru
Write-Output "Unreal verification exit: $($taskProcess.ExitCode)"

if (!(Test-Path -LiteralPath $taskReport)) { throw 'No verification report was produced' }
if ((Get-Item -LiteralPath $taskReport).LastWriteTime -lt $taskStarted) { throw 'Verification report is stale' }
$taskResult = Get-Content -LiteralPath $taskReport -Raw | ConvertFrom-Json
if (!$taskResult.passed) { throw "Verification assertions failed: $($taskResult.error)" }
if ($taskProcess.ExitCode -ne 0) { throw "Unreal exited with $($taskProcess.ExitCode)" }

$taskMapCheckErrors = @(Select-String -LiteralPath $taskLog -Pattern 'MapCheck: Error:|Map check complete: [1-9][0-9]* Error' -ErrorAction SilentlyContinue)
if ($taskMapCheckErrors.Count -gt 0) { throw "Map Check errors found in $taskLog" }
Write-Output "Main raid map verification passed: $taskReport"
