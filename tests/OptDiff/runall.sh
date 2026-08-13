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
#   demand_cyclic_1, demand_recursive_content_1 (the D2.c nested-lowering
#     fences: recursive demand / recursive-content demanded body — each rejects
#     in all 4 modes via its -demand[-instance] .drflags; cyclic compiles under
#     plain -demand and rejects only under -demand-instance, recursive_content
#     is the upstream plain-demand body-walk reject. The @differential
#     summarized-input fence was LIFTED at D3.a.2 — demand_diff_input_1 is now
#     the diff-input x diff-demand composition eqgate witness and
#     demand_diff_neighborhood_witness the e5 diff-input x mono-demand carrier,
#     both ordinary auto-discovered golden cases, not diagnostics),
#   demand_multi_adorn_1 (via its -demand .drflags sidecar: a demanded query
#     name with two binding patterns where one is left-linear — since D3.a.3 the
#     reject is the per-adornment left-linear fence, not the per-name belt),
#   demand_multi_adorn_allfree_1 (a demanded query name carrying a BOUND and an
#     all-free sibling adornment — the all-free cursor would read the guarded pub
#     and under-answer; the D3.a.3 all-free-sibling fence),
#   negate_never_diff_1 (@never over a differential negated view — the
#     DS-R4-10 post-fixpoint fence; the directed order-hole shape that was
#     mode-split before D3.a.1),
#   product_in_scc_diff_1 (on-cycle differential @product, the F23 shape:
#     rejected by the ViewSelfReachable fence in Program::Build's pre-pass;
#     pinned at the F26 round after the recorded opt-mode exit-139 crash
#     stopped reproducing),
#   demand_agg_body_1, demand_kv_body_1, demand_config_agg_body_1 (the
#     RegionalDataFlowCore pre-Stage-A landing set, owner-ratified D3.3: an
#     over(){} aggregate / KV merge / config-@recompute aggregate inside a
#     demanded body — each rejects via the demand-sink or R-MAT diagnostic
#     under its -demand .drflags; each carries .batches + oracle goldens
#     pinning the DEFINITIONAL answer now, BEFORE any Stage-C reject lift),
#   demand_mutual_content_1 (mutual recursion INSIDE a demanded body — the
#     shadowed recursive-content belt, R-BODYWALK; .batches + oracle goldens
#     pin the answer for the Stage-D lift, the stage-d E2 referee hole),
#   demand_two_queries_1 (two independent bound query names — R-1BOUND;
#     expected to lift at Stage C as a clean capability add)
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
#   any case with a cases/<name>.batches sidecar ALSO runs run_refinterp (the
#   I0 referee, stage-i0-interpreter.md H6 + the S7 amendments): the reference
#   interpreter's CBF is byte-compared against goldens/<name>.behavioral.stdout,
#   and the PLAIN-compiled behavioral binary (never .drflags) must byte-agree
#   across all 4 modes and match the same golden. Diagnostic cases run
#   interp-only. A REFINTERP-DISAGREE is adjudicated per the stage doc's S3
#   protocol (finding, never fudge).
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
  # ADJ-K1-G: never bless THROUGH a symlink. A symlink golden is a twin-
  # equivalence claim (key_tc_witness, key_neighborhood_witness,
  # key_multi_adorn_witness.stdout, ...); a plain `cp` writes through it and
  # silently corrupts the TWIN's real golden, masking the very divergence the
  # witness exists to catch. $1=produced src  $2=golden dest  $3=label.
  bless_copy() {
    if [ -L "$2" ]; then
      if cmp -s "$1" "$2"; then
        echo "skipped $3 (symlink, byte-identical)"
      else
        echo "BLESS-REFUSED $3: golden is a symlink and bytes diverge"
        exit 1
      fi
      return 1
    fi
    # Census honesty (F32-adjacent, 2026-08-05): a byte-identical re-bless is
    # a no-op — report it as a skip, never as "blessed", so the BLESS count
    # reflects real content deltas.
    if [ -f "$2" ] && cmp -s "$1" "$2"; then
      echo "skipped $3 (byte-identical)"
      return 1
    fi
    cp "$1" "$2"
    echo "blessed $3"
    return 0
  }
  n=0
  for d in "$WORKROOT"/*/; do
    name=$(basename "$d")
    echo "$name" | grep -qE "$FILTER" || continue
    src="$d$name.opt/stdout"
    if [ -f "$src" ]; then
      bless_copy "$src" "$HERE/goldens/$name.stdout" "$name" && n=$((n + 1))
    fi
    osrc="$d$name.oracle/stdout"
    if [ -f "$osrc" ]; then
      bless_copy "$osrc" "$HERE/goldens/$name.oracle.stdout" "$name.oracle" \
        && n=$((n + 1))
    fi
    msrc="$d$name.monotone/stdout"
    if [ -f "$msrc" ]; then
      bless_copy "$msrc" "$HERE/goldens/$name.monotone.stdout" "$name.monotone" \
        && n=$((n + 1))
    fi
    bsrc="$d$name.refinterp/behavioral.opt"
    if [ -f "$bsrc" ]; then
      bless_copy "$bsrc" "$HERE/goldens/$name.behavioral.stdout" \
        "$name.behavioral" && n=$((n + 1))
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
        bless_copy "$isrc" "$HERE/goldens/$name.$surface.$mode.golden" \
          "$name.$surface.$mode" && n=$((n + 1))
      done < "$HERE/cases/$name.irgold"
    fi
  done
  # kvindex_1 runs outside diffrun.sh, so its workdir layout is flat.
  if [ -f "$WORKROOT/kvindex_1.opt/stdout" ] \
      && echo kvindex_1 | grep -qE "$FILTER"; then
    bless_copy "$WORKROOT/kvindex_1.opt/stdout" "$HERE/goldens/kvindex_1.stdout" \
      "kvindex_1" && n=$((n + 1))
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
REFINTERP=${REFINTERP:-$(dirname "$DR")/drlojekyll-refinterp}
case $REFINTERP in /*) ;; *) REFINTERP=$(pwd)/$REFINTERP ;; esac
export REFINTERP
REFHARNESS=${REFHARNESS:-$(dirname "$DR")/drlojekyll-refharness}
case $REFHARNESS in /*) ;; *) REFHARNESS=$(pwd)/$REFHARNESS ;; esac
export REFHARNESS
export CXX=${CXX:-clang++}
export TIMEOUT=${TIMEOUT:-120}

# ---- per-case worker (invoked by the parallel driver below) ----
if [ "${1:-}" = "--one" ]; then
  NAME=$2
  WORKROOT=$3
  DRC="$HERE/cases/$NAME.dr"
  DRV="$HERE/cases/$NAME.main.cpp"
  REPO_ROOT=$(cd "$HERE/../.." && pwd)

  mode_flags_of() {  # optimization-mode flags ONLY — never the .drflags
                     # sidecar. run_refinterp's behavioral compile uses this
                     # directly: the behavioral binary is the PLAIN program
                     # (the 2026-08-03 adjudication, IMPLEMENTED 2026-08-05
                     # at F32 — the compile had silently been demand-ON via
                     # flags_of since the D3.a.1 differential witness).
    case $1 in
      opt) echo "" ;;
      nodf) echo "-disable-dataflow-opt" ;;
      nocf) echo "-disable-controlflow-opt" ;;
      none) echo "-disable-dataflow-opt -disable-controlflow-opt" ;;
    esac
  }

  flags_of() {
    mflags=$(mode_flags_of "$1")
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
          -contract-out "$iout/contract.$mode.out" \
          -rel-out "$iout/rel.$mode.out" \
          -region-out "$iout/region.$mode.out" \
          -origin-out "$iout/origin.$mode.out" \
          -instanceflow-out "$iout/instanceflow.$mode.out" \
          -materialization-out "$iout/materialization.$mode.out" \
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

  run_refinterp() {  # I0 referee (stage-i0-interpreter.md H6 + §7): any case
                     # with a .batches sidecar. The interpreter (demand-blind,
                     # mode-independent) is byte-compared against the FROZEN
                     # behavioral golden; the behavioral binary (the PLAIN
                     # program — never .drflags, the 2026-08-03 adjudication —
                     # against the stable ABI) is built in all 4 modes and
                     # must byte-agree across them (ABI mode-invariance).
                     # Diagnostic cases run interp-only (no behavioral binary).
    batches="$HERE/cases/$NAME.batches"
    if [ ! -f "$batches" ]; then
      return 0
    fi
    ri=0
    out="$WORKROOT/$NAME/$NAME.refinterp"
    mkdir -p "$out"
    pargs=""
    if [ -f "$HERE/cases/$NAME.probes" ]; then
      pargs="$HERE/cases/$NAME.probes"
    fi

    # 1. The I0 run (once; no modes, no .drflags).
    # shellcheck disable=SC2086
    if ! timeout "$TIMEOUT" "$REFINTERP" "$DRC" "$batches" $pargs \
        >"$out/interp.cbf" 2>"$out/interp.stderr"; then
      echo "$NAME refinterp REFINTERP-FAIL"
      return 1
    fi

    # Diagnostic case: nothing compiles; the definitional answer is pinned by
    # the oracle/monotone goldens, so a clean interp run is the whole gate.
    if [ ! -f "$HERE/goldens/$NAME.stdout" ]; then
      echo "$NAME refinterp OK-DIAGNOSTIC"
      return 0
    fi

    # 2. The behavioral binary, 4 modes, PLAIN compile (mode_flags_of, never
    #    flags_of: the .drflags sidecar must not reach this compile — F32).
    #    An @key-pragma case's behavioral binary is inherently pragma-
    #    activated (the pragma is in-source); today all such cases agree
    #    with the demand-blind interp, and a future DIFFERENTIAL @key
    #    .batches case surfacing REFINTERP-DISAGREE is a real adjudication
    #    event, not noise.
    if ! "$REFHARNESS" "$DRC" -o "$out/behavioral_main.cpp" \
        2>"$out/harness.stderr"; then
      echo "$NAME refinterp HARNESS-FAIL"
      return 1
    fi
    for bmode in opt nodf nocf none; do
      # shellcheck disable=SC2046
      if ! "$DR" "$DRC" $(mode_flags_of "$bmode") -cpp-out "$out/gen.$bmode" \
          >"$out/drc.$bmode.log" 2>&1; then
        echo "$NAME refinterp DRC-FAIL($bmode)"
        return 1
      fi
      if ! "$CXX" -std=c++23 -I "$REPO_ROOT/include" -I "$out/gen.$bmode" \
          "$out/behavioral_main.cpp" "$out/gen.$bmode/datalog.cpp" \
          "$REPO_ROOT/lib/Runtime/Allocator.cpp" -o "$out/behavioral.$bmode.bin" \
          2>"$out/cxx.$bmode.stderr"; then
        echo "$NAME refinterp CXX-FAIL($bmode)"
        return 1
      fi
      # shellcheck disable=SC2086
      if ! timeout "$TIMEOUT" "$out/behavioral.$bmode.bin" "$batches" $pargs \
          >"$out/behavioral.$bmode" 2>"$out/run.$bmode.stderr"; then
        echo "$NAME refinterp RUN-FAIL($bmode)"
        return 1
      fi
    done

    # 3. ABI mode-invariance: 4-mode byte agreement (a split is ALWAYS a
    #    finding — codegen determinism or harness bug, never accepted).
    for bmode in nodf nocf none; do
      if ! cmp -s "$out/behavioral.opt" "$out/behavioral.$bmode"; then
        echo "$NAME refinterp BEHAVIORAL-MODE-SPLIT($bmode)"
        ri=1
      fi
    done

    # 4. The freeze referee + THE I0 GATE, against the blessed golden.
    if [ ! -f "$HERE/goldens/$NAME.behavioral.stdout" ]; then
      echo "$NAME refinterp BEHAVIORAL-MISSING"
      ri=1
    else
      if ! cmp -s "$HERE/goldens/$NAME.behavioral.stdout" \
          "$out/behavioral.opt"; then
        echo "$NAME refinterp BEHAVIORAL-DIVERGE"
        ri=1
      fi
      if ! cmp -s "$HERE/goldens/$NAME.behavioral.stdout" "$out/interp.cbf"; then
        echo "$NAME refinterp REFINTERP-DISAGREE"
        ri=1
      fi
    fi
    if [ "$ri" = 0 ]; then
      echo "$NAME refinterp OK"
    fi
    return $ri
  }

  run_crossfamily() {  # XFAM (residual iii, 2026-08-05): the oracle's
                       # published-message projection (--project-published,
                       # the DIFFERENTIAL evaluator — a fourth code-disjoint
                       # implementation) must byte-equal the FINAL block of
                       # the behavioral golden (or, for interp-only
                       # diagnostic cases, this run's live interp.cbf). A
                       # derived cross-family check; blesses NOTHING.
                       # Vacuous (empty==empty) for cases with no published
                       # #message — no exclusion list needed. Well-defined
                       # only post-F32 (plain behavioral == demand-blind
                       # oracle: both publish the full closure).
    batches="$HERE/cases/$NAME.batches"
    if [ ! -f "$batches" ]; then
      return 0
    fi
    out="$WORKROOT/$NAME/$NAME.crossfamily"
    mkdir -p "$out"
    if ! timeout "$TIMEOUT" "$ORACLE" "$DRC" "$batches" --project-published \
        >"$out/oracle_pub" 2>"$out/stderr"; then
      echo "$NAME crossfamily XFAM-FAIL"
      return 1
    fi
    ref="$HERE/goldens/$NAME.behavioral.stdout"
    if [ ! -f "$ref" ]; then
      ref="$WORKROOT/$NAME/$NAME.refinterp/interp.cbf"
    fi
    if [ ! -f "$ref" ]; then
      echo "$NAME crossfamily XFAM-NOREF"
      return 1
    fi
    awk '/^FINAL$/{f=1;next} /^QUERY /{f=0} f&&NF>0{print}' "$ref" \
        >"$out/final"
    if ! cmp -s "$out/final" "$out/oracle_pub"; then
      echo "$NAME crossfamily XFAM-DIVERGE"
      return 1
    fi
    echo "$NAME crossfamily OK"
    return 0
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
    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|negate_never_diff_1|nonascii_1|truncated_decl_1|product_in_scc_diff_1|key_wildcard_1|key_anon_1|key_dup_1|key_unknown_1)
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
  run_refinterp || st=1
  run_crossfamily || st=1
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

# ---- the parse/sema REJECT corpus (rejects/*.dr). Adopted from the ToB
# parse_errors branch (2026-08-04; data/invalid_syntax_examples, 30 cases)
# and EXPANDED for the modern surface (region-key brackets, `:-`, mutable
# algebra, pragma misuse, the reserved demand__ prefix incl. the cross-kind
# collision this corpus's expansion found+fixed). Each case must exit 1
# CLEANLY in BOTH mode extremes: rc=0 is a LOST CHECK, a timeout/signal exit
# (>=124: 134=SIGABRT assert, 139=SIGSEGV) is a CRASH finding -- either
# fails the suite. Driverless and goldenless by design (the diagnostic TEXT
# is not pinned; the CLASS pins live in each case's header comment).
# Optional rejects/<name>.drflags appends per-case compiler flags.
ls "$HERE"/rejects/*.dr 2>/dev/null | sed 's|.*/||; s|\.dr$||' \
    | grep -E "$FILTER" > "$WORKROOT/rejectlist" || true
while read -r rname; do
  rflags=""
  if [ -f "$HERE/rejects/$rname.drflags" ]; then
    rflags=$(cat "$HERE/rejects/$rname.drflags")
  fi
  for rmode in "" "-disable-dataflow-opt -disable-controlflow-opt"; do
    # shellcheck disable=SC2086
    timeout "$TIMEOUT" "$DR" "$HERE/rejects/$rname.dr" $rmode $rflags \
        >"$WORKROOT/$rname.reject.log" 2>&1
    rc=$?
    if [ $rc -ne 1 ]; then
      if [ $rc -ge 124 ]; then
        echo "$rname reject REJECT-CRASH($rc)" >> "$WORKROOT/verdicts"
      else
        echo "$rname reject REJECT-EXPECT-ERROR-GOT($rc)" >> "$WORKROOT/verdicts"
      fi
    fi
  done
done < "$WORKROOT/rejectlist"

# Verdict aggregation is a WHITELIST (F32 leg (b), 2026-08-05): any line not
# ending in an OK shape fails the suite. The former failure-token blacklist
# ('FAIL|DIVERGE|EXPECT-ERROR|MISSING|CRASH') silently dropped
# REFINTERP-DISAGREE and BEHAVIORAL-MODE-SPLIT — the I0 referee fired on
# every green run for the four differential-regime demand cases and was
# never surfaced. An unknown future token can no longer pass silently.
if grep -vE '^$| OK(-DIAGNOSTIC)?$' "$WORKROOT/verdicts" | grep -q .; then
  echo "SUITE: FAIL"
  grep -vE '^$| OK(-DIAGNOSTIC)?$' "$WORKROOT/verdicts"
  exit 1
fi
# Coverage census: a worker killed before emitting ANY verdict line is
# invisible to both blacklist and whitelist — every case must have spoken.
while read -r cname; do
  if ! grep -q "^$cname " "$WORKROOT/verdicts"; then
    echo "SUITE: FAIL"
    echo "$cname NO-VERDICT (worker died before emitting any line)"
    exit 1
  fi
done < "$WORKROOT/caselist"
echo "SUITE: PASS ($(( $(wc -l < "$WORKROOT/caselist") + $(wc -l < "$WORKROOT/rejectlist") )) cases)"
