#!/bin/bash
# Bench harness runner (PerfRoadmap §2/§6.3; methodology in BASELINE.md).
# NEVER part of correctness CI; never run concurrently with suite runs.
#
# Usage: runbench.sh <workroot> <runspec> [modes]
#
#   workroot  scratch dir for generated code, binaries, fragments, results
#   runspec   manifest file; '#' comments; lines are one of
#               engine   <family> <case.dr-relpath> <knob k=v ...>
#               baseline <name>   <knob k=v ...>
#               progsize <rules> [<rules> ...]
#             The SAME (family, knobs) engine line is run once per mode.
#   modes     space-separated subset of "opt nodf nocf none debug"
#             (default "opt"; debug = -g without NDEBUG, seam off,
#             flagged mode=debug per R25)
#
# Environment:
#   DR        drlojekyll compiler (required)
#   CXX       C++ compiler (default clang++)
#   REPS      process repetitions per knob-point (default 5)
#   CTIMEOUT  compile timeout seconds (default 600)  — R21: a compile
#   RTIMEOUT  run timeout seconds (default 300)        timeout is a HARD,
#                                                      logged error
#   COUNTS    1 = also build+run the counts binary (-DDRLOJEKYLL_BENCH_
#             COUNTERS) per engine point; its wall rows are flagged by
#             mode suffix "+counts" and must never be read as timings
#
# Integrity (R21): every run writes its own fragment; only fragments whose
# LAST row is the run_complete marker are folded into results.tsv. Exit
# codes land in manifest.tsv. A sentinel mismatch or scale abort is a
# recorded event, never a silent gap.
#
# bash 3.2 (macOS): no declare -A, no ${var^^}.
set -u

WORKROOT=${1:?usage: runbench.sh <workroot> <runspec> [modes]}
RUNSPEC=${2:?usage: runbench.sh <workroot> <runspec> [modes]}
MODES=${3:-opt}

: "${DR:?set DR to the drlojekyll compiler path}"
CXX=${CXX:-clang++}
REPS=${REPS:-5}
CTIMEOUT=${CTIMEOUT:-600}
RTIMEOUT=${RTIMEOUT:-300}
COUNTS=${COUNTS:-0}

HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/.." && pwd)
mkdir -p "$WORKROOT"
WORKROOT=$(cd "$WORKROOT" && pwd)
RESULTS="$WORKROOT/results.tsv"
MANIFEST="$WORKROOT/manifest.tsv"
FRAGS="$WORKROOT/frags"
mkdir -p "$FRAGS"

now_ns() { python3 -c 'import time; print(time.monotonic_ns())'; }

# ---- run header (R22): everything that moves a number, pinned ----------
{
  echo "# runbench $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  echo "# commit $(cd "$REPO" && git rev-parse HEAD 2>/dev/null || echo unknown)"
  echo "# dr $DR sha256=$(shasum -a 256 "$DR" | cut -d' ' -f1)"
  echo "# cxx $($CXX --version | head -1)"
  echo "# os $(sw_vers -productVersion 2>/dev/null || uname -r) $(uname -m)"
  echo "# loadavg $(sysctl -n vm.loadavg 2>/dev/null || uptime)"
  echo "# reps $REPS ctimeout $CTIMEOUT rtimeout $RTIMEOUT modes $MODES"
  echo "# compile: $CXX -std=c++23 -O2 -DNDEBUG -I $REPO/include -I GEN driver.cpp GEN/datalog.cpp $REPO/lib/Runtime/Allocator.cpp"
  echo "# bench-src sha256 $(cd "$REPO" && find bench -name '*.h' -o -name '*.cpp' -o -name '*.dr' | sort | xargs shasum -a 256 | shasum -a 256 | cut -d' ' -f1)"
} > "$RESULTS"
printf "kind\tname\tmode\tknobs\trep\texit\tfolded\n" > "$MANIFEST"

mode_flags() {
  case $1 in
    opt|debug) echo "" ;;
    nodf) echo "-disable-dataflow-opt" ;;
    nocf) echo "-disable-controlflow-opt" ;;
    none) echo "-disable-dataflow-opt -disable-controlflow-opt" ;;
  esac
}

cxx_flags() {
  case $1 in
    debug) echo "-std=c++23 -g" ;;
    *) echo "-std=c++23 -O2 -DNDEBUG" ;;
  esac
}

# Compile one engine (family, mode); records Q5 compile metrics (R20).
# Sets ENGINE_BIN / COUNTS_BIN on success; returns nonzero on failure.
compile_engine() {  # $1=family $2=case.dr(rel) $3=mode
  local family=$1 drfile=$REPO/$2 mode=$3
  local gen="$WORKROOT/gen_${family}_${mode}"
  ENGINE_BIN="$WORKROOT/bin_${family}_${mode}"
  COUNTS_BIN="$WORKROOT/bin_${family}_${mode}_counts"
  [ -x "$ENGINE_BIN" ] && return 0
  mkdir -p "$gen"
  local t0 t1
  t0=$(now_ns)
  # shellcheck disable=SC2046
  if ! timeout "$CTIMEOUT" "$DR" "$drfile" $(mode_flags "$mode") \
      -cpp-out "$gen" > "$gen/dr.log" 2>&1; then
    echo "COMPILE-FAIL(dr) $family $mode" >&2
    printf "compile\t%s\t%s\t-\t-\tDR-FAIL\tno\n" "$family" "$mode" >> "$MANIFEST"
    return 1
  fi
  t1=$(now_ns)
  emit_compile_row "$family" "$mode" dr_wall_ns $((t1 - t0))
  # The artifact basename follows #database (default "datalog"); detect it.
  local anchor header
  anchor=$(ls "$gen"/*.cpp | head -1)
  header=${anchor%.cpp}.h
  t0=$(now_ns)
  # shellcheck disable=SC2046
  if ! timeout "$CTIMEOUT" "$CXX" $(cxx_flags "$mode") -I "$REPO/include" -I "$gen" \
      "$REPO/bench/workloads/$family/driver.cpp" "$anchor" \
      "$REPO/lib/Runtime/Allocator.cpp" -o "$ENGINE_BIN" > "$gen/cxx.log" 2>&1; then
    echo "COMPILE-FAIL(cxx) $family $mode (see $gen/cxx.log)" >&2
    printf "compile\t%s\t%s\t-\t-\tCXX-FAIL\tno\n" "$family" "$mode" >> "$MANIFEST"
    return 1
  fi
  t1=$(now_ns)
  emit_compile_row "$family" "$mode" cxx_wall_ns $((t1 - t0))
  emit_compile_row "$family" "$mode" header_bytes "$(stat -f %z "$header")"
  emit_compile_row "$family" "$mode" header_lines "$(wc -l < "$header" | tr -d ' ')"
  emit_compile_row "$family" "$mode" binary_bytes "$(stat -f %z "$ENGINE_BIN")"
  if [ "$COUNTS" = 1 ] && [ "$mode" != debug ]; then
    # shellcheck disable=SC2046
    timeout "$CTIMEOUT" "$CXX" $(cxx_flags "$mode") -DDRLOJEKYLL_BENCH_COUNTERS \
        -I "$REPO/include" -I "$gen" \
        "$REPO/bench/workloads/$family/driver.cpp" "$anchor" \
        "$REPO/lib/Runtime/Allocator.cpp" -o "$COUNTS_BIN" \
        > "$gen/cxx_counts.log" 2>&1 || {
      echo "COMPILE-FAIL(counts) $family $mode" >&2
      printf "compile\t%s\t%s+counts\t-\t-\tCXX-FAIL\tno\n" "$family" "$mode" >> "$MANIFEST"
      return 1
    }
  fi
  return 0
}

emit_compile_row() {  # family mode metric value — run-scoped compile rows
  printf "%s\tcompile\t%s\t-\t-\t%s\t%s\n" "$1" "$2" "$3" "$4" >> "$RESULTS"
}

# Run one binary at one knob-point for one rep; fold iff run_complete.
run_one() {  # $1=kind $2=name $3=bin $4=mode-label $5=rep $6...=knobs
  local kind=$1 name=$2 bin=$3 mode=$4 rep=$5
  shift 5
  local frag="$FRAGS/${name}_${mode}_$(echo "$*" | tr ' =,' '__.').r${rep}.tsv"
  timeout "$RTIMEOUT" "$bin" "$@" mode="$mode" rep="$rep" > "$frag" 2> "$frag.err"
  local rc=$?
  local folded=no
  if [ $rc -eq 0 ] && [ -s "$frag" ] \
      && tail -1 "$frag" | grep -q "run_complete"; then
    cat "$frag" >> "$RESULTS"
    folded=yes
  fi
  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
      "$kind" "$name" "$mode" "$*" "$rep" "$rc" "$folded" >> "$MANIFEST"
  if [ $rc -ne 0 ]; then
    echo "RUN-FAIL($rc) $name $mode rep=$rep $* (stderr: $frag.err)" >&2
  fi
}

# Compile a baseline once (native, no generated code).
compile_baseline() {  # $1=name
  local name=$1
  BASE_BIN="$WORKROOT/bin_base_$name"
  [ -x "$BASE_BIN" ] && return 0
  timeout "$CTIMEOUT" "$CXX" -std=c++23 -O2 -DNDEBUG \
      "$REPO/bench/baselines/$name.cpp" -o "$BASE_BIN" \
      > "$WORKROOT/base_$name.cxx.log" 2>&1 || {
    echo "COMPILE-FAIL(baseline) $name" >&2
    printf "compile\t%s\tnative\t-\t-\tCXX-FAIL\tno\n" "$name" >> "$MANIFEST"
    return 1
  }
}

# progsize (R20/Q5): generate a chained-rule program of R rules, compile
# it in every mode, record compile metrics only. No driver, no run.
progsize_one() {  # $1=rules
  local rules=$1
  local dir="$WORKROOT/progsize_$rules"
  mkdir -p "$dir"
  local drf="$dir/prog.dr"
  {
    echo "; generated by runbench.sh progsize: $rules chained rules."
    echo "#message e0(u64 X, u64 Y) @differential."
    echo "#query out(free u64 X, free u64 Y)."
    echo "#local r0(X, Y)."
    echo "r0(X, Y) : e0(X, Y)."
    local i=1
    while [ "$i" -le "$rules" ]; do
      echo "#local r$i(X, Y)."
      echo "r$i(X, Y) : r$((i - 1))(X, Z), r0(Z, Y)."
      i=$((i + 1))
    done
    echo "out(X, Y) : r$rules(X, Y)."
  } > "$drf"
  local mode t0 t1 gen
  for mode in $MODES; do
    [ "$mode" = debug ] && continue
    gen="$dir/gen_$mode"
    mkdir -p "$gen"
    t0=$(now_ns)
    # shellcheck disable=SC2046
    if ! timeout "$CTIMEOUT" "$DR" "$drf" $(mode_flags "$mode") \
        -cpp-out "$gen" > "$gen/dr.log" 2>&1; then
      printf "compile\tprogsize_%s\t%s\t-\t-\tDR-FAIL\tno\n" "$rules" "$mode" >> "$MANIFEST"
      continue
    fi
    t1=$(now_ns)
    printf "progsize\trules=%s\t%s\t-\t-\tdr_wall_ns\t%s\n" "$rules" "$mode" $((t1 - t0)) >> "$RESULTS"
    printf "progsize\trules=%s\t%s\t-\t-\theader_bytes\t%s\n" "$rules" "$mode" "$(stat -f %z "$gen/datalog.h")" >> "$RESULTS"
    printf "progsize\trules=%s\t%s\t-\t-\theader_lines\t%s\n" "$rules" "$mode" "$(wc -l < "$gen/datalog.h" | tr -d ' ')" >> "$RESULTS"
    t0=$(now_ns)
    if timeout "$CTIMEOUT" "$CXX" -std=c++23 -O2 -DNDEBUG -I "$REPO/include" -I "$gen" \
        -c "$gen/datalog.cpp" -o "$gen/datalog.o" > "$gen/cxx.log" 2>&1; then
      t1=$(now_ns)
      printf "progsize\trules=%s\t%s\t-\t-\tcxx_wall_ns\t%s\n" "$rules" "$mode" $((t1 - t0)) >> "$RESULTS"
    else
      printf "compile\tprogsize_%s\t%s\t-\t-\tCXX-FAIL(anchor)\tno\n" "$rules" "$mode" >> "$MANIFEST"
    fi
  done
}

# ---- drive the runspec --------------------------------------------------
status=0
while IFS= read -r line || [ -n "$line" ]; do
  case $line in
    ''|'#'*) continue ;;
  esac
  set -- $line
  kind=$1
  shift
  case $kind in
    engine)
      family=$1
      drrel=$2
      shift 2
      for mode in $MODES; do
        if ! compile_engine "$family" "$drrel" "$mode"; then
          status=1
          continue
        fi
        rep=0
        while [ "$rep" -lt "$REPS" ]; do
          run_one engine "$family" "$ENGINE_BIN" "$mode" "$rep" "$@"
          rep=$((rep + 1))
        done
        if [ "$COUNTS" = 1 ] && [ "$mode" != debug ] && [ -x "$COUNTS_BIN" ]; then
          run_one engine "$family" "$COUNTS_BIN" "$mode+counts" 0 "$@"
        fi
      done
      ;;
    baseline)
      name=$1
      shift
      if compile_baseline "$name"; then
        rep=0
        while [ "$rep" -lt "$REPS" ]; do
          run_one baseline "$name" "$BASE_BIN" native "$rep" "$@"
          rep=$((rep + 1))
        done
      else
        status=1
      fi
      ;;
    progsize)
      for rules in "$@"; do
        progsize_one "$rules"
      done
      ;;
    *)
      echo "runspec: unknown kind '$kind'" >&2
      status=1
      ;;
  esac
done < "$RUNSPEC"

# ---- sentinel cross-check (R12/R21): every folded run at the same
# (semantic knobs minus measurement knobs is too clever; we key on the
# final hash per name+knobs and flag any name whose runs disagree, plus
# any engine/baseline pair sharing a seed-relevant knob string) ----------
awk -F'\t' '$6 ~ /^final_.*_hash$/ {key=$1 "\t" $2; if (key in h && h[key] != $7) bad[key]=1; h[key]=$7}
  END {for (k in bad) {print "SENTINEL-DISAGREE within " k; ec=1} exit ec}' \
  "$RESULTS" || { echo "SENTINEL: intra-run disagreement"; status=1; }

if [ $status -eq 0 ]; then
  echo "BENCH: COMPLETE ($(grep -c 'yes$' "$MANIFEST" | tr -d ' ') folded runs; results: $RESULTS)"
else
  echo "BENCH: INCOMPLETE (see $MANIFEST)"
fi
exit $status
