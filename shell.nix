# NixOS / Nix dev-shell for League-Soccer / Gameplay Football.
#
# Nix is declarative, so rather than running scripts/setup_linux_deps.sh
# (which installs system packages), enter a dev environment that provides
# every dependency for building, running, and testing:
#
#     nix-shell            # classic entry (this file)
#     nix develop          # flake-style entry (with `nix flake init` or a
#                          #   flake.nix that imports this as a devShell)
#
# then configure/build as usual:
#
#     cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build --parallel
#     ctest --test-dir build --output-on-failure

{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    ninja
    gcc
    gnumake
    pkg-config
    # Build + runtime libraries
    SDL2
    SDL2_image
    SDL2_ttf
    SDL2_gfx
    openal
    mesa
    sqlite
    boost
    # Headless display for tests (no DISPLAY needed)
    xorg.xvfb
    # Tools (clang-format / clang-tidy / doxygen / graphviz)
    clang-tools
    llvm
    doxygen
    graphviz
  ];

  shellHook = ''
    echo "League-Soccer dev shell. Build with:  ./build.sh --no-deps"
  '';
}
