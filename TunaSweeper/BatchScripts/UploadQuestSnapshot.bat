@echo off
setlocal
chcp 65001 >nul
pushd "%~dp0..\..\Tools\QuestStudio"
if errorlevel 1 exit /b 1
node scripts\quest-exchange.cjs upload
set "QUEST_EXCHANGE_EXIT=%ERRORLEVEL%"
popd
if not defined QUEST_EXCHANGE_NO_PAUSE pause
exit /b %QUEST_EXCHANGE_EXIT%
