@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "PROJECT_FILE=%PROJECT_ROOT%\TunaSweeper.uproject"
set "TS_PROJECT_FILE=%PROJECT_FILE%"
set "TS_PROJECT_ROOT=%PROJECT_ROOT%"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: "%PROJECT_FILE%"
    exit /b 1
)

echo Checking TunaSweeper build and packaging processes...
echo Project: "%PROJECT_FILE%"
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$projectPath = [System.IO.Path]::GetFullPath($env:TS_PROJECT_FILE);" ^
    "$projectName = [System.IO.Path]::GetFileName($projectPath);" ^
    "$projectRoot = [System.IO.Path]::GetFullPath($env:TS_PROJECT_ROOT);" ^
    "$scope = @($projectPath, $projectName, $projectRoot) | ForEach-Object { $_.Replace([char]92, '/') };" ^
    "$markers = @('Build.bat', 'RunUAT.bat', 'UnrealBuildTool', 'AutomationTool', 'BuildCookRun', 'BuildAndRunTunaSweeper.bat', 'PackageTunaSweeper');" ^
    "try {" ^
    "    $candidates = Get-CimInstance Win32_Process -ErrorAction Stop;" ^
    "} catch {" ^
    "    Write-Host ('Failed to read build processes: {0}' -f $_.Exception.Message);" ^
    "    Write-Host 'Run this script from a normal user shell or an elevated shell if the build was started elevated.';" ^
    "    exit 2;" ^
    "}" ^
    "$processes = $candidates | Where-Object {" ^
    "    if (-not $_.CommandLine -or $_.Name -notin @('cmd.exe', 'powershell.exe', 'pwsh.exe', 'dotnet.exe', 'UnrealBuildTool.exe', 'AutomationTool.exe')) { return $false }" ^
    "    $cmdSlash = $_.CommandLine.Replace([char]92, '/');" ^
    "    if ($cmdSlash.IndexOf('KillTunaSweeperBuild.bat', [System.StringComparison]::OrdinalIgnoreCase) -ge 0) { return $false }" ^
    "    $inProject = $false; foreach ($part in $scope) { if ($cmdSlash.IndexOf($part, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) { $inProject = $true; break } };" ^
    "    $isBuild = $false; foreach ($marker in $markers) { if ($cmdSlash.IndexOf($marker, [System.StringComparison]::OrdinalIgnoreCase) -ge 0) { $isBuild = $true; break } };" ^
    "    $inProject -and $isBuild" ^
    "};" ^
    "if (-not $processes) {" ^
    "    Write-Host 'No matching TunaSweeper build or packaging process found.';" ^
    "    exit 0;" ^
    "}" ^
    "$failed = $false;" ^
    "foreach ($process in $processes) {" ^
    "    Write-Host ('Killing PID {0}: {1}' -f $process.ProcessId, $process.CommandLine);" ^
    "    & taskkill.exe /PID $process.ProcessId /T /F;" ^
    "    if ($LASTEXITCODE -ne 0) { $failed = $true }" ^
    "}" ^
    "if ($failed) { exit 1 }" ^
    "exit 0;"
set "KILL_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%KILL_EXIT_CODE%"=="0" (
    echo Done.
) else (
    echo Failed to kill one or more matching build processes. Exit code %KILL_EXIT_CODE%.
)

exit /b %KILL_EXIT_CODE%
