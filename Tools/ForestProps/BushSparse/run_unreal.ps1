param([switch]$VerifyOnly)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskAudit = Join-Path $taskRoot 'TunaSweeper/Saved/BushSparseAudit'
$taskProject = Join-Path $taskAudit 'BushSparseAudit.uproject'
$taskSourceContent = Join-Path $taskAudit 'Content/Nature/ForestProps/BushSparse'
$taskDestination = Join-Path $taskRoot 'TunaSweeper/Content/Nature/ForestProps/BushSparse'
New-Item -ItemType Directory -Force -Path $taskAudit, $taskSourceContent | Out-Null
'{"FileVersion":3,"EngineAssociation":"5.7","Plugins":[{"Name":"PythonScriptPlugin","Enabled":true},{"Name":"EditorScriptingUtilities","Enabled":true}]}' | Set-Content -LiteralPath $taskProject -Encoding UTF8
$taskNames = @('SM_BushSparse.uasset','M_BushSparse.uasset','T_BushSparse_Palette.uasset')
if ($VerifyOnly) {
    foreach ($taskName in $taskNames) { Copy-Item -LiteralPath (Join-Path $taskDestination $taskName) -Destination (Join-Path $taskSourceContent $taskName) -Force }
}
$taskMode = if ($VerifyOnly) { 'Reload' } else { 'Import' }
$taskPrevious = $env:BUSH_SPARSE_VERIFY_ONLY
$taskStarted = Get-Date
try {
    $env:BUSH_SPARSE_VERIFY_ONLY = if ($VerifyOnly) { '1' } else { '0' }
    & 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' $taskProject `
      '-run=pythonscript' "-script=$PSScriptRoot/import_unreal.py" '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
      '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
      "-abslog=$taskAudit/$taskMode.log" *> "$taskAudit/$taskMode.stdout.log"
    if ($LASTEXITCODE -ne 0) { throw "Unreal $taskMode failed: $LASTEXITCODE; see $taskAudit/$taskMode.stdout.log" }
    $taskReportName = if ($VerifyOnly) { 'unreal_reload_validation.json' } else { 'unreal_import_validation.json' }
    $taskReportPath = Join-Path $taskRoot "TunaSweeper/SourceArt/Environment/ForestProps/BushSparse/$taskReportName"
    if ((Get-Item -LiteralPath $taskReportPath).LastWriteTime -lt $taskStarted) { throw 'Stale validation report' }
    if (-not (Get-Content -Raw -LiteralPath $taskReportPath | ConvertFrom-Json).passed) { throw 'Validation failed' }
    if (-not $VerifyOnly) {
        New-Item -ItemType Directory -Force -Path $taskDestination | Out-Null
        foreach ($taskName in $taskNames) { Copy-Item -LiteralPath (Join-Path $taskSourceContent $taskName) -Destination (Join-Path $taskDestination $taskName) -Force }
    }
    $taskHashes = foreach ($taskName in $taskNames) {
        $taskHash = (Get-FileHash -LiteralPath (Join-Path $taskSourceContent $taskName)).Hash
        if ($taskHash -ne (Get-FileHash -LiteralPath (Join-Path $taskDestination $taskName)).Hash) { throw "Copy mismatch: $taskName" }
        @{name=$taskName;sha256=$taskHash}
    }
    $taskHashes | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskRoot 'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse/unreal_content_hashes.json') -Encoding UTF8
    Write-Output "BushSparse $taskMode passed; 3 destination assets hash matched."
} finally { $env:BUSH_SPARSE_VERIFY_ONLY = $taskPrevious }
