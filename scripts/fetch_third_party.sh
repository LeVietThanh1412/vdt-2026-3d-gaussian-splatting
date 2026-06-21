#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TP_DIR="$ROOT/third_party"
HAPPLY_DIR="$TP_DIR/happly"
TINY_DIR="$TP_DIR/tinygltf"

mkdir -p "$HAPPLY_DIR" "$TINY_DIR"


download() {
	local url="$1"
	local out="$2"

	if command -v curl >/dev/null 2>&1; then
		curl -L "$url" -o "$out"
	elif command -v wget >/dev/null 2>&1; then
		wget -O "$out" "$url"
	else
		echo "Error: curl or wget is required." >&2
		exit 1
	fi
}

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

echo "Done. Files are in third_party/."
