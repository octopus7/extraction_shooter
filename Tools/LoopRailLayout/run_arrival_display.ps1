$ErrorActionPreference = 'Stop'
$railRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$railReport = Join-Path $railRoot 'TunaSweeper/Saved/LoopRailLayout/ArrivalDisplay/verification.json'
$railLog = Join-Path $railRoot 'TunaSweeper/Saved/Logs/LoopRailArrivalDisplay.log'
$railStarted = Get-Date
$railArgs = @(
    (Join-Path $railRoot 'TunaSweeper/TunaSweeper.uproject'),
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities', '-RenderOffscreen',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoWebBrowser', '-DDC-ForceMemoryCache',
    '-stdout', '-FullStdOutLogOutput', '-windowed', '-ResX=1600', '-ResY=900',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    '-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0',
    "-ExecutePythonScript=$PSScriptRoot/verify_arrival_display.py", "-abslog=$railLog"
)
$railQuoted = $railArgs | ForEach-Object { '"' + $_ + '"' }
$railProcess = Start-Process 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $railQuoted -WindowStyle Hidden -Wait -PassThru
if (!(Test-Path -LiteralPath $railReport)) { throw 'Arrival display report missing' }
if ((Get-Item -LiteralPath $railReport).LastWriteTime -lt $railStarted) { throw 'Arrival display report stale' }
$railResult = Get-Content -LiteralPath $railReport -Raw | ConvertFrom-Json
if (!$railResult.passed) { throw $railResult.error }
if ($railProcess.ExitCode -ne 0) { throw "Editor exited with $($railProcess.ExitCode)" }
Write-Output "Arrival display review passed: $railReport"
