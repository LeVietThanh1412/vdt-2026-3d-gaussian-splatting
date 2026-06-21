# ──────────────────────────────────────────────────────────
#  Multi-stage build for the 3DGS AR Pipeline CLI tool
# ──────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip tar pkg-config \
    libeigen3-dev libfmt-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source
COPY . .

# Fetch header-only third-party libs
RUN bash scripts/fetch_third_party.sh

# Download and extract Open3D pre-built SDK
ARG OPEN3D_VERSION=0.19.0
RUN curl -L "https://github.com/isl-org/Open3D/releases/download/v${OPEN3D_VERSION}/open3d-devel-linux-x86_64-cxx11-abi-${OPEN3D_VERSION}.tar.xz" \
    -o /tmp/open3d.tar.xz && \
    tar -xf /tmp/open3d.tar.xz -C /opt && \
    rm /tmp/open3d.tar.xz

ENV OPEN3D_ROOT=/opt/open3d-devel-linux-x86_64-cxx11-abi-${OPEN3D_VERSION}

# Install CLI11 via vcpkg or system
RUN apt-get update && apt-get install -y libcli11-dev || true

ARG NUM_JOBS=2
# Build the project
RUN mkdir -p build && \
    cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_STANDARD=17 && \
    cmake --build build --config Release -j${NUM_JOBS}

# ── Runtime stage ─────────────────────────────────────────
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/gsplat_ar /app/build/gsplat_ar
COPY --from=builder /opt/open3d-devel-linux-x86_64-cxx11-abi-0.19.0/lib /opt/open3d/lib

ENV LD_LIBRARY_PATH=/opt/open3d/lib:$LD_LIBRARY_PATH

WORKDIR /app
ENTRYPOINT ["/app/build/gsplat_ar"]
