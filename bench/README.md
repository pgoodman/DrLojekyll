# bench/ — the COST instrument

Performance harness for the generated incremental databases (PerfRoadmap.md
§2; methodology + first-run caveats in [BASELINE.md](BASELINE.md)). NEVER
part of correctness CI: the OptDiff suite stays the correctness net, perf
runs never gate it, and goldens never change from here.

## Layout

    common/bench.h          driver kit: TSV schema, knob parsing, timing,
                            peak-RSS, counting allocator, FNV sentinel hash
    workloads/<family>/     one .dr program + one driver per family
                            (drivers use the hidden-friend surface)
    baselines/              hand-written competitors (COST honesty):
                            from-scratch + incremental per family where a
                            competent hand implementation exists
    runbench.sh             manifest-driven runner; compiles per (family,
                            mode), runs per (knob-point, rep), folds only
                            complete fragments, records Q5 compile metrics
    BASELINE.md             methodology + the accepted-run ledger

## Running

    export DR=build/debug/bin/drlojekyll
    bench/runbench.sh /tmp/benchwork runspec.txt "opt nodf nocf none"

runspec lines: `engine <family> <case.dr relpath> <k=v ...>`,
`baseline <name> <k=v ...>`, `progsize <rules> ...`. See runbench.sh's
header comment for REPS/CTIMEOUT/RTIMEOUT/COUNTS.

Never rebuild the compiler mid-run; never time bench runs concurrently
with suite runs or each other.

## Discipline (the short version; BASELINE.md is normative)

- Timing binaries are `-O2 -DNDEBUG`, seam OFF. Counts binaries add
  `-DDRLOJEKYLL_BENCH_COUNTERS`; their wall is discarded. Wall and counts
  are separate narratives — never multiply them.
- Comparisons key on (case, mode, knobs) SEMANTICS, never generated-text
  hashes (emitted text is not run-stable; PerfRoadmap §1).
- Every run ends with a `run_complete` TSV row; runbench folds only
  complete fragments and manifests every exit code. A mid-run abort
  (counter saturation, row-id exhaustion) means the knob-point is too
  large: back off and record, never trim silently.
- Sentinels: every binary emits `final_*_count`/`final_*_hash`; runbench
  fails on intra-name (cross-mode/cross-rep) disagreement. Engine-vs-
  baseline agreement at shared stream knobs is checked in analysis (the
  knob strings differ by measurement-only keys); a disagreement is a
  FINDINGS-class event.
- The counter seam's no-op-when-off gates (object-file byte-compare,
  suite PASS, on-path golden check) are recorded in the seam commit
  message; re-verify after any Runtime header change.
