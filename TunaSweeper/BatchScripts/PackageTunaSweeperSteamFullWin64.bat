@echo off
setlocal

set "PROJECT_FILE=%~dp0..\TunaSweeper.uproject"
set "TARGET_NAME=TunaSweeper"
set "CUSTOM_CONFIG=Full"
set "ARCHIVE_DIR=%~dp0..\Builds\Steam\Full"
set "DISPLAY_NAME=TunaSweeper Steam Full"

set "CONFIGURATION=%~1"
if "%CONFIGURATION%"=="" set "CONFIGURATION=Shipping"
if /I not "%CONFIGURATION%"=="Development" if /I not "%CONFIGURATION%"=="Shipping" (
    echo Usage: %~nx0 [Development^|Shipping]
    exit /b 2
)

set "UE_ROOT=%UE_5_7_ROOT%"
if "%UE_ROOT%"=="" set "UE_ROOT=C:\Program Files\Epic Games\UE_5.7"

set "RUN_UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%RUN_UAT%" set "RUN_UAT=%UE_ROOT%\Build\BatchFiles\RunUAT.bat"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: "%PROJECT_FILE%"
    exit /b 1
)

if not exist "%RUN_UAT%" (
    echo Unreal RunUAT.bat not found.
    echo Checked UE root: "%UE_ROOT%"
    echo Set UE_5_7_ROOT to your Unreal Engine 5.7 install directory and run again.
    exit /b 1
)

set "ARCHIVE_PLATFORM_DIR=%ARCHIVE_DIR%\Windows"
if exist "%ARCHIVE_PLATFORM_DIR%" (
    echo Removing stale packaged output: "%ARCHIVE_PLATFORM_DIR%"
    rmdir /s /q "%ARCHIVE_PLATFORM_DIR%"
    if exist "%ARCHIVE_PLATFORM_DIR%" (
        echo Failed to remove stale packaged output.
        exit /b 1
    )
)

echo Cooking and packaging %DISPLAY_NAME% Win64 %CONFIGURATION%
echo Target: "%TARGET_NAME%"
echo Project: "%PROJECT_FILE%"
echo Unreal: "%RUN_UAT%"
echo Output: "%ARCHIVE_DIR%"
echo.

call "%RUN_UAT%" BuildCookRun ^
    -project="%PROJECT_FILE%" ^
    -noP4 ^
    -target=%TARGET_NAME% ^
    -customconfig=%CUSTOM_CONFIG% ^
    -platform=Win64 ^
    -clientconfig=%CONFIGURATION% ^
    -serverconfig=%CONFIGURATION% ^
    -build ^
    -cook ^
    -stage ^
    -package ^
    -pak ^
    -iostore ^
    -compressed ^
    -prereqs ^
    -archive ^
    -archivedirectory="%ARCHIVE_DIR%" ^
    -unattended ^
    -utf8output
set "PACKAGE_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%PACKAGE_EXIT_CODE%"=="0" (
    echo Package succeeded.
    echo Output: "%ARCHIVE_PLATFORM_DIR%"
) else (
    echo Package failed with exit code %PACKAGE_EXIT_CODE%.
)

exit /b %PACKAGE_EXIT_CODE%
