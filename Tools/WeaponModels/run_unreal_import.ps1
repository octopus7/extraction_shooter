param([switch]$VerifyOnly, [switch]$SkipShowcase, [switch]$ReuseImported, [string]$ProjectRoot = '')
$ErrorActionPreference = 'Stop'
$weaponRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$weaponProjectRoot = if ($ProjectRoot) { (Resolve-Path $ProjectRoot).Path } else { $weaponRoot }
$weaponEngine = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$weaponMode = if ($VerifyOnly) { 'Reload' } else { 'Import' }
$weaponScript = if ($VerifyOnly) { 'verify_unreal.py' } else { 'import_unreal.py' }
$weaponPreviousShowcase = $env:WEAPON_SKIP_SHOWCASE
$weaponPreviousReuse = $env:WEAPON_REUSE_IMPORTED
$weaponStarted = Get-Date
try {
    $env:WEAPON_SKIP_SHOWCASE = if ($SkipShowcase) { '1' } else { '0' }
    $env:WEAPON_REUSE_IMPORTED = if ($ReuseImported) { '1' } else { '0' }
    & $weaponEngine (Join-Path $weaponProjectRoot 'TunaSweeper\TunaSweeper.uproject') `
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
        '-run=pythonscript' "-script=$PSScriptRoot\$weaponScript" `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.SyncToBrowser=0' `
        '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0' `
        '-unattended' '-nullrhi' '-nosplash' '-nosound' '-stdout' '-FullStdOutLogOutput' `
        "-abslog=$weaponRoot\TunaSweeper\Saved\Logs\WeaponModels_$weaponMode.log" `
        *> "$weaponRoot\TunaSweeper\Saved\WeaponModels_$weaponMode.stdout.log"
    $weaponEngineExit = $LASTEXITCODE
    $weaponReportName = if ($VerifyOnly) { 'unreal_reload_validation.json' } else { 'unreal_import_validation.json' }
    $weaponReportPath = Join-Path $weaponRoot "TunaSweeper\SourceArt\Weapons\TunaWeaponCollection\$weaponReportName"
    if (-not (Test-Path -LiteralPath $weaponReportPath)) { throw "Weapon $weaponMode produced no report; engine exit $weaponEngineExit" }
    if ((Get-Item -LiteralPath $weaponReportPath).LastWriteTime -lt $weaponStarted) { throw 'Weapon report is stale' }
    $weaponReport = Get-Content -LiteralPath $weaponReportPath -Raw | ConvertFrom-Json
    if (-not $weaponReport.passed) { throw 'Weapon asset validation did not pass' }
    Write-Output "Weapon asset validation passed: $weaponReportPath (engine exit $weaponEngineExit)"
    if ($weaponEngineExit -ne 0) {
        throw "Assets passed validation, but Unreal exited with $weaponEngineExit. Review WeaponModels_$weaponMode.stdout.log for engine/startup errors."
    }
} finally {
    $env:WEAPON_SKIP_SHOWCASE = $weaponPreviousShowcase
    $env:WEAPON_REUSE_IMPORTED = $weaponPreviousReuse
}
