#!/bin/bash
# This file was generated with the assistance of an AI coding tool.
#
# One file at a time through bonsaiviewer_mass_convert_bench, smallest first,
# each run in its own memory-capped scope (a file over the cap is reported as
# killed instead of making the machine swap).
#
#   run_series.sh <ifc_dir> <log> <ifcview|rdb> <mem_cap e.g. 4G> [bench args: threads|auto zstd]
#
# BENCH overrides the bench path (default: next to BonsaiViewer in ../BUILD).
set -u
DIR="$1"; LOG="$2"; MODE="$3"; CAP="$4"; shift 4
BENCH="${BENCH:-$HOME/Documents/05 - Programmation/BUILD/bonsaiviewer/bonsaiviewer_mass_convert_bench}"
: > "$LOG"
ls -1S --reverse "$DIR"/*.ifc | while read -r f; do
  start=$(date +%s)
  out=$(systemd-run --user --scope -q -p MemoryMax="$CAP" -p MemorySwapMax=0 nice -n 10 \
          "$BENCH" "$MODE" "$f" "$@" 2>/dev/null | grep RESULT)
  if [ -z "$out" ]; then
    out="RESULT kind=$MODE file=$(basename "$f") mib=$(( $(stat -c %s "$f") / 1048576 ))"
    out="$out total=$(( ($(date +%s) - start) * 1000 )) ok=0 error=\"killed (over $CAP?)\""
  fi
  echo "$out" >> "$LOG"
done
echo DONE >> "$LOG"
