param([Parameter(Mandatory)][string]$DepotVdf)
$ErrorActionPreference = 'Stop'
$path = (Resolve-Path -LiteralPath $DepotVdf).Path
$config = [IO.File]::ReadAllText($path)
if ($config -notmatch '(?m)^\s*"DepotID"\s+"5137901"\s*$' -or $config -notmatch '\}\s*\z') {
    throw 'Expected the Steam Full depot configuration.'
}

# Paths are relative to the Windows package root. Preserve SDK-local mapping and existing exclusions.
$patterns = @(
    'TunaSweeper/Content/Data/ExcelExport/*'
    'Manifest_DebugFiles_Win64.txt'
    'Manifest_NonUFSFiles_Win64.txt'
    'Manifest_UFSFiles_Win64.txt'
    'TunaSweeper/Saved/*'
)
$newline = if ($config.Contains("`r`n")) { "`r`n" } else { "`n" }
$rules = foreach ($pattern in $patterns) {
    $match = '(?m)^\s*"FileExclusion"\s+"' + [regex]::Escape($pattern) + '"\s*$'
    if ($config -notmatch $match) { "`t`"FileExclusion`" `"$pattern`"$newline" }
}
if ($rules) {
    $updated = $config.Insert($config.LastIndexOf('}'), ($rules -join ''))
    [IO.File]::WriteAllText($path, $updated, [Text.UTF8Encoding]::new($false))
}
