@echo off
setlocal EnableExtensions
chcp 65001 >nul

cd /d "%~dp0"
title Quest Flow Simulator - Cloudflare Deploy

set "QUEST_DEPLOY_EXIT=0"
set "QUEST_DEPLOY_PAUSE=1"
set "QUEST_DEPLOY_WORKER_ONLY=0"

if /i "%~1"=="--no-pause" set "QUEST_DEPLOY_PAUSE=0"
if /i "%~2"=="--no-pause" set "QUEST_DEPLOY_PAUSE=0"
if /i "%~1"=="--worker-only" set "QUEST_DEPLOY_WORKER_ONLY=1"
if /i "%~1"=="--help" goto :usage

echo.
echo ============================================================
echo   Quest Flow Simulator - Cloudflare manual deployment
echo ============================================================
echo   Project : %CD%
echo   Worker  : quest
echo   URL     : https://quest.oc7.workers.dev
echo ============================================================
echo.

if not exist "node_modules\.bin\wrangler.cmd" (
  echo [ERROR] Local Wrangler is not installed.
  echo         Run "npm install" in this directory first.
  set "QUEST_DEPLOY_EXIT=1"
  goto :finish
)

echo [1/5] Checking the active Cloudflare account...
call npx wrangler whoami
if errorlevel 1 goto :failed

echo.
echo [2/5] Building the production bundle...
call npm run build
if errorlevel 1 goto :failed

echo.
echo [3/5] Validating the Worker bundle with a dry run...
call npx wrangler deploy --dry-run
if errorlevel 1 goto :failed

if "%QUEST_DEPLOY_WORKER_ONLY%"=="1" (
  echo.
  echo [4/5] Skipping D1 migration and quest seed (--worker-only).
  goto :deploy_worker
)

echo.
echo [4/5] Applying remote D1 migrations and quest seed...
call npm run db:migrate:remote
if errorlevel 1 goto :failed
call npm run db:seed:remote
if errorlevel 1 goto :failed

:deploy_worker
echo.
echo [5/5] Deploying Worker quest...
call npx wrangler deploy
if errorlevel 1 goto :failed

echo.
echo ============================================================
echo   DEPLOY SUCCEEDED
echo   https://quest.oc7.workers.dev
echo ============================================================
goto :finish

:failed
set "QUEST_DEPLOY_EXIT=%errorlevel%"
if "%QUEST_DEPLOY_EXIT%"=="0" set "QUEST_DEPLOY_EXIT=1"
echo.
echo ============================================================
echo   DEPLOY FAILED - exit code %QUEST_DEPLOY_EXIT%
echo   Review the command output above.
echo ============================================================
goto :finish

:usage
echo.
echo Usage:
echo   deploy.bat
echo       Build, dry-run, migrate and seed D1, then deploy Worker.
echo.
echo   deploy.bat --worker-only
echo       Build, dry-run, and deploy Worker without changing D1.
echo.
echo   Add --no-pause as the first or second argument to skip pause.
echo.
goto :finish

:finish
echo.
if "%QUEST_DEPLOY_PAUSE%"=="1" pause
exit /b %QUEST_DEPLOY_EXIT%
