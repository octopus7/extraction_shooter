param()
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskOutput=Join-Path $taskRoot 'TunaSweeper/Saved/VillageLayoutPreview'
New-Item -ItemType Directory -Force $taskOutput | Out-Null
$taskStarted=Get-Date
$taskArgs=@(
    (Join-Path $taskRoot 'TunaSweeper/TunaSweeper.uproject'),
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-RenderOffscreen','-unattended','-nop4','-nosplash','-nosound','-NoWebBrowser',
    '-DDC-ForceMemoryCache','-stdout','-FullStdOutLogOutput',
    '-windowed','-ResX=1280','-ResY=1280',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    '-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0',
    "-ExecutePythonScript=$PSScriptRoot/verify_preview.py",
    "-abslog=$taskOutput/verify.log"
)
$taskQuoted=$taskArgs | ForEach-Object {'"'+$_+'"'}
$taskProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $taskQuoted -WindowStyle Hidden -Wait -PassThru
$taskReport=Join-Path $taskOutput 'verification.json'
if (!(Test-Path -LiteralPath $taskReport)) {throw 'Verification report missing'}
if ((Get-Item -LiteralPath $taskReport).LastWriteTime -lt $taskStarted) {throw 'Verification report stale'}
$taskResult=Get-Content -LiteralPath $taskReport -Raw | ConvertFrom-Json
if (!$taskResult.passed) {throw $taskResult.error}
if ($taskProcess.ExitCode -ne 0) {throw "Editor exit: $($taskProcess.ExitCode)"}
Write-Output "Saved village layout preview verification passed: $taskReport"
