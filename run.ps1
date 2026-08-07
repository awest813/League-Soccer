# run.ps1 – launch the built game, making sure its runtime assets are found.
#
# The game reads media/, databases/, locale/ and football.config relative to
# its working directory. CMake copies them next to the binary, so we launch
# from build-win\<Config>\ (the multi-config VS generator nests it).
#
# Mirrors ./run.sh on Linux.
#
# Usage:
#   .\run.ps1 [options] [-- extra args passed to the game]
#
# Options:
#   -DebugBuild  Run the Debug binary (build-win\Debug\gameplayfootball.exe)
#   -Help        Show this message
#
# First run only, if PowerShell refuses to run the script:
#   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#
# NOTE: PowerShell reserves the -Debug common parameter, so this script uses
# -DebugBuild to select the debug binary instead.

[CmdletBinding()]
param(
  [switch]$DebugBuild,
  [switch]$Help
)

$ErrorActionPreference = 'Stop'

if ($Help) {
  Get-Help $MyInvocation.MyCommand.Path -Detailed
  exit 0
}

# Parse a "--" separator: args before it are ours, after it are forwarded to
# the game (matches run.sh's `--` handling).
$GameArgs = @()
$splitSeen = $false
foreach ($a in $args) {
  if (-not $splitSeen -and $a -eq '--') { $splitSeen = $true; continue }
  if ($splitSeen) { $GameArgs += $a }
}

# Build the candidate list. If -DebugBuild is given, prefer the Debug binary;
# otherwise prefer Release (then fall back to Debug if only that exists).
$scriptDir = $PSScriptRoot
if (-not $scriptDir) { $scriptDir = (Get-Location).Path }

$candidates = @()
if ($DebugBuild) {
  $candidates += (Join-Path $scriptDir 'build-win\Debug\gameplayfootball.exe')
  $candidates += (Join-Path $scriptDir 'build-win\Release\gameplayfootball.exe')
} else {
  $candidates += (Join-Path $scriptDir 'build-win\Release\gameplayfootball.exe')
  $candidates += (Join-Path $scriptDir 'build-win\Debug\gameplayfootball.exe')
}

$exe = $null
foreach ($cand in $candidates) {
  if (Test-Path $cand) { $exe = $cand; break }
}

if (-not $exe) {
  Write-Host 'Game executable not found under build-win\.' -ForegroundColor Red
  Write-Host 'Build it first with:  .\build.ps1' -ForegroundColor Red
  exit 1
}

$runDir = Split-Path -Parent $exe
Write-Host "Launching $exe from $runDir"
Set-Location $runDir

# Invoke directly so the game gets the console for any stdout/stderr it prints.
& '.\gameplayfootball.exe' @GameArgs
exit $LASTEXITCODE
