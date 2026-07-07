#!/bin/bash
# Differential test runner for Dr. Lojekyll optimization passes.
#
# For one Datalog case, compiles the program in four optimization modes
# (opt = default, nodf = -disable-dataflow-opt, nocf = -disable-controlflow-opt,
# none = both disabled), builds the generated C++ against the runtime with the
# case's driver, runs each binary, and compares every mode's output against
# the fully optimized build.
#
# Usage: diffrun.sh <case.dr> <driver.cpp> <workdir>
#
# Environment:
#   DR       path to the drlojekyll compiler         (required)
#   CXX      C++ compiler                            (default: clang++)
#   TIMEOUT  per-stage timeout in seconds            (default: 60)
#
# Exit status 0 iff every mode compiles, generates buildable C++, runs
# cleanly, and produces byte-identical output to the optimized build.
# Verdict lines are printed to stdout, one per mode:
#   <case> <mode> OK | DR-FAIL(<code>) | CXX-FAIL | RUN-FAIL(<code>) | DIVERGE

set -u

CASE_DR=$1
DRIVER=$2
WORK=$3

: "${DR:?set DR to the drlojekyll compiler path}"
CXX=${CXX:-clang++}
TIMEOUT=${TIMEOUT:-60}
REPO_ROOT=$(cd "$(dirname "$0")/../.." && pwd)
NAME=$(basename "$CASE_DR" .dr)

mkdir -p "$WORK"
status=0

for mode in opt nodf nocf none; do
  case $mode in
    opt) flags="" ;;
    nodf) flags="-disable-dataflow-opt" ;;
    nocf) flags="-disable-controlflow-opt" ;;
    none) flags="-disable-dataflow-opt -disable-controlflow-opt" ;;
  esac

  out="$WORK/$NAME.$mode"
  mkdir -p "$out"

  # shellcheck disable=SC2086  # $flags holds zero or more separate words
  timeout "$TIMEOUT" "$DR" "$CASE_DR" $flags -cpp-out "$out" \
       >"$out/dr.log" 2>&1
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "$NAME $mode DR-FAIL($rc)"
    status=1
    continue
  fi

  if ! "$CXX" -std=c++23 -g -I "$REPO_ROOT/include" -I "$out" \
       "$DRIVER" "$out/datalog.cpp" "$REPO_ROOT/lib/Runtime/Allocator.cpp" \
       -o "$out/case" >"$out/cxx.log" 2>&1; then
    echo "$NAME $mode CXX-FAIL"
    status=1
    continue
  fi

  timeout "$TIMEOUT" "$out/case" >"$out/stdout" 2>"$out/stderr"
  rc=$?
  if [[ $rc -ne 0 ]]; then
    echo "$NAME $mode RUN-FAIL($rc)"
    status=1
    continue
  fi

  if [[ $mode == opt ]]; then
    echo "$NAME opt OK"
  elif cmp -s "$WORK/$NAME.opt/stdout" "$out/stdout"; then
    echo "$NAME $mode OK"
  else
    echo "$NAME $mode DIVERGE"
    status=1
  fi
done

exit $status
