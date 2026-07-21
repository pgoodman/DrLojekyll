#!/bin/bash
# Full golden-master suite runner: every case in cases/ through diffrun.sh,
# in parallel, with per-case verdict lines and a final summary. Every mode's
# stdout is byte-compared against the case's committed golden in goldens/.
#
# Usage: runall.sh <workroot> [jobs] [name-filter-regex]
#        runall.sh --bless <workroot> [name-filter-regex]
#
# Environment:
#   DR       path to the drlojekyll compiler         (required, except --bless)
#   ORACLE   path to the drlojekyll-oracle binary    (default: the
#            drlojekyll-oracle next to $DR; built via
#            `cmake --build <builddir> --target drlojekyll-oracle` if missing)
#   CXX      C++ compiler                            (default: clang++)
#   TIMEOUT  per-stage timeout in seconds            (default: 120)
#
# Case expectations:
#   kvindex_2/3/4, agg_in_scc_1, kv_in_scc_1, algebra_dup_1,
#   algebra_conflict_1, evm_func_parse, nonascii_1, truncated_decl_1,
#   demand_cyclic_1, demand_recursive_content_1, demand_diff_input_1 (the D2.c
#     nested-lowering fences: recursive demand / recursive-content demanded body
#     / a @differential summarized input — each rejects in all 4 modes via its
#     -demand[-instance] .drflags; cyclic + diff_input compile under plain
#     -demand and reject only under -demand-instance, recursive_content is the
#     upstream plain-demand body-walk reject),
#   demand_multi_adorn_1 (via its -demand .drflags sidecar: a demanded query
#     name with two binding patterns — the adornment cross-wire reject)
#     — the compiler must exit 1 with a rendered diagnostic (no assert/crash)
#     in all 4 modes (evm_func_parse: unstratified negation, rejected by the
#     dataflow Stratify pass; agg_in_scc_1/kv_in_scc_1: unstratified
#     aggregation — an aggregate/KV index over its own recursive result,
#     rejected by the same Stratify pass as the delta-relational-IR R3 sibling
#     of the negation reject, BEFORE the F14 control-flow pre-pass;
#     algebra_dup_1/algebra_conflict_1: R3c-i @-algebra pragma surface —
#     a duplicate pragma / the mutually-exclusive @invertible+@recompute pair,
#     rejected in Functor.cpp before any optimization runs; nonascii_1:
#     invalid byte in the display stream, F21; truncated_decl_1: file ends
#     mid-declaration, F21). These cases have an inert .main.cpp (never
#     compiled): unlike the feature-gap cases they can never compile.
#   kvindex_1 — runs under opt/nocf, each matching goldens/kvindex_1.stdout;
#     exits 1 with a rendered diagnostic under nodf/none (KVINDEX->TUPLE
#     elimination is a dataflow optimization).
#   every other case — diffrun.sh must pass: all 4 modes compile, build,
#     run, and match goldens/<case>.stdout.
#   any case with a cases/<name>.batches sidecar additionally runs the
#     derivation-counter oracle (drlojekyll-oracle <case.dr> <case.batches>);
#     its stdout is byte-compared against goldens/<name>.oracle.stdout. The
#     same case then runs the monotone projection
#     (drlojekyll-oracle <case.dr> <case.batches> --project-monotone: the
#     program over only the surviving inputs), byte-compared against
#     goldens/<name>.monotone.stdout.
#   any case with a cases/<name>.drflags sidecar has that file's contents
#     appended to EVERY mode's compiler invocation (e.g. `-demand` for
#     demand_tc_witness — the demand transform is orthogonal to the four
#     optimization modes, never a fifth mode). The oracle NEVER receives
#     these flags: it referees ANSWER-identity from the plain program
#     (demand must change materialization, never answers). Cases without a
#     sidecar are untouched.
#   any case with a cases/<name>.eqgate sidecar additionally runs run_eqgate:
#     the case is re-driven under the nested lowering (its .drflags plus the
#     -demand-instance selector) with the SAME driver, in all 4 optimization
#     modes, and each mode's stdout is byte-compared against the case's
#     committed golden (flat==nested==golden, refereed LIVE, never blessed).
#
# Blessing: goldens are updated ONLY by an explicit --bless invocation, after
# reviewing the outputs of a run — never automatically on failure. --bless
# copies each case's opt-mode stdout out of <workroot> into goldens/, each
# case's oracle stdout into goldens/<name>.oracle.stdout, and each case's
# monotone-projection stdout into goldens/<name>.monotone.stdout.
#
# Prints "SUITE: PASS (<n> cases)" and exits 0 iff every case meets its
# expectation; otherwise prints the failing verdict lines and exits 1.
set -u

HERE=$(cd "$(dirname "$0")" && pwd)

# ---- bless mode: promote opt-mode outputs from a workroot into goldens/ ----
if [ "${1:-}" = "--bless" ]; then
  WORKROOT=${2:?usage: runall.sh --bless <workroot> [name-filter-regex]}
  FILTER=${3:-.}
  mkdir -p "$HERE/goldens"
  n=0
  for d in "$WORKROOT"/*/; do
    name=$(basename "$d")
    echo "$name" | grep -qE "$FILTER" || continue
    src="$d$name.opt/stdout"
    if [ -f "$src" ]; then
      cp "$src" "$HERE/goldens/$name.stdout"
      echo "blessed $name"
      n=$((n + 1))
    fi
    osrc="$d$name.oracle/stdout"
    if [ -f "$osrc" ]; then
      cp "$osrc" "$HERE/goldens/$name.oracle.stdout"
      echo "blessed $name.oracle"
      n=$((n + 1))
    fi
    msrc="$d$name.monotone/stdout"
    if [ -f "$msrc" ]; then
      cp "$msrc" "$HERE/goldens/$name.monotone.stdout"
      echo "blessed $name.monotone"
      n=$((n + 1))
    fi
    # IR-golden surfaces (T3): driven by the case's .irgold sidecar; a pinned
    # surface whose produced file is missing is a HARD ERROR (never the
    # [ -f ] && cp skip idiom — a silent under-bless must be loud).
    if [ -f "$HERE/cases/$name.irgold" ] && [ -d "$d$name.irgold" ]; then
      while read -r surface mode; do
        [ -n "$surface" ] || continue
        isrc="$d$name.irgold/$surface.$mode.out"
        if [ ! -f "$isrc" ]; then
          echo "FATAL: $name.irgold pins '$surface $mode' but $isrc is missing"
          exit 1
        fi
        cp "$isrc" "$HERE/goldens/$name.$surface.$mode.golden"
        echo "blessed $name.$surface.$mode"
        n=$((n + 1))
      done < "$HERE/cases/$name.irgold"
    fi
  done
  # kvindex_1 runs outside diffrun.sh, so its workdir layout is flat.
  if [ -f "$WORKROOT/kvindex_1.opt/stdout" ] \
      && echo kvindex_1 | grep -qE "$FILTER"; then
    cp "$WORKROOT/kvindex_1.opt/stdout" "$HERE/goldens/kvindex_1.stdout"
    echo "blessed kvindex_1"
    n=$((n + 1))
  fi
  echo "BLESS: $n golden(s) updated"
  exit 0
fi

: "${DR:?set DR to the drlojekyll compiler path}"
case $DR in /*) ;; *) DR=$(pwd)/$DR ;; esac
export DR
ORACLE=${ORACLE:-$(dirname "$DR")/drlojekyll-oracle}
case $ORACLE in /*) ;; *) ORACLE=$(pwd)/$ORACLE ;; esac
export ORACLE
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
      opt) mflags="" ;;
      nodf) mflags="-disable-dataflow-opt" ;;
      nocf) mflags="-disable-controlflow-opt" ;;
      none) mflags="-disable-dataflow-opt -disable-controlflow-opt" ;;
    esac
    # Per-case extra compiler flags (the .drflags sidecar; see the header).
    if [ -f "$HERE/cases/$NAME.drflags" ]; then
      mflags="$mflags $(cat "$HERE/cases/$NAME.drflags")"
    fi
    echo "$mflags"
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

  run_vs_golden() {  # $1=mode; compile, build, run, compare against golden
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
    if [ ! -f "$HERE/goldens/$NAME.stdout" ]; then
      echo "$NAME $1 GOLDEN-MISSING"
      return 1
    fi
    if ! cmp -s "$HERE/goldens/$NAME.stdout" "$out/stdout"; then
      echo "$NAME $1 GOLDEN-DIVERGE"
      return 1
    fi
    return 0
  }

  run_oracle() {  # oracle step: any case with a .batches sidecar runs the
                  # derivation-counter oracle against its own golden, then the
                  # monotone projection (the program over only the surviving
                  # inputs) against its own golden
    batches="$HERE/cases/$NAME.batches"
    if [ ! -f "$batches" ]; then
      return 0
    fi
    orc=0

    out="$WORKROOT/$NAME/$NAME.oracle"
    mkdir -p "$out"
    if ! timeout "$TIMEOUT" "$ORACLE" "$DRC" "$batches" \
        >"$out/stdout" 2>"$out/stderr"; then
      echo "$NAME oracle ORACLE-FAIL"
      orc=1
    elif [ ! -f "$HERE/goldens/$NAME.oracle.stdout" ]; then
      echo "$NAME oracle GOLDEN-MISSING"
      orc=1
    elif ! cmp -s "$HERE/goldens/$NAME.oracle.stdout" "$out/stdout"; then
      echo "$NAME oracle GOLDEN-DIVERGE"
      orc=1
    else
      echo "$NAME oracle OK"
    fi

    # Monotone projection: the program evaluated as if nothing had been
    # removed — over exactly the surviving inputs. Its output is the
    # ground-truth final materialization and a standing F16-class gate
    # (a spurious cyclic residue would surface here as a divergence).
    mout="$WORKROOT/$NAME/$NAME.monotone"
    mkdir -p "$mout"
    if ! timeout "$TIMEOUT" "$ORACLE" "$DRC" "$batches" --project-monotone \
        >"$mout/stdout" 2>"$mout/stderr"; then
      echo "$NAME monotone MONO-FAIL"
      orc=1
    elif [ ! -f "$HERE/goldens/$NAME.monotone.stdout" ]; then
      echo "$NAME monotone MONO-MISSING"
      orc=1
    elif ! cmp -s "$HERE/goldens/$NAME.monotone.stdout" "$mout/stdout"; then
      echo "$NAME monotone MONO-DIVERGE"
      orc=1
    else
      echo "$NAME monotone OK"
    fi

    return $orc
  }

  run_irgold() {  # IR-golden step (T3): a case with a .irgold sidecar gets its
                  # pinned dump surfaces produced ONCE per pinned mode (one
                  # compile emits all four surfaces; `h` has no named-path
                  # stream, so it is post-copied out of the -cpp-out dir) and
                  # STRICT byte-compared against
                  # goldens/$NAME.<surface>.<mode>.golden. Sidecars are
                  # permitted only on all-4-modes-clean golden cases.
    sidecar="$HERE/cases/$NAME.irgold"
    if [ ! -f "$sidecar" ]; then
      return 0
    fi
    irc=0
    iout="$WORKROOT/$NAME/$NAME.irgold"
    mkdir -p "$iout"
    for mode in opt nodf nocf none; do
      grep -q " $mode\$" "$sidecar" || continue
      # shellcheck disable=SC2046
      if ! timeout "$TIMEOUT" "$DR" "$DRC" $(flags_of "$mode") \
          -df-out "$iout/df.$mode.out" \
          -deltarel-out "$iout/deltarel.$mode.out" \
          -ir-out "$iout/ir.$mode.out" \
          -cpp-out "$iout/cpp.$mode" \
          >"$iout/dr.$mode.log" 2>&1; then
        echo "$NAME irgold $mode IRGOLD-FAIL"
        irc=1
        continue
      fi
      cp "$iout/cpp.$mode/"*.h "$iout/h.$mode.out" 2>/dev/null || true
    done
    while read -r surface mode; do
      [ -n "$surface" ] || continue
      produced="$iout/$surface.$mode.out"
      golden="$HERE/goldens/$NAME.$surface.$mode.golden"
      if [ ! -f "$produced" ]; then
        echo "$NAME irgold $surface.$mode IRGOLD-FAIL"
        irc=1
      elif [ ! -f "$golden" ]; then
        echo "$NAME irgold $surface.$mode IRGOLD-MISSING"
        irc=1
      elif ! cmp -s "$golden" "$produced"; then
        echo "$NAME irgold $surface.$mode IRGOLD-DIVERGE"
        irc=1
      else
        echo "$NAME irgold $surface.$mode OK"
      fi
    done < "$sidecar"
    return $irc
  }

  run_eqgate() {  # equivalence gate (D2.c): a case with a .eqgate sidecar is
                  # re-driven under the nested lowering (.drflags + the
                  # -demand-instance selector) with the SAME driver, in ALL FOUR
                  # optimization modes, and each mode's stdout is byte-compared
                  # against the case's committed golden. diffrun already proved
                  # flat==golden per mode, so nested==golden per mode gives
                  # flat==nested transitively across all four modes. No nested
                  # golden is ever blessed -- the two-lowerings answer-identity
                  # gate is refereed live (OD-10/OWN-5).
    sidecar="$HERE/cases/$NAME.eqgate"
    if [ ! -f "$sidecar" ]; then
      return 0
    fi
    golden="$HERE/goldens/$NAME.stdout"
    if [ ! -f "$golden" ]; then
      echo "$NAME eqgate EQGATE-GOLDEN-MISSING"
      return 1
    fi
    eqrc=0
    for mode in opt nodf nocf none; do
      out="$WORKROOT/$NAME/$NAME.eqgate.$mode"
      mkdir -p "$out"
      # shellcheck disable=SC2046  # flags_of emits zero or more separate words
      if ! timeout "$TIMEOUT" "$DR" "$DRC" $(flags_of "$mode") -demand-instance \
          -cpp-out "$out" >"$out/dr.log" 2>&1; then
        echo "$NAME eqgate $mode EQGATE-DR-FAIL"
        eqrc=1
        continue
      fi
      if ! "$CXX" -std=c++23 -g -I "$REPO_ROOT/include" -I "$out" \
          "$DRV" "$out/datalog.cpp" "$REPO_ROOT/lib/Runtime/Allocator.cpp" \
          -o "$out/case" >"$out/cxx.log" 2>&1; then
        echo "$NAME eqgate $mode EQGATE-CXX-FAIL"
        eqrc=1
        continue
      fi
      if ! timeout "$TIMEOUT" "$out/case" >"$out/stdout" 2>"$out/stderr"; then
        echo "$NAME eqgate $mode EQGATE-RUN-FAIL"
        eqrc=1
        continue
      fi
      if ! cmp -s "$golden" "$out/stdout"; then
        echo "$NAME eqgate $mode NESTED-GOLDEN-DIVERGE"
        eqrc=1
        continue
      fi
      echo "$NAME eqgate $mode OK"
    done
    return $eqrc
  }

  st=0
  case $NAME in
    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|nonascii_1|truncated_decl_1|demand_multi_adorn_1|demand_cyclic_1|demand_recursive_content_1|demand_diff_input_1)
      for mode in opt nodf nocf none; do
        expect_diagnostic $mode || exit 1
      done
      echo "$NAME all-modes-diagnostic OK"
      ;;
    kvindex_1)
      run_vs_golden opt || exit 1
      run_vs_golden nocf || exit 1
      expect_diagnostic nodf || exit 1
      expect_diagnostic none || exit 1
      echo "$NAME modesplit OK"
      ;;
    *)
      "$HERE/diffrun.sh" "$DRC" "$DRV" "$WORKROOT/$NAME" || st=1
      ;;
  esac
  run_oracle || st=1
  run_eqgate || st=1
  run_irgold || st=1
  exit $st
fi

# ---- parallel driver ----
WORKROOT=${1:?usage: runall.sh <workroot> [jobs] [name-filter-regex]}
JOBS=${2:-6}
FILTER=${3:-.}

# Build the oracle if it is missing (it lives in the same build tree as DR).
if [ ! -x "$ORACLE" ]; then
  builddir=$(dirname "$ORACLE")/..
  echo "building drlojekyll-oracle in $builddir"
  if ! cmake --build "$builddir" --target drlojekyll-oracle >/dev/null; then
    echo "FATAL: cannot build drlojekyll-oracle; set ORACLE= explicitly"
    exit 1
  fi
fi
if [ ! -x "$ORACLE" ]; then
  echo "FATAL: oracle binary not found at $ORACLE; set ORACLE= explicitly"
  exit 1
fi

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

if grep -qE 'FAIL|DIVERGE|EXPECT-ERROR|MISSING' "$WORKROOT/verdicts"; then
  echo "SUITE: FAIL"
  grep -E 'FAIL|DIVERGE|EXPECT-ERROR|MISSING' "$WORKROOT/verdicts"
  exit 1
fi
echo "SUITE: PASS ($(wc -l < "$WORKROOT/caselist" | tr -d ' ') cases)"
