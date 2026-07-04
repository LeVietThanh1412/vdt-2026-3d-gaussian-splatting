#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root_dir}/build"

# ── Tìm đường dẫn vcpkg ──────────────────────────────────
if [ -z "${VCPKG_ROOT:-}" ]; then
  if [ -d "${root_dir}/vcpkg" ] && [ -x "${root_dir}/vcpkg/vcpkg" ]; then
    VCPKG_ROOT="${root_dir}/vcpkg"
  elif [ -d "${HOME}/vcpkg" ] && [ -x "${HOME}/vcpkg/vcpkg" ]; then
    VCPKG_ROOT="${HOME}/vcpkg"
  fi
fi

if [ -n "${VCPKG_ROOT:-}" ] && [ ! -x "${VCPKG_ROOT}/vcpkg" ]; then
  echo "Không tìm thấy vcpkg ở: ${VCPKG_ROOT}/vcpkg" >&2
  echo "Hãy trỏ VCPKG_ROOT tới thư mục có file vcpkg." >&2
  exit 1
fi

# ── Tìm đường dẫn Open3D SDK ─────────────────────────────
if [ -z "${OPEN3D_ROOT:-}" ]; then
  if [ -d "${root_dir}/third_party/open3d-devel-linux-x86_64-cxx11-abi-0.19.0" ]; then
    OPEN3D_ROOT="${root_dir}/third_party/open3d-devel-linux-x86_64-cxx11-abi-0.19.0"
  elif [ -d "${HOME}/open3d-devel-linux-x86_64-cxx11-abi-0.19.0" ]; then
    OPEN3D_ROOT="${HOME}/open3d-devel-linux-x86_64-cxx11-abi-0.19.0"
  fi
fi

if [ -z "${OPEN3D_ROOT:-}" ] || [ ! -d "$OPEN3D_ROOT" ]; then
  echo "Không tìm thấy Open3D SDK." >&2
  echo "Đường dẫn mong đợi là:" >&2
  echo "  - ${root_dir}/third_party/open3d-devel-linux-x86_64-cxx11-abi-0.19.0" >&2
  echo "  - ${HOME}/open3d-devel-linux-x86_64-cxx11-abi-0.19.0" >&2
  echo "  - hoặc đường dẫn tự đặt qua OPEN3D_ROOT" >&2
  exit 1
fi

# ── Né lỗi NTFS: chuyển build sang phân vùng native nếu cần ──
FS_TYPE=$(df -T "$root_dir" | tail -n 1 | awk '{print $2}')
if [[ "$FS_TYPE" == *"ntfs"* ]] || [[ "$FS_TYPE" == "fuseblk" ]]; then
  echo "Project đang nằm trên phân vùng NTFS ($FS_TYPE)."
  echo "Driver ntfs3 trên Linux hay bị treo khi build song song."
  echo "Tự chuyển thư mục build sang phân vùng native: /home/archlinux/.cache/3dgs_ar_build"
  build_dir="/home/archlinux/.cache/3dgs_ar_build"
  mkdir -p "$build_dir"
fi

# Tính số luồng build theo RAM để đỡ bị OOM.
# Open3D thường ngốn khoảng 3-4GB RAM cho mỗi luồng.
TOTAL_MEM_GB=8
if [ -f /proc/meminfo ]; then
  TOTAL_MEM_KB=$(grep MemTotal /proc/meminfo | awk '{print $2}')
  TOTAL_MEM_GB=$((TOTAL_MEM_KB / 1024 / 1024))
fi

# Có thể tự đặt NUM_JOBS, không thì script sẽ tự tính mức an toàn.
if [ -z "${NUM_JOBS:-}" ]; then
  CPU_CORES=$(nproc)
  SAFE_JOBS=$((TOTAL_MEM_GB / 4))
  if [ $SAFE_JOBS -lt 1 ]; then SAFE_JOBS=1; fi
  if [ $SAFE_JOBS -gt "$CPU_CORES" ]; then SAFE_JOBS=$CPU_CORES; fi
  NUM_JOBS=$SAFE_JOBS
fi

echo "=== Build 3DGS AR Pipeline ==="
echo "  vcpkg:  ${VCPKG_ROOT:-<chưa đặt>}"
echo "  Open3D: $OPEN3D_ROOT"
echo "  Thư mục build: $build_dir"
echo "  RAM:          ${TOTAL_MEM_GB}GB"
echo "  Luồng:        $NUM_JOBS (để tránh OOM)"
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

cmake_args+=("-DOPEN3D_ROOT=${OPEN3D_ROOT}")

echo "[1/2] Đang cấu hình..."
cmake "${cmake_args[@]}"

echo ""
echo "[2/2] Đang build..."
cmake --build "$build_dir" --config Release -j"$NUM_JOBS"

echo ""
echo "=== Build xong! ==="
echo "  File chạy: ${build_dir}/gsplat_ar"

# Cố gắng copy binary về build/ ở root để VS Code và tool khác dễ trỏ tới.
local_build_dir="${root_dir}/build"
if [ "$build_dir" != "$local_build_dir" ]; then
  mkdir -p "$local_build_dir" 2>/dev/null || true
  cp "${build_dir}/gsplat_ar" "${local_build_dir}/gsplat_ar" 2>/dev/null || true
  if [ -f "${local_build_dir}/gsplat_ar" ]; then
    echo "  Đã copy binary về workspace: ${local_build_dir}/gsplat_ar"
  fi
fi

echo ""
echo "Cách chạy:"
echo "  ${build_dir}/gsplat_ar -i data/input/scene.ply -o data/output/scene.glb"
