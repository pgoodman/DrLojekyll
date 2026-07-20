SUPERSEDED IN PART (2026-07-19, the P1 judge round — ledger §17,
binding record p1-pinned.md): P1 LANDED at 0af322a2. THREE §2/§4
claims below are WRONG and corrected in p1-pinned.md — (1) the
-disable-dataflow-opt alias is the ENUMERATED
{df.cse,df.canon,df.dfe,df.sink}, NEVER df.* (df.simplify and
df.demand run OUTSIDE the optimize guard in all four golden modes);
(2) "~10 driver call sites" is actually 3 outer wholesale-skip
guards + 8 library-internal gate names + 2 Main.cpp consumption
sites; (3) df.demand is UN-GATED (reserved, unregistered) — -demand
is SEMANTICS and is never silently neuterable by a pass policy or
bisect limit (the P1 Fable review). §1's df.dfe = EliminateDeadFlows
ONLY (RemoveUnusedViews is REQUIRED hygiene). P2-P5 remain DRAFT.

# The cross-IR pass harness — DRAFT (2026-07-18; judge before code)

Owner directives: (i) EVERY optimization gatable from the command
line (or equivalent) — never getenv scaffolding; (ii) LLVM-inspired
diagnosis workflows — "the bug showed up here" narrowing; (iii) the
SAME flexibility powers IR-level golden masters (what does the IR
look like after pass X; is it firing; regression when it stops); (iv)
and pass-ordering speculation/fuzz (A∘B vs B∘A: same result? faster?);
(v) the system is GENERIC/SEMI-GENERIC ACROSS IR LEVELS — one
harness spanning DataFlow / ControlFlow / DeltaRel (and the codegen
boundary), not per-level one-offs.

## 0. Research digest (what we're borrowing, what we're not)

- LLVM `-opt-bisect-limit=N`: a single MONOTONE COUNTER over all
  skippable pass APPLICATIONS; every gateable action asks "may I
  run?"; applications above N are skipped; `-1` runs everything but
  prints each application with its index. Binary search over N
  narrows a miscompile to ONE pass application. Passes REQUIRED for
  correct lowering never consult the gate. (docs: llvm.org OptBisect)
- LLVM DebugCounter (`-debug-counter=<name>-skip=A,-count=B`): gates
  INDIVIDUAL TRANSFORMATION INSTANCES inside one pass (e.g. "perform
  only the 17th CSE merge") — the finer knife after bisect finds the
  pass. Bisectable automatically.
- `-print-before/after=<pass>`, `-print-after-all`, `-print-changed`:
  dump IR at pass boundaries; print-changed only when the text
  differs — locates "suspect IR introduced here" by reading, without
  bisection reruns.
- llvm-reduce / bugpoint: INPUT reduction against an interestingness
  test — orthogonal to pass narrowing; shrink the program while the
  bug reproduces. (Residue here; our corpus cases are already small.)
- MLIR (the cross-level model): ONE pass manager over multiple
  dialects/levels, a TEXTUAL pipeline spec, uniform print-ir-after
  hooks, crash reproducers capturing (input, pipeline). This is the
  "generic across IR levels" shape the owner asked for.

NOT borrowed: LLVM's pass-plugin machinery, analysis-manager
invalidation, or per-function granularity — hyde compiles one module
through a fixed spine; the harness gates and observes, it does not
need dynamic scheduling.

## 1. The pass inventory (what becomes nameable)

Namespaced dotted names, stable across releases (they become CLI
surface and golden-sidecar vocabulary):

  DataFlow (QueryImpl::Optimize + neighbors, lib/DataFlow/):
    df.simplify        Simplify() (SELECT-CSE + 2× JOIN canon + prune)
    df.canon           Canonicalize() fixpoint driver (per-round
                       gateable application; see §2 bisect note)
    df.cse             the CSE() fixpoint
    df.dfe             dead-flow elimination / RemoveUnusedViews
    df.sink            union sinking (currently commented out —
                       registering it is how it comes back testable)
    df.demand          ApplyDemandTransform (already CLI-gated by
                       -demand; joins the registry as a peer)
  ControlFlow (ProgramImpl::Optimize, lib/ControlFlow/Optimize.cpp):
    cf.regionopt       the interleaved per-region-kind fixpoint
                       (PARALLEL/INDUCTION/SERIES/LET/TUPLECMP/CALL/
                       GENERATOR/UPDATECOUNT/CHANGERECORD/CLAIM/
                       CHECKMEMBER/CHECKRECORD OptimizeImpl overloads
                       — ONE pass with PER-KIND DEBUG COUNTERS, not
                       13 passes: the loop is a joint fixpoint, the
                       kinds are not independently orderable)
    cf.procdedup       procedure deduplication
  DeltaRel (lib/DeltaRel/ post-rename): the checked stages are NOT
    optimizations (derive-strata, linearize, validate, census are
    REQUIRED) — they register as OBSERVATION POINTS (print-after
    targets), never as skippable passes.
  Codegen: an observation point (the .h emission), never skippable.

REQUIRED vs OPTIONAL is a first-class attribute (the LLVM rule):
build steps, ConnectInsertsToSelects, stratification, DeltaRel
derivation/validation, codegen never consult the gate. Only OPTIONAL
passes are bisectable/skippable. The always-on V-* validators and the
suite's oracle/answer gates referee every configuration — a config
that produces invalid flow ABORTS loudly (validators survive NDEBUG),
it never silently miscompiles.

## 2. The flag surface (one harness, all levels)

  -opt-disable=<glob>[,<glob>]   skip passes by name (df.cse, cf.*)
  -opt-only=<glob>[,...]         run only these OPTIONAL passes
      (the legacy -disable-dataflow-opt / -disable-controlflow-opt
      become exact aliases of -opt-disable=df.* / -opt-disable=cf.* —
      the 4 golden modes are UNCHANGED, now expressed in the new
      vocabulary)
  -opt-bisect-limit=N            the global cross-level application
      counter; -1 = run-all-and-print-indices. An "application" =
      one gateable invocation: one df.canon ROUND, one df.cse run,
      one cf.regionopt round, df.demand once. Deterministic because
      the compiler is single-threaded and (post-(F)) iteration is
      id-ordered — the same input yields the same index sequence,
      which is what makes binary search sound.
  -opt-counter=<name>:skip=A:count=B   DebugCounter analog, gating
      individual TRANSFORMATION INSTANCES: first counters at
      df.cse.merge (each view-pair merge), df.canon.<nodekind>
      (each per-node rewrite that reports a change), and
      cf.regionopt.<regionkind> (each region rewrite). This is the
      "narrow to the exact rewrite" knife.
  -print-after=<name|all> -print-dir=<DIR>   dump the OWNING LEVEL's
      textual form after each named application into DIR with
      deterministic names (<seq>.<pass>.<level>.txt): DataFlow → the
      -df-out BB form, ControlFlow → the .ir form, DeltaRel →
      -deltarel-out form. -print-changed variant: emit only when the
      dump's hash differs from the previous dump AT THE SAME LEVEL
      (the "it showed up HERE" reading tool).
  -opt-stats[=<PATH>]            deterministic per-pass fired/changed
      counts (one line per registered pass, id-ordered). The cheap
      "did it stop firing" regression signal — goldenable.
  -passes=<spec>                 (stage P4) textual pipeline override
      within the legal region, e.g.
      df:[simplify,fixpoint(canon),cse,dfe] — enables A∘B vs B∘A
      speculation and ordering fuzz. -print-pipeline emits the
      default spec. REQUIRED stages are implicit and immovable; the
      spec orders only OPTIONAL passes.

Mechanism: a single PassPolicy object (parsed from CLI in Main.cpp)
threaded through Query::Build / Program::Build alongside the existing
`optimize` booleans (which it subsumes); each registered pass site
calls policy.Gate(PassId, unit_desc) — one line per site. No
globals beyond the registry table; no getenv anywhere (house law:
no env-gated scaffolding).

## 3. What each workflow looks like afterwards

  "A golden went red after my change — where?"
    1. -opt-bisect-limit binary search over the failing case (the
       suite's diffrun is the oracle) → pass application #k.
    2. -opt-counter on that pass → the exact merge/rewrite instance.
    3. -print-after/-print-changed at the boundary → read the two
       dumps; the diff IS the bug report. Minutes, no recompiles of
       the compiler, no scaffolding commits.
    ((F) postscript: bisect + print-changed over cf14_1 would have
    pinned the induction-vector permutation to the induction-build
    boundary in one session — this harness is the generalization of
    what the (F) hunt did by hand.)
  "Is optimization X working? Did it regress?"
    A pass-point IR golden: sidecar pins (mode, level, after=<pass>)
    and the harness byte-compares the dump — plus the -opt-stats
    golden asserting X fired N times on the witness. Both bless-only.
  "What if we reorder A and B?"
    -passes specs A∘B / B∘A; referee stack: oracle answer-identity
    (must), final-IR hash agreement map (report), suite stdout
    goldens (must), Q5 ABABAB timing (report). A fuzz driver
    permutes specs across the corpus overnight; divergences file as
    FINDINGS candidates with the two pipeline specs as the repro.

## 4. Staging (each stage lands with the standing gates)

  P1  Registry + PassPolicy + -opt-disable/-opt-only + legacy-alias
      rewiring + -opt-bisect-limit at the ~10 driver call sites.
      Byte-identity: default config is the current pipeline exactly.
  P2  -print-after/-print-changed/-print-dir wired to the three
      dumps (depends on the dump diffs landing; DeltaRel dump = an
      observation point at validate-exit).
  P3  -opt-counter instances (df.cse.merge first), -opt-stats.
  P4  -passes spec + -print-pipeline + the ordering-fuzz driver
      (scripts/, corpus-wide, oracle-refereed).
  P5  (residue) the .dr reducer (llvm-reduce analog, interestingness
      = a diffrun predicate).

Prerequisite for P2+: the (F) fix (dumps must be deterministic for
print-changed/hashing to mean anything) — which is already the
epoch's do-first diff.

## 5. Predictions (pre-registered, per house method)

  - P1 is emission-neutral by construction (default = current
    pipeline); suite 168 zero churn, byte-identical binaries modulo
    the new flag parsing. Any churn = a P1 bug.
  - The 4 golden modes re-expressed as -opt-disable aliases produce
    BYTE-IDENTICAL suite results vs the legacy flags.
  - First real catch: expected in the df.canon fixpoint (per-round
    bisect exposes round-order-sensitive canonicalizations, the F20
    latent-comparator class).
