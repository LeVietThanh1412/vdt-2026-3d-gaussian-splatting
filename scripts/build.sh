#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root_dir}/build"

# ── NTFS Workaround: redirect build dir to native partition to avoid driver deadlocks ──
FS_TYPE=$(df -T "$root_dir" | tail -n 1 | awk '{print $2}')
if [[ "$FS_TYPE" == *"ntfs"* ]] || [[ "$FS_TYPE" == "fuseblk" ]]; then
  echo "Project is on an NTFS partition ($FS_TYPE)."
  echo "The Linux ntfs3 driver often hangs/deadlocks under parallel C++ writes."
  echo "Automatically redirecting build files to native partition: /home/archlinux/.cache/3dgs_ar_build"
  build_dir="/home/archlinux/.cache/3dgs_ar_build"
  mkdir -p "$build_dir"
fi

# ── Resolve Open3D SDK path ──────────────────────────────
if [ -z "${OPEN3D_ROOT:-}" ]; then
  # Default path for Arch Linux manual install
  OPEN3D_ROOT="$HOME/open3d-devel-linux-x86_64-cxx11-abi-0.19.0"
fi

if [ ! -d "$OPEN3D_ROOT" ]; then
  echo " Open3D SDK not found at $OPEN3D_ROOT"
  echo "Set OPEN3D_ROOT env var or download from:"
  echo "  https://github.com/isl-org/Open3D/releases"
  exit 1
fi

# Determine build threads based on available memory to avoid OOM hang/freeze
# Open3D compilation needs about 3-4GB of RAM per thread.
TOTAL_MEM_GB=8
if [ -f /proc/meminfo ]; then
  TOTAL_MEM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
  TOTAL_MEM_GB=$((TOTAL_MEM_KB / 1024 / 1024))
fi

# Allow setting NUM_JOBS manually, otherwise calculate a safe default
if [ -z "${NUM_JOBS:-}" ]; then
  # Safe default: 1 thread per 4GB of RAM, capped at number of CPU cores
  CPU_CORES=$(nproc)
  SAFE_JOBS=$((TOTAL_MEM_GB / 4))
  if [ $SAFE_JOBS -lt 1 ]; then SAFE_JOBS=1; fi
  if [ $SAFE_JOBS -gt "$CPU_CORES" ]; then SAFE_JOBS=$CPU_CORES; fi
  NUM_JOBS=$SAFE_JOBS
fi

echo "=== 3DGS AR Pipeline Build ==="
echo "  Open3D: $OPEN3D_ROOT"
echo "  Build:  $build_dir"
echo "  Memory: ${TOTAL_MEM_GB}GB"
echo "  Jobs:   $NUM_JOBS (to prevent Out-Of-Memory hang)"
echo ""

cmake_args=(
  -S "$root_dir"
  -B "$build_dir"
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if [ -n "${VCPKG_ROOT:-}" ]; then
  cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
fi

echo "[1/2] Configuring..."
cmake "${cmake_args[@]}"

echo ""
echo "[2/2] Building..."
cmake --build "$build_dir" --config Release -j"$NUM_JOBS"

echo ""
echo "=== Build complete! ==="
echo "  Binary: ${build_dir}/gsplat_ar"

# Try to copy/link the binary to project root build directory for tool/VS Code compatibility
local_build_dir="${root_dir}/build"
if [ "$build_dir" != "$local_build_dir" ]; then
  mkdir -p "$local_build_dir" 2>/dev/null || true
  cp "${build_dir}/gsplat_ar" "${local_build_dir}/gsplat_ar" 2>/dev/null || true
  if [ -f "${local_build_dir}/gsplat_ar" ]; then
    echo "  Copied binary to workspace: ${local_build_dir}/gsplat_ar"
  fi
fi

echo ""
echo "Usage:"
echo "  ${build_dir}/gsplat_ar -i data/input/scene.ply -o data/output/scene.glb"
