param(
    [string]$ProjectDirectory,
    [ValidateSet('Build', 'Packaging', 'All')]
    [string]$Mode = 'All'
)

function Get-TunaSweeperStopTargets {
    param(
        [string]$ProjectDirectory,
        [ValidateSet('Build', 'Packaging', 'All')]
        [string]$Mode,
        [object[]]$Processes
    )

    $projectPrefix = [IO.Path]::GetFullPath($ProjectDirectory).TrimEnd('\', '/').Replace('\', '/') + '/'
    $byId = @{}
    $builds = @{}
    $packages = @{}
    foreach ($process in $Processes) {
        $processId = [int]$process.ProcessId
        $byId[$processId] = $process
        if (-not $process.CommandLine -or $process.Name -notin @(
            'cmd.exe', 'powershell.exe', 'pwsh.exe', 'dotnet.exe', 'UnrealBuildTool.exe', 'AutomationTool.exe'
        )) { continue }

        $command = $process.CommandLine.Replace('\', '/')
        if ($command.IndexOf($projectPrefix, [StringComparison]::OrdinalIgnoreCase) -lt 0 -or
            $command -match '\b(KillTunaSweeperBuild\.bat|StopTunaSweeperBuildProcesses\.ps1)\b') { continue }

        if ($command -match '\bPackageTunaSweeper[^/"\s]*\.bat\b' -or
            ($command -match '\bBuildCookRun\b' -and $command -match '\b(RunUAT\.bat|AutomationTool(?:\.(?:dll|exe))?)\b')) {
            $packages[$processId] = $process
        } elseif ($command -match '\b(BuildAndRunTunaSweeper\.bat|Build\.bat|UnrealBuildTool)\b') {
            $builds[$processId] = $process
        }
    }

    function Get-ProcessAncestorIds($process) {
        $seen = @{ ([int]$process.ProcessId) = $true }
        while ($parent = $byId[[int]$process.ParentProcessId]) {
            $parentId = [int]$parent.ProcessId
            if ($seen.ContainsKey($parentId) -or $parent.CreationDate -gt $process.CreationDate) { break }
            $seen[$parentId] = $true
            $parentId
            $process = $parent
        }
    }

    $targets = @{}
    if ($Mode -ne 'Build') {
        foreach ($processId in $packages.Keys) { $targets[$processId] = $packages[$processId] }
    }
    if ($Mode -eq 'All') {
        foreach ($processId in $builds.Keys) { $targets[$processId] = $builds[$processId] }
    } elseif ($Mode -eq 'Build') {
        $packageAncestors = @{}
        foreach ($package in $packages.Values) {
            foreach ($ancestorId in (Get-ProcessAncestorIds $package)) { $packageAncestors[$ancestorId] = $true }
        }
        foreach ($processId in $builds.Keys) {
            $build = $builds[$processId]
            if ($packageAncestors.ContainsKey($processId) -or
                @(Get-ProcessAncestorIds $build | Where-Object { $packages.ContainsKey($_) }).Count -gt 0) { continue }
            $targets[$processId] = $build
        }
    }

    foreach ($target in $targets.Values) {
        if (@(Get-ProcessAncestorIds $target | Where-Object { $targets.ContainsKey($_) }).Count -eq 0) {
            $target
        }
    }
}

if ($MyInvocation.InvocationName -eq '.') { return }
$ErrorActionPreference = 'Stop'
try {
    $processes = Get-CimInstance Win32_Process
    $targets = @(Get-TunaSweeperStopTargets -ProjectDirectory $ProjectDirectory -Mode $Mode -Processes $processes)
    if ($targets.Count -eq 0) {
        Write-Host "No matching TunaSweeper $Mode process found."
    }

    $failed = $false
    foreach ($target in $targets) {
        $current = Get-CimInstance Win32_Process -Filter "ProcessId = $($target.ProcessId)"
        if ($null -eq $current -or $current.CreationDate -ne $target.CreationDate) { continue }
        Write-Host ("Stopping {0} PID {1} ({2})" -f $Mode, $target.ProcessId, $target.Name)
        & taskkill.exe /PID $target.ProcessId /T /F
        if ($LASTEXITCODE -ne 0) { $failed = $true }
    }
    if ($failed) { exit 1 }
} catch {
    Write-Host ("Failed to stop TunaSweeper {0} processes: {1}" -f $Mode, $_.Exception.Message)
    exit 2
}
