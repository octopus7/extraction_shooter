[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Demo', 'Full')]
    [string]$Flavor,
    [string]$ProjectDirectory = ''
)

$ErrorActionPreference = 'Stop'
$messages = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'QuestValidationStrings.psd1')
try {
    if ([string]::IsNullOrWhiteSpace($ProjectDirectory)) { $ProjectDirectory = Join-Path $PSScriptRoot '..' }
    $paths = @((Join-Path $ProjectDirectory 'Content/Data/QuestTextStrings.csv'))
    if ($Flavor -eq 'Full') { $paths += Join-Path $ProjectDirectory 'External/MainPayload/Data/QuestTextStrings.csv' }
    $errors = 0
    $count = 0
    $keys = @{}
    foreach ($path in $paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            Write-Host ($messages.MissingFile -f $path)
            $errors++
            continue
        }
        $header = Get-Content -LiteralPath $path -Encoding UTF8 -TotalCount 1
        if ($null -eq $header -or $header.Trim() -ne 'string_key,ko,en,ja') {
            Write-Host ($messages.Header -f $path)
            $errors++
            continue
        }
        $record = 1
        foreach ($row in @(Import-Csv -LiteralPath $path -Encoding UTF8)) {
            $record++
            $count++
            $key = ([string]$row.string_key).Trim()
            if ([string]::IsNullOrWhiteSpace($key)) {
                Write-Host ($messages.EmptyKey -f $path, $record)
                $errors++
            } elseif ($keys.ContainsKey($key)) {
                Write-Host ($messages.Duplicate -f $path, $key)
                $errors++
            } else { $keys[$key] = $true }
            $missing = @('ko', 'en', 'ja' | Where-Object { [string]::IsNullOrWhiteSpace([string]$row.$_) })
            if ($missing.Count -gt 0) {
                Write-Host ($messages.Missing -f $path, $key, ($missing -join ','))
                $errors++
            }
        }
    }
    if ($errors -gt 0) {
        Write-Host ($messages.Failed -f $errors)
        exit 1
    }
    Write-Host ($messages.Passed -f $count)
    exit 0
} catch {
    Write-Host ($messages.Invalid -f $_.Exception.Message)
    exit 1
}
