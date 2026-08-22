@echo off
setlocal EnableExtensions DisableDelayedExpansion

for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"

set "STEAMWORKS_ROOT=%REPO_ROOT%\store\steamworks"
set "STEAM_LOGIN_FILE=%STEAMWORKS_ROOT%\steamid.txt"
set "CONTENT_BUILDER=%STEAMWORKS_ROOT%\sdk\tools\ContentBuilder"
set "STEAMCMD=%CONTENT_BUILDER%\builder\steamcmd.exe"
set "DEMO_VDF=%CONTENT_BUILDER%\scripts\app_5158070.vdf"

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

pushd "%CONTENT_BUILDER%\builder" || exit /b 1
"%STEAMCMD%" +login "%STEAM_LOGIN%" +run_app_build "%DEMO_VDF%" +quit
set "UPLOAD_EXIT_CODE=%ERRORLEVEL%"
popd

exit /b %UPLOAD_EXIT_CODE%
