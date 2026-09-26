@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
set "OUT=%~dp0..\..\TunaSweeper\Intermediate\StoveTests"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /EHsc /W4 /std:c++17 /utf-8 "%~dp0StovePolicyTests.cpp" /Fo"%OUT%\StovePolicyTests.obj" /Fe"%OUT%\StovePolicyTests.exe"
if errorlevel 1 exit /b %ERRORLEVEL%
"%OUT%\StovePolicyTests.exe"
exit /b %ERRORLEVEL%
