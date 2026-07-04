#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TP_DIR="$ROOT/third_party"
HAPPLY_DIR="$TP_DIR/happly"
TINY_DIR="$TP_DIR/tinygltf"
VCPKG_DIR="$ROOT/vcpkg"
OPEN3D_VERSION="0.19.0"
OPEN3D_ARCHIVE="open3d-devel-linux-x86_64-cxx11-abi-${OPEN3D_VERSION}.tar.xz"
OPEN3D_URL="https://github.com/isl-org/Open3D/releases/download/v${OPEN3D_VERSION}/${OPEN3D_ARCHIVE}"
OPEN3D_DIR="$TP_DIR/open3d-devel-linux-x86_64-cxx11-abi-${OPEN3D_VERSION}"

mkdir -p "$HAPPLY_DIR" "$TINY_DIR"


download() {
	local url="$1"
	local out="$2"

	if command -v curl >/dev/null 2>&1; then
		curl -L "$url" -o "$out"
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$out" "$url"
	else
		echo "Lỗi: cần có curl hoặc wget." >&2
		exit 1
	fi
}

ensure_vcpkg() {
	if [ -x "$VCPKG_DIR/vcpkg" ]; then
		echo "[vcpkg] Đã thấy vcpkg local ở: $VCPKG_DIR"
		return
	fi

	if [ ! -d "$VCPKG_DIR/.git" ]; then
		echo "[vcpkg] Đang clone vcpkg vào: $VCPKG_DIR"
		git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
	fi

	echo "[vcpkg] Đang bootstrap vcpkg"
	"$VCPKG_DIR/bootstrap-vcpkg.sh"
}

ensure_open3d() {
	if [ -d "$OPEN3D_DIR" ]; then
		echo "[Open3D] Đã thấy SDK ở: $OPEN3D_DIR"
		return
	fi

	local archive_path="$TP_DIR/$OPEN3D_ARCHIVE"
	echo "[Open3D] Đang tải SDK: $OPEN3D_URL"
	download "$OPEN3D_URL" "$archive_path"

	echo "[Open3D] Đang giải nén vào: $TP_DIR"
	tar -xf "$archive_path" -C "$TP_DIR"

	if [ ! -d "$OPEN3D_DIR" ]; then
		echo "[Open3D] Lỗi: giải nén xong mà không thấy thư mục mong đợi: $OPEN3D_DIR" >&2
		exit 1
	fi
}

ensure_vcpkg
download "https://raw.githubusercontent.com/nmwsharp/happly/master/happly.h" \
	"${HAPPLY_DIR}/happly.h"

download "https://raw.githubusercontent.com/syoyo/tinygltf/master/tiny_gltf.h" \
	"${TINY_DIR}/tiny_gltf.h"

download "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp" \
	"${TINY_DIR}/json.hpp"
download "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h" \
	"${TINY_DIR}/stb_image.h"
download "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h" \
	"${TINY_DIR}/stb_image_write.h"

ensure_open3d

echo "Đã xong."
echo "- vcpkg: $VCPKG_DIR"
echo "- Open3D SDK: $OPEN3D_DIR"
echo "- Thư viện header-only: $TP_DIR"
