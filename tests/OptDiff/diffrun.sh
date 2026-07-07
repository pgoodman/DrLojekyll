#!/bin/bash
# Golden-master test runner for Dr. Lojekyll optimization passes.
#
# For one Datalog case, compiles the program in four optimization modes
# (opt = default, nodf = -disable-dataflow-opt, nocf = -disable-controlflow-opt,
# none = both disabled), builds the generated C++ against the runtime with the
# case's driver, runs each binary, and byte-compares EVERY mode's stdout
# against the case's committed golden master (goldens/<case>.stdout).
# Mode agreement is implied: all four modes must equal the same golden.
#
# Usage: diffrun.sh <case.dr> <driver.cpp> <workdir>
#
# Environment:
#   DR       path to the drlojekyll compiler         (required)
#   CXX      C++ compiler                            (default: clang++)
#   TIMEOUT  per-stage timeout in seconds            (default: 60)
#
# Exit status 0 iff every mode compiles, generates buildable C++, runs
# cleanly, and matches the golden. Verdict lines, one per mode:
#   <case> <mode> OK | DR-FAIL(<code>) | CXX-FAIL | RUN-FAIL(<code>)
#          | GOLDEN-DIVERGE | GOLDEN-MISSING
# A missing golden is a failure: after reviewing a new case's output, bless
# it deliberately with  runall.sh --bless <workroot> <case-name>.

set -u

CASE_DR=$1
DRIVER=$2
WORK=$3

: "${DR:?set DR to the drlojekyll compiler path}"
CXX=${CXX:-clang++}
TIMEOUT=${TIMEOUT:-60}
HERE=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$HERE/../.." && pwd)
NAME=$(basename "$CASE_DR" .dr)
GOLDEN="$HERE/goldens/$NAME.stdout"

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

  if [[ ! -f $GOLDEN ]]; then
    echo "$NAME $mode GOLDEN-MISSING"
    status=1
  elif cmp -s "$GOLDEN" "$out/stdout"; then
    echo "$NAME $mode OK"
  else
    echo "$NAME $mode GOLDEN-DIVERGE"
    status=1
  fi
done

exit $status
