param([switch]$AllowMissingAssets)

$ErrorActionPreference = 'Stop'
$sstoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$sstoProject = Join-Path $sstoRoot 'TunaSweeper/TunaSweeper.uproject'
$sstoEditor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$sstoSaved = Join-Path $sstoRoot 'TunaSweeper/Saved/SSTOLander_20261003'
$sstoReport = Join-Path $sstoSaved 'verification.json'
$sstoLog = Join-Path $sstoSaved 'verify_ssto.log'
$sstoRequired = @(
    'TunaSweeper/Content/MainRaid/RaidPlains.umap',
    'TunaSweeper/Content/MainRaid/SSTO/BP_SSTOLander.uasset',
    'TunaSweeper/Content/MainRaid/SSTO/BP_SSTOUpperShell.uasset',
    'TunaSweeper/Content/MainRaid/SSTO/Materials/M_SSTO_VerticalReveal.uasset',
    'TunaSweeper/SourceArt/Environment/SSTO_Lander/Manifests/SSTO_Lander.json',
    'TunaSweeper/SourceArt/Environment/SSTO_Lander/Manifests/SSTO_Placement.json'
)
$sstoMissing = @($sstoRequired | Where-Object { !(Test-Path -LiteralPath (Join-Path $sstoRoot $_)) })
if ($sstoMissing.Count -gt 0) {
    $sstoMessage = 'Missing SSTO inputs: ' + ($sstoMissing -join ', ')
    if ($AllowMissingAssets) {
        Write-Output ('Expected red baseline. ' + $sstoMessage)
        exit 3
    }
    throw $sstoMessage
}

New-Item -ItemType Directory -Path $sstoSaved -Force | Out-Null
$sstoStarted = Get-Date
$sstoArgs = @(
    $sstoProject,
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-RenderOffscreen', '-NullRHI', '-unattended', '-nop4', '-nosplash', '-nosound',
    '-NoWebBrowser', '-ddc=InstalledNoZenLocalFallback', '-stdout', '-FullStdOutLogOutput',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    "-ExecutePythonScript=$PSScriptRoot/verify_ssto.py",
    "-abslog=$sstoLog"
)
$sstoQuotedArgs = $sstoArgs | ForEach-Object { '"' + $_ + '"' }
$sstoProcess = Start-Process -FilePath $sstoEditor -ArgumentList $sstoQuotedArgs -WindowStyle Hidden -Wait -PassThru
Write-Output "Unreal SSTO verification exit: $($sstoProcess.ExitCode)"
if (!(Test-Path -LiteralPath $sstoReport)) { throw 'No SSTO verification report was produced' }
if ((Get-Item -LiteralPath $sstoReport).LastWriteTime -lt $sstoStarted) { throw 'SSTO verification report is stale' }
$sstoResult = Get-Content -LiteralPath $sstoReport -Raw | ConvertFrom-Json
if (!$sstoResult.passed) { throw "SSTO verification failed: $($sstoResult.error)" }
if ($sstoProcess.ExitCode -ne 0) { throw "Unreal exited with $($sstoProcess.ExitCode)" }
$sstoMapErrors = @(Select-String -LiteralPath $sstoLog -Pattern 'MapCheck: Error:|Map check complete: [1-9][0-9]* Error' -ErrorAction SilentlyContinue)
if ($sstoMapErrors.Count -gt 0) { throw "Map Check errors found in $sstoLog" }
Write-Output "SSTO saved-map structure and collision verification passed: $sstoReport"
