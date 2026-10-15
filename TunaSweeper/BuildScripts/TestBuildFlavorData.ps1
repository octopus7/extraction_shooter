$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('build-flavor-data-' + [guid]::NewGuid())
$utf8 = [Text.UTF8Encoding]::new($false)
function Write-Fixture([string]$RelativePath, [string]$Content) {
    $path = Join-Path $fixture $RelativePath
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllText($path, $Content, $utf8)
}
try {
    Write-Fixture 'External/MainPayload/main-payload.json' '{"schema_version":1,"build_flavor":"main","initial_level":"/Game/Test","quest_definitions":"Data/QuestDefinitions.json","quest_text_strings":"Data/QuestTextStrings.csv","scenario_definitions":"Data/ScenarioDefinitions.json","scenario_text_strings":"Data/ScenarioTextStrings.csv"}'
    Write-Fixture 'External/MainPayload/Data/QuestDefinitions.json' '[]'
    Write-Fixture 'External/MainPayload/Data/QuestTextStrings.csv' "string_key,ko,en,ja`n"
    Write-Fixture 'External/MainPayload/Data/ScenarioDefinitions.json' '{"schema_version":1,"build_flavor":"main","scenarios":[{"id":"fixture"}]}'
    Write-Fixture 'External/MainPayload/Data/ScenarioTextStrings.csv' "string_key,ko,en,ja`n"
    Write-Fixture 'BuildScripts/BuildFlavorData.ps1' (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'BuildFlavorData.ps1') -Raw)
    $script = Join-Path $fixture 'BuildScripts/BuildFlavorData.ps1'
    $staged = Join-Path $fixture 'Content/Data/MainPayloadStaged/Data'
    & $script -Mode PrepareMain
    if (Test-Path -LiteralPath (Join-Path $staged 'ShopDefinitions.json')) { throw 'Absent optional shop catalog was synthesized.' }
    $optionalFiles = @('ShopDefinitions.json', 'EnemySpawns.json', 'LootContainerSpawns.json', 'MemoSpawns.json')
    foreach ($name in $optionalFiles) {
        Write-Fixture ('External/MainPayload/Data/' + $name) '[{"fixture_id":17}]'
    }
    Write-Fixture 'External/MainPayload/Data/ShopDefinitions.json' '   '
    $rejected = $false
    try { & $script -Mode PrepareMain } catch { $rejected = $true }
    if (-not $rejected) { throw 'Empty optional runtime JSON was accepted.' }
    Write-Fixture 'External/MainPayload/Data/ShopDefinitions.json' '[{"fixture_id":17}]'
    & $script -Mode PrepareMain
    foreach ($name in $optionalFiles) {
        $path = Join-Path $staged $name
        if (-not (Test-Path -LiteralPath $path)) { throw "Optional runtime JSON was not staged: $name" }
        if ((Get-Content -LiteralPath $path -Raw) -ne '[{"fixture_id":17}]') { throw "Staged JSON changed: $name" }
    }
    foreach ($name in $optionalFiles) {
        Write-Fixture ('External/MainPayload/Data/' + $name) '{invalid-json'
        $rejected = $false
        try { & $script -Mode PrepareMain } catch { $rejected = $true }
        if (-not $rejected) { throw "Malformed optional runtime JSON was accepted: $name" }
        Write-Fixture ('External/MainPayload/Data/' + $name) '[{"fixture_id":17}]'
    }
    & $script -Mode PrepareMain
    Write-Fixture 'Content/Data/ScenarioDefinitions.json' '{"schema_version":1,"build_flavor":"demo","scenarios":[{"id":"fixture"}]}'
    Write-Fixture 'Content/Data/ScenarioTextStrings.csv' "string_key,ko,en,ja`n"
    & $script -Mode PrepareDemo
    if (Test-Path -LiteralPath (Join-Path $fixture 'Content/Data/MainPayloadStaged')) { throw 'Demo retained Main runtime data.' }
    Write-Output 'PASS: optional runtime JSON staging, malformed JSON rejection, and Demo cleanup.'
} finally {
    $resolved = [IO.Path]::GetFullPath($fixture)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $resolved.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test cleanup path' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
