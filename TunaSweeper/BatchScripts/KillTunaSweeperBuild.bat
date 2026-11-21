@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "PROJECT_FILE=%PROJECT_ROOT%\TunaSweeper.uproject"
set "MODE=%~1"
if not defined MODE set "MODE=All"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: "%PROJECT_FILE%"
    exit /b 1
)

echo Checking TunaSweeper %MODE% processes...
echo Project: "%PROJECT_FILE%"
echo.

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_ROOT%\BuildScripts\StopTunaSweeperBuildProcesses.ps1" -ProjectDirectory "%PROJECT_ROOT%" -Mode "%MODE%"
set "KILL_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%KILL_EXIT_CODE%"=="0" (
    echo Done.
) else (
    echo Failed to kill one or more matching build processes. Exit code %KILL_EXIT_CODE%.
)

exit /b %KILL_EXIT_CODE%
