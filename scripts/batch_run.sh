#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="$ROOT/build/gsplat_ar"
INPUT_DIR="$ROOT/data/input"
OUTPUT_DIR="$ROOT/data/output"
LOG_DIR="$ROOT/data/logs"

# Load tham số từ file config
if [ -f "$ROOT/scripts/params.conf" ]; then
    source "$ROOT/scripts/params.conf"
    echo "--- Đã load tham số từ params.conf ---"
else
    echo "Lỗi: Không tìm thấy file params.conf"
    exit 1
fi

mkdir -p "$OUTPUT_DIR" "$LOG_DIR"

echo "=== Bắt đầu xử lý Batch cho $(ls -1 "$INPUT_DIR"/*.ply | wc -l) file ==="

for f in "$INPUT_DIR"/*.ply; do
    [ -e "$f" ] || continue
    BASENAME=$(basename "$f" .ply)
    
    echo "Đang xử lý: $BASENAME..."
    
    # Chạy pipeline với tham số từ params.conf
    # Chú ý: $BINARY được lấy từ biến trên
    "$BINARY" -i "$f" -o "$OUTPUT_DIR/${BASENAME}.glb" \
        -p "${OPACITY:-0.1}" \
        -d "${DEPTH:-10}" \
        --density-quantile "${DENSITY_Q:-0.05}" \
        -f "${FACES:-50000}" \
        --nav-slope "${NAV_SLOPE:-45.0}" > "$LOG_DIR/${BASENAME}.log" 2>&1
    
    if [ $? -eq 0 ]; then
        echo "  [OK] Đã xong: ${BASENAME}.glb"
    else
        echo "  [LỖI] Xem log tại: $LOG_DIR/${BASENAME}.log"
    fi
done
