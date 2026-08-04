[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$pluginRoot = Split-Path -Parent $scriptDirectory
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $pluginRoot "../.."))
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot ".."))

$forbiddenPrefixes = @(
    "TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload/",
    "TunaSweeper/Content/Data/QuestDatasetGenerated/"
)

function Assert-NoForbiddenFiles {
    param(
        [string[]]$Files,
        [string]$SourceDescription
    )

    foreach ($file in $Files) {
        $normalizedFile = $file.Replace('\', '/')
        foreach ($prefix in $forbiddenPrefixes) {
            if ($normalizedFile.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                throw "Production path found in ${SourceDescription}: $normalizedFile"
            }
        }
    }
}

$trackedFiles = @(& git -C $repositoryRoot ls-files)
if ($LASTEXITCODE -ne 0) {
    throw "Could not list public repository files."
}
Assert-NoForbiddenFiles -Files $trackedFiles -SourceDescription "public Git tracking"

$stagedFiles = @(& git -C $repositoryRoot diff --cached --name-only)
if ($LASTEXITCODE -ne 0) {
    throw "Could not list staged public repository files."
}
Assert-NoForbiddenFiles -Files $stagedFiles -SourceDescription "public Git staging"

foreach ($relativePath in @(
    "TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload/payload-manifest.json",
    "TunaSweeper/Content/Data/QuestDatasetGenerated/active-dataset.json"
)) {
    & git -C $repositoryRoot check-ignore --quiet -- $relativePath
    if ($LASTEXITCODE -ne 0) {
        throw "Required public ignore rule is missing for: $relativePath"
    }
}

Write-Host "Public repository quest dataset safety check passed."
