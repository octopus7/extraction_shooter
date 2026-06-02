@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "PROJECT_FILE=%PROJECT_ROOT%\TunaSweeper.uproject"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: "%PROJECT_FILE%"
    exit /b 1
)

set "TS_PROJECT_FILE=%PROJECT_FILE%"

echo Checking UnrealEditor.exe processes for TunaSweeper project arguments...
echo Project: "%PROJECT_FILE%"
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$projectPath = [System.IO.Path]::GetFullPath($env:TS_PROJECT_FILE);" ^
    "$projectName = [System.IO.Path]::GetFileName($projectPath);" ^
    "$projectSlash = $projectPath.Replace('\', '/');" ^
    "try {" ^
    "    $unrealProcesses = Get-CimInstance Win32_Process -Filter 'Name = ''UnrealEditor.exe''';" ^
    "} catch {" ^
    "    Write-Host ('Failed to read UnrealEditor.exe command lines: {0}' -f $_.Exception.Message);" ^
    "    Write-Host 'Run this script from a normal user shell or an elevated shell if the editor was started elevated.';" ^
    "    exit 2;" ^
    "}" ^
    "$processes = $unrealProcesses | Where-Object {" ^
    "    if (-not $_.CommandLine) { return $false }" ^
    "    $cmdSlash = $_.CommandLine.Replace('\', '/');" ^
    "    ($cmdSlash.IndexOf($projectSlash, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) -or" ^
    "    ($cmdSlash.IndexOf($projectName, [System.StringComparison]::OrdinalIgnoreCase) -ge 0)" ^
    "};" ^
    "if (-not $processes) {" ^
    "    Write-Host 'No matching TunaSweeper UnrealEditor.exe process found.';" ^
    "    exit 0;" ^
    "}" ^
    "$failed = $false;" ^
    "foreach ($process in $processes) {" ^
    "    Write-Host ('Killing PID {0}: {1}' -f $process.ProcessId, $process.CommandLine);" ^
    "    & taskkill.exe /PID $process.ProcessId /T /F;" ^
    "    if ($LASTEXITCODE -ne 0) { $failed = $true }" ^
    "}" ^
    "if ($failed) { exit 1 }" ^
    "exit 0;"
set "KILL_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%KILL_EXIT_CODE%"=="0" (
    echo Done.
) else (
    echo Failed to kill one or more matching processes. Exit code %KILL_EXIT_CODE%.
)

exit /b %KILL_EXIT_CODE%
