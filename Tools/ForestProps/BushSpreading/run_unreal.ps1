param([switch]$VerifyOnly,[switch]$InspectReferences,[switch]$Render)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskScratch=Join-Path $taskRoot 'TunaSweeper/Saved/BushSpreadingValidation'
$taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
New-Item -ItemType Directory -Force "$taskScratch/Content/Nature" | Out-Null
$taskProject=Join-Path $taskScratch 'BushSpreadingValidation.uproject'
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content $taskProject
if ($InspectReferences) {
    foreach ($taskName in @('Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic')) {
        Copy-Item -LiteralPath "$taskRoot/TunaSweeper/Content/Nature/$taskName" -Destination "$taskScratch/Content/Nature" -Recurse -Force
    }
}
$taskMode=if($InspectReferences){'references'}elseif($Render){'render'}elseif($VerifyOnly){'reload'}else{'import'}
$taskScript=if($InspectReferences){'inspect_unreal.py'}elseif($Render){'render_unreal.py'}else{'import_unreal.py'}
$taskOldVerify=$env:BUSH_SPREADING_VERIFY_ONLY
$taskOldDdc=[Environment]::GetEnvironmentVariable('UE-LocalDataCachePath','Process')
$taskStarted=Get-Date
try {
    $env:BUSH_SPREADING_VERIFY_ONLY=if($VerifyOnly){'1'}else{'0'}
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',"$taskScratch/DDC",'Process')
    $taskArgs=@($taskProject,'-run=pythonscript',"-script=$PSScriptRoot/$taskScript",'-ddc=InstalledNoZenLocalFallback','-unattended','-nosplash','-nosound','-stdout','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0')
    if($Render){
        $taskEngine='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
        $taskArgs=$taskArgs | Where-Object { $_ -ne '-run=pythonscript' -and $_ -notlike '-script=*' }
        $taskArgs+=@("-ExecutePythonScript=$PSScriptRoot/$taskScript",'-RenderOffscreen','-d3d12')
    }else{$taskArgs+='-nullrhi'}
    if($Render){
        $taskQuotedArgs=$taskArgs | ForEach-Object { '"'+$_+'"' }
        $taskProcess=Start-Process -FilePath $taskEngine -ArgumentList $taskQuotedArgs -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput "$taskScratch/$taskMode.log" -RedirectStandardError "$taskScratch/$taskMode.stderr.log"
        $taskExit=$taskProcess.ExitCode
    }else{
        & $taskEngine @taskArgs *> "$taskScratch/$taskMode.log"
        $taskExit=$LASTEXITCODE
    }
    if($taskExit -ne 0){throw "UE $taskMode failed: exit $taskExit; inspect $taskScratch/$taskMode.log"}
    if($Render){
        $taskImage=Join-Path $taskRoot 'TunaSweeper/SourceArt/ForestProps/BushSpreading/Previews/unreal_hero.png'
        if(!(Test-Path -LiteralPath $taskImage) -or (Get-Item -LiteralPath $taskImage).LastWriteTime -lt $taskStarted){throw 'Renderer produced no fresh image'}
        @{passed=$true;exit_code=$taskExit;renderer='UE 5.7 full editor / DirectX 12 / transient world';preview='Previews/unreal_hero.png';bytes=(Get-Item -LiteralPath $taskImage).Length} | ConvertTo-Json | Set-Content (Join-Path $taskRoot 'TunaSweeper/SourceArt/ForestProps/BushSpreading/unreal_render_validation.json')
    }
    if(-not $InspectReferences -and -not $Render){
        $taskReport=Join-Path $taskRoot "TunaSweeper/SourceArt/ForestProps/BushSpreading/unreal_${taskMode}_validation.json"
        if(!(Test-Path $taskReport) -or (Get-Item $taskReport).LastWriteTime -lt $taskStarted){throw 'Missing or stale verification report'}
        if(!(Get-Content $taskReport -Raw | ConvertFrom-Json).passed){throw 'Verification failed'}
    }
    Write-Output "UE BushSpreading $taskMode passed."
} finally {
    $env:BUSH_SPREADING_VERIFY_ONLY=$taskOldVerify
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskOldDdc,'Process')
}
