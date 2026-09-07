param([switch]$VerifyOnly,[switch]$Render)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path "$PSScriptRoot/../../..").Path
$taskPrevious=$env:GRASS_LONG_VERIFY_ONLY
$taskPreviousCache=[Environment]::GetEnvironmentVariable('UE-LocalDataCachePath')
$taskMode=if($Render){'Render'}elseif($VerifyOnly){'Reload'}else{'Import'}
$taskScript=if($Render){"$PSScriptRoot/render_unreal.py"}else{"$PSScriptRoot/import_unreal.py"}
$taskRhi=if($Render){@('-RenderOffscreen','-d3d11','-sm5','-noraytracing',"-ExecutePythonScript=$taskScript")}else{@('-nullrhi','-run=pythonscript',"-script=$taskScript")}
$taskProject="$taskRoot/TunaSweeper/TunaSweeper.uproject"
if($Render){
    # Render byte-identical saved packages without unrelated game module startup.
    $taskPreview="$taskRoot/TunaSweeper/Saved/GrassLongCurvedPreview"
    $taskPreviewContent="$taskPreview/Content/Nature/ForestProps/GrassLongCurved"
    New-Item -ItemType Directory -Force $taskPreviewContent | Out-Null
    $taskFiles=Get-ChildItem "$taskRoot/TunaSweeper/Content/Nature/ForestProps/GrassLongCurved" -Filter '*.uasset' -File
    if($taskFiles.Count -ne 3){throw 'Expected exactly 3 grass packages'}
    $taskHashes=@{}
    foreach($taskFile in $taskFiles){
        Copy-Item -LiteralPath $taskFile.FullName -Destination $taskPreviewContent -Force
        $taskHash=(Get-FileHash -LiteralPath $taskFile.FullName -Algorithm SHA256).Hash
        if((Get-FileHash -LiteralPath "$taskPreviewContent/$($taskFile.Name)" -Algorithm SHA256).Hash -ne $taskHash){throw 'Preview copy mismatch'}
        $taskHashes[$taskFile.Name]=$taskHash
    }
    $taskHashes | ConvertTo-Json | Set-Content "$taskRoot/TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved/unreal_preview_package_hashes.json" -Encoding utf8
    $taskProject="$taskPreview/GrassLongCurvedPreview.uproject"
    '{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content $taskProject -Encoding utf8
}
$taskStart=Get-Date
try {
    $env:GRASS_LONG_VERIFY_ONLY=if($VerifyOnly){'1'}else{'0'}
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',"$taskRoot/TunaSweeper/DerivedDataCache/GrassLongCurved")
    & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' $taskProject `
      '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
      '-unattended' @taskRhi '-nosplash' '-nosound' '-stdout' '-NoZenAutoLaunch' `
      '-DDC=InstalledNoZenLocalFallback' "-LocalDataCachePath=$taskRoot/TunaSweeper/DerivedDataCache/GrassLongCurved" `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
      "-abslog=$taskRoot/TunaSweeper/Saved/Logs/GrassLongCurved_$taskMode.log" `
      *> "$taskRoot/TunaSweeper/Saved/GrassLongCurved_$taskMode.stdout.log"
    $taskExit=$LASTEXITCODE
    $taskReportName=if($Render){'unreal_render_validation.json'}elseif($VerifyOnly){'unreal_reload_validation.json'}else{'unreal_import_validation.json'}
    $taskReport="$taskRoot/TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved/$taskReportName"
    if(!(Test-Path $taskReport)){throw "No validation report. UE exit $taskExit"}
    if((Get-Item $taskReport).LastWriteTime -lt $taskStart){throw 'Stale validation report'}
    if(!(Get-Content $taskReport -Raw | ConvertFrom-Json).passed){throw 'Validation failed'}
    if($taskExit -ne 0){throw "Validation passed but engine exit $taskExit; see log"}
    Write-Output "GrassLongCurved $taskMode passed; UE exit $taskExit"
} finally {
    $env:GRASS_LONG_VERIFY_ONLY=$taskPrevious
    [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath',$taskPreviousCache)
}
