@echo off
setlocal EnableExtensions DisableDelayedExpansion

for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"

set "STEAMWORKS_ROOT=%REPO_ROOT%\store\steamworks"
set "STEAM_LOGIN_FILE=%STEAMWORKS_ROOT%\steamid.txt"
set "CONTENT_BUILDER=%STEAMWORKS_ROOT%\sdk\tools\ContentBuilder"
set "STEAMCMD=%CONTENT_BUILDER%\builder\steamcmd.exe"
set "DEMO_VDF=%CONTENT_BUILDER%\scripts\app_5158070.vdf"
set "STEAM_APPID_SOURCE=%~dp0Demo\Windows\TunaSweeper\Binaries\Win64\steam_appid.txt"
set "STEAM_APPID_TEMP=%~dp0steam_appid.txt"
set "STEAM_APPID_MOVED=0"

if not exist "%STEAM_LOGIN_FILE%" (
    echo [ERROR] Steam login file not found:
    echo         %STEAM_LOGIN_FILE%
    echo Create the file and put the Steam account name on its first line.
    exit /b 1
)

set "STEAM_LOGIN="
set /p "STEAM_LOGIN="<"%STEAM_LOGIN_FILE%"
if not defined STEAM_LOGIN (
    echo [ERROR] Steam login file is empty:
    echo         %STEAM_LOGIN_FILE%
    exit /b 1
)

if not exist "%STEAMCMD%" (
    echo [ERROR] steamcmd.exe not found:
    echo         %STEAMCMD%
    exit /b 1
)

if not exist "%DEMO_VDF%" (
    echo [ERROR] Demo VDF not found:
    echo         %DEMO_VDF%
    exit /b 1
)

rem Keep the local testing AppID file out of the Steam depot during upload.
if exist "%STEAM_APPID_SOURCE%" (
    if exist "%STEAM_APPID_TEMP%" del /q "%STEAM_APPID_TEMP%"
    move /y "%STEAM_APPID_SOURCE%" "%STEAM_APPID_TEMP%" >nul
    if errorlevel 1 (
        echo [ERROR] Could not temporarily move steam_appid.txt out of the depot.
        exit /b 1
    )
    set "STEAM_APPID_MOVED=1"
)

pushd "%CONTENT_BUILDER%\builder"
if errorlevel 1 (
    if "%STEAM_APPID_MOVED%"=="1" move /y "%STEAM_APPID_TEMP%" "%STEAM_APPID_SOURCE%" >nul
    exit /b 1
)
"%STEAMCMD%" +login "%STEAM_LOGIN%" +run_app_build "%DEMO_VDF%" +quit
set "UPLOAD_EXIT_CODE=%ERRORLEVEL%"
popd

if "%STEAM_APPID_MOVED%"=="1" (
    move /y "%STEAM_APPID_TEMP%" "%STEAM_APPID_SOURCE%" >nul
    if errorlevel 1 (
        echo [ERROR] Upload finished, but steam_appid.txt could not be restored.
        exit /b 1
    )
)

exit /b %UPLOAD_EXIT_CODE%
