# scripts/package_windows.ps1 – assemble a runnable Windows release folder.
#
# Copies gameplayfootball.exe, runtime DLLs from vcpkg, and game assets into
# an output directory suitable for CI artifacts or local distribution.
#
# Usage:
#   .\scripts\package_windows.ps1 `
#     -BuildDir build-win\Release `
#     -OutDir dist\windows-x64 `
#     -VcpkgRoot $env:VCPKG_ROOT `
#     -Triplet x64-windows

[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string]$BuildDir,
  [Parameter(Mandatory = $true)][string]$OutDir,
  [string]$VcpkgRoot = $(if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { "vcpkg" }),
  [string]$Triplet = $(if ($env:VCPKG_DEFAULT_TRIPLET) { $env:VCPKG_DEFAULT_TRIPLET } else { "x64-windows" }),
  [string]$RepoRoot = (Get-Location)
)

$ErrorActionPreference = "Stop"

$exe = Join-Path $BuildDir "gameplayfootball.exe"
if (-not (Test-Path $exe)) {
  throw "Executable not found: $exe"
}

if (Test-Path $OutDir) {
  Remove-Item -Recurse -Force $OutDir
}
New-Item -ItemType Directory -Path $OutDir | Out-Null

Copy-Item $exe $OutDir

# Prefer DLLs already next to the exe (CMake TARGET_RUNTIME_DLLS / local runs).
Get-ChildItem -Path $BuildDir -Filter *.dll -ErrorAction SilentlyContinue |
  ForEach-Object { Copy-Item $_.FullName $OutDir -Force }

# Also pull shared libs from vcpkg install trees (classic + manifest layouts).
$binCandidates = @(
  (Join-Path $VcpkgRoot "installed\$Triplet\bin"),
  (Join-Path $RepoRoot "vcpkg_installed\$Triplet\bin"),
  (Join-Path (Split-Path $BuildDir -Parent) "vcpkg_installed\$Triplet\bin")
)
$needed = @(
  "SDL2.dll", "SDL2_image.dll", "SDL2_ttf.dll", "SDL2_gfx.dll",
  "OpenAL32.dll", "sqlite3.dll", "libpng16.dll", "zlib1.dll",
  "jpeg62.dll", "libjpeg-turbo*.dll", "turbojpeg.dll",
  "brotlicommon.dll", "brotlidec.dll", "bz2.dll", "freetype.dll",
  "liblzma.dll"
)
foreach ($vcpkgBin in $binCandidates) {
  if (-not (Test-Path $vcpkgBin)) { continue }
  foreach ($pattern in $needed) {
    Get-ChildItem -Path $vcpkgBin -Filter $pattern -ErrorAction SilentlyContinue |
      ForEach-Object { Copy-Item $_.FullName $OutDir -Force }
  }
}

$dataSrc = Join-Path $RepoRoot "data"
Copy-Item (Join-Path $dataSrc "football.config") $OutDir -Force
Copy-Item (Join-Path $dataSrc "media") (Join-Path $OutDir "media") -Recurse -Force
Copy-Item (Join-Path $dataSrc "databases") (Join-Path $OutDir "databases") -Recurse -Force
Copy-Item (Join-Path $dataSrc "locale") (Join-Path $OutDir "locale") -Recurse -Force

New-Item -ItemType Directory -Path (Join-Path $OutDir "data") -Force | Out-Null
Copy-Item (Join-Path $dataSrc "football.config") (Join-Path $OutDir "data\football.config") -Force
Copy-Item (Join-Path $dataSrc "locale") (Join-Path $OutDir "data\locale") -Recurse -Force

$dllCount = (Get-ChildItem $OutDir -Filter *.dll | Measure-Object).Count
Write-Host "Packaged Windows build into $OutDir ($dllCount DLLs + assets)."
