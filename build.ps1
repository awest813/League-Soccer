# build.ps1 – one-shot build for League-Soccer / Gameplay Football on Windows.
#
# Bootstraps vcpkg, installs dependencies (if needed), configures CMake, and
# builds the game. Runtime assets (media/, databases/, locale/,
# football.config) and required DLLs are copied next to the binary
# automatically by the CMake POST_BUILD step.
#
# Mirrors ./build.sh on Linux.
#
# Usage:
#   .\build.ps1 [options]
#
# Options:
#   -Release     Optimised build (default)
#   -DebugBuild  Debug build with assertions
#   -Clean       Remove build-win\ before building
#   -NoDeps      Skip the dependency-install step (use an existing vcpkg tree)
#   -Jobs N      Parallel compile jobs (default: memory-aware, see below)
#   -VcpkgRoot   Path to vcpkg (default: $env:VCPKG_ROOT or .\vcpkg)
#   -Triplet     vcpkg triplet (default: $env:VCPKG_DEFAULT_TRIPLET or x64-windows)
#   -Help        Show this message
#
# Examples:
#   .\build.ps1                       # release build, bootstraps vcpkg + deps first
#   .\build.ps1 -DebugBuild -Clean    # clean debug build
#   .\build.ps1 -NoDeps               # rebuild using already-installed deps
#
# First run only, if PowerShell refuses to run the script:
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#
# NOTE: PowerShell reserves the -Debug common parameter, so this script uses
# -DebugBuild for the debug configuration instead.

[CmdletBinding()]
param(
  [switch]$Release,
  [switch]$DebugBuild,
  [switch]$Clean,
  [switch]$NoDeps,
  [int]$Jobs = 0,
  [string]$VcpkgRoot = $(if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path (Get-Location) 'vcpkg' }),
  [string]$Triplet    = $(if ($env:VCPKG_DEFAULT_TRIPLET) { $env:VCPKG_DEFAULT_TRIPLET } else { 'x64-windows' }),
  [switch]$Help
)

$ErrorActionPreference = 'Stop'

# ── Help ────────────────────────────────────────────────────────────────────
if ($Help) {
  Get-Help $MyInvocation.MyCommand.Path -Detailed
  exit 0
}

# Resolve build configuration. -DebugBuild overrides -Release.
if ($DebugBuild) {
  $Config = 'Debug'
} else {
  $Config = 'Release'
}

# ── Prerequisites ───────────────────────────────────────────────────────────
# Windows users hit missing-prereq failures far more often than Linux users,
# so check up front and point them at the install links rather than letting
# CMake emit a wall of cryptic errors.
function Test-Command {
  param([string]$Name)
  return [bool](Get-Command $Name -ErrorAction SilentlyContinue)
}

foreach ($tool in 'git', 'cmake') {
  if (-not (Test-Command $tool)) {
    Write-Host "ERROR: '$tool' not found on PATH." -ForegroundColor Red
    Write-Host "  Install it and add it to PATH before running .\build.ps1." -ForegroundColor Red
    Write-Host '    git   : https://git-scm.com/download/win'
    Write-Host '    cmake : https://cmake.org/download/'
    exit 1
  }
}

# Visual Studio 2022 is detected via its cmake generator presence rather than
# a registry lookup; if cmake can't use the VS generator, the configure step
# below will fail with a clear message, so we just warn here.
$vsRoots = @(
  'C:\Program Files\Microsoft Visual Studio\2022',
  'C:\Program Files (x86)\Microsoft Visual Studio\2022'
)
$vsFound = $false
foreach ($root in $vsRoots) {
  if (Test-Path $root) { $vsFound = $true; break }
}
if (-not $vsFound) {
  Write-Host 'WARNING: Visual Studio 2022 not found in the usual location.' -ForegroundColor Yellow
  Write-Host '  Install "Desktop development with C++" (VS 2022):' -ForegroundColor Yellow
  Write-Host '    https://visualstudio.microsoft.com/downloads/' -ForegroundColor Yellow
  Write-Host '  Configure will fail if the "Visual Studio 17 2022" generator is unavailable.' -ForegroundColor Yellow
}

# ── Memory-aware default job count ──────────────────────────────────────────
# Mirrors the logic in build.sh. Each compiler process can use ~1-3 GB of RAM
# on the Boost-heavy translation units in this project, so naively using all
# cores can exhaust memory and hang the machine on low-RAM laptops. Allow
# ~3 GB per job based on total physical RAM, and never exceed the CPU count.
#   8 GB  -> 2 jobs   16 GB -> 5 jobs   64 GB -> 21 jobs
function Get-DefaultJobs {
  $cpus = 4
  if ($env:NUMBER_OF_PROCESSORS -and [int]$env:NUMBER_OF_PROCESSORS -gt 0) {
    $cpus = [int]$env:NUMBER_OF_PROCESSORS
  }
  $ramGB = 0
  try {
    $ramBytes = (Get-CimInstance Win32_ComputerSystem -ErrorAction Stop).TotalPhysicalMemory
    $ramGB = [math]::Round([double]$ramBytes / 1GB)
  } catch {
    # If we can't read RAM, fall back to a conservative cpu-based default.
  }
  $jobs = if ($ramGB -ge 3) { [int]($ramGB / 3) } else { 1 }
  if ($jobs -gt $cpus) { $jobs = $cpus }
  if ($jobs -lt 1)     { $jobs = 1 }
  return $jobs
}

if ($Jobs -le 0) {
  $Jobs = Get-DefaultJobs
  Write-Host "==> No -Jobs given; using $Jobs parallel job(s) (memory-aware)."
}

# ── 1. Dependencies ─────────────────────────────────────────────────────────
if (-not $NoDeps) {
  Write-Host '==> Installing build dependencies (vcpkg manifest mode)…'
  $depsScript = Join-Path $PSScriptRoot 'scripts\setup_windows_deps.ps1'
  if (-not (Test-Path $depsScript)) {
    Write-Host "ERROR: dependency script not found: $depsScript" -ForegroundColor Red
    exit 1
  }
  & $depsScript -VcpkgRoot $VcpkgRoot -Triplet $Triplet
  if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: dependency install failed (exit $LASTEXITCODE)." -ForegroundColor Red
    exit $LASTEXITCODE
  }
}

# ── 2. Clean ────────────────────────────────────────────────────────────────
if ($Clean) {
  Write-Host '==> Removing build-win\ …'
  if (Test-Path 'build-win') {
    Remove-Item -Recurse -Force 'build-win'
  }
}

# ── 3. Configure ────────────────────────────────────────────────────────────
Write-Host "==> Configuring ($Config)…"
$toolchainFile = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path $toolchainFile)) {
  Write-Host "ERROR: vcpkg toolchain file not found: $toolchainFile" -ForegroundColor Red
  Write-Host "  Run .\build.ps1 without -NoDeps first to bootstrap vcpkg," -ForegroundColor Red
  Write-Host "  or pass -VcpkgRoot <path-to-your-vcpkg>." -ForegroundColor Red
  exit 1
}

cmake -S . -B build-win `
  -G 'Visual Studio 17 2022' -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile" `
  "-DVCPKG_TARGET_TRIPLET=$Triplet"
if ($LASTEXITCODE -ne 0) {
  Write-Host "ERROR: cmake configure failed (exit $LASTEXITCODE)." -ForegroundColor Red
  exit $LASTEXITCODE
}

# ── 4. Build ────────────────────────────────────────────────────────────────
Write-Host "==> Building $Config with $Jobs parallel job(s)…"
cmake --build build-win --config $Config --parallel $Jobs
if ($LASTEXITCODE -ne 0) {
  Write-Host "ERROR: build failed (exit $LASTEXITCODE)." -ForegroundColor Red
  exit $LASTEXITCODE
}

Write-Host ''
Write-Host 'Build complete. Run the game with:  .\run.ps1'
Write-Host "(or:  .\build-win\$Config\gameplayfootball.exe)"
