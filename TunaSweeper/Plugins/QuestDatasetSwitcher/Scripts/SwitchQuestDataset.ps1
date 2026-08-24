[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Public", "ProductionDemo", "ProductionRelease")]
    [string]$Dataset,

    [switch]$VerifyOnly
)

$ErrorActionPreference = "Stop"

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$pluginRoot = Split-Path -Parent $scriptDirectory
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $pluginRoot "../.."))
$payloadRoot = Join-Path $pluginRoot "ProductionPayload"
$generatedRoot = Join-Path $projectRoot "Content/Data/QuestDatasetGenerated"
$publicTextPath = Join-Path $projectRoot "Content/Data/QuestTextStrings.csv"

function Assert-ChildPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Parent,

        [Parameter(Mandatory = $true)]
        [string]$Child
    )

    $parentFull = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $childFull = [System.IO.Path]::GetFullPath($Child)
    if (-not $childFull.StartsWith($parentFull, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Unsafe child path: $childFull"
    }
}

function Remove-GeneratedDataset {
    Assert-ChildPath -Parent $projectRoot -Child $generatedRoot
    if (Test-Path -LiteralPath $generatedRoot) {
        Remove-Item -LiteralPath $generatedRoot -Recurse -Force
    }
}

if ($Dataset -eq "Public") {
    if ($VerifyOnly) {
        if (Test-Path -LiteralPath $generatedRoot) {
            throw "Public verification failed: generated production data still exists at $generatedRoot"
        }

        Write-Host "Public quest dataset verification passed."
        exit 0
    }

    Remove-GeneratedDataset
    Write-Host "Quest dataset switched to Public."
    exit 0
}

$payloadManifestPath = Join-Path $payloadRoot "payload-manifest.json"
if (-not (Test-Path -LiteralPath $payloadManifestPath)) {
    throw "Production payload is not installed: $payloadManifestPath"
}

$payloadManifest = Get-Content -LiteralPath $payloadManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($payloadManifest.plugin_name -ne "QuestDatasetSwitcher") {
    throw "Production payload plugin_name must be QuestDatasetSwitcher."
}

$datasetRoot = Join-Path $payloadRoot ("Datasets/" + $Dataset)
$datasetManifestPath = Join-Path $datasetRoot "dataset-manifest.json"
$questDefinitionsPath = Join-Path $datasetRoot "QuestDefinitions.json"
$questTextOverridesPath = Join-Path $datasetRoot "QuestTextOverrides.csv"

foreach ($requiredPath in @(
    $datasetManifestPath,
    $questDefinitionsPath,
    $questTextOverridesPath,
    $publicTextPath
)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required quest dataset file is missing: $requiredPath"
    }
}

$datasetManifest = Get-Content -LiteralPath $datasetManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
$expectedDatasetId = if ($Dataset -eq "ProductionDemo") {
    "production_demo"
} else {
    "production_release"
}

if ($datasetManifest.dataset_id -ne $expectedDatasetId) {
    throw "Dataset id mismatch. Expected '$expectedDatasetId'."
}

foreach ($requiredField in @("dataset_revision", "save_compatibility_id")) {
    $fieldValue = $datasetManifest.$requiredField
    if ([string]::IsNullOrWhiteSpace([string]$fieldValue)) {
        throw "Dataset manifest field '$requiredField' is required."
    }
}

$questDefinitions = Get-Content -LiteralPath $questDefinitionsPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Dataset -ne "ProductionDemo" -and ($null -eq $questDefinitions -or @($questDefinitions).Count -eq 0)) {
    throw "QuestDefinitions.json must contain at least one quest."
}

$questIds = @{}
foreach ($quest in @($questDefinitions)) {
    $questId = [string]$quest.quest_id
    if ([string]::IsNullOrWhiteSpace($questId)) {
        throw "Every production quest must have quest_id."
    }
    if ($questIds.ContainsKey($questId)) {
        throw "Duplicate production quest_id: $questId"
    }
    $questIds[$questId] = $true
}

foreach ($quest in @($questDefinitions)) {
    foreach ($requiredQuestId in @($quest.required_completed_quest_ids)) {
        if (-not [string]::IsNullOrWhiteSpace([string]$requiredQuestId) -and
            -not $questIds.ContainsKey([string]$requiredQuestId)) {
            throw "Quest '$($quest.quest_id)' requires missing quest '$requiredQuestId'."
        }
    }
}

$publicTextRows = @(Import-Csv -LiteralPath $publicTextPath)
$overrideTextRows = @(Import-Csv -LiteralPath $questTextOverridesPath)
$textRowsByKey = [ordered]@{}
foreach ($row in $publicTextRows + $overrideTextRows) {
    $key = [string]$row.string_key
    if ([string]::IsNullOrWhiteSpace($key)) {
        throw "Quest text CSV contains an empty string_key."
    }
    $textRowsByKey[$key] = $row
}

if ($VerifyOnly) {
    Write-Host "$Dataset quest dataset verification passed."
    Write-Host "Quest count: $(@($questDefinitions).Count)"
    exit 0
}

Remove-GeneratedDataset
New-Item -ItemType Directory -Path $generatedRoot -Force | Out-Null
Copy-Item -LiteralPath $questDefinitionsPath -Destination (Join-Path $generatedRoot "QuestDefinitions.json")
$textRowsByKey.Values |
    Select-Object string_key, ko, en, ja |
    Export-Csv -LiteralPath (Join-Path $generatedRoot "QuestTextStrings.csv") -NoTypeInformation -Encoding UTF8

$productionRevision = "unknown"
$normalizedPayloadRoot = $payloadRoot.Replace('\', '/')
$revisionOutput = & git -c "safe.directory=$normalizedPayloadRoot" -C $payloadRoot rev-parse HEAD 2>$null
if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace([string]$revisionOutput)) {
    $productionRevision = ([string]$revisionOutput).Trim()
} else {
    Write-Warning "Could not read the ProductionPayload Git revision."
}

$activeDataset = [ordered]@{
    schema_version = 1
    dataset_id = $datasetManifest.dataset_id
    dataset_revision = $datasetManifest.dataset_revision
    save_compatibility_id = $datasetManifest.save_compatibility_id
    production_repository_revision = $productionRevision
    synchronized_at_utc = [DateTime]::UtcNow.ToString("o")
}

$activeDataset |
    ConvertTo-Json |
    Set-Content -LiteralPath (Join-Path $generatedRoot "active-dataset.json") -Encoding UTF8

Write-Host "Quest dataset switched to $Dataset."
Write-Host "Quest count: $(@($questDefinitions).Count)"
Write-Host "Save compatibility: $($datasetManifest.save_compatibility_id)"
Write-Host "Restart Unreal Editor before testing the new dataset."
