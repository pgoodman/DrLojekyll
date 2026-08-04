# The post-K6 architecture — whole-program pseudocode + the path forward as diffs (session-7 close, 2026-08-04)

Purpose: ground the NEXT session in a whole-program view at the session-7
close (K1 multi-@key + the K6 riders LANDED; tips `e641be46` [K1],
`b95d3d29` [K6], + the docs commits). SEED discipline: single pass,
orchestrator-authored from full session context — the next session MUST
fleet-re-verify every anchor before building (functions/files exact, line
numbers approximate at this grain). ONE-AUTHORITY RULE: the normative
design record is `region-model-diffs.md` (RP-1..RP-10, the SESSION-7
RATIFICATIONS + THE SUBGRAPH-AUTHORITY FRAMING); the K1 working authority
is `k1-multikey.md`; the K6 working authority is `k6-riders.md` (both carry
their panel records + adjudications); `key-pragma-landed-seed.md` (s6) is
superseded by THIS seed at the whole-program grain.

---

## Part 1 — the LANDED pipeline (pseudocode, post-K6)

```
CompileModule(module):                          # bin/drlojekyll/Main.cpp
  # ---- PARSE (lib/Lex, lib/Parse) ----
  #   `@key` lexes kPragmaKey. ParseLocalExport (Parser.cpp, ONE template
  #   for #local+#export), case 8 pragma tail:
  #     kPragmaKey -> state 21 -> state 22 (arg list, IMMEDIATE resolution
  #       against local->parameters; rejects per pragma: anonymous, dup
  #       column, unknown column, empty, trailing comma, unexpected token).
  #     At each set's `)` close: ADJ-K1-A order-free DUPLICATE-SET reject
  #       (canon-sorted compare vs every stored set), then the set + its
  #       DisplayRange(key_pragma_tok .. `)` end) are pushed and cleared —
  #       a SECOND `@key` re-enters state 21 and opens a fresh set (RP-10;
  #       the old second-pragma reject is DELETED).
  #   storage (ParsedDeclarationImpl): instance_key_param_index_sets
  #     (vector<vector<unsigned>>, written order) + the PARALLEL
  #     instance_key_ranges (K6-3). Retired bracket `rel[K...]` still
  #     draws the redirect diagnostic. Formatter round-trips N pragmas.
  #   accessors (Parse.cpp): HasInstanceKey()/InstanceKeys()/
  #     InstanceKeyRanges() resolve THROUGH the declaration context
  #     (first key-bearing redeclaration wins — K6-4b): a pragma on ANY
  #     redeclaration activates regardless of declaration order
  #     (F-K6-SHADOW is dead; the pre-K6 per-impl read silently dropped
  #     non-first pragmas).
  #   FinalizeDeclAndCheckConsistency (Parser.cpp):
  #     F31 FIXED — prev_decl = redecls[num_redecls-2u] (was [n-1] =
  #     SELF, which had made EVERY redecl consistency check dead forever;
  #     the revived parameter type/name divergence checks are LIVE, zero
  #     corpus fallout). On the revived site: the IDENTICAL-OR-ABSENT
  #     instance-key check — a @key-bearing redeclaration must declare
  #     the identical set-of-sets (order-free) or omit @key to inherit;
  #     divergence rejects anchored at the offending pragma + a
  #     previous-declaration note.

  query := Query::Build(module, log, policy, demand_mode, demand_retract,
                        suppress_demand=false)   # lib/DataFlow/Build.cpp
    proxy_view_to_decl := {}                     # Build-SCOPED local
    ConnectInsertsToSelects(log, proxy_view_to_decl)  # lib/DataFlow/Connect.cpp
      # stamps proxy->rel->declaration BEFORE rel->inserts.Clear()
    ApplyDemandTransform(...)                    # lib/DataFlow/Demand.cpp
      if suppress_demand: return true            # bin/Oracle passes true
      # ACTIVATION GATE (RP-6): ParsedModuleIterator scan over
      #   Locals()+Exports(), dedup by decl Id, HasInstanceKey() —
      #   context-resolving since K6, so order-independent.
      if !demand_mode && !pragma_activated: return true   # containment
      # reject() advice fork on pragma_activated; the THREE demand__
      #   collision rejects fork too since K6-1:
      #   "...; rename it or remove the @key pragma" (pragma arm)
      #   "...; rename it or recompile without -demand" (flag arm).
      Step 1: bound queries; unseeded-@key reject ("declares an instance
        key but no bound #query exists..."); R-1BOUND (>1 name reject).
      Loop 1 per adornment (UniqueRedeclarations + seen_variants dedup):
        trace p_bound, locate guards, run EVERY fence (NEGATE/AGG sink,
        second-read, sideways, multi-clause, all-free sibling).
      post-Loop-1: p_demanded_decl via proxy_view_to_decl (T1-DECL-MISS
        abort); RP-6 realization ("declares an instance key but is not
        the demanded relation") — all "instance key" wording since K6-2.
      Step 2b (V-DECLARED-KEY, RP-10): TWO-ARM TOTAL BIJECTION —
        declared set-of-sets == inferred set-of-sets, order-free.
        Arm A (declared surplus): index-preserving iteration over
          InstanceKeys()[j], anchored at InstanceKeyRanges()[j] (the
          offending pragma's tokens — K6-3).
        Arm B (inferred surplus): decl-anchored (no pragma to point at);
          "declare @key(...) or remove the @key pragma".
        N=1 byte-compatible with the retired single-set check.
      Step 4 stray-consumer union (once, between loops).
      Loop 2 mints per forcing: guard JOINs, QueryDemandForcing registry,
        RecognizedSubgraph{forcing_index, demanded_view, key_cols,
        pub_view, guard_annotation_indices, demanded_decl}.
    Optimize; Stratify; InferConservativeRowContracts
    # -contract-out: ONE `declared-key rel=... declared=(...)
    #   inferred=(...)` line PER SET (written-pragma order, always-on
    #   no-match belt; key_multi_adorn_witness.contract.opt.golden
    #   byte-locks the two-line pairing).

  frozen := FrozenRegionalProgram::Build(query, log)   # lib/Regional/Planning.cpp
    # STILL the degenerate no-extraction planner (ONE root + R0):
    # request ports per forcing, Tier-1 interior row-contracts,
    # permanent roots, V-REGION-CENSUS recount. -region-dot-out badges
    # `declared-key` on pragma'd contracts (DOT-only; the 16 .region
    # TEXT goldens byte-unchanged).

  program := Program::Build(frozen, log, first_id, policy, demand_instance)
    query = frozen.Query()                       # H4 thin seam
    # per-forcing admissibility (cyclic_demand / recursive_content), TWO
    # consumers: -demand-instance strict rejects; @key pragma arm ->
    # effective_demand_instance := any && all_admissible &&
    #   subgraphs[0].demanded_decl.HasInstanceKey()   # + the ADJ-K1-D
    #   assert: every RecognizedSubgraph shares ONE demanded_decl
    # nested: BuildSubgraphInstanceOps per forcing (kSubgraphInstantiate/
    # kInstanceSeal/kInstanceDeath, band-(a2)/(a2') rebuild arms); N
    # adornments -> N disjoint stores over ONE pub (V-INST-SOLE re-keyed
    # (pub_table, forcing_index)); else SILENT flat fallback (recursive).

  # DOT twins (ALL advisory, never goldened): -dot-out (dataflow; no
  # _MissingVar since K6-6 — fabricated columns render field labels),
  # -region-dot-out (declared-key badge), -rel-dot-out (NEW K6-7b: the
  # Rel-IR twin — cluster-per-stratum, id-ordered ops/vecs, def/use
  # edges, census-free).

  # SEEDING: both lowerings still publish the fabricated demand__ message
  # through the SYNTHESIZED INJECTOR — the seam Stage C deletes.
```

REFEREE STACK at tip: **250 = 203 golden cases + 47 rejects**; ctest 7/7;
eqgate family 6. Key witnesses: `key_tc_witness` (flat fallback, 11
symlinks); `key_neighborhood_witness` (flagless nested, kSubgraph=1);
`key_multi_adorn_witness` (flagless nested N=2, kSubgraph=2 — its .dr now
OPENS with a pragma-free redeclaration, the previously-broken shadow
shape, so byte-identical goldens pin order-independence; stdout symlink;
own rel.opt + contract.opt; since K6-5 also .batches/.probes with
oracle/monotone SYMLINKED to the demand twin's and a REAL behavioral
golden [CBF header embeds the name]); `demand_multi_adorn_witness` (the
flat twin, now .batches-refereed); the TEN flagless key_* diagnostics;
`reject_key_double_1` (parse dup-set, WITH a bound query — LOST-CHECK
guard); `reject_key_redecl_1` (divergent-key redecl; identical middle
pair exercises the comparator TRUE branch). BLESS: `bless_copy`
mechanizes never-write-through-symlinks (skip on identity, BLESS-REFUSED
on divergence).

FINDINGS this session (FINDINGS.md Round 14): **F31** (the redecl
consistency off-by-one — every type/name check dead forever; panel-found,
lldb-proven, fixed) + **F-K6-SHADOW** (order-dependent silent pragma
drop; probe-found, fixed). Both witnesses of the method: the panel is a
filter, the PROBE and the SUITE are referees.

## Part 2 — the path forward as DIFFS on Part 1

- **DIFF-NEXT-S1 (K5, Tier-2 provenance).** Origin decl-sets on models
  (Connect erasure site, union-only on CSE folds) + D2.9 proxy-role
  inheritance as ONE slice. Unblocks the deferred ADJ-R3-C
  column-survival belt + undemanded interior naming. The panel MUST
  litigate the post-F1 maintained-satellite question explicitly.
- **DIFF-NEXT-S2 (K4, key-subset covering-array fuzz arm).** Placements
  over (relation x key-subset x redeclaration); verdict =
  accept-iff-exact-match + clean-reject-otherwise + NEVER abort; feeds
  rejects/. Small, independent. NOTE: K6's identical-or-absent check and
  the revived F31 checks WIDEN the reject surface the fuzzer can hit —
  the fuzz oracle must know both.
- **DIFF-NEXT-S3 (K3, the Stage-C re-brief — THE OWNERSHIP FLIP).** The
  normative input is region-model-diffs.md "THE SUBGRAPH-AUTHORITY
  FRAMING" (s7): today one mint authority (RecognizedSubgraph, a rewrite
  pass's record) + two checked projections (regional ports/contracts,
  DR-IR lifecycle ops) + runtime InstanceStores — inherent multi-IR
  layering EXCEPT that the authority sits in the wrong layer. Stage C
  flips ownership: request edges + instance lifecycle first-class in the
  frozen regional program, injector seam deleted, RecognizedSubgraph
  demoted to a walk->extraction handoff (or dissolved). The D2.6
  reader-handle STOP is SHARPENED: do readers hand onto the REGION
  (post-flip authority) or the STORE (current lowering artifact)?
  BLOCKED on: D2.6 + the §6-vs-§11 routing rule (owner STOPs) + folding
  the DIFF-R1 panel's 15 amendments into stage-c-diff.md. Design-only
  until answered.
- **DIFF-NEXT-S4 (residual riders, any ranking).** (i) the request-port
  `declared` badge (needs the RecognizedSubgraphs join at freeze);
  (ii) the runall.sh/CLAUDE.md "behavioral binary is PLAIN, never
  .drflags" claim CONTRADICTS the code (flags_of appends .drflags to the
  behavioral compile — recorded s7, unreconciled; investigate whether
  the comment or the code is right, and what demand-gated behavioral
  goldens actually pin); (iii) no oracle-vs-behavioral cross-family byte
  check exists (each family compares only its own golden — recorded);
  (iv) K2 best-effort re-opens only on explicit owner call.

## Part 3 — what the next session verifies before building

1. Baselines: clean tree at the session-7 close tip (PUSHED); rebuild;
   `SUITE: PASS (250)`; ctest 7/7.
2. Fleet re-verify Part 1's anchors (functions exact, lines approximate):
   Parser.cpp case 8 + FinalizeDeclAndCheckConsistency (the F31 site),
   Parse.cpp context-resolving accessors, Demand.cpp Step 2b two-arm
   anchoring, the -rel-dot-out emitter, the K6-5 sidecar/golden layout —
   k1-multikey.md + k6-riders.md are the fine-grain authorities.
3. The owner's RANKING across S1/S2/S3(/S4) — STOP if unranked and
   load-bearing; S3 carries the two explicit owner STOPs (D2.6
   region-vs-store, §6-vs-§11) that MUST be answered before any
   implementation there.
