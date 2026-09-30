<#
.SYNOPSIS
    Ensures the ARM64 solution's v143 toolchain has its matching ATL library.
.DESCRIPTION
    Windows SDK audiomediatypecrt.lib requests atls.lib even with UseOfAtl=false.
    The ARM runner has v143 compilers alongside VS 2026's newer tools, but may
    omit v143 ATL. Install the matching component, not latest (v145) ATL.
    Component IDs: https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-professional
    PlanOnly is read-only. Execution modifies only the selected VS installation
    when the library is absent; CI invokes it only on disposable ARM64 runners.
#>
[CmdletBinding()]
param(
    [string] $VisualStudioPath,
    [string] $InstallerPath,
    [switch] $PlanOnly
)
$ErrorActionPreference = 'Stop'

if (-not $VisualStudioPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw "vswhere not found: $vswhere" }
    $VisualStudioPath = & $vswhere -products * -requires Microsoft.Component.MSBuild -latest -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $VisualStudioPath) { throw 'Visual Studio installation not found' }
}
if (-not $InstallerPath) {
    $InstallerPath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\setup.exe'
}

# Build-Solution.ps1 uses v143 on ARM64. Its currently supported 14.44 line
# has a version-specific component ID valid in both VS 2022 and VS 2026.
# Do not silently accept a different toolchain's library if the runner drifts.
$toolsRoot = Join-Path $VisualStudioPath 'VC\Tools\MSVC'
$tools = Get-ChildItem -LiteralPath $toolsRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^14\.44\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (-not $tools) { throw "Required v143 14.44 compiler tools not found under $toolsRoot" }
$library = Join-Path $tools.FullName 'atlmfc\lib\arm64\atls.lib'
$component = 'Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL.ARM64'
$plan = [pscustomobject]@{
    Action = if (Test-Path -LiteralPath $library -PathType Leaf) { 'Ready' } else { 'Install' }
    Library = $library
    Component = $component
    Installer = $InstallerPath
    Arguments = @('modify', '--installPath', ('"{0}"' -f $VisualStudioPath),
        '--add', $component, '--quiet', '--norestart')
}
if ($PlanOnly -or $plan.Action -eq 'Ready') { return $plan }
if (-not (Test-Path -LiteralPath $InstallerPath -PathType Leaf)) {
    throw "Visual Studio installer not found: $InstallerPath"
}

Write-Host "Installing $component for missing $library"
# setup.exe does not support --wait; wait through Start-Process instead.
# 3010 requests a reboot, which is never initiated here. In either success
# case verify the actual library before allowing the build to proceed.
$process = Start-Process -FilePath $InstallerPath -ArgumentList $plan.Arguments `
    -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode -notin @(0, 3010)) {
    throw "Visual Studio ATL installation failed with exit code $($process.ExitCode)"
}
if (-not (Test-Path -LiteralPath $library -PathType Leaf)) {
    throw "ARM64 atls.lib still missing after installation: $library"
}
$plan.Action = 'Installed'
return $plan
