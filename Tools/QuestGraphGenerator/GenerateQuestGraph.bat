@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

pushd "%SCRIPT_DIR%..\.."
if errorlevel 1 (
    echo Failed to enter project root.
    echo.
    pause
    exit /b 1
)

set "PROJECT_ROOT=%CD%"
dotnet run --project "%SCRIPT_DIR%QuestGraphGenerator.csproj" -- --project-root "%PROJECT_ROOT%"
set "EXIT_CODE=%ERRORLEVEL%"
popd

echo.
if "%EXIT_CODE%"=="0" (
    echo Quest graph generated successfully.
) else (
    echo Quest graph generation failed with exit code %EXIT_CODE%.
)
echo Output: %PROJECT_ROOT%\Docs\quest_graph.md
echo.
pause
exit /b %EXIT_CODE%
