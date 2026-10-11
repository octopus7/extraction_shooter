$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskProject = Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'
$taskEditor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
$taskReport = Join-Path $taskRoot 'TunaSweeper/Saved/LoopRailLayout/Previews/capture.json'
$taskLog = Join-Path $taskRoot 'TunaSweeper/Saved/Logs/LoopRailLayout_Capture.log'
$taskStarted = Get-Date
$taskArgs = @(
    $taskProject, '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-RenderOffscreen', '-unattended', '-nop4', '-nosplash', '-nosound',
    '-NoWebBrowser', '-DDC-ForceMemoryCache', '-stdout', '-FullStdOutLogOutput',
    '-windowed', '-ResX=1600', '-ResY=900',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    '-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0',
    "-ExecutePythonScript=$PSScriptRoot/capture_layout.py", "-abslog=$taskLog"
)
$taskQuotedArgs = $taskArgs | ForEach-Object { '"' + $_ + '"' }
$taskProcess = Start-Process -FilePath $taskEditor -ArgumentList $taskQuotedArgs -WindowStyle Hidden -Wait -PassThru
if (!(Test-Path -LiteralPath $taskReport)) { throw 'No railway capture report was produced' }
if ((Get-Item -LiteralPath $taskReport).LastWriteTime -lt $taskStarted) { throw 'Railway capture report is stale' }
$taskResult = Get-Content -LiteralPath $taskReport -Raw | ConvertFrom-Json
if (!$taskResult.passed) { throw "Railway capture failed: $($taskResult.error)" }
if ($taskProcess.ExitCode -ne 0) { throw "Unreal exited with $($taskProcess.ExitCode)" }
Write-Output "Railway captures passed: $taskReport"
