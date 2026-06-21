#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_DIR="$ROOT/third_party/vcpkg"

if [ ! -d "$VCPKG_DIR/.git" ]; then
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

"$VCPKG_DIR/bootstrap-vcpkg.sh"
"$VCPKG_DIR/vcpkg" install open3d cli11

echo "Done. vcpkg is in third_party/vcpkg."
