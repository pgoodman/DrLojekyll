# The post-K5 architecture — whole-program pseudocode + the path forward as diffs (session-8 close, 2026-08-04)

Purpose: ground the NEXT session in a whole-program view at the session-8
close (K5 Tier-2 origin provenance LANDED, impl `81385a92`). SEED
discipline: single pass, orchestrator-authored from full session context —
the next session MUST fleet-re-verify every anchor before building
(functions/files exact, line numbers approximate at this grain).
ONE-AUTHORITY RULE: the normative design record is `region-model-diffs.md`
(RP-1..RP-10 + THE SUBGRAPH-AUTHORITY FRAMING); the K5 working authority is
`k5-provenance.md` (Parts A-E + ADJUDICATIONS + the 17-finding panel record
— Part A's 23-site replacement census and Part E's padding-computed
byte-diff method are reusable instruments); the K1/K6 working authorities
are `k1-multikey.md` / `k6-riders.md`; `k6-landed-seed.md` (s7) is
SUPERSEDED by THIS seed at the whole-program grain.

---

## Part 1 — the LANDED pipeline (pseudocode, post-K5)

```
CompileModule(module):                          # bin/drlojekyll/Main.cpp
  # ---- PARSE (lib/Lex, lib/Parse) ----
  #   `@key` lexes kPragmaKey. ParseLocalExport (Parser.cpp, ONE template
  #   for #local+#export), case 8 pragma tail -> state 21 -> state 22
  #   (arg list, IMMEDIATE resolution; per-pragma rejects; at each `)`:
  #   ADJ-K1-A order-free duplicate-SET reject, set + DisplayRange pushed;
  #   a SECOND @key re-enters state 21 — RP-10 multi-set).
  #   storage: instance_key_param_index_sets + parallel instance_key_ranges
  #   (ParsedDeclarationImpl); accessors HasInstanceKey()/InstanceKeys()/
  #   InstanceKeyRanges() resolve THROUGH the declaration context (first
  #   key-bearing redeclaration wins — F-K6-SHADOW dead).
  #   FinalizeDeclAndCheckConsistency: prev_decl = redecls[n-2u] (F31
  #   FIXED — the revived type/name divergence checks are LIVE) + the
  #   IDENTICAL-OR-ABSENT instance-key check on the revived site.
  #   decl.Id() is DOMINATED by the STRING-POOL INTERN OFFSET of the name
  #   (atom_name_id = IdentifierId(), first-occurrence interning with
  #   NUL-suffix aliasing: `edge` aliases the tail of `add_edge\0`) —
  #   NEVER assume declaration order (K5P-det-1; Tier-2 contract order
  #   and the tc pin E1=edge/E2=tc hang on this).

  query := Query::Build(module, log, policy, demand_mode, demand_retract,
                        suppress_demand=false)   # lib/DataFlow/Build.cpp
    proxy_view_to_decl := {}                     # Build-SCOPED local
    ConnectInsertsToSelects(log, proxy_view_to_decl)  # lib/DataFlow/Connect.cpp
      for REL *rel : relations:
        insert_proxy := CreateProxyForMutableParams(
                          CreateProxyOfInserts(rel->inserts), decl)
          # ALWAYS a MERGE on the rel->inserts path (the has_one_insert
          # dead branch: Swap empties `inserts` BEFORE the Size() read,
          # Connect.cpp:14-17 — K5-D8 RIDER-1, latent, K5-immune) |
          # TUPLE-over-KVINDEX (mutable arm).
        proxy_view_to_decl.emplace(insert_proxy, rel->declaration)  # T1 stamp
        if !decl.IsInline() && !decl.IsQuery():                     # K5 SEED
          assert(insert_proxy->origin_decls.empty())                # seed-once
          insert_proxy->origin_decls = { rel->declaration }
          # IsInline() SUBSUMES IsQuery() at tip (Parse.cpp) — the
          # !IsQuery() conjunct is documented forward-looking hygiene.
        rel->inserts.Clear()                     # severs REL->proxy
        ProxySelects(...); if decl.IsQuery(): re-mint terminal INSERT
    ApplyDemandTransform(...)                    # lib/DataFlow/Demand.cpp
      if suppress_demand: return true            # bin/Oracle passes true
      # ACTIVATION GATE (RP-6): parsed-module scan, HasInstanceKey()
      #   context-resolving; !demand_mode && !pragma_activated -> return.
      # reject() advice + the three demand__ collision rejects fork on
      #   pragma_activated (K6-1).
      Step 1: bound queries; unseeded-@key reject; R-1BOUND.
      Loop 1 per adornment (UniqueRedeclarations + seen_variants): trace
        p_bound, locate guards, run EVERY fence.
      post-Loop-1: p_demanded_decl via proxy_view_to_decl (T1-DECL-MISS);
        RP-6 realization reject.
      Step 2b (V-DECLARED-KEY, RP-10): two-arm TOTAL BIJECTION, declared
        set-of-sets == inferred, order-free; Arm A pragma-anchored (K6-3).
      Step 4 stray-consumer union; Loop 2 mints RecognizedSubgraph{...,
        demanded_decl} per forcing.
    Optimize                                     # lib/DataFlow/Optimize.cpp
      # EVERY view-replacement routes (or bypasses) ONE choke point:
      QueryViewImpl::CopyDifferentialAndGroupIdsTo(that)   # View.cpp
        that->group_ids += this->group_ids; sort
        that->origin_decls ∪= this->origin_decls           # K5 UNION
          # loser→survivor, sort+unique by decl.Id(), NO clear-on-this
          # (a SET, not a counted scalar — contrast guard_annotation_
          # index's clear). Tigerstyle pair: is_sorted AND adjacent_
          # find==end (is_sorted alone tolerates adjacent equals under a
          # strict comparator — the s8 referee-pass fix). NEVER in
          # Hash/Equals, NEVER a lowering input: a missed union
          # UNDER-NAMES, never miscompiles. FIVE documented bypass gaps
          # (KV wrap, CMP-sink replicas x2, Merge unused-col guard,
          # ProxyInsertWithTuple) — accepted, closed per real consumer.
        that->can_receive_deletions |= ...       # LOCKSTEP — the
          # differentialness-migration invariant that makes Tier-2
          # support= sound (carriers of one decl agree on CRD).
    Stratify; InferConservativeRowContracts
    #ifndef NDEBUG:                              # K5 conservation belt
      every RecognizedSubgraph.demanded_decl origin-reachable at a live
      view (RESCOPED to the demanded-interior class; owner overruled the
      panel's delete — the TIGERSTYLE standing directive; redundant-with-
      Tier-1 defense-in-depth).
    # -contract-out: per-set `declared-key` lines (K1). -origin-out (K5,
    # NEW): advisory per-view dump, header `origin-sets`, rows ordered
    # (min decl.Id() in set, det_seq tie-break), decls in stored Id
    # order; drained right after Query::Build; NEVER goldened;
    # run_irgold smoke-PRODUCES it every .irgold case (crash = red).

  frozen := FrozenRegionalProgram::Build(query, log)   # lib/Regional/Planning.cpp
    contracts := CollectContractInserts(query)         # R-STORE (queries+inserts)
               ++ CollectDemandInteriorDecls(query)    # Tier-1 (demanded_decl)
               ++ CollectOriginInteriorDecls(query)    # K5 TIER-2: the residue
      # outside insert-named ∪ Tier-1, from live views' OriginDecls(),
      # ascending decl.Id(); member-key = AllParamNames positional;
      # support= ResolveOriginSupport (OR over live carriers' CRD,
      # loud-abort belt + DEBUG support-agreement assert); negative-space
      # asserts (never inline/query/insert-named/Tier-1).
    census.row_contracts = |R-STORE| + |Tier-1| + |Tier-2|   # same-collector:
      # DeriveRegionalCensus and the build loop call the SAME functions —
      # V-REGION-CENSUS green by construction.
    # RENDER LAW (E-K5-PAD): the region emitter's member-key column width
    # is a PER-DUMP MAX (Regional/Format.cpp Pad) — a new longest
    # member-key RE-PADS existing lines. Region-golden predictions must
    # COMPUTE padding, never assume byte-additivity (join_1's E0/E1
    # gained 3 interior spaces at K5).

  program := Program::Build(frozen, log, first_id, policy, demand_instance)
    query = frozen.Query()                       # H4 thin seam
    # per-forcing admissibility; -demand-instance strict rejects vs the
    # @key pragma arm (effective_demand_instance, ADJ-K1-D assert);
    # nested BuildSubgraphInstanceOps per forcing | SILENT flat fallback.
    BuildDataModel/FillDataModel                 # ControlFlow — AFTER freeze;
      # EquivalenceSet drives model sharing (merge_2's `outer` UNION
      # model-shares q_outer's table — "no owned table" != "unstored").

  # DOT twins (advisory, never goldened): -dot-out, -region-dot-out
  # (declared-key badge), -rel-dot-out (K6-7b).

  # SEEDING: both lowerings still publish the fabricated demand__ message
  # through the SYNTHESIZED INJECTOR — the seam Stage C deletes.
```

REFEREE STACK at tip: **250 = 203 golden cases + 47 rejects**; ctest 7/7;
eqgate 6; the `.region` golden family is **20 = 5 cases × 4 modes**
(demand_tc_witness [+key_tc symlinks], join_1, merge_2,
demand_multi_adorn_witness, tc_nonlinear_diff — the K5 fold-migration
witness whose load-bearing byte is `support=differential`, not line
existence). The K5 union referee is MODE-PARTIAL by design (frozen region
output is a pure function of the DATAFLOW-opt toggle; nodf/none reach
every decl via the seed alone — a deleted union reddens only the opt/nocf
pins; K5P-oracle-1/2). Predict-then-verify record: all 12 K5 golden deltas
matched Part E byte-for-byte on first implementation; the orchestrator
referee pass hardened one assert (asserts get refereed too).

## Part 2 — the path forward as DIFFS on Part 1

- **DIFF-NEXT-S2 (K4, key-subset covering-array fuzz arm).** A diff on
  the PARSE + Step-2b pseudocode: generated placements over (relation x
  key-subset x redeclaration), verdict = accept-iff-exact-match +
  clean-reject-otherwise + NEVER abort; feeds rejects/. The oracle must
  model the K6-WIDENED reject surface: the F31-revived type/name checks,
  IDENTICAL-OR-ABSENT cross-redecl keys, the RP-10 bijection arms, the
  parse-time dup-SET reject, and the decl.Id() intern-order fact. The
  has_one_insert dead branch (RIDER-1) is a candidate fuzz-adjacent
  one-line fix. Small, independent.
- **DIFF-NEXT-S3 (K3, the Stage-C re-brief — THE OWNERSHIP FLIP).** The
  diff deletes Part 1's last block: request edges + instance lifecycle
  first-class in the frozen regional program, the SYNTHESIZED INJECTOR
  seam dies, RecognizedSubgraph demotes to a walk->extraction handoff.
  Normative input: region-model-diffs.md THE SUBGRAPH-AUTHORITY FRAMING.
  K5's origin sets are now AVAILABLE INPUT for Stage-C demand-area member
  naming (the consumer k5-provenance.md B5.2 anticipated). BLOCKED on:
  the D2.6 STOP (readers hand onto the REGION or the STORE?), the
  §6-vs-§11 routing rule, and folding the DIFF-R1 panel's 15 amendments
  into stage-c-diff.md. DESIGN-ONLY until answered.
- **DIFF-NEXT-S4 (residual riders — SWEPT session 9, 2026-08-05).**
  (i) the request-port `declared` badge: DROPPED (owner-ratified s9). The
  badge inherently splits the activation-equivalence twins —
  key_tc_witness's four `.region` goldens are SYMLINKS to
  demand_tc_witness's, and a pragma-derived badge makes the dumps differ
  by design, forcing symlink demotion; the equivalence pin is worth more,
  and the info is already derivable from `-contract-out`'s declared-key
  lines. (Mechanics, should it ever re-open: no new plumbing —
  `RecognizedSubgraph::demanded_decl.HasInstanceKey()` joined by
  forcing_index inside the existing freeze loop; E-K5-PAD applies to the
  shared port-line columns; "declared" would mean target-has-pragma, not
  activation-reason — activation is module-global OR.)
  (ii) LANDED as F32 (session 9): the contradiction was a LIVE composed
  finding — the four differential-regime behavioral goldens pinned
  demand-ON output, and the suite aggregator's token blacklist silently
  dropped the `REFINTERP-DISAGREE` verdicts that had fired on every green
  run. Fixed: `mode_flags_of` plain behavioral compile, aggregator
  whitelist + per-case verdict coverage, four goldens re-blessed to the
  definitional CBF (pure additions), A-K6-5 superseded (pointer added).
  See FINDINGS.md F32.
  (iii) the missing oracle-vs-behavioral cross-family byte check —
  formulation ratified for s9 (post-F32 the demand-case confound is gone);
  (iv) K2 best-effort re-opens only on explicit owner call;
  (v) the K5-D8 riders: RIDER-1 RESOLVED s9 (owner-ratified): the dead
  has_one_insert branch DELETED (not activated — Demand.cpp's AsMerge()
  descent load-bearingly depends on always-MERGE, now documented at the
  Connect.cpp mint site; zero golden movement), RIDER-2 the
  Merge.cpp-vs-Join.cpp CDaGI guard-tuple inconsistency (census gap #15;
  formulation + directed-witness attempt ratified for s9), RIDER-3
  D2.9(β) proxy-role-at-mint (deferred; re-opens on a named dump
  consumer);
  (vi) LANDED s9 with F32: bless_copy identity-skips byte-identical
  non-symlink re-blesses ("skipped ... (byte-identical)"); `BLESS: N`
  counts only real content deltas.
- **RETIRED:** S1/K5 (landed, `81385a92`); ADJ-R3-C (MOOT post-RP-6:
  every accepted @key'd relation is demanded — `key_undemanded_1` + the
  unseeded reject close the routes; re-opens only with a non-reject
  activation route, e.g. K2 best-effort).

## Part 3 — what the next session verifies before building

1. Baselines: clean tree at the session-8 close tip (PUSHED); rebuild;
   `SUITE: PASS (250)`; ctest 7/7.
2. Fleet re-verify Part 1's anchors (functions exact, lines approximate):
   the Connect seed guard + seed-once assert, the View.cpp CDaGI union +
   Tigerstyle assert pair, CollectOriginInteriorDecls/ResolveOriginSupport
   + the third census term (Planning.cpp), the Build-tail conservation
   belt, the -origin-out plumbing + the run_irgold smoke line, the
   20-file .region census, E-K5-PAD against Regional/Format.cpp, and the
   K1/K6 parse-layer anchors (case 8 tail, F31 site, context-resolving
   accessors) — k5-provenance.md / k1-multikey.md / k6-riders.md are the
   fine-grain authorities.
3. The owner's RANKING across S2/S3/S4 — STOP if unranked and
   load-bearing; S3 carries the two explicit owner STOPs (D2.6
   region-vs-store, §6-vs-§11) that MUST be answered before any
   implementation there.
