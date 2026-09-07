param([ValidateSet('Inspect','Import','Reload','Preview')][string]$Mode='Import')
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskReview=Join-Path $taskRoot 'TunaSweeper/Saved/BushRoundReview'
New-Item -ItemType Directory -Force $taskReview | Out-Null
if (-not (Test-Path "$taskReview/Content")) {
    New-Item -ItemType Junction -Path "$taskReview/Content" -Target "$taskRoot/TunaSweeper/Content" | Out-Null
}
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content "$taskReview/BushRoundReview.uproject"
$taskPrevious=$env:BUSHROUND_VERIFY_ONLY
try {
    $taskStarted=Get-Date
    $env:BUSHROUND_VERIFY_ONLY=if ($Mode -eq 'Reload') {'1'} else {'0'}
    $taskScript=if ($Mode -eq 'Inspect') {'inspect_unreal.py'} elseif ($Mode -eq 'Preview') {'preview_unreal.py'} else {'import_unreal.py'}
    $taskExe='C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $taskArgs=@("$taskReview/BushRoundReview.uproject",'-unattended','-nosplash','-nosound','-stdout','-ddc=InstalledNoZenLocalFallback','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0','-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0')
    if ($Mode -eq 'Preview') {$taskArgs+=@("-ExecCmds=py $PSScriptRoot/$taskScript",'-RenderOffscreen','-NoLoadStartupPackages')} else {$taskArgs+=@('-run=pythonscript',"-script=$PSScriptRoot/$taskScript",'-nullrhi')}
    & $taskExe @taskArgs *> "$taskReview/$Mode.log"
    if ($LASTEXITCODE -ne 0) {throw "Unreal $Mode failed: $LASTEXITCODE; see $taskReview/$Mode.log"}
    $taskReportName=switch ($Mode) {'Inspect' {'existing_assets.json'} 'Import' {'unreal_import_validation.json'} 'Reload' {'unreal_reload_validation.json'} 'Preview' {'unreal_display_validation.json'}}
    $taskReportPath=Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ForestProps/BushRound/$taskReportName"
    if (-not (Test-Path -LiteralPath $taskReportPath)) {throw "Missing $taskReportPath"}
    if ((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted) {throw "Stale $taskReportPath"}
    if ($Mode -in @('Import','Reload')) {
        $taskReport=Get-Content -LiteralPath $taskReportPath -Raw | ConvertFrom-Json
        if (-not $taskReport.passed) {throw "Asset validation failed: $taskReportPath"}
    }
    Write-Output "Unreal $Mode exit 0; log: $taskReview/$Mode.log"
} finally {$env:BUSHROUND_VERIFY_ONLY=$taskPrevious}
