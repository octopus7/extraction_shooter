param([Parameter(Mandatory=$true)][string]$PackageRoot)
$ErrorActionPreference = 'Stop'
$resolvedRoot = (Resolve-Path -LiteralPath $PackageRoot).Path
$files = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File)
$forbidden = @($files | Where-Object {
    $_.Name -match '^TunaBuildHelper\.' -or $_.Name -eq 'MovementBookmarks.json' -or
    $_.FullName -match '[\\/](TunaSweeperDeveloper|MovementCheats)[\\/]'
})
if ($forbidden.Count) { throw 'Package contains local developer movement-cheat data or helper artifacts.' }
$executables = @($files | Where-Object { $_.Name -match '^TunaSweeper(Demo|NoStore|NoStoreDemo|Stove|StoveDemo)?(-Win64-(Shipping|Test))?\.exe$' -and $_.Directory.Name -eq 'Win64' })
if ($executables.Count -ne 1) { throw 'Expected one TunaSweeper game executable under Binaries/Win64.' }
$bytes = [IO.File]::ReadAllBytes($executables[0].FullName)
function U16([int]$offset) { [BitConverter]::ToUInt16($bytes, $offset) }
function U32([int]$offset) { [BitConverter]::ToUInt32($bytes, $offset) }
function U64([int]$offset) { [BitConverter]::ToUInt64($bytes, $offset) }
$pe = U32 60
if ((U16 0) -ne 0x5A4D -or (U32 $pe) -ne 0x4550 -or (U16 ($pe+24)) -ne 0x20B) { throw 'Expected an x64 PE image.' }
$sectionCount = U16 ($pe+6)
$sections = $pe+24+(U16 ($pe+20))
function FileOffset([uint32]$rva) {
    for ($i=0; $i -lt $sectionCount; $i++) {
        $s = $sections + $i*40
        $start = U32 ($s+12)
        $rawSize = U32 ($s+16)
        if ($rva -ge $start -and [uint64]$rva -lt ([uint64]$start + $rawSize)) {
            return [int]((U32 ($s+20)) + $rva - $start)
        }
    }
    throw 'Export must have file-backed initialized data.'
}
$exportRva = U32 ($pe+136)
$exportSize = U32 ($pe+140)
if ($exportRva -eq 0) { throw 'Movement cheat control export missing.' }
$directory = FileOffset $exportRva
$nameCount = U32 ($directory+24)
if ($nameCount -gt 1000000) { throw 'Invalid export name count.' }
$names = FileOffset (U32 ($directory+32))
$ordinals = FileOffset (U32 ($directory+36))
$functions = FileOffset (U32 ($directory+28))
function FindControlExport([string]$exportName) {
for ($i=0; $i -lt $nameCount; $i++) {
    $nameOffset = FileOffset (U32 ($names+$i*4))
    $end = [Array]::IndexOf($bytes, [byte]0, $nameOffset)
    if ($end -lt $nameOffset) { throw 'Invalid export string.' }
    $name = [Text.Encoding]::ASCII.GetString($bytes, $nameOffset, $end-$nameOffset)
    if ($name -eq $exportName) {
        $ordinal = U16 ($ordinals+$i*2)
        if ($ordinal -ge (U32 ($directory+20))) { throw 'Invalid export ordinal.' }
        $controlRva = U32 ($functions+$ordinal*4)
        if ($controlRva -ge $exportRva -and [uint64]$controlRva -lt ([uint64]$exportRva+$exportSize)) { throw 'Forwarded control export is not supported.' }
        return $controlRva
    }
}
throw "Required cheat control export missing: $exportName"
}
$control = FileOffset (FindControlExport 'TunaSweeperMovementCheatControl')
if ((U64 $control) -ne 0x54534D4F56454348 -or (U32 ($control+8)) -ne 1 -or (U32 ($control+12)) -ne 32) { throw 'Unexpected movement cheat control protocol.' }
if ((U32 ($control+16)) -ne 0 -or (U32 ($control+20)) -ne 0 -or (U64 ($control+24)) -ne 0) { throw 'Packaged movement cheat control must be inactive with zero runtime state.' }
$gateControlRva = FindControlExport 'TunaSweeperGateCheatControl'
$gateControl = FileOffset $gateControlRva
# The independent gate mailbox has no persisted request or activation state.
# Header validation and the complete zero-runtime-state check are kept in sync
# with FTunaSweeperGateCheatControl when its protocol changes.
if ((U64 $gateControl) -ne 0x5453474154454348 -or (U32 ($gateControl+8)) -ne 1 -or (U32 ($gateControl+12)) -ne 25864) { throw 'Unexpected gate cheat control protocol.' }
for ($i = 16; $i -lt 25864; $i++) {
    $offset = FileOffset ([uint32]($gateControlRva + $i))
    if ($bytes[$offset] -ne 0) { throw 'Packaged gate cheat mailbox must have zero runtime state and no queued reset.' }
}
Write-Host 'Cheat package guard passed: inactive movement gate, empty gate-reset mailbox; no local bookmark/helper artifacts.'
