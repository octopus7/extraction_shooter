# Close the project editor according to Docs/unreal_editor_coordination.md first.
$ErrorActionPreference = 'Stop'
$sstoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$sstoOutput = Join-Path $sstoRoot 'TunaSweeper/Saved/SSTOEndingAutomation'
New-Item -ItemType Directory -Path $sstoOutput -Force | Out-Null
$sstoLog = Join-Path $sstoOutput 'EndingPlay.log'
$sstoArguments = @(
    (Join-Path $sstoRoot 'TunaSweeper/TunaSweeper.uproject'),
    '-RenderOffscreen', '-windowed', '-ResX=1280', '-ResY=800',
    '-unattended', '-nop4', '-nosplash', '-nosound', '-NoWebBrowser',
    '-ddc=InstalledNoZenLocalFallback',
    ('-UserDir=' + $sstoOutput + '/'),
    '-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry',
    '-ExecCmds=t.IdleWhenNotForeground 0,Slate.bAllowThrottling 0,Automation RunTests TunaSweeper.SSTO.EndingPlay',
    '-TestExit=Automation Test Queue Empty',
    ('-abslog=' + $sstoLog)
)
$sstoStarted = Get-Date
$sstoProcess = Start-Process 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
    -ArgumentList ($sstoArguments | ForEach-Object { '"' + $_ + '"' }) -WindowStyle Hidden -PassThru
# Retain the process handle before it exits, so ExitCode is available on Windows.
$null = $sstoProcess.Handle
$sstoProcess.WaitForExit()
if (!(Test-Path -LiteralPath $sstoLog) -or (Get-Item -LiteralPath $sstoLog).LastWriteTime -lt $sstoStarted) {
    throw 'Missing fresh SSTO ending play log'
}
$sstoText = Get-Content -LiteralPath $sstoLog -Raw
$sstoProcess.Refresh()
if ($sstoProcess.ExitCode -ne 0 -or $sstoText -notmatch 'Result=\{Success\} Name=\{EndingPlay\}' -or $sstoText -match 'Result=\{Fail\}') {
    throw ('SSTO ending play failed. Inspect ' + $sstoLog)
}
Write-Output ('SSTO normal/true ending and continue passed: ' + $sstoLog)
