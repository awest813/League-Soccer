# scripts/setup_windows_deps.ps1 – bootstrap vcpkg and install Windows deps.
#
# Prerequisites: Visual Studio 2022 (C++ workload), Git, CMake on PATH.
#
# Usage (from repo root, PowerShell):
#   .\scripts\setup_windows_deps.ps1
#   .\scripts\setup_windows_deps.ps1 -VcpkgRoot C:\dev\vcpkg -Triplet x64-windows
#
# Prefer repo-root vcpkg.json (manifest mode). This script still works for
# classic installs when CMAKE_TOOLCHAIN_FILE points at the same vcpkg tree.

[CmdletBinding()]
param(
  [string]$VcpkgRoot = $(if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path (Get-Location) "vcpkg" }),
  [string]$Triplet = $(if ($env:VCPKG_DEFAULT_TRIPLET) { $env:VCPKG_DEFAULT_TRIPLET } else { "x64-windows" })
)

$ErrorActionPreference = "Stop"

Write-Host "Using VCPKG_ROOT=$VcpkgRoot triplet=$Triplet"

if (-not (Test-Path (Join-Path $VcpkgRoot "vcpkg.exe"))) {
  if (-not (Test-Path $VcpkgRoot)) {
    git clone https://github.com/microsoft/vcpkg $VcpkgRoot
  }
  & (Join-Path $VcpkgRoot "bootstrap-vcpkg.bat") -disableMetrics
}

$env:VCPKG_ROOT = $VcpkgRoot
$env:VCPKG_DEFAULT_TRIPLET = $Triplet

$manifest = Join-Path (Get-Location) "vcpkg.json"
if (Test-Path $manifest) {
  Write-Host "Manifest mode: installing from vcpkg.json"
  & (Join-Path $VcpkgRoot "vcpkg.exe") install --triplet $Triplet
} else {
  & (Join-Path $VcpkgRoot "vcpkg.exe") install --triplet $Triplet `
    boost-signals2 boost-smart-ptr `
    sdl2 "sdl2-image[libjpeg-turbo]" sdl2-ttf sdl2-gfx `
    opengl openal-soft sqlite3
}

Write-Host ""
Write-Host "Configure example:"
Write-Host "  cmake -S . -B build-win -G `"Visual Studio 17 2022`" -A x64 ``"
Write-Host "    -DCMAKE_TOOLCHAIN_FILE=`"$VcpkgRoot\scripts\buildsystems\vcpkg.cmake`" ``"
Write-Host "    -DVCPKG_TARGET_TRIPLET=`"$Triplet`""
