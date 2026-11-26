@echo off
setlocal EnableExtensions DisableDelayedExpansion

for %%I in ("%~dp0..\..") do set "REPO_ROOT=%%~fI"
set "STEAMWORKS_ROOT=%REPO_ROOT%\store\steamworks"
set "STEAM_LOGIN_FILE=%STEAMWORKS_ROOT%\steamid.txt"
set "CONTENT_BUILDER=%STEAMWORKS_ROOT%\sdk\tools\ContentBuilder"
set "STEAMCMD=%CONTENT_BUILDER%\builder\steamcmd.exe"
set "FULL_VDF=%CONTENT_BUILDER%\scripts\app_5137900.vdf"
set "DEPOT_VDF=%CONTENT_BUILDER%\scripts\depot_5137901.vdf"
set "PACKAGE_ROOT=%~dp0Steam\Full\Windows"

if not exist "%STEAMCMD%" (
    echo [ERROR] SteamCMD not found: "%STEAMCMD%"
    exit /b 1
)
if not exist "%FULL_VDF%" (
    echo [ERROR] Full app upload config not found: "%FULL_VDF%"
    exit /b 1
)
if not exist "%DEPOT_VDF%" (
    echo [ERROR] Full depot upload config not found: "%DEPOT_VDF%"
    exit /b 1
)
if not exist "%PACKAGE_ROOT%\TunaSweeper.exe" (
    echo [ERROR] Build the Steam Full package before uploading.
    exit /b 1
)
rem Keep local App ID files in place; the depot excludes them from upload.
findstr /L /C:"\"FileExclusion\" \"steam_appid.txt\"" "%DEPOT_VDF%" >nul
if errorlevel 1 (
    echo [ERROR] Full depot must exclude steam_appid.txt.
    exit /b 1
)
findstr /L /C:"\"FileExclusion\" \"*/steam_appid.txt\"" "%DEPOT_VDF%" >nul
if errorlevel 1 (
    echo [ERROR] Full depot must exclude */steam_appid.txt.
    exit /b 1
)

set "POWERSHELL=powershell.exe"
where pwsh.exe >nul 2>nul
if not errorlevel 1 set "POWERSHELL=pwsh.exe"
"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\BuildScripts\SetSteamFullDepotExclusions.ps1" -DepotVdf "%DEPOT_VDF%"
if errorlevel 1 exit /b 1
"%POWERSHELL%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0..\BuildScripts\ValidateMovementCheatPackage.ps1" -PackageRoot "%PACKAGE_ROOT%"
if errorlevel 1 exit /b 1

if not exist "%STEAM_LOGIN_FILE%" (
    echo [ERROR] Put the Steam account name on the first line of "%STEAM_LOGIN_FILE%".
    exit /b 1
)
set "STEAM_LOGIN="
set /p "STEAM_LOGIN="<"%STEAM_LOGIN_FILE%"
if not defined STEAM_LOGIN (
    echo [ERROR] Steam login file is empty.
    exit /b 1
)

pushd "%CONTENT_BUILDER%\builder"
if errorlevel 1 exit /b 1
"%STEAMCMD%" +login "%STEAM_LOGIN%" +run_app_build "%FULL_VDF%" +quit
set "UPLOAD_EXIT_CODE=%ERRORLEVEL%"
popd
exit /b %UPLOAD_EXIT_CODE%
