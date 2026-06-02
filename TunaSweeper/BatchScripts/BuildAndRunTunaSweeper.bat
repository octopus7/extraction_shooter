@echo off
setlocal

set "PROJECT_FILE=%~dp0..\TunaSweeper.uproject"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=TunaSweeperEditor"

set "PLATFORM=%~2"
if "%PLATFORM%"=="" set "PLATFORM=Win64"

set "CONFIGURATION=%~3"
if "%CONFIGURATION%"=="" set "CONFIGURATION=Development"

set "UE_ROOT=%UE_5_7_ROOT%"
if "%UE_ROOT%"=="" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.7"

set "BUILD_BAT=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
if not exist "%BUILD_BAT%" set "BUILD_BAT=%UE_ROOT%\Build\BatchFiles\Build.bat"

set "EDITOR_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR_EXE%" set "EDITOR_EXE=%UE_ROOT%\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: "%PROJECT_FILE%"
    exit /b 1
)

if not exist "%BUILD_BAT%" (
    echo Unreal Build.bat not found.
    echo Checked UE root: "%UE_ROOT%"
    echo Set UE_5_7_ROOT to your Unreal Engine 5.7 install directory and run again.
    exit /b 1
)

echo Building %TARGET% %PLATFORM% %CONFIGURATION%
echo Project: "%PROJECT_FILE%"
echo Unreal: "%BUILD_BAT%"
echo.

call "%BUILD_BAT%" %TARGET% %PLATFORM% %CONFIGURATION% -Project="%PROJECT_FILE%" -WaitMutex
set "BUILD_EXIT_CODE=%ERRORLEVEL%"

echo.
if not "%BUILD_EXIT_CODE%"=="0" (
    echo Build failed with exit code %BUILD_EXIT_CODE%.
    exit /b %BUILD_EXIT_CODE%
)

echo Build succeeded.

if not exist "%EDITOR_EXE%" (
    echo UnrealEditor.exe not found.
    echo Checked UE root: "%UE_ROOT%"
    echo Set UE_5_7_ROOT to your Unreal Engine 5.7 install directory and run again.
    exit /b 1
)

echo Opening TunaSweeper editor...
echo Editor: "%EDITOR_EXE%"
start "" "%EDITOR_EXE%" "%PROJECT_FILE%"

exit /b 0
