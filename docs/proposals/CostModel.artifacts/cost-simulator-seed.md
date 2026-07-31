# COST-MODEL SEED — the NUMERIC SIMULATOR (whole-program pseudocode + path-forward-as-diffs)

> **Status.** Written 2026-07-31 by ONE session (the cost-model-epoch open), SINGLE-PASS,
> NO fleet — the next session's fleet MUST re-derive + verify this from code before building.
> Supersedes the SYMBOLIC framing of `CostModel.md` §1–§3 (owner steer 2026-07-31: numbers,
> not symbols; "how are you evaluating these / is it make-believe"). The reshape rationale +
> the first measured calibration are in [[measured-calibration-1]]; the scenario-family
> principle in [[cost-scenario-family]]; the methodology in [[predict-then-verify-ir]].
> This seed is the whole-program view for steps 2 (reshape CostModel.md) + 3 (build the tool).

===============================================================================
## §1 WHAT LANDED (the ground the next session stands on)

- **The recognizer + Prov substrate are LANDED** (this session, committed): `lib/DataFlow/
  Prov.{h,cpp}` (keyset value-containment provenance + always-on V-PROV-*), `lib/DataFlow/
  IdentityJoin.cpp` (the identity-join drop, gate `df.ident_join`). Prov is REUSABLE by the
  cost model's L1. See `prov-recognizer-impl.md`.
- **The double-join is grounded + fixed + MEASURED**: `grounding-double-join.md` (normal=0
  joins → -demand=2, step-8 a provable identity), `measured-calibration-1.md` (dropping it =
  ~45% fewer hash ops/probe, the law **ΔidxAdds = F·K**, measured via bench counters).
- **The ground truth exists**: `include/drlojekyll/Runtime/BenchCounters.h` — a process-global
  `gBenchCounters` with fields `finds / probe_steps / idx_first / idx_hops / idx_adds /
  member_checks / present_checks / folds_plus / commit_visits / commit_publishes / touch_calls
  / claims_* / netbatch_* / …`. Built with `-DDRLOJEKYLL_BENCH_COUNTERS`. A driver snapshots
  it around an epoch (delta = after − before). THIS is what the cost model predicts.
- **The `.rel` (DR-IR) is the per-config cost artifact**: `-rel-out` emits typed ops with a
  census + (per access) a committed `Lowering ∈ {kPointTest, kSectionWalk, kFullScan, kSeek}`
  (`lib/Rel/Rel.h:483`, `PlanNode.lowering` + `bound_cols` keyed on GetOrCreateIndex identity).
  The op vocabulary that costs work: `kEagerJoin`/`kJoinEmit` (table joins), `kEagerInsert`/
  `kIngestFold` (materialize), `kEagerForward`/`kEagerSelect` (cheap), `kFixpointFire`/
  `kChainFold` (fixpoint rounds), `kCommitSweep`, `kNegateGate`, `kGroupUpdate`.

===============================================================================
## §2 THE NUMERIC SIMULATOR — WHOLE-PROGRAM PSEUDOCODE

The cost model is NOT symbolic algebra. It is a two-pass numeric SIMULATION over the real IR,
per SCENARIO, whose predictions are VALIDATED against `gBenchCounters`.

    cost(program, scenario):                       # scenario = concrete input data / distribution
      # ---- L1: CARDINALITY (concrete row counts per view), over the Query graph ----
      card = {}                                     # view -> rows, keyed by (view, demand-key slice)
      for view in QueryGraph in depth order:
        card[view] = L1_rule(view, card, scenario)  # §2.1; uses Prov for demand-key slices
      # ---- L2: WORK (predicted operation counts), over the Rel/DR-IR ops ----
      pred = ZeroCounters()                         # same fields as gBenchCounters
      for op in RelFlow:                            # the .rel ops, in schedule order
        pred += L2_op_cost(op, card, scenario)      # §2.2; reads op.Lowering + card
      return pred                                   # a predicted BenchCounters

    calibrate(program, scenario):                   # the ACTIONABILITY (predict-then-verify)
      pred     = cost(program, scenario)
      measured = run_with_counters(program, scenario)   # compile -DDRLOJEKYLL_BENCH_COUNTERS, snapshot
      assert close(pred, measured)                  # V-COST-CALIB; loud fail if the model is wrong

    judge(program, optimization):                   # the "is it worth it / does it explode" verdict
      for scenario in SCENARIO_FAMILY:              # [[cost-scenario-family]] — NOT one workload
        on  = cost(program@opt_on,  scenario)
        off = cost(program@opt_off, scenario)
        report(scenario, on, off)                   # per-scenario delta
      report_spread()                               # good ONLY if it wins/ties in EVERY scenario

### §2.1 L1 cardinality rules (concrete numbers, over Query.h nodes)
    SELECT(msg)      : card = scenario.size(msg)                         # a base input
    SELECT(demand d) : card = scenario.demanded_keys(d)                 # D distinct keys
    TUPLE/CMP filter : card = card(in) * scenario.selectivity(view)     # filter shrinks
    JOIN R⋈S on A    : card = est_join(card(R), card(S), scenario.fanout(A))   # matches
                       # demand slices: use Prov to bound R to the demanded key set (join.6 shape)
    MERGE            : card = sum(card(arms))                            # union (set-dedup optional)
    MAP (F free)     : card = card(in)                                  # 1:1 (functor per row)
    NEGATE/AGG/KV    : card = est_* (regroup / anti-join)               # per algebra
    # fixpoint (inductive MERGE): card = closure size Z from scenario (sampled, not derived)

### §2.2 L2 op-cost rules (predicted counter deltas, over Rel.h ops)
    kIngestFold(t)   : pred.idx_adds += card(t) ; pred.finds += card(t)      # dedup+insert per row
    kEagerInsert(t)  : pred.idx_adds += card(t) ; pred.finds += card(t)      # <-- ΔidxAdds=F·K lives here
    kJoinEmit(join)  : # per driven row: an index probe into the joined table
                       pred.idx_first += card(driver) ;
                       pred.idx_hops  += card(join_output) ;                 # First+Next chain
                       pred.finds     += card(driver)
    kEagerForward/Select : ~0 (pointer rebinds)
    kEagerCompare    : pred.member/present_checks += card(in) * (Lowering cost)
    kFixpointFire/kChainFold : per round r in 1..R: the seed/frontier work at card(frontier_r)
    kCommitSweep(t)  : pred.commit_visits += touched(t) ; pred.commit_publishes += changed(t)
    # EVERY access multiplies by its Lowering (Rel.h:483):
    #   kPointTest=1  kSectionWalk=matches(f)  kFullScan=card(table)  kSeek=log(card)
    # a probe→scan regression (p1) shows here as base 1 -> base card(table).

**Calibration anchor (measured, must reproduce):** dropping join.7 removes one `kEagerInsert`
into the intermediate table (F rows/probe) ⇒ `ΔidxAdds = F·K`. The L2 `kEagerInsert` rule
above predicts exactly this. That is the first V-COST-CALIB golden.

===============================================================================
## §3 THE PATH FORWARD AS DIFFS

    r0  RESHAPE CostModel.md (step 2): DELETE the symbolic semiring / polynomial-normalizer
    -   §1 "Cardinality C ... commutative semiring ... lfp"      # symbolic algebra
    -   §3.4 V-COST-XCHECK "complete polynomial normalizer"      # the normalizer risk
    +   §1 concrete numeric cardinality per (view, scenario)     # numbers
    +   §2.2 L2 op-cost -> gBenchCounters FIELDS (the mapping above)
    +   §3.4 V-COST-CALIB: predicted BenchCounters ≈ measured gBenchCounters (the real check)
        Keep: §0 two-layer (L1 Query cardinality / L2 Rel work) — STILL RIGHT, just numeric.
        Keep: §4 scenario FAMILY. Keep: §2.8 spine/Lowering (that IS the access-path/index answer).

    r1  bin/Cost skeleton (drlojekyll-cost, peer of bin/Oracle). THE SEAM IS A DESIGN FORK
        (grounded, RESOLVE FIRST): bin/Oracle LINKS the internal libs and builds the Query
        graph IN-MEMORY via the PUBLIC `Query::Build(module,log,optimize)` (bin/CMakeLists.txt:39;
        bin/Oracle/Main.cpp:7). So:
        - L1 IN-MEMORY is the clean choice: bin/Cost links internal lib/DataFlow, calls
          Query::Build, and REUSES Prov DIRECTLY (ComputeColumnProvenance, on QueryImpl) — no
          re-implementation, no dump-parsing. (Caveat: Prov is on the INTERNAL QueryViewImpl,
          so bin/Cost includes lib/DataFlow/Query.h, exactly as the compiler does.)
        - L2/Rel HAS NO in-memory public accessor — the ONLY public seam is SetRelDumpStream
          (ControlFlow/Format.h:17), a TEXT sink. So L2 EITHER (a) parses the `-rel-out` dump,
          OR (b) adds a small lib/Rel accessor to hand the built Rel graph to the tool. Fork
          for the fleet: (a) decoupled but dump-format-fragile; (b) a tiny internal surface but
          in-memory Lowering/ops. RECOMMEND (b) if the Rel graph outlives Program::Build, else
          (a). Do NOT assume offline.

    r2  L1 cardinality pass (§2.1) over the in-memory Query graph, REUSING Prov for demand-key
        slices (the containment reasoning is already implemented — this is the payoff of the
        shared substrate).

    r3  L2 op-cost pass (§2.2) over the Rel ops (in-memory per r1(b), or the dump per r1(a)),
        reading each op's Lowering (Rel.h:483). Emit a predicted BenchCounters per (scenario, config).

    r4  THE CALIBRATION HARNESS (the actionability, r4 is the POINT): compile the case
        `-DDRLOJEKYLL_BENCH_COUNTERS`, run a scenario driver, snapshot gBenchCounters, and
        V-COST-CALIB predicted≈measured. First golden: mono `ΔidxAdds = F·K`. (The measure.cpp
        in scratchpad/benchmeasure is the prototype driver.)

    r5  SCENARIO FAMILY + goldens (§2.1 of CostModel.md as amended): 3 witnesses
        (mono double-join; a shallow_sparse where demand LOSES; a deep_dense_closure / tc where
        it WINS — the r0-recursive-demand gate's home). Pin per-scenario predicted+measured.

    r6  (LATER) wire judge() into the recursive-demand r0 gate: nested `D·Z_key` vs flat `Z_S^d`
        across the scenario family — no recursive-demand lowering lands without passing it.

===============================================================================
## §4 ANCHORS (next session: re-verify at code before building)
1. include/drlojekyll/Runtime/BenchCounters.h — the counter fields (the prediction target).
2. lib/Rel/Rel.h:470/483 — PlanKind + Lowering (the access-path the L2 spine reads).
3. The `.rel` census line + op list (`-rel-out`) — the L2 op vocabulary + counts.
4. lib/DataFlow/Prov.{h,cpp} — the keyset reasoning L1 reuses.
5. scratchpad/benchmeasure/measure.cpp — the calibration driver prototype (ΔidxAdds=F·K).
6. bin/Oracle/Main.cpp:7 + bin/CMakeLists.txt:39 — the peer-tool pattern: LINKS internal libs,
   builds the Query graph IN-MEMORY via public Query::Build (so Prov is reusable in-process).
   include/drlojekyll/ControlFlow/Format.h:17 SetRelDumpStream — the ONLY public Rel seam (text);
   the L1-in-memory / L2-dump-or-accessor fork (r1) hangs off this.
7. docs/proposals/CostModel.md §0/§2.8/§4 — what to KEEP; §1–§3 symbolic — what to RESHAPE.
8. measured-calibration-1.md — the reshape rationale + the first calibration law.
