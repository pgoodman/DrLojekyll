# The @key-era architecture — whole-program pseudocode + the path forward as diffs (session-6 close, 2026-08-04, tip 48cd0a4f PUSHED)

Purpose: ground the NEXT session in a whole-program view at the session-6
close. SEED discipline: single pass, orchestrator-authored from full
session context — the next session MUST fleet-re-verify every anchor
before building (line numbers are approximate at this grain; functions and
file names are exact). ONE-AUTHORITY RULE: the normative design record is
`region-model-diffs.md` (session-5 AMENDMENTS + RES-1..6 + the session-6
@DEMAND/@KEY sections, RP-1..RP-9, findings T1-IMPL-1, R3A-IMPL-1/2,
S6-IMPL-1); the landed dump bytes are `regional-dump-stage-b-desired-states.md`
§9/§10; `regional-arch-pseudocode.md` Part R3 is **STALE-BY-SUPERSESSION**
(it describes the retired bracket surface, parser state 20, the deferred
resolve, and pre-activation Step-2b — Part 1 below supersedes it at the
whole-program grain; a next-session pseudocode pass should re-ground Part
R3's fine grain against the tip rather than patch it).

---

## Part 1 — the LANDED pipeline (pseudocode, @key era)

```
CompileModule(module):                          # bin/drlojekyll/Main.cpp
  # ---- PARSE (lib/Lex, lib/Parse) ----
  #   `@key` lexes as kPragmaKey (Lexer.cpp "@key" ladder arm; Token.h).
  #   ParseLocalExport (Parser.cpp, the ONE template covering #local+#export):
  #     case 8 (pragma tail, post-parameter-list):
  #       kPragmaKey -> [second-pragma? clean not-yet-supported reject
  #                      ("multiple instance keys"; repetition = the RESERVED
  #                      multi-adornment lift)] -> state 21
  #     state 21: expect `(`; state 22: the arg list — named vars resolved
  #       IMMEDIATELY against local->parameters by IdentifierId (params are
  #       bound at the pragma site — no deferred resolve exists anymore);
  #       rejects: unnamed/anonymous, duplicate, unknown column, empty,
  #       trailing comma, unexpected token; EOF rides the state!=9 gate.
  #     storage: ParsedDeclarationImpl::instance_key_param_indices (WRITTEN
  #       order — parse-layer capture; the SET is the logical key); public
  #       accessors ParsedDeclaration::HasInstanceKey()/InstanceKey().
  #     the RETIRED bracket `rel[K...]` draws a pointed redirect diagnostic
  #       (case 1; lexemes kPuncOpen/CloseBracket retained, parser-unconsumed).
  #     decl formatter (Parse/Format.cpp) prints ` @key(K, ...)` (round-trip).

  query := Query::Build(module, log, policy, demand_mode, demand_retract,
                        suppress_demand=false)   # lib/DataFlow/Build.cpp
    proxy_view_to_decl := {}                     # Build-SCOPED local (RES-3a)
    ... Simplify ...
    ConnectInsertsToSelects(log, proxy_view_to_decl)
      # per relation: insert_proxy minted; STAMPS proxy->rel->declaration
      # into the map BEFORE rel->inserts.Clear() severs the only REL edge.
    ApplyDemandTransform(module, log, demand_mode, demand_retract,
                         suppress_demand, proxy_view_to_decl)  # Demand.cpp
      if suppress_demand: return true       # S6-IMPL-1: bin/Oracle passes
                                            # true — demand-blind referees
                                            # MUST override flag AND pragmas.
      # ACTIVATION GATE (RP-6): scan the PARSED module (ParsedModuleIterator
      # over Locals()+Exports(), dedup by decl Id — a #local's flows leave
      # `relations` at Connect; the DECL is the durable carrier) for
      # HasInstanceKey. pragma_activated := any found.
      if !demand_mode && !pragma_activated: return true   # containment gate
      # reject-advice fork: pragma_activated -> "fix or remove the @key
      # pragma"; else -> "recompile without -demand" (NEC-2).
      Step 1: collect bound queries;
        empty && pragma_activated -> REJECT (unseeded @key — RP-6)
        empty && flag-only        -> benign no-op
        >1 name -> R-1BOUND reject (strict — pragma or flag alike)
      Loop 1 (per adornment): Steps 1b+2+3 — trace p_bound, locate guard
        sites, run EVERY fence (NEGATE/AGG sink, sideways, second-read...).
      post-Loop-1:
        p_demanded_decl := proxy_view_to_decl.at(plan.front().p_merge)
                                            # T1; miss -> T1-DECL-MISS abort
        RP-6 realization: every HasInstanceKey decl must == p_demanded_decl
                                            # else "not the demanded relation"
        Step 2b (V-DECLARED-KEY, RES-1 slot — fences ran FIRST):
          if p_demanded_decl.HasInstanceKey():
            plan.size() >= 2 -> strict single-forcing reject (ADJ-R3-A)
            set(declared) != set(p_bound) per adornment -> mismatch reject
                                            # RP-3: unprovable-> hard reject
      Step 4 stray-consumer union; Loop 2 mints (fabricate demand__ msg+local,
        guard JOINs, forcing registry, RecognizedSubgraph{...,
        demanded_decl=p_demanded_decl}  # the Tier-1 snapshot)
    Optimize; ...; Stratify; row_contracts := InferConservativeRowContracts
    # -contract-out: per-view contracts + the pragma-SCOPED line
    #   `declared-key rel=<p> declared=(K...) inferred=(p_bound...)`

  frozen := FrozenRegionalProgram::Build(query, log)   # lib/Regional/Planning.cpp
    # R-STORE insert contracts (unchanged) + the TIER-1 INTERIOR arm:
    #   existence+census DECL-DRIVEN (distinct RecognizedSubgraphs()
    #   demanded_decl Ids, forcing order, deduped vs insert decls);
    #   member-key = decl AllFields positional; support = ROLE-BLIND OR over
    #   the forcing's live annotated guard JOINs' CanReceiveDeletions()
    #   (T1-IMPL-1: PromoteSurvivorToBody kills any role-filtered resolve);
    #   zero live guard JOINs for a counted decl -> freeze ABORT.
    # DeriveRegionalCensus = the single count authority (freeze recount +
    # the Rel.cpp V-REGION-CENSUS both consult it).

  program := Program::Build(frozen, log, first_id, policy, demand_instance)
    query = frozen.Query()                       # H4 thin seam
    # per-forcing admissibility flags (cyclic_demand / recursive_content,
    # from LIVE guard JOINs) computed ONCE, consumed by TWO arms (RP-9):
    #   -demand-instance FLAG: inadmissible -> STRICT reject (demand_cyclic_1
    #     and demand_recursive_content stay diagnostics — the dev override)
    #   @key PRAGMA (no flag): effective_demand_instance :=
    #     any_forcing && all_forcings_admissible &&
    #     RecognizedSubgraphs()[0].demanded_decl.HasInstanceKey()
    #     -> nested keyed-instance lowering; else SILENT FLAT FALLBACK
    #     (all-or-nothing across forcings — R-1BOUND makes that exact).
    context.demand_instance_enabled = effective_demand_instance
    # ... BuildDataModel -> DR-IR (nested: BuildSubgraphInstanceOps,
    #     kSubgraphInstantiate/kInstanceDeath...) -> eager web -> Optimize ...

  # SEEDING (both lowerings, unchanged): the query entry point publishes the
  # fabricated demand__ message through the SYNTHESIZED INJECTOR (the
  # QueryDemandForcing registry) — the "injected @first" seam the owner
  # wants DELETED; that deletion is Stage C (first-class request edges),
  # NOT this slice.
```

REFEREE STACK at tip: 246 = 200 golden cases + 46 rejects (driverless,
both mode extremes, rc=0 = lost check, >=124 = crash finding); ctest 7/7.
Key witnesses: `key_tc_witness` (recursive -> FLAT fallback; 12 SYMLINKED
goldens = activation equivalence, pragma == -demand byte-for-byte; own
real contract [declared-key line] + behavioral [CBF header embeds case
name]); `key_neighborhood_witness` (non-recursive -> NESTED selection
flagless; own rel.opt golden pins kSubgraphInstantiate=1; stdout SYMLINK
to the flat twin = nested-vs-flat answer identity per mode); the 8
flagless `key_*` diagnostics; `demand_prefix_collision_{local,msg}_1` (F30,
kind-blind scan). BLESS PROTOCOL: NEVER bless the symlinked surfaces of
key_tc_witness / key_neighborhood_witness directly — bless writes THROUGH
symlinks into the twins' goldens; verify byte-identity first.

## Part 2 — the path forward as DIFFS on Part 1

- **DIFF-NEXT-K1 (multi-@key repetition — the multi-adornment surface lift).**
  Parser: lift the second-pragma reject; N `@key(...)` pragmas -> a LIST of
  key sets on the impl. Step 2b: reconcile the declared SET-OF-SETS against
  the per-adornment inferred sets (bijection, order-free) — replaces the
  ADJ-R3-A strict single-forcing reject for pragma'd relations. The D3.a.3
  N-store machinery already exists (demand_multi_adorn_witness); the lift is
  surface+checking only. OPEN semantics: must every adornment be declared
  (total bijection) or may pragmas cover a subset? (Recommend: total — a
  partially-declared multi-adornment relation is the single-adorn ambiguity
  reborn.) Witness path: a pragma'd demand_multi_adorn_witness twin
  (flagless, eqgate-style) + key_multi_adorn_1 flips reject->golden.
- **DIFF-NEXT-K2 (the RP-8 auto-sweep STOP — `-demand` strict vs
  best-effort).** OWNER STOP, unratified. Best-effort (recommended
  endpoint: pragma=contract/strict, flag=advisory/cost-ranked) FLIPS the
  demand_agg_body_1/demand_kv_body_1/demand_config_agg_body_1/
  demand_mutual_content_1/demand_two_queries_1 reject goldens to
  compile-full-materialization (their .batches/oracle goldens already pin
  the definitional answers — built anticipating this). Do not implement
  before the explicit call.
- **DIFF-NEXT-K3 (Stage-C re-brief — request edges; kills the injector).**
  The owner's stated intent ("keyed instances, not the injected @first")
  lands HERE: first-class request-edge node + lifecycle ops; the forcing
  registry/injector seam deleted; `@key`'s pragma bit flows into the frozen
  regional program (a `declared` marker on the request port is the natural
  provenance surface — render-only, never a lowering input). BLOCKED ON two
  owner STOPs: D2.6 reader-handle schema, §6-vs-§11 routing rule; plus
  folding the DIFF-R1 panel's 15 normative amendments into stage-c-diff.md.
- **DIFF-NEXT-K4 (key-subset covering-array fuzz arm — slotted follow-on).**
  Placements over (relation x key-subset x redeclaration), ORDER dropped as
  inert (oracle-3/necessity-3); verdict = accept-iff-exact-match +
  clean-reject-otherwise + NEVER abort; feeds rejects/ with interesting
  placements. Slots beside the existing covering-array machinery.
- **DIFF-NEXT-K5 (Tier-2 provenance mini-slice).** Origin decl-sets on
  models (Connect erasure site, union-only on CSE folds) + D2.9 proxy-role
  inheritance as ONE slice; unblocks the deferred ADJ-R3-C column-survival
  belt + undemanded interior naming; the first sanctioned post-F1
  maintained satellite (panel must litigate that explicitly).
- **DIFF-NEXT-K6 (riders, any ranking).** (i) cross-REDECLARATION @key
  consistency check (recorded obligation — an #export re-declared with a
  differing pragma is silently unreconciled); (ii) Step-2b error ranges
  anchored at the PRAGMA tokens, not decl-range column 1 (store the pragma
  DisplayRange on the impl); (iii) DOT riders — a `declared` badge on the
  dataflow/region DOT twins + a DR-IR DOT twin (the nested selection is
  invisible in DOT today; advisory, never goldened); (iv) the
  `KEY (_MissingVar)` render wart on demand-minted views in -dot-out
  (fabricated columns lack parsed variables — cosmetic, DIFF-NEXT-4
  family); (v) the declared x nested eqgate on a non-recursive base is
  SUPERSEDED by key_neighborhood_witness (landed) — drop from candidate
  lists.

## Part 3 — what the next session verifies before building

1. Baselines: tip 48cd0a4f pushed; clean tree; SUITE: PASS (246); ctest 7/7.
2. Fleet re-verify Part 1's anchors (functions exact, lines approximate) —
   Parser.cpp case 8/21/22, Demand.cpp activation gate + Step 2b,
   Build.cpp admissibility fork, Planning.cpp interior arms — and re-ground
   regional-arch-pseudocode.md Part R3's fine grain (stale-by-supersession).
3. The owner's RANKING across DIFF-NEXT-K1..K5 — STOP if unranked and
   load-bearing; K2 and K3 carry explicit owner STOPs that MUST be answered
   before their implementation.
