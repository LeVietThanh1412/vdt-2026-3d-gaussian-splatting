#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────
#  Interactive pipeline runner.
#  Lists all .ply files in data/input/ and lets the user
#  pick one. Output .glb is written to data/output/.
# ──────────────────────────────────────────────────────────
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="$ROOT/data/input"
OUTPUT_DIR="$ROOT/data/output"
BINARY="$ROOT/build/gsplat_ar"

# ── Check binary exists ──────────────────────────────────
if [ ! -x "$BINARY" ]; then
  echo "Binary not found: $BINARY"
  echo "Run ./scripts/build.sh first."
  exit 1
fi

# ── List .ply files ──────────────────────────────────────
mkdir -p "$OUTPUT_DIR"

echo ""
echo "╔══════════════════════════════════════════════════╗"
echo "║       3DGS AR Pipeline — Interactive Runner     ║"
echo "╚══════════════════════════════════════════════════╝"
echo ""

PLY_FILES=()
while IFS= read -r -d '' f; do
  PLY_FILES+=("$f")
done < <(find "$INPUT_DIR" -maxdepth 1 -name "*.ply" -print0 2>/dev/null | sort -z)

if [ ${#PLY_FILES[@]} -eq 0 ]; then
  echo "No .ply files found in $INPUT_DIR"
  echo "Place your .ply file(s) there and try again."
  exit 1
fi

echo "Files in data/input/:"
echo "────────────────────────────────────────────────────"
for i in "${!PLY_FILES[@]}"; do
  BASENAME=$(basename "${PLY_FILES[$i]}")
  SIZE=$(du -h "${PLY_FILES[$i]}" | cut -f1)
  printf "  [%d] %-40s %s\n" $((i + 1)) "$BASENAME" "$SIZE"
done
echo "────────────────────────────────────────────────────"
echo ""

# ── User selects a file ──────────────────────────────────
read -rp "Select file number [1-${#PLY_FILES[@]}]: " SELECTION

if ! [[ "$SELECTION" =~ ^[0-9]+$ ]] || [ "$SELECTION" -lt 1 ] || [ "$SELECTION" -gt ${#PLY_FILES[@]} ]; then
  echo "Invalid selection."
  exit 1
fi

SELECTED="${PLY_FILES[$((SELECTION - 1))]}"
INPUT_BASENAME=$(basename "$SELECTED" .ply)
OUTPUT_FILE="$OUTPUT_DIR/${INPUT_BASENAME}.glb"

echo ""
echo "Input:  $SELECTED"
echo "Output: $OUTPUT_FILE"
echo ""

# ── Pipeline parameters (with sensible defaults) ─────────
OPACITY=${OPACITY:-0.15}
DEPTH=${DEPTH:-8}
DENSITY_Q=${DENSITY_Q:-0.05}
FACES=${FACES:-50000}
NAV_SLOPE=${NAV_SLOPE:-45.0}

echo "Parameters (override via env vars: OPACITY, DEPTH, DENSITY_Q, FACES, NAV_SLOPE):"
echo "Opacity Threshold : $OPACITY"
echo "Poisson Depth     : $DEPTH"
echo "Density Quantile  : $DENSITY_Q"
echo "Target Faces      : $FACES"
echo "Nav Slope         : $NAV_SLOPE°"
echo ""

read -rp "Press Enter to run pipeline (or Ctrl+C to cancel)... "

# ── Run pipeline ─────────────────────────────────────────
"$BINARY" \
  -i "$SELECTED" \
  -o "$OUTPUT_FILE" \
  -p "$OPACITY" \
  -d "$DEPTH" \
  --density-quantile "$DENSITY_Q" \
  -f "$FACES" \
  --nav-slope "$NAV_SLOPE"

echo ""
echo "════════════════════════════════════════════════════"
echo "Done! Output saved to:"
echo "$OUTPUT_FILE"
echo ""
echo "To view in browser:"
echo "python3 web_demo/serve.py"
echo "Open http://localhost:8000"
echo "════════════════════════════════════════════════════"
