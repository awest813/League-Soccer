# League-Soccer / Gameplay Football – development container
#
# Build:   docker build -t gameplayfootball-dev .
# Or use:  docker compose up
#
# The container provides every dependency needed to:
#   • compile and test the project (including headless tests)
#   • run the game on a host with X11 forwarding or Xvfb
#
# ─── Stage 1: base dependencies ──────────────────────────────────────────────
FROM ubuntu:22.04 AS base

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update -qq && apt-get install -y --no-install-recommends \
    # Build toolchain
    build-essential \
    cmake \
    ninja-build \
    git \
    # Game runtime libraries
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libsdl2-dev \
    libsdl2-image-dev \
    libsdl2-ttf-dev \
    libsdl2-gfx-dev \
    libopenal-dev \
    libboost-system-dev \
    libboost-signals2-dev \
    libsqlite3-dev \
    # Documentation
    doxygen \
    graphviz \
    # Headless display for running the game inside the container
    xvfb \
    # Static analysis and formatting
    clang \
    clang-format \
    clang-tidy \
    # Utilities
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# ─── Stage 2: developer image ────────────────────────────────────────────────
FROM base AS dev

WORKDIR /workspace

# Copy project source
COPY . .

# Set up data assets so the executable can find them inside the container
RUN bash scripts/setup_assets.sh --build-dir /workspace/build 2>/dev/null || true

# Pre-configure a Debug build (useful for IDE attach / container-based dev loops)
RUN cmake -S . -B build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DBUILD_TESTING=ON \
      -DGAMEPLAYFOOTBALL_BUILD_GAME=ON

# Build the project (cached layer – re-run `docker build` to update)
RUN cmake --build build --parallel "$(nproc)"

# Default command: run the test suite inside Xvfb then drop to a shell
CMD ["bash", "-c", \
     "Xvfb :99 -screen 0 1280x720x24 &> /tmp/xvfb.log & \
      export DISPLAY=:99 && \
      ctest --test-dir build --output-on-failure && \
      bash"]
