param([string]$Script='import_unreal.py',[switch]$VerifyOnly,[switch]$Render)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path "$PSScriptRoot/../../..").Path
$taskHost=Join-Path $taskRoot 'TunaSweeper/Saved/LeafLitterHost'
New-Item -ItemType Directory -Force $taskHost | Out-Null
# A content-only UE 5.7 host avoids rebuilding unrelated gameplay modules in a fresh worktree.
# Its /Game mount is the actual project's Content directory; scripts restrict writes to LeafLitter.
$taskProject=Join-Path $taskHost 'LeafLitterHost.uproject'
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content -LiteralPath $taskProject
if (-not (Test-Path "$taskHost/Content")) { New-Item -ItemType Junction -Path "$taskHost/Content" -Target "$taskRoot/TunaSweeper/Content" | Out-Null }
$taskPrevious=$env:LEAFLITTER_VERIFY_ONLY
$taskPreviousDDC=[Environment]::GetEnvironmentVariable('UE-LocalDataCachePath')
try {
 [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',"$taskHost/DerivedDataCache")
 $env:LEAFLITTER_VERIFY_ONLY=if($VerifyOnly){'1'}else{'0'}
 $taskArgs=@($taskProject,'-run=pythonscript',"-script=$PSScriptRoot/$Script",'-unattended','-nosplash','-nosound','-stdout','-FullStdOutLogOutput','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0')
 $taskArgs+='-ddc=InstalledNoZenLocalFallback'
 if(-not $Render){$taskArgs+='-nullrhi'}else{$taskArgs+='-AllowCommandletRendering'}
 $taskLog="$taskHost/$Script.$(Get-Date -Format 'HHmmss').log"
 & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' @taskArgs *> $taskLog
 if($LASTEXITCODE -ne 0){throw "UE exited $LASTEXITCODE; see $taskHost/$Script.log"}
} finally { $env:LEAFLITTER_VERIFY_ONLY=$taskPrevious; [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskPreviousDDC) }
