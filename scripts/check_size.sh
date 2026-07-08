#!/usr/bin/env bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="$ROOT/data/input"
OUTPUT_DIR="$ROOT/data/output"

for f in "$INPUT_DIR"/*.ply; do
    [ -e "$f" ] || continue
    BASENAME=$(basename "$f" .ply)
    
    INPUT_SIZE=$(du -sh "$f" | cut -f1)
    
    OUT_FILE="$OUTPUT_DIR/${BASENAME}.glb"
    if [ -f "$OUT_FILE" ]; then
        OUTPUT_SIZE=$(du -sh "$OUT_FILE" | cut -f1)
    else
        OUTPUT_SIZE="---"
    fi
    
    printf "  %-22s %-19s %-18s \n" "$BASENAME" "$INPUT_SIZE" "$OUTPUT_SIZE"
done