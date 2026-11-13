#!/bin/bash
# This file was generated with the assistance of an AI coding tool.
#
# Several files at once through bonsaiviewer_mass_convert_bench (biggest first,
# shared queue, no RAM check: wrap it in a capped scope, see README.md).
#
#   run_parallel.sh <ifc_dir> <log> <ifcview|rdb> <workers> [threads per .ifcview file]
#
# BENCH overrides the bench path (default: next to BonsaiViewer in ../BUILD).
set -u
DIR="$1"; LOG="$2"; MODE="$3"; W="$4"; T="${5:-4}"
export BENCH="${BENCH:-$HOME/Documents/05 - Programmation/BUILD/bonsaiviewer/bonsaiviewer_mass_convert_bench}"
export MODE T
: > "$LOG"
start=$(date +%s.%N)
ls -1S "$DIR"/*.ifc | tr '\n' '\0' | xargs -0 -P "$W" -I{} sh -c '
  if [ "$MODE" = ifcview ]; then set -- "$1" "$T" 9; else set -- "$1"; fi
  "$BENCH" "$MODE" "$@" 2>/dev/null | grep RESULT || echo "RESULT file=$(basename "$1") ok=0 error=killed"
' _ {} >> "$LOG"
echo "WALL $(echo "$(date +%s.%N) - $start" | bc) s" >> "$LOG"
