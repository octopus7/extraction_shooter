param(
    [Parameter(Mandatory = $true)]
    [string]$SourceFile,

    [string]$DestinationPath = "/Game/UI/Title",

    [string]$AssetName = "",

    [string]$ProjectPath = "",

    [string]$UnrealEditorExe = "",

    [switch]$NoReplace
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    $ProjectPath = Join-Path (Get-Location) "TunaSweeper\TunaSweeper.uproject"
}

$ResolvedProjectPath = (Resolve-Path -LiteralPath $ProjectPath).Path
$ResolvedSourceFile = (Resolve-Path -LiteralPath $SourceFile).Path

if ([string]::IsNullOrWhiteSpace($AssetName)) {
    $AssetName = [System.IO.Path]::GetFileNameWithoutExtension($ResolvedSourceFile)
}

if ([string]::IsNullOrWhiteSpace($UnrealEditorExe)) {
    if ($env:UNREAL_EDITOR_EXE) {
        $UnrealEditorExe = $env:UNREAL_EDITOR_EXE
    } elseif (Test-Path -LiteralPath "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe") {
        $UnrealEditorExe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
    } else {
        $UnrealEditorExe = "UnrealEditor.exe"
    }
}

$EditorArgs = @(
    $ResolvedProjectPath,
    "-unattended",
    "-nop4",
    "-TunaSweeperImportUiTextureSource=$ResolvedSourceFile",
    "-TunaSweeperImportUiTextureDest=$DestinationPath",
    "-TunaSweeperImportUiTextureName=$AssetName",
    "-TunaSweeperImportUiTextureQuit"
)

if ($NoReplace) {
    $EditorArgs += "-TunaSweeperImportUiTextureNoReplace"
}

& $UnrealEditorExe @EditorArgs
if ($LASTEXITCODE -ne 0) {
    throw "UnrealEditor import failed with exit code $LASTEXITCODE."
}

[PSCustomObject]@{
    SourceFile = $ResolvedSourceFile
    DestinationPath = $DestinationPath
    AssetName = $AssetName
    ObjectPath = "$DestinationPath/$AssetName.$AssetName"
}
