param(
    [Parameter(Mandatory)][string]$PackageRoot,
    [Parameter(Mandatory)][string]$UnrealPak
)
$ErrorActionPreference = 'Stop'
$content = (Resolve-Path -LiteralPath (Join-Path $PackageRoot 'TunaSweeper/Content')).Path
$placements = Join-Path $content 'Data/MainPayloadStaged/Data/LootContainerSpawns.json'
if (-not (Test-Path -LiteralPath $placements)) {
    $placements = Join-Path $content 'Data/MainRuntimeDefaults/LootContainerSpawns.json'
}
$rows = Get-Content -LiteralPath $placements -Raw -Encoding UTF8 | ConvertFrom-Json
$listing = [IO.Path]::GetTempFileName()
try {
    & $UnrealPak "-ListContainer=$(Join-Path $content 'Paks/*.utoc')" "-csv=$listing"
    if ($LASTEXITCODE -ne 0) { throw 'Could not list packaged loot classes.' }
    $entries = [IO.File]::ReadAllText($listing)
    foreach ($class in ($rows.loot_container_class | Sort-Object -Unique)) {
        if ($class -notlike '/Game/*') { continue }
        $package = $class.Split('.')[0].Substring('/Game/'.Length)
        if (-not $entries.Contains("/TunaSweeper/Content/$package.uasset,")) {
            throw "Missing packaged loot container class: $class"
        }
    }
} finally {
    Remove-Item -LiteralPath $listing
}
