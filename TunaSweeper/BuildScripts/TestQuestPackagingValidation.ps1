$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('quest-validation-' + [guid]::NewGuid())
$validator = Join-Path $PSScriptRoot 'ValidateQuestTranslations.ps1'
$utf8 = [Text.UTF8Encoding]::new($false)
function Write-Fixture([string]$RelativePath, [string]$Content) {
    $path = Join-Path $fixture $RelativePath
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($path)) | Out-Null
    [IO.File]::WriteAllText($path, $Content, $utf8)
}
function Assert-Validation([string]$Flavor, [bool]$Success, [string[]]$Expected = @()) {
    $output = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $validator -Flavor $Flavor -ProjectDirectory $fixture 2>&1 | Out-String
    if (($LASTEXITCODE -eq 0) -ne $Success) { throw "Unexpected validator result: $output" }
    foreach ($text in $Expected) { if (-not $output.Contains($text)) { throw "Missing diagnostic '$text': $output" } }
}
try {
    Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en,ja`npublic.title,KO,EN,JA`n"
    Write-Fixture 'External/MainPayload/Data/QuestTextStrings.csv' "string_key,ko,en,ja`nmain.title,KO,,`nmain.description,KO,EN,`n"
    Write-Fixture 'Authoring/QuestStudio/QuestStudioNotes.ko.csv' "string_key,ko`nnotes,KO`n"
    Assert-Validation Demo $true
    Assert-Validation Full $false @('main.title', 'main.description', 'en', 'ja', 'QuestTextStrings.csv')
    Write-Fixture 'External/MainPayload/Data/QuestTextStrings.csv' "string_key,ko,en,ja`nmain.title,KO,EN,JA`n"
    Assert-Validation Full $true
    Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en,ja`nquoted.title,`"KO, text`nsecond line`",EN,JA`n"
    Assert-Validation Demo $true
    Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en,ja`npublic.title,KO,`"  `",JA`n"
    Assert-Validation Demo $false @('public.title', 'en')
    Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en`npublic.title,KO,EN`n"
    Assert-Validation Demo $false @('QuestTextStrings.csv')
    Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en,ja`npublic.title,KO,,JA`n"
    Write-Fixture 'TunaSweeper.uproject' '{}'
    Write-Fixture 'FakeUE/Engine/Build/BatchFiles/RunUAT.bat' "@echo UAT_REACHED`n@exit /b 77`n"
    Write-Fixture 'BuildScripts/BuildFlavorData.ps1' "throw 'PREPARE_REACHED'"
    Copy-Item -LiteralPath $validator -Destination (Join-Path $fixture 'BuildScripts')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'QuestValidationStrings.psd1') -Destination (Join-Path $fixture 'BuildScripts')
    $oldEngine = $env:UE_5_7_ROOT
    try {
        $env:UE_5_7_ROOT = Join-Path $fixture 'FakeUE'
        foreach ($bat in Get-ChildItem (Join-Path $PSScriptRoot '../BatchScripts/Package*.bat')) {
            Write-Fixture ('BatchScripts/' + $bat.Name) (Get-Content -LiteralPath $bat.FullName -Raw)
            $output = & cmd.exe /d /c (Join-Path $fixture ('BatchScripts/' + $bat.Name)) 2>&1 | Out-String
            if ($LASTEXITCODE -eq 0 -or -not $output.Contains('public.title') -or $output.Contains('UAT_REACHED') -or $output.Contains('PREPARE_REACHED')) {
                throw "Packaging guard failed for $($bat.Name): $output"
            }
        }
        Write-Fixture 'Content/Data/QuestTextStrings.csv' "string_key,ko,en,ja`npublic.title,KO,EN,JA`n"
        Write-Fixture 'BuildScripts/BuildFlavorData.ps1' "Write-Output 'PREPARE_REACHED'"
        foreach ($bat in Get-ChildItem (Join-Path $fixture 'BatchScripts/Package*.bat')) {
            $output = & cmd.exe /d /c $bat.FullName 2>&1 | Out-String
            if ($LASTEXITCODE -ne 77 -or -not $output.Contains('UAT_REACHED') -or -not $output.Contains('PREPARE_REACHED')) {
                throw "Packaging pass-through failed for $($bat.Name): $output"
            }
        }
    } finally { $env:UE_5_7_ROOT = $oldEngine }
    Write-Output 'PASS: translation validation and all four BAT preflight guards.'
} finally {
    $resolved = [IO.Path]::GetFullPath($fixture)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $resolved.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test cleanup path' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
