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

PRUNE="0.15"
MAX_SCALE="10.0"
SOR_K="30"
SOR_STD="2.0"
NO_SOR="false"
DEPTH="9"
DENSITY_Q="0.05"
FACES="50000"
NAV_SLOPE="45.0"

while true; do
  echo "  [1]  Đầu vào (Input)           : $SELECTED"
  echo "  [2]  Đầu ra (Output)           : $OUTPUT_FILE"
  echo "  [3]  Lọc Opacity (--prune)     : $PRUNE"
  echo "  [4]  Tỉ lệ tối đa (--max-scale): $MAX_SCALE"
  echo "  [5]  Láng giềng kNN (--sor-k)  : $SOR_K"
  echo "  [6]  Độ lệch chuẩn (--sor-std) : $SOR_STD"
  echo "  [7]  Tắt lọc SOR (--no-sor)    : $NO_SOR"
  echo "  [8]  Độ sâu Poisson (--depth)  : $DEPTH"
  echo "  [9]  Phân vị mật độ (--density): $DENSITY_Q"
  echo "  [10] Số mặt mục tiêu (--faces) : $FACES"
  echo "  [11] Góc nghiêng (--nav-slope) : $NAV_SLOPE"
  read -rp "Chọn mục để sửa [1-11, R, Q] (Mặc định: R): " CHOICE
  
  if [ -z "$CHOICE" ] || [ "$CHOICE" = "R" ] || [ "$CHOICE" = "r" ]; then
    break
  elif [ "$CHOICE" = "Q" ] || [ "$CHOICE" = "q" ]; then
    exit 0
  fi
  
  case "$CHOICE" in
    1)
      echo "Các file trong data/input/:"
      for i in "${!PLY_FILES[@]}"; do
        printf "  [%d] %s\n" $((i + 1)) "$(basename "${PLY_FILES[$i]}")"
      done
      read -rp "Chọn số file mới: " NEW_SEL
      if [[ "$NEW_SEL" =~ ^[0-9]+$ ]] && [ "$NEW_SEL" -ge 1 ] && [ "$NEW_SEL" -le ${#PLY_FILES[@]} ]; then
        SELECTED="${PLY_FILES[$((NEW_SEL - 1))]}"
        INPUT_BASENAME=$(basename "$SELECTED" .ply)
        OUTPUT_FILE="$OUTPUT_DIR/${INPUT_BASENAME}.glb"
      else
        echo "Lựa chọn không hợp lệ."
      fi
      ;;
    2)
      read -rp "Nhập đường dẫn file đầu ra mới (Mặc định: $OUTPUT_FILE): " NEW_VAL
      if [ -n "$NEW_VAL" ]; then
        OUTPUT_FILE="$NEW_VAL"
      fi
      ;;
    3)
      read -rp "Nhập ngưỡng opacity (0.0 - 1.0) [$PRUNE]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then PRUNE="$NEW_VAL"; fi
      ;;
    4)
      read -rp "Nhập tỉ lệ vật lý tối đa [$MAX_SCALE]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then MAX_SCALE="$NEW_VAL"; fi
      ;;
    5)
      read -rp "Nhập số láng giềng kNN cho SOR [$SOR_K]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then SOR_K="$NEW_VAL"; fi
      ;;
    6)
      read -rp "Nhập hệ số nhân độ lệch chuẩn cho SOR [$SOR_STD]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then SOR_STD="$NEW_VAL"; fi
      ;;
    7)
      if [ "$NO_SOR" = "true" ]; then
        NO_SOR="false"
      else
        NO_SOR="true"
      fi
      echo "Đã đổi trạng thái --no-sor thành: $NO_SOR"
      ;;
    8)
      read -rp "Nhập độ sâu cây bát phân Poisson (depth) [$DEPTH]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then DEPTH="$NEW_VAL"; fi
      ;;
    9)
      read -rp "Nhập ngưỡng phân vị mật độ (density-quantile) [$DENSITY_Q]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then DENSITY_Q="$NEW_VAL"; fi
      ;;
    10)
      read -rp "Nhập số mặt tam giác mục tiêu (faces) [$FACES]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then FACES="$NEW_VAL"; fi
      ;;
    11)
      read -rp "Nhập góc nghiêng tối đa (nav-slope) [$NAV_SLOPE]: " NEW_VAL
      if [ -n "$NEW_VAL" ]; then NAV_SLOPE="$NEW_VAL"; fi
      ;;
    *)
      echo "Lựa chọn không hợp lệ."
      ;;
  esac
done

CMD=("$BINARY" -i "$SELECTED" -o "$OUTPUT_FILE")

if [ -n "$PRUNE" ]; then
  CMD+=(-p "$PRUNE")
fi

if [ -n "$MAX_SCALE" ]; then
  CMD+=(--max-scale "$MAX_SCALE")
fi

if [ "$NO_SOR" = "true" ]; then
  CMD+=(--no-sor)
else
  if [ -n "$SOR_K" ]; then
    CMD+=(--sor-k "$SOR_K")
  fi
  if [ -n "$SOR_STD" ]; then
    CMD+=(--sor-std "$SOR_STD")
  fi
fi

if [ -n "$DEPTH" ]; then
  CMD+=(-d "$DEPTH")
fi

if [ -n "$DENSITY_Q" ]; then
  CMD+=(--density-quantile "$DENSITY_Q")
fi

if [ -n "$FACES" ]; then
  CMD+=(-f "$FACES")
fi

if [ -n "$NAV_SLOPE" ]; then
  CMD+=(--nav-slope "$NAV_SLOPE")
fi

echo ""
echo "Command chạy:"
echo "${CMD[*]}"
echo ""

read -rp "Nhấn Enter để bắt đầu chạy"
"${CMD[@]}"
