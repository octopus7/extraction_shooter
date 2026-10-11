param([switch]$Runtime)
$ErrorActionPreference='Stop'
$railRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$railOutput=Join-Path $railRoot 'TunaSweeper/Saved/LoopRailLayout'
New-Item -ItemType Directory -Force $railOutput | Out-Null
$railMode=if($Runtime){'runtime'}else{'verification'}
$railReport=Join-Path $railOutput ($railMode+'.json')
$railLog=Join-Path $railOutput ($railMode+'.log')
$railStarted=Get-Date
$railArgs=@(
    (Join-Path $railRoot 'TunaSweeper/TunaSweeper.uproject'),
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities',
    '-RenderOffscreen','-NullRHI','-unattended','-nop4','-nosplash','-nosound','-NoWebBrowser',
    '-DDC-ForceMemoryCache','-stdout','-FullStdOutLogOutput',
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    '-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0',
    "-ExecutePythonScript=$PSScriptRoot/verify_village_rail.py",
    "-abslog=$railLog"
)
if($Runtime){$railArgs+='-VillageRailRuntime'}
$railQuoted=$railArgs|ForEach-Object{'"'+$_+'"'}
$railProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $railQuoted -WindowStyle Hidden -Wait -PassThru
if(!(Test-Path -LiteralPath $railReport)){throw 'Village rail report missing'}
if((Get-Item -LiteralPath $railReport).LastWriteTime -lt $railStarted){throw 'Village rail report stale'}
$railResult=Get-Content -LiteralPath $railReport -Raw|ConvertFrom-Json
if(!$railResult.passed){throw $railResult.error}
if($railProcess.ExitCode -ne 0){throw "Editor exit: $($railProcess.ExitCode)"}
$railErrors=@(Select-String -LiteralPath $railLog -Pattern 'MapCheck: Error:|Map check complete: [1-9][0-9]* Error|Assertion failed|Fatal error' -ErrorAction SilentlyContinue)
if($railErrors.Count -gt 0){throw "Editor or Map Check error in $railLog"}
Write-Output "Village rail $railMode passed: $railReport"
