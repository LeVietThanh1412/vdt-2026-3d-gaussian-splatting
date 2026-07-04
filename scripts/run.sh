#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────
#  Script chạy pipeline tương tác.
#  Liệt kê các file .ply trong data/input/ rồi cho chọn một file.
#  File .glb sẽ được ghi ra data/output/.
# ──────────────────────────────────────────────────────────
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="$ROOT/data/input"
OUTPUT_DIR="$ROOT/data/output"
BINARY="$ROOT/build/gsplat_ar"

# ── Check binary exists ──────────────────────────────────
if [ ! -x "$BINARY" ]; then
  echo "Không tìm thấy binary: $BINARY"
  echo "Chạy ./scripts/build.sh trước đã."
  exit 1
fi

# ── List .ply files ──────────────────────────────────────
mkdir -p "$OUTPUT_DIR"

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║        3DGS AR Pipeline — Chạy tương tác         ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""

PLY_FILES=()
while IFS= read -r -d '' f; do
  PLY_FILES+=("$f")
done < <(find "$INPUT_DIR" -maxdepth 1 -name "*.ply" -print0 2>/dev/null | sort -z)

if [ ${#PLY_FILES[@]} -eq 0 ]; then
  echo "Không thấy file .ply nào trong $INPUT_DIR"
  echo "Bỏ file .ply vào đó rồi chạy lại nhé."
  exit 1
fi

echo "Các file trong data/input/:"
echo "──────────────────────────────────────────────────────────"
for i in "${!PLY_FILES[@]}"; do
  BASENAME=$(basename "${PLY_FILES[$i]}")
  SIZE=$(du -h "${PLY_FILES[$i]}" | cut -f1)
  printf "  [%d] %-40s %s\n" $((i + 1)) "$BASENAME" "$SIZE"
done
echo "────────────────────────────────────────────────────"
echo ""

# ── User selects a file ──────────────────────────────────
read -rp "Chọn số file [1-${#PLY_FILES[@]}]: " SELECTION

if ! [[ "$SELECTION" =~ ^[0-9]+$ ]] || [ "$SELECTION" -lt 1 ] || [ "$SELECTION" -gt ${#PLY_FILES[@]} ]; then
  echo "Chọn sai rồi."
  exit 1
fi

SELECTED="${PLY_FILES[$((SELECTION - 1))]}"
INPUT_BASENAME=$(basename "$SELECTED" .ply)
OUTPUT_FILE="$OUTPUT_DIR/${INPUT_BASENAME}.glb"

echo ""
echo "Đầu vào : $SELECTED"
echo "Đầu ra  : $OUTPUT_FILE"
echo ""

# ── Pipeline parameters (with sensible defaults) ─────────
OPACITY_DEFAULT=${OPACITY:-0.15}
DEPTH_DEFAULT=${DEPTH:-9}
DENSITY_Q_DEFAULT=${DENSITY_Q:-0.05}
FACES_DEFAULT=${FACES:-50000}
NAV_SLOPE_DEFAULT=${NAV_SLOPE:-45.0}

echo "Tham số (có thể ghi đè bằng biến môi trường: OPACITY, DEPTH, DENSITY_Q, FACES, NAV_SLOPE):"
echo "Ngưỡng opacity    : $OPACITY_DEFAULT"
echo "Độ sâu Poisson    : $DEPTH_DEFAULT"
echo "Ngưỡng density    : $DENSITY_Q_DEFAULT"
echo "Số mặt mục tiêu   : $FACES_DEFAULT"
echo "Góc nav slope     : $NAV_SLOPE_DEFAULT°"
echo ""

read -r -p "Nhập 5 giá trị cách nhau bằng dấu cách (opacity depth density_q faces nav_slope), hoặc nhấn Enter để dùng mặc định: " PARAM_LINE

if [ -n "$PARAM_LINE" ]; then
  read -r OPACITY DEPTH DENSITY_Q FACES NAV_SLOPE <<< "$PARAM_LINE"
  if [ -z "${NAV_SLOPE:-}" ]; then
    echo "Cần đúng 5 giá trị: opacity depth density_q faces nav_slope" >&2
    exit 1
  fi
else
  OPACITY="$OPACITY_DEFAULT"
  DEPTH="$DEPTH_DEFAULT"
  DENSITY_Q="$DENSITY_Q_DEFAULT"
  FACES="$FACES_DEFAULT"
  NAV_SLOPE="$NAV_SLOPE_DEFAULT"
fi

echo ""
echo "Đang dùng các tham số:"
echo "Ngưỡng opacity    : $OPACITY"
echo "Độ sâu Poisson    : $DEPTH"
echo "Ngưỡng density    : $DENSITY_Q"
echo "Số mặt mục tiêu   : $FACES"
echo "Góc nav slope     : $NAV_SLOPE°"
echo ""

read -rp "Nhấn Enter để chạy pipeline (hoặc Ctrl+C để hủy)... "

# ── Run pipeline ─────────────────────────────────────────
"$BINARY" \
  -i "$SELECTED" \
  -o "$OUTPUT_FILE" \
  -p "$OPACITY" \
  -d "$DEPTH" \
  --density-quantile "$DENSITY_Q" \
  -f "$FACES" \
  --nav-slope "$NAV_SLOPE"
