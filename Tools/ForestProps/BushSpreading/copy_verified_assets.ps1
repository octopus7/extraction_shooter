$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$taskSource=Join-Path $taskRoot 'TunaSweeper/Saved/BushSpreadingValidation/Content/Nature/ForestProps/BushSpreading'
$taskDest=Join-Path $taskRoot 'TunaSweeper/Content/Nature/ForestProps/BushSpreading'
$taskArt=Join-Path $taskRoot 'TunaSweeper/SourceArt/ForestProps/BushSpreading'
$taskReport=Get-Content "$taskArt/unreal_reload_validation.json" -Raw | ConvertFrom-Json
if(!$taskReport.passed){throw 'Fresh UE reload must pass before copying'}
foreach($taskProperty in $taskReport.source_sha256.PSObject.Properties){
    if((Get-FileHash (Join-Path $taskArt $taskProperty.Name) -Algorithm SHA256).Hash.ToLower() -ne $taskProperty.Value){throw 'Source changed after verification'}
}
$taskNames=@('SM_BushSpreading.uasset','M_BushSpreading.uasset','T_BushSpreading_Palette.uasset')
foreach($taskName in $taskNames){
    $taskAsset=Join-Path $taskSource $taskName
    if(!(Test-Path -LiteralPath $taskAsset)){throw "Missing asset $taskName"}
    if((Get-Item -LiteralPath $taskAsset).LastWriteTime -gt (Get-Item "$taskArt/unreal_reload_validation.json").LastWriteTime){throw 'Asset changed after reload verification'}
}
New-Item -ItemType Directory -Force $taskDest | Out-Null
$taskHashes=@{}
foreach($taskName in $taskNames){
    $taskFrom=Join-Path $taskSource $taskName
    $taskTo=Join-Path $taskDest $taskName
    Copy-Item -LiteralPath $taskFrom -Destination $taskTo -Force
    $taskHash=(Get-FileHash -LiteralPath $taskFrom -Algorithm SHA256).Hash
    if($taskHash -ne (Get-FileHash -LiteralPath $taskTo -Algorithm SHA256).Hash){throw 'Copied asset hash differs'}
    $taskHashes[$taskName]=$taskHash
}
@{passed=$true;destination='TunaSweeper/Content/Nature/ForestProps/BushSpreading';identical_to_freshly_reloaded_assets=$true;sha256=$taskHashes} | ConvertTo-Json -Depth 4 | Set-Content "$taskArt/project_copy_validation.json"
Write-Output "Verified three assets copied to $taskDest"
