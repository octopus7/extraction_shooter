[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('PrepareMain', 'PrepareDemo', 'Clean', 'VerifyDemo')]
    [string]$Mode,

    [string]$ArchiveDirectory = ''
)

$ErrorActionPreference = 'Stop'
$projectDirectory = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$mainSourceDirectory = [System.IO.Path]::GetFullPath((Join-Path $projectDirectory 'External\MainPayload'))
$stagedDirectory = [System.IO.Path]::GetFullPath((Join-Path $projectDirectory 'Content\Data\MainPayloadStaged'))
$contentDirectory = [System.IO.Path]::GetFullPath((Join-Path $projectDirectory 'Content'))
$optionalRuntimeDataFiles = @(
    'BunkerCharacterSpawns.json',
    'EnemySpawns.json',
    'GameplayInteractionSpawns.json',
    'LootContainerSpawns.json',
    'MemoSpawns.json',
    'TransparentObstacleSpawns.json',
    'WarpPointSpawns.json',
    'WorldProgressObjectSpawns.json'
)

if (-not $stagedDirectory.StartsWith($contentDirectory, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Unsafe staged Main payload path: $stagedDirectory"
}

function Remove-StagedMainPayload {
    if (Test-Path -LiteralPath $stagedDirectory) {
        Remove-Item -LiteralPath $stagedDirectory -Recurse -Force
    }
}

function Assert-MainPayloadValid {
    $manifestPath = Join-Path $mainSourceDirectory 'main-payload.json'
    $definitionsPath = Join-Path $mainSourceDirectory 'Data\QuestDefinitions.json'
    $stringsPath = Join-Path $mainSourceDirectory 'Data\QuestTextStrings.csv'
    $scenarioDefinitionsPath = Join-Path $mainSourceDirectory 'Data\ScenarioDefinitions.json'
    $scenarioStringsPath = Join-Path $mainSourceDirectory 'Data\ScenarioTextStrings.csv'
    foreach ($requiredPath in @($manifestPath, $definitionsPath, $stringsPath, $scenarioDefinitionsPath, $scenarioStringsPath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "Required Main payload file is missing: $requiredPath"
        }
    }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.schema_version -ne 1 -or $manifest.build_flavor -ne 'main') {
        throw 'Main payload manifest must declare schema_version 1 and build_flavor main.'
    }
    if ([string]::IsNullOrWhiteSpace([string]$manifest.initial_level)) {
        throw 'Main payload manifest initial_level must not be empty.'
    }
    if ($manifest.quest_definitions -ne 'Data/QuestDefinitions.json' -or
        $manifest.quest_text_strings -ne 'Data/QuestTextStrings.csv' -or
        $manifest.scenario_definitions -ne 'Data/ScenarioDefinitions.json' -or
        $manifest.scenario_text_strings -ne 'Data/ScenarioTextStrings.csv') {
        throw 'Main payload manifest data paths do not match the supported layout.'
    }

	$definitionsText = (Get-Content -LiteralPath $definitionsPath -Raw).Trim()
	$null = $definitionsText | ConvertFrom-Json
	if (-not $definitionsText.StartsWith('[') -or -not $definitionsText.EndsWith(']')) {
        throw 'Main QuestDefinitions.json must contain a JSON array.'
    }

    $header = Get-Content -LiteralPath $stringsPath -TotalCount 1
    if ($header.Trim() -ne 'string_key,ko,en,ja') {
        throw 'Main QuestTextStrings.csv header must be string_key,ko,en,ja.'
    }

    $scenarioPack = Get-Content -LiteralPath $scenarioDefinitionsPath -Raw | ConvertFrom-Json
    if ($scenarioPack.schema_version -ne 1 -or $scenarioPack.build_flavor -ne 'main' -or -not $scenarioPack.scenarios) {
        throw 'Main ScenarioDefinitions.json must declare schema_version 1, build_flavor main, and at least one scenario.'
    }
    $scenarioHeader = Get-Content -LiteralPath $scenarioStringsPath -TotalCount 1
    if ($scenarioHeader.Trim() -ne 'string_key,ko,en,ja') {
        throw 'Main ScenarioTextStrings.csv header must be string_key,ko,en,ja.'
    }
}

function Assert-DemoScenarioPackValid {
    $scenarioDefinitionsPath = Join-Path $projectDirectory 'Content\Data\ScenarioDefinitions.json'
    $scenarioStringsPath = Join-Path $projectDirectory 'Content\Data\ScenarioTextStrings.csv'
    foreach ($requiredPath in @($scenarioDefinitionsPath, $scenarioStringsPath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "Required Demo scenario file is missing: $requiredPath"
        }
    }
    $scenarioPack = Get-Content -LiteralPath $scenarioDefinitionsPath -Raw | ConvertFrom-Json
    if ($scenarioPack.schema_version -ne 1 -or $scenarioPack.build_flavor -ne 'demo' -or -not $scenarioPack.scenarios) {
        throw 'Demo ScenarioDefinitions.json must declare schema_version 1, build_flavor demo, and at least one scenario.'
    }
    $scenarioHeader = Get-Content -LiteralPath $scenarioStringsPath -TotalCount 1
    if ($scenarioHeader.Trim() -ne 'string_key,ko,en,ja') {
        throw 'Demo ScenarioTextStrings.csv header must be string_key,ko,en,ja.'
    }
}

switch ($Mode) {
    'PrepareMain' {
        Remove-StagedMainPayload
        Assert-MainPayloadValid
        New-Item -ItemType Directory -Path (Join-Path $stagedDirectory 'Data') -Force | Out-Null
        Copy-Item -LiteralPath (Join-Path $mainSourceDirectory 'main-payload.json') -Destination $stagedDirectory
        Copy-Item -LiteralPath (Join-Path $mainSourceDirectory 'Data\QuestDefinitions.json') -Destination (Join-Path $stagedDirectory 'Data')
        Copy-Item -LiteralPath (Join-Path $mainSourceDirectory 'Data\QuestTextStrings.csv') -Destination (Join-Path $stagedDirectory 'Data')
        Copy-Item -LiteralPath (Join-Path $mainSourceDirectory 'Data\ScenarioDefinitions.json') -Destination (Join-Path $stagedDirectory 'Data')
        Copy-Item -LiteralPath (Join-Path $mainSourceDirectory 'Data\ScenarioTextStrings.csv') -Destination (Join-Path $stagedDirectory 'Data')
        foreach ($fileName in $optionalRuntimeDataFiles) {
            $sourcePath = Join-Path $mainSourceDirectory (Join-Path 'Data' $fileName)
            if (Test-Path -LiteralPath $sourcePath -PathType Leaf) {
                Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $stagedDirectory 'Data')
            }
        }
        Write-Host "Validated and staged Main payload: $stagedDirectory"
    }
    'PrepareDemo' {
        Remove-StagedMainPayload
        Assert-DemoScenarioPackValid
        if (Test-Path -LiteralPath $stagedDirectory) {
            throw "Demo preparation failed to remove Main payload staging: $stagedDirectory"
        }
        Write-Host 'Demo payload safety check passed.'
    }
    'Clean' {
        Remove-StagedMainPayload
        Write-Host 'Transient Main payload staging removed.'
    }
    'VerifyDemo' {
        if (Test-Path -LiteralPath $stagedDirectory) {
            throw "Demo verification found Main payload staging: $stagedDirectory"
        }
        if (-not [string]::IsNullOrWhiteSpace($ArchiveDirectory) -and (Test-Path -LiteralPath $ArchiveDirectory)) {
            $forbiddenFiles = Get-ChildItem -LiteralPath $ArchiveDirectory -Recurse -File -ErrorAction Stop |
                Where-Object { $_.Name -eq 'main-payload.json' -or $_.FullName -match 'MainPayloadStaged' }
            if ($forbiddenFiles) {
                throw "Demo archive contains Main payload files: $($forbiddenFiles.FullName -join ', ')"
            }
        }
        Write-Host 'Demo archive Main payload safety check passed.'
    }
}
