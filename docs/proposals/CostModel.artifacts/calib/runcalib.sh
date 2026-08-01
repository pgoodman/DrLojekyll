#!/usr/bin/env bash
# Copyright 2026, Peter Goodman. All rights reserved.
#
# Reproduces the identity-join recognizer's runtime cost from tracked source
# (cost-model-findings.md #7; measured-calibration-1.md). It:
#   1. regenerates the recognizer-ON and recognizer-OFF codegen for the mono
#      demand witness from tracked flags;
#   2. compiles the parametric calib_driver.cpp against each,
#      `-O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS` (a COUNTS binary, never a timed
#      one);
#   3. runs a scenario sweep and, for each (N,F,K,DUP), prints the ON/OFF probe-
#      epoch counter deltas and CHECKS the exact law  idx_adds(OFF) -
#      idx_adds(ON) == F*K  by integer equality;
#   4. runs the negative / held-out / sweep scenarios that keep the law honest.
#
# Exit non-zero on any law violation or build error. This is a MEASUREMENT
# harness: it observes gBenchCounters. It does not statically predict them.
#
# Usage:  DR=build/debug/bin/drlojekyll ./runcalib.sh
#         (DR defaults to <repo>/build/debug/bin/drlojekyll)

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$here/../../../.." && pwd)"
DR="${DR:-$repo/build/debug/bin/drlojekyll}"
CASE="$repo/tests/OptDiff/cases/demand_neighborhood_mono_witness.dr"
CXX="${CXX:-clang++}"

if [ ! -x "$DR" ]; then
  echo "compiler not found/executable: $DR (build: cmake --build --preset debug)" >&2
  exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/gen_on" "$work/gen_off"

echo "== regenerating ON/OFF codegen =="
"$DR" "$CASE" -demand -cpp-out "$work/gen_on/"
"$DR" "$CASE" -demand -opt-disable=df.ident_join -cpp-out "$work/gen_off/"

COMMON="-std=c++23 -O2 -DNDEBUG -DDRLOJEKYLL_BENCH_COUNTERS -I $repo/include"
echo "== compiling calib_driver against each config =="
$CXX $COMMON -DCALIB_CONFIG='"on"'  -I "$work/gen_on" \
     "$here/calib_driver.cpp" "$work/gen_on/datalog.cpp" \
     "$repo/lib/Runtime/Allocator.cpp" -o "$work/calib_on"
$CXX $COMMON -DCALIB_CONFIG='"off"' -I "$work/gen_off" \
     "$here/calib_driver.cpp" "$work/gen_off/datalog.cpp" \
     "$repo/lib/Runtime/Allocator.cpp" -o "$work/calib_off"

# counter <file> <field>  ->  the delta value printed for that field.
counter() { grep "^COUNTER $2 " "$1" | awk '{print $3}'; }

fail=0

# min a b
minv() { if [ "$1" -le "$2" ]; then echo "$1"; else echo "$2"; fi; }

# scenario NAME N F K DUP REPEAT EXPECT_LAW(1|0)
# The exact law is  idx_adds(OFF) - idx_adds(ON) == F * min(N,K)  over the
# POPULATED probes (keys < N materialize F rows each; keys >= N stand up an
# empty demand and materialize nothing). REPEAT>1 must NOT change the delta
# (demand idempotence). expect_law=0 marks a scenario shown for its boundary
# behaviour rather than checked.
scenario() {
  local name="$1" N="$2" F="$3" K="$4" DUP="$5" REPEAT="$6" expect_law="$7"
  "$work/calib_on"  "$N" "$F" "$K" "$DUP" "$REPEAT" > "$work/on.txt"
  "$work/calib_off" "$N" "$F" "$K" "$DUP" "$REPEAT" > "$work/off.txt"
  local ia_on ia_off delta populated law
  ia_on="$(counter "$work/on.txt" idx_adds)"
  ia_off="$(counter "$work/off.txt" idx_adds)"
  delta="$(( ia_off - ia_on ))"
  populated="$(minv "$N" "$K")"
  law="$(( F * populated ))"
  printf '%-22s N=%-6s F=%-3s K=%-4s DUP=%-2s R=%-2s | ON=%-6s OFF=%-6s Δ=%-6s F*min(N,K)=%-6s' \
    "$name" "$N" "$F" "$K" "$DUP" "$REPEAT" "$ia_on" "$ia_off" "$delta" "$law"
  if [ "$expect_law" = 1 ]; then
    if [ "$delta" -eq "$law" ]; then echo "  [LAW OK]"; else echo "  [LAW FAIL]"; fail=1; fi
  else
    echo "  [law N/A]"
  fi
}

echo
echo "== POSITIVE + FANOUT SWEEP (law: idx_adds(OFF)-idx_adds(ON) == F*min(N,K)) =="
scenario "positive/base"      256 4  256 1 1 1
scenario "fanout F=1"         256 1  256 1 1 1
scenario "fanout F=2"         256 2  256 1 1 1
scenario "fanout F=8"         256 8  256 1 1 1
scenario "fanout F=16"        256 16 256 1 1 1
scenario "held-out K=100"     256 4  100 1 1 1
scenario "held-out N=1000"   1000 4  256 1 1 1
scenario "held-out N=10000" 10000 4  256 1 1 1

echo
echo "== DUPLICATE-ROW (set/distinct semantics: probe-epoch idx_adds DUP-independent) =="
# DUP copies of every edge. The live neighborhood is still F distinct rows, so
# the probe-epoch idx_adds delta must remain F*min(N,K) -- proving idx_adds
# counts distinct stored rows, not ingest events (cost-model-findings.md #5).
scenario "dup x4"             256 4  256 4  1 1
scenario "dup x16"            256 4  256 16 1 1

echo
echo "== EMPTY / STANDING-EMPTY DEMAND (probe keys with no out-edges) =="
# N < K: keys [N,K) are demanded but have no edges. They still stand up a demand
# instance but materialize nothing. Raw F*K would MISPREDICT 1024 here; the
# corrected law F*min(N,K)=256 is what the runtime actually does -- so this is a
# CHECKED refutation of the naive F*K rule, not a hand-waved boundary.
scenario "empty tail refute" 64 4 256 1 1 1

echo
echo "== REPEATED DEMAND (idempotence: re-probing STANDING keys re-materializes nothing) =="
# REPEAT=4 probes each of the K keys four times inside the measured region. The
# first probe of a key stands its instance up; the next three re-read but add no
# index rows, so the delta stays F*min(N,K) rather than scaling by REPEAT.
scenario "repeat x4"          256 4  256 1 4 1
scenario "repeat x4 K=N=64"    64 4   64 1 4 1

if [ "$fail" -ne 0 ]; then
  echo; echo "CALIB: FAIL"; exit 1
fi
echo; echo "CALIB: PASS"
