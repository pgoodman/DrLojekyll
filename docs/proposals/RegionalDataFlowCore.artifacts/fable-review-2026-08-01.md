# RegionalDataFlowCore.md — first review (Fable, 2026-08-01)

Reviewed at tip f0c913e0 ("Revise regional dataflow architecture
proposal"). Verdict: the diagnosis is correct and demonstrably earned
from landed code; Stages A and B are unambiguously good; the
request-edge/routed-result model is the right semantic object. The
reservations concentrate in Stage C's sequencing and in three places
where the doc's silence hides a real decision. Nothing here disputes
the target architecture — these are amendments to sequencing and to
unstated defaults.

## Where the diagnosis is proven by existing code

- **F1 (two demand authorities synchronized by annotation)** is not
  hypothetical: D3.a.3 had to teach `ProxyMergedViews` (Link.cpp) to
  PRESERVE a guard annotation through the R-DUP union's
  identity-restore collapse just so the recognizer could still find
  the shape. Every optimizer change is a fresh chance for the
  annotation to detach from the shape it describes.
- **F4 (no request owner)** is already visible as workarounds: the
  multi-adornment refcounted-union pub, the V-INST-SOLE re-key on
  `(pub_table, forcing_index)`, the singleton-vs-multi guard-group
  rewire split. `RequestEdgeRelation` makes primary the thing those
  approximate.
- **F2 (physical row equality as member identity)** is the
  `agg_distinct_1` story — the advisory projection lint marks a trap
  that `MemberProjection` vs `DistinctProjection` turns into a typed
  impossibility.
- The unification is real: late subscriber, detachment, birth, fanout,
  AND the e5 P-STORE/P-DEATH divergence all collapse into one
  differential join. The "soundness by irrevocability" special case
  disappears entirely because demand becomes uniformly retractable —
  a genuine consolidation.

## Concern 1 — Stage C's oracle arrives after the demolition

Stage C lands the full lifecycle AND deletes `-demand` /
`-demand-instance`, the eqgate family, and the flat/nested
equivalence tests in one cutover, replacing them with a reference
relational interpreter (§12.2) that does not exist yet. The eqgate
tests are precisely the oracle class that caught HP-5
over-materialization. Two amendments that do NOT violate the
"no compatibility path" rule:

1. The reference interpreter is a **Stage A/B deliverable**, validated
   against the CURRENT compiler across the 180-case corpus before
   Stage C starts. Then it is a trusted referee for the cutover, not a
   thing built during it.
2. "No old/new selector" bans a runtime fork, not a test harness.
   Bless the pre-deletion compiler's behavioral outputs (final
   membership, sorted published deltas) as goldens from a TAGGED
   BINARY. The datasets survive per §12.4 anyway; the old binary as an
   external oracle through the cutover costs nothing architecturally.

## Concern 2 — what does a bound query mean when extraction is inadmissible?

§8.1 says a failed slice "remains in its current regional scope" and
the program still lowers. Concretely: the queried relation is fully
materialized in the observation root and the lease just scopes a
cursor — today's flag-off behavior plus lease bookkeeping. That is
answer-correct, but the Stage C exit criterion says "every SUPPORTED
bound query uses an exact regional request lease," and "supported" is
doing quiet load-bearing work. The doc should state explicitly:
inadmissible extraction ⇒ full materialization, never a reject — and
enumerate what today's clean diagnostics (`demand_cyclic_1` recursive
demand, `demand_recursive_content_1`, `demand_multi_adorn_allfree_1`)
become. Silent full materialization is a BEHAVIOR CHANGE from today's
rejects — arguably an improvement, but it must be a stated decision.

## Concern 3 — "legality-driven" extraction IS a cost policy

Extraction is answer-neutral; it is purely a materialization/perf
choice. `FirstStableAdmissibleChild` with no profitability input means
extract-whenever-legal — and the demand cost model work
([[demand-cost-model]]; CostModel.artifacts/measured-calibration-1.md)
establishes that message-rooted trivial shapes never pay for demand
machinery. Today that is handled by `-demand` being opt-in; §14
deletes the knob. Excluding cost MODELS from the proposal is right,
but the first planner's extract-always default should be NAMED as a
provisional policy with a known failure mode, so the later cost work
has a declared seam (the planner's candidate-selection point) rather
than rediscovering that the planner was making cost decisions all
along. Note the identity-join recognizer (§20(AV), df.ident_join)
already removed the double-join half of the machinery cost on the
supported monotone slice — the cost seam and the regional planner will
meet.

## Concern 4 — RoutedResultRelation's physical realization

The routed result's identity includes `RequestEdgeId`, so a naive
lowering materializes O(edges × results). §9 correctly leaves physical
strategy open, but the doc should name the intended FIRST realization
— which is exactly the shape D3.a.3 landed: results stored once,
per-owner routing/retraction derived from the edge relation (the
refcounted-union pub, generalized). Without that sentence the first
implementer of Stage C reads the join literally.

## Smaller notes

- "Greenfield" in the preamble overstates: §7.1 keeps the existing Rel
  differential fixpoint inside regions, and the Rel-epoch machinery
  (claim gates, commit sweeps, ingest folds, the 29-kind census, the
  V-* validators, the `.rel` dump surface) survives as the local-graph
  lowering. It is greenfield at the DEMAND layer only. §9/§11 should
  connect to the existing validator census explicitly (presumably:
  per-region local census + the new lifecycle census).
- Sunk cost, stated honestly: §14 deletes essentially everything from
  D2.b through D3.a.3 shortly after it reached its most capable state.
  That is fine — the §1 audit could only be written BECAUSE that
  machinery exists; the findings table is the receipt. What survives:
  the witnesses (rewritten against edges/leases) and the findings.

## Ranking (this reviewer's recommended order)

1. **Stage A now** — independently valuable (fixes agg identity
   semantics regardless of the cutover), lowest risk.
2. **Reference interpreter** — built and corpus-validated against the
   CURRENT compiler; the referee must precede the cutover.
3. **Stage B** — pure refactor; 180 goldens pin it byte-exact.
4. **Stage C** — with the old tagged binary held as external oracle
   through the cutover.
5. **Stage D** — honestly a separate mountain.
