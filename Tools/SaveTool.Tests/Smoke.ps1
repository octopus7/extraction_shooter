param([Parameter(Mandatory=$true)][string]$Fixture)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$automation = [IO.Path]::GetFullPath((Join-Path $repo 'TunaSweeper/Saved/Automation/SaveTool'))
$source = (Resolve-Path -LiteralPath $Fixture).Path
if (!$source.StartsWith($automation + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Only a synthetic fixture under Saved/Automation/SaveTool is accepted.'
}
$run = Join-Path $automation ('Smoke ' + [guid]::NewGuid().ToString('N') + ' 한글')
New-Item -ItemType Directory -Path $run | Out-Null
$save = Join-Path $run 'TunaSweeperSave_Slot01.sav'
Copy-Item -LiteralPath $source -Destination $save
$cli = Join-Path $repo 'Tools/SaveTool/bin/Release/net10.0/SaveTool.exe'
$identity = @('--save', $save, '--flavor', 'Demo', '--slot', '1')
function Invoke-ToolCheck([string[]]$ToolArguments, [bool]$ExpectedSuccess = $true) {
    $raw = & $cli @ToolArguments --json
    $exit = $LASTEXITCODE
    $result = ($raw -join "`n") | ConvertFrom-Json
    if ($result.ok -ne $ExpectedSuccess -or ($ExpectedSuccess -and $exit -ne 0) -or (!$ExpectedSuccess -and $exit -ne 1)) {
        throw "Unexpected result: $($ToolArguments[0]) exit=$exit code=$($result.code)"
    }
    Write-Host "PASS $($ToolArguments[0]) $($result.code)"
    return $result
}
$before = (Get-FileHash -LiteralPath $save -Algorithm SHA256).Hash
$inspection = Invoke-ToolCheck (@('inspect') + $identity)
if ($inspection.validationCode -ne 'ok') { throw 'Fixture is not valid.' }
$preview = Invoke-ToolCheck (@('add') + $identity + @('--item', '5006', '--quantity', '1'))
if ($preview.committed -or (Get-FileHash -LiteralPath $save -Algorithm SHA256).Hash -ne $before) { throw 'Preview modified source.' }
$added = Invoke-ToolCheck (@('add') + $identity + @('--item', '5006', '--quantity', '1', '--commit', '--expected-hash', $inspection.hash))
if (!$added.committed -or (Get-FileHash -LiteralPath $added.backupPath -Algorithm SHA256).Hash -ne $before) { throw 'Original backup mismatch.' }
$null = Invoke-ToolCheck (@('add') + $identity + @('--item', '5006', '--commit', '--expected-hash', $inspection.hash)) $false
$presetFile = Join-Path $run 'exported.json'
$null = Invoke-ToolCheck (@('preset-export') + $identity + @('--output', $presetFile))
$null = Invoke-ToolCheck (@('preset-apply') + $identity + @('--preset', $presetFile, '--commit'))
$null = Invoke-ToolCheck (@('validate') + $identity)
$listing = Invoke-ToolCheck @('list', '--directory', $run)
if ($listing.slots.Count -ne 1) { throw 'Unexpected slot list.' }
$human = & $cli inspect @identity --lang ko
if ($LASTEXITCODE -ne 0 -or ($human -join "`n") -notmatch '장착 장비') { throw 'Human rendering failed.' }
$human | Set-Content -LiteralPath (Join-Path $run 'human.txt') -Encoding utf8
Write-Host "PASS human output; artifacts: $run"
