# Verification Record — CostModel session 2026-07-31 (tip after `61b17a15`)

Branch `keyed-instances`. All commands run from the repo root. Build and test
were kept sequential per operating rule 6; no test ran against a tree while it
was being rebuilt.

## Gates run (with results)

| # | Command | Result |
|---|---|---|
| 1 | `cmake --build --preset debug` (clean, pre-edit) | exit 0 |
| 2 | `ctest --test-dir build/debug --output-on-failure` (pre-edit) | 6/6 passed |
| 3 | `cmake --build --preset debug` (after catch-all removal + canon cap-abort) | exit 0 |
| 4 | `ctest` Debug (post-edit) | 6/6 passed |
| 5 | `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh <work> 4` (post-edit) | **SUITE: PASS (181 cases)** |
| 6 | `cmake --build --preset release` (after catch-all fix) | exit 0 |
| 7 | `ctest` Release (after catch-all fix) | 6/6 passed |
| 8 | canonicalization exit-reason sweep over 181 cases (temporary instrumentation) | 519 Canonicalize calls: 423 converged, 96 cycle-band-aid, **0 cap** |
| 9 | identity-join on/off structural delta, all 4 modes (`-df-out`/`-rel-out`) | opt/nocf=1 join, nodf/none=2 joins (see calibration-slice.md) |
| 10 | `docs/proposals/CostModel.artifacts/calib/runcalib.sh` | **CALIB: PASS** (all sweep + negative cases) |
| 11 | `docs/proposals/CostModel.artifacts/regen-mono-artifacts.sh` | regenerated normal/on/off; joins 0/1/2 |

The Release gate (6, 7) is the one that specifically validates the catch-all
removal: release calibration is only trustworthy once the `NDEBUG`-silent
`catch(...)` continuation is gone, and Release CTest passes with it gone.

Follow-up after gate 5: a `num_views &&` guard was added to the cap-abort so an
empty graph (`max_iters == 0`, loop never entered) cannot abort spuriously. This
strictly NARROWS the abort condition and changes no path for any corpus program
(every corpus program has `num_views ≥ 8`), so the full-suite result stands; it
was re-verified by Debug CTest (6/6) and the calib runner (CALIB: PASS) rather
than a second full-suite run.

## Gates NOT run (explicit)

- **Sanitizer (ASAN) tree.** No ASAN build tree was present/configured; not run.
  The touched paths (`Build.cpp` exception-handler removal, an `abort()` in
  `Optimize.cpp`) add no new allocation/lifetime surface, but this is stated as
  un-run, not assumed-clean.
- **Full OptDiff under Release.** Only Debug ran the 181-case suite; the suite
  harness targets the Debug compiler. Release was validated by CTest only.
- **libFuzzer / fuzz workflow.** Still inert (missing `BackendFuzzer.cpp`, dead
  parser round-trip). Untouched this session; see test-coverage-gaps.md.
- **The canonicalization cap-abort's failure path.** No program in the corpus
  reaches the cap (0/519), so the `abort()` branch is not exercised by any test.
  It is a tripwire for out-of-corpus non-termination, not a covered branch. A
  directed test would need a constructed non-terminating rewrite, which does not
  exist today.
- **Static cost predictor.** Not built (deferred behind the Rel decision); there
  is therefore no predictor gate to run.

## Reproduce the measured law from a clean tree

```sh
cmake --build --preset debug
docs/proposals/CostModel.artifacts/calib/runcalib.sh   # -> CALIB: PASS
```

Saved output: `docs/proposals/CostModel.artifacts/calib/last-run.txt`.

## Source changes this session

- `lib/DataFlow/Build.cpp`: deleted the `try { … } catch(...) { assert(false); … }`
  wrapper around Optimize/finalization (findings #13); statements retained.
- `lib/DataFlow/Optimize.cpp`: promoted the canonicalization iteration-cap exit
  to an always-on `fprintf`+`abort()` (findings #14); added `<cstdio>`/
  `<cinttypes>`; kept the cyclic band-aid with a corrected comment. The
  temporary exit-reason instrumentation was removed after the measurement.
- `docs/proposals/CostModel.artifacts/`: new `calib/` (driver + runner +
  last-run), `regen-mono-artifacts.sh`, renamed mono artifacts, and the audit
  decision/record files. No golden was blessed; no commit was made.
