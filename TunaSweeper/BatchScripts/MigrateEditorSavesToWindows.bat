@echo off
setlocal EnableExtensions EnableDelayedExpansion

echo TunaSweeper editor save migration
echo.

if "%LOCALAPPDATA%"=="" (
    echo ERROR: LOCALAPPDATA is not set.
    goto :fail
)

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"

set "SOURCE_SAVE_DIR=%~1"
if "%SOURCE_SAVE_DIR%"=="" (
    set "SOURCE_SAVE_DIR=%PROJECT_ROOT%\Saved\SaveGames"
)

set "TARGET_SAVED_DIR=%LOCALAPPDATA%\TunaSweeper\Saved"
set "TARGET_SAVE_DIR=%TARGET_SAVED_DIR%\SaveGames"
set "BACKUP_ROOT=%TARGET_SAVED_DIR%\SaveGames_Backups"

echo Source: "%SOURCE_SAVE_DIR%"
echo Target: "%TARGET_SAVE_DIR%"
echo.

if not exist "%SOURCE_SAVE_DIR%\" (
    echo ERROR: Source SaveGames folder does not exist.
    echo Pass the editor SaveGames folder as the first argument if this script was moved.
    goto :fail
)

dir /b "%SOURCE_SAVE_DIR%\TunaSweeperSave*.sav" >nul 2>nul
if errorlevel 1 (
    echo ERROR: No TunaSweeperSave*.sav files found in the source folder.
    goto :fail
)

for /f %%I in ('powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-Date -Format yyyyMMdd_HHmmss"') do set "STAMP=%%I"
if "%STAMP%"=="" set "STAMP=unknown_time"

if exist "%TARGET_SAVE_DIR%\" (
    dir /b "%TARGET_SAVE_DIR%" >nul 2>nul
    if not errorlevel 1 (
        set "BACKUP_DIR=%BACKUP_ROOT%\BeforeEditorSaveMigration_%STAMP%"
        echo Backing up existing Windows build saves to:
        echo "!BACKUP_DIR!"
        mkdir "!BACKUP_DIR!" >nul 2>nul
        if errorlevel 1 (
            echo ERROR: Failed to create backup folder.
            goto :fail
        )

        robocopy "%TARGET_SAVE_DIR%" "!BACKUP_DIR!" /E /R:1 /W:1 >nul
        set "BACKUP_RC=!ERRORLEVEL!"
        if !BACKUP_RC! GEQ 8 (
            echo ERROR: Backup failed. Robocopy exit code !BACKUP_RC!.
            goto :fail
        )
        echo Backup complete.
        echo.
    ) else (
        echo Existing target SaveGames folder is empty. Backup skipped.
        echo.
    )
) else (
    echo Existing target SaveGames folder was not found. Backup skipped.
    echo.
)

if not exist "%TARGET_SAVE_DIR%\" (
    mkdir "%TARGET_SAVE_DIR%" >nul 2>nul
    if errorlevel 1 (
        echo ERROR: Failed to create target SaveGames folder.
        goto :fail
    )
)

echo Copying editor saves...
for %%F in ("%SOURCE_SAVE_DIR%\TunaSweeperSave*.sav") do (
    echo   %%~nxF
    copy /Y "%%~fF" "%TARGET_SAVE_DIR%\" >nul
    if errorlevel 1 (
        echo ERROR: Failed to copy "%%~nxF".
        goto :fail
    )
)

echo.
echo Migration complete.
echo.
pause
exit /b 0

:fail
echo.
echo Migration failed. No target files were deleted.
echo.
pause
exit /b 1
