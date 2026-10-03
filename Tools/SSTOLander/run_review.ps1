param([ValidateSet('Runtime','Capture')][string]$Mode='Runtime')
$ErrorActionPreference='Stop'
$sstoRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$sstoSaved=Join-Path $sstoRoot 'TunaSweeper/Saved/SSTOLander_20261003'
New-Item -ItemType Directory -Path $sstoSaved -Force | Out-Null
$sstoScript=if($Mode -eq 'Runtime'){'verify_runtime.py'}else{'capture_ssto.py'}
$sstoReport=if($Mode -eq 'Runtime'){Join-Path $sstoSaved 'runtime.json'}else{Join-Path $sstoSaved 'Previews/capture.json'}
$sstoStarted=Get-Date
$sstoArgs=@((Join-Path $sstoRoot 'TunaSweeper/TunaSweeper.uproject'),'-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities','-RenderOffscreen','-windowed','-ResX=1600','-ResY=900','-unattended','-nop4','-nosplash','-nosound','-NoWebBrowser','-ddc=InstalledNoZenLocalFallback','-stdout','-FullStdOutLogOutput','-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0','-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',('-ExecutePythonScript='+$PSScriptRoot+'/'+$sstoScript),('-abslog='+$sstoSaved+'/'+$Mode+'.log'))
$sstoQuoted=$sstoArgs|ForEach-Object{'"'+$_+'"'}
$sstoProcess=Start-Process 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $sstoQuoted -WindowStyle Hidden -Wait -PassThru
if(!(Test-Path -LiteralPath $sstoReport)){throw 'Missing SSTO report'}
if((Get-Item -LiteralPath $sstoReport).LastWriteTime -lt $sstoStarted){throw 'Stale SSTO report'}
$sstoResult=Get-Content -LiteralPath $sstoReport -Raw|ConvertFrom-Json
if(!$sstoResult.passed -or $sstoProcess.ExitCode -ne 0){throw ('SSTO '+$Mode+' failed: '+$sstoResult.error+'; exit='+$sstoProcess.ExitCode)}
Write-Output ('SSTO '+$Mode+' passed: '+$sstoReport)
