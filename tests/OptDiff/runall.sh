#!/bin/bash
# Full OptDiff suite runner: every case in cases/ through diffrun.sh, in
# parallel, with per-case verdict lines and a final summary.
#
# Usage: runall.sh <workroot> [jobs] [name-filter-regex]
#
# Environment:
#   DR       path to the drlojekyll compiler         (required)
#   CXX      C++ compiler                            (default: clang++)
#   TIMEOUT  per-stage timeout in seconds            (default: 120)
#
# Case expectations:
#   aggregate_1, kvindex_2/3/4 — the compiler must exit 1 with a rendered
#     diagnostic (no assert/crash) in all 4 modes.
#   kvindex_1 — compiles and runs byte-identically under opt/nocf; exits 1
#     with a rendered diagnostic under nodf/none (KVINDEX->TUPLE elimination
#     is a dataflow optimization).
#   every other case — diffrun.sh must pass: all 4 modes compile, build,
#     run, and produce byte-identical stdout.
#
# Prints "SUITE: PASS (<n> cases)" and exits 0 iff every case meets its
# expectation; otherwise prints the failing verdict lines and exits 1.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)

: "${DR:?set DR to the drlojekyll compiler path}"
case $DR in /*) ;; *) DR=$(pwd)/$DR ;; esac
export DR
export CXX=${CXX:-clang++}
export TIMEOUT=${TIMEOUT:-120}

# ---- per-case worker (invoked by the parallel driver below) ----
if [ "${1:-}" = "--one" ]; then
  NAME=$2
  WORKROOT=$3
  DRC="$HERE/cases/$NAME.dr"
  DRV="$HERE/cases/$NAME.main.cpp"
  REPO_ROOT=$(cd "$HERE/../.." && pwd)

  flags_of() {
    case $1 in
      opt) echo "" ;;
      nodf) echo "-disable-dataflow-opt" ;;
      nocf) echo "-disable-controlflow-opt" ;;
      none) echo "-disable-dataflow-opt -disable-controlflow-opt" ;;
    esac
  }

  expect_diagnostic() {  # $1=mode; exit 1 unless the compiler exits 1 cleanly
    out="$WORKROOT/$NAME.$1"
    mkdir -p "$out"
    # shellcheck disable=SC2046
    timeout "$TIMEOUT" "$DR" "$DRC" $(flags_of "$1") -cpp-out "$out" \
        >"$out/dr.log" 2>&1
    rc=$?
    if [ $rc -ne 1 ]; then
      echo "$NAME $1 EXPECT-ERROR-GOT($rc)"
      return 1
    fi
    return 0
  }

  compile_build_run() {  # $1=mode; exit 1 on any stage failure
    out="$WORKROOT/$NAME.$1"
    mkdir -p "$out"
    # shellcheck disable=SC2046
    if ! timeout "$TIMEOUT" "$DR" "$DRC" $(flags_of "$1") -cpp-out "$out" \
        >"$out/dr.log" 2>&1; then
      echo "$NAME $1 DR-FAIL"
      return 1
    fi
    if ! "$CXX" -std=c++23 -g -I "$REPO_ROOT/include" -I "$out" \
        "$DRV" "$out/datalog.cpp" "$REPO_ROOT/lib/Runtime/Allocator.cpp" \
        -o "$out/case" >"$out/cxx.log" 2>&1; then
      echo "$NAME $1 CXX-FAIL"
      return 1
    fi
    if ! timeout "$TIMEOUT" "$out/case" >"$out/stdout" 2>"$out/stderr"; then
      echo "$NAME $1 RUN-FAIL"
      return 1
    fi
    return 0
  }

  case $NAME in
    aggregate_1|kvindex_2|kvindex_3|kvindex_4)
      for mode in opt nodf nocf none; do
        expect_diagnostic $mode || exit 1
      done
      echo "$NAME all-modes-diagnostic OK"
      ;;
    kvindex_1)
      compile_build_run opt || exit 1
      compile_build_run nocf || exit 1
      if ! cmp -s "$WORKROOT/$NAME.opt/stdout" "$WORKROOT/$NAME.nocf/stdout"; then
        echo "$NAME nocf DIVERGE"
        exit 1
      fi
      expect_diagnostic nodf || exit 1
      expect_diagnostic none || exit 1
      echo "$NAME modesplit OK"
      ;;
    *)
      exec "$HERE/diffrun.sh" "$DRC" "$DRV" "$WORKROOT/$NAME"
      ;;
  esac
  exit 0
fi

# ---- parallel driver ----
WORKROOT=${1:?usage: runall.sh <workroot> [jobs] [name-filter-regex]}
JOBS=${2:-6}
FILTER=${3:-.}

mkdir -p "$WORKROOT"
WORKROOT=$(cd "$WORKROOT" && pwd)

ls "$HERE"/cases/*.dr | sed 's|.*/||; s|\.dr$||' | grep -E "$FILTER" \
    > "$WORKROOT/caselist"
if [ ! -s "$WORKROOT/caselist" ]; then
  echo "no cases match filter: $FILTER"
  exit 1
fi

xargs -P "$JOBS" -I{} "$HERE/runall.sh" --one {} "$WORKROOT" \
    < "$WORKROOT/caselist" > "$WORKROOT/verdicts" 2>&1

if grep -qE 'FAIL|DIVERGE|EXPECT-ERROR' "$WORKROOT/verdicts"; then
  echo "SUITE: FAIL"
  grep -E 'FAIL|DIVERGE|EXPECT-ERROR' "$WORKROOT/verdicts"
  exit 1
fi
echo "SUITE: PASS ($(wc -l < "$WORKROOT/caselist" | tr -d ' ') cases)"
