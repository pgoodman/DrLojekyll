# The post-s9 architecture — whole-program pseudocode + the path forward as diffs (session-9 close, 2026-08-05)

Purpose: ground session 10 in a whole-program view at the session-9 close
(tip `a727b74c`, pushed: F32 + XFAM + RIDER-1/2/2b + the typed
instance-handle vocabulary, all owner-ratified). SEED discipline: single
pass, orchestrator-authored from full session context — the next session
MUST fleet-re-verify every anchor before building (functions/files exact,
line numbers approximate at this grain). SUPERSEDES `k5-landed-seed.md` at
the whole-program grain (that seed's Part 1 compiler pipeline remains
accurate EXCEPT the four s9 deltas inlined below; its Part 2 S4 entries
carry the landed dispositions). ONE-AUTHORITY RULE unchanged: normative
design record `region-model-diffs.md` (RP-1..10 + THE SUBGRAPH-AUTHORITY
FRAMING); fine-grain authorities `k5-provenance.md` (+ the s9 RIDER-2
CLOSED record), `k1-multikey.md`, `k6-riders.md` (+ the s9 A-K6-5
supersession), `tests/OptDiff/FINDINGS.md` F32,
`s9-mint-sloc-diag-formulation.md` (S5/S6).

---

## Part 1 — the LANDED architecture (pseudocode, post-s9)

### 1a. The compiler pipeline (k5-landed-seed.md Part 1 + four s9 deltas)

```
CompileModule(module):                       # bin/drlojekyll/Main.cpp
  # PARSE: @key -> InstanceKeySet (s9 TYPED: using InstanceKey = unsigned,
  #   using InstanceKeySet = std::vector<InstanceKey>, hyde ns, Parse.h;
  #   storage std::vector<InstanceKeySet> instance_key_param_index_sets +
  #   parallel instance_key_ranges; NO invalid sentinel BY DESIGN —
  #   immediate resolution at the pragma site or reject).
  # F31 site, context-resolving accessors, intern-offset decl.Id() — all
  #   unchanged from the k5 seed.

  query := Query::Build(module, log, policy, demand_mode, ...)
    ConnectInsertsToSelects:                 # lib/DataFlow/Connect.cpp
      CreateProxyOfInserts(rel->inserts)
        # s9 DELTA 1 (RIDER-1): the dead has_one_insert branch is DELETED.
        # ALWAYS-MERGE is now a DOCUMENTED LOAD-BEARING invariant at the
        # mint site: Demand.cpp's descent does unconditional AsMerge() in
        # all 4 modes. Activating a bare-TUPLE single-insert shape would
        # break the demand pass AND drift nodf/none IR goldens corpus-wide.
      # K5 origin seed unchanged (!IsInline() && !IsQuery(), seed-once).
    ApplyDemandTransform: unchanged (RP-6 gate, two-phase per-adornment
      loop, V-DECLARED-KEY RP-10 bijection, RecognizedSubgraph mints).
      # Guard JOINs are stamped guard_annotation_index — s9 TYPED:
      # using GuardAnnotationIndex = unsigned (public DataFlow/Query.h),
      # sentinel QueryView::kNoGuardAnnotation now used at ALL internals
      # (View/Demand/IdentityJoin/Link/Join) — zero bare ~0u remain.
    Optimize:
      CopyDifferentialAndGroupIdsTo (CDaGI): unchanged mechanics, BUT its
        guard_annotation_index CLEAR-ON-MOVE is sound ONLY for SUPERSEDED
        losers (this = being-replaced). Two live-loser sites adjudicated s9:
      # s9 DELTA 2 (RIDER-2 CLOSED): Merge.cpp unused-col guard mints its
      #   guard TUPLE with a NARROWED 3-field copy (group_ids + OR'd
      #   diff-flags + origin_decls), NEVER full CDaGI — the merged view
      #   STAYS LIVE (guard reads from it). All three fields are
      #   redundant-by-reconstruction (RelabelGroupIDs re-derives group_ids
      #   at every CSE entry/merge + ClearGroupIDs at exit;
      #   TrackDifferentialUpdates full-re-derives flags AFTER the last
      #   guard mint in every guard-minting mode — guards need df.canon
      #   which travels with df.ident_join whose Optimize tail runs TDU
      #   unconditionally; origins co-carried by the live source, and the
      #   guard NEVER fires on a materialized Tier-2 carrier's merge — a
      #   merge feeding an INSERT has no unused column). Defense-in-depth,
      #   byte-neutral verified. 7 corpus reachers (compare_3/4/6, join_6,
      #   merge_2, reconverge_1, view_5; opt-family only).
      # s9 DELTA 3 (RIDER-2b): Join.cpp ProxyUnusedInputColumns keeps its
      #   FULL CDaGI but now has an ALWAYS-ON fprintf+abort tripwire
      #   before it: an annotated guard JOIN reaching this site would have
      #   its annotation clear-on-moved onto the proxy (recognition
      #   strands, missing answers). Census: ZERO corpus reachers incl.
      #   nested/@key. The day it fires: narrow like Merge, or teach
      #   recognition to resolve through the proxy.
    Stratify; InferConservativeRowContracts; K5 conservation belt: unchanged.

  frozen := FrozenRegionalProgram::Build(query, log)    # unchanged (R-STORE
    # + Tier-1 + Tier-2 contracts, E-K5-PAD render law, V-REGION-CENSUS).
  program := Program::Build(frozen, ...)                # unchanged (H4 seam).
  # Runtime: InstanceStore handle s9 TYPED: using InstanceId = uint32_t
  #   (hyde::rt; the Key template param is the TUPLE — this is the id OF an
  #   instance, not its key); kNoInstance : InstanceId. Codegen emits
  #   `auto` + the named sentinel, so generated text never moved.
```

### 1b. The referee/suite architecture (s9 made this first-class — F32/XFAM)

```
runall.sh <workroot> [jobs] [filter]:          # tests/OptDiff/runall.sh
  per case (xargs -P, worker = runall.sh --one):
    mode_flags_of(mode):  # s9: optimization-mode flags ONLY — the ONE
                          # helper the behavioral compile may use (F32).
    flags_of(mode) = mode_flags_of(mode) + .drflags sidecar  # everything else
    dispatch:
      diagnostic-listed cases -> expect_diagnostic x4
      kvindex_1 -> modesplit
      else -> diffrun.sh (4-mode stdout vs THE golden; flags_of)
    run_oracle:      oracle vs .oracle.stdout; --project-monotone vs
                     .monotone.stdout                      # flags-blind tool
    run_refinterp:   interp CBF (demand-blind, OG1) ONCE;
                     diagnostic case -> OK-DIAGNOSTIC (interp-only);
                     else behavioral binary x4 modes, PLAIN COMPILE
                     (mode_flags_of — F32; an @key case is inherently
                     pragma-activated, in-source) ->
                     4-mode byte-agreement (BEHAVIORAL-MODE-SPLIT) +
                     behavioral.opt == golden (BEHAVIORAL-DIVERGE) +
                     interp.cbf == golden (REFINTERP-DISAGREE).
                     # THE F32 INVARIANT: golden == plain behavioral ==
                     # demand-blind interp CBF, three-way, byte-exact.
                     # The four differential-regime demand goldens were
                     # re-blessed to the definitional CBF (pure additions);
                     # demand-GATED published behavior stays pinned by the
                     # .stdout family (drivers compile WITH .drflags).
    run_crossfamily: # s9 NEW (XFAM). oracle --project-published (the
                     # DIFFERENTIAL evaluator — 4th code-disjoint
                     # implementation) emits published-message final
                     # membership in CBF FINAL byte shape (GLOBAL STRING
                     # sort — RefInterp's sort, NOT DumpRelations' typed
                     # sort; name-projection between the families is
                     # IMPOSSIBLE, namespaces disjoint). Byte-compare vs
                     # the behavioral golden's FINAL block (awk-extracted),
                     # or live interp.cbf for diagnostics. XFAM-* tokens.
                     # Vacuous-by-data when nothing publishes. ZERO goldens.
                     # Catches: the F27 detached-published-tap class.
    run_eqgate:      unchanged (nested re-drive vs .stdout golden, LIVE).
    run_irgold:      unchanged (T3 surfaces incl. -origin-out smoke).
  aggregation:       # s9 REWRITTEN (F32 leg (b)) — the old failure-token
                     # blacklist DROPPED REFINTERP-DISAGREE and
                     # BEHAVIORAL-MODE-SPLIT (the I0 referee fired on every
                     # green run, unseen, since D3.a.1).
    WHITELIST: any verdict line not matching / OK(-DIAGNOSTIC)?$/ fails.
    COVERAGE:  every caselist name must appear >=1 time in verdicts
               (a worker killed before any echo is loud, not silent).
  rejects lane: unchanged (rc==1 both extremes; >=124 = crash finding).
  --bless:           # s9: bless_copy identity-skips byte-identical
                     # NON-symlink re-blesses too ("skipped ...
                     # (byte-identical)") — BLESS: N counts only real
                     # content deltas. Symlink rule unchanged (skip /
                     # BLESS-REFUSED).
bin/Oracle --project-published:   # s9 NEW: runs the differential path,
    # dumps io.Transmits() ViewModels' in_i rows, string-sorted.
```

## Part 2 — the path forward as DIFFS on Part 1

- **DIFF-NEXT-S2 (K4, key-subset covering-array fuzz arm).** Unchanged
  from the k5 seed: generated placements over (relation x key-subset x
  redeclaration), accept-iff-exact-match + clean-reject + NEVER abort;
  oracle must model the K6-widened reject surface (F31-revived checks,
  IDENTICAL-OR-ABSENT, RP-10 bijection, parse-time dup-SET, intern-order
  Ids). s9 makes the fuzz oracle's job easier: the reject surface is
  unchanged but the harness now fails loud on ANY unknown verdict token.
- **DIFF-NEXT-S3 (K3, the Stage-C ownership flip).** Unchanged: deletes
  Part 1a's injector seam — request edges + instance lifecycle
  first-class in the frozen regional program, RecognizedSubgraph demotes
  to a walk->extraction handoff. STILL BLOCKED on the two owner STOPs
  (D2.6 readers->REGION-or-STORE; the §6-vs-§11 routing rule) + the
  DIFF-R1 15-amendment fold. K5 origin sets + the s9 typed vocabulary are
  available inputs. DESIGN-ONLY until the STOPs are answered.
- **DIFF-NEXT-S5 (Mint mint-site source locations — probe-validated,
  formulated).** `Mint(list, args...)` CTAD wrapper (the ONLY legal host
  for a defaulted trailing std::source_location after a deduced pack —
  the naive Create default is unreachable for deduced calls, the
  ctor-hosted default captures DefUse.h itself), mint_loc on Def<T>, the
  413-site mechanical sweep (Build.cpp 76, Merge.cpp 54, ...), rendering
  ADVISORY-ONLY (DOT twins always; textual dumps under an opt-in modifier
  the suite never passes — goldened dumps must never carry churning line
  numbers). Full formulation: s9-mint-sloc-diag-formulation.md §1.
- **DIFF-NEXT-S6 (broad-AND-narrow diagnostics, domain-sliced — owner
  STANDING RULE).** Every diagnostic anchors BOTH the broad context range
  AND the narrowest sub-range; ErrorLog::Append(range, sub_range) exists,
  5/304 sites use it; golden-safe (rejects pin CLASS, never text).
  Slice 1 = the @key/demand family (InstanceKeyRanges(), pragma ranges,
  the F31-revived consistency diagnostics). Formulation:
  s9-mint-sloc-diag-formulation.md §2. Binds all NEW diagnostics NOW.
- **RESIDUALS:** RIDER-3/D2.9(β) deferred (re-opens on a named dump
  consumer); K2 best-effort only on explicit owner call; F32-Q2 accepted
  residual (demand-ON 4-mode ABI invariance covered only via .stdout/
  eqgate hooks — a demand-ON invariance check is future work if a real
  carrier appears); F20 the sole open ledger note.
- **RETIRED at s9:** S1/K5; S4 (ALL dispositions landed or dropped:
  F32=(ii)+(vi), XFAM=(iii), RIDER-1 deleted, RIDER-2 closed narrowed +
  RIDER-2b tripwire, badge DROPPED — it would split the key_tc_witness
  .region activation-equivalence symlinks; A-K6-5 superseded).

## Part 3 — what session 10 verifies before building

1. Baselines: clean tree at `a727b74c` (PUSHED); rebuild; `SUITE: PASS
   (250 cases)` + 66 `crossfamily OK` verdicts; ctest 7/7.
2. Fleet re-verify Part 1's anchors: the four s9 compiler deltas
   (Connect.cpp always-MERGE comment; Merge.cpp narrowed 3-field copy;
   Join.cpp RIDER-2b tripwire; the typed aliases in Parse.h /
   InstanceStore.h / DataFlow Query.h) + the harness architecture
   (mode_flags_of vs flags_of and WHICH compile uses which; the whitelist
   + coverage aggregation; run_crossfamily + oracle --project-published;
   bless_copy identity-skip) + the F32 three-way invariant on one
   differential-regime case (e.g. demand_diff_pub_1: golden == plain
   behavioral == interp.cbf) + the k5 seed's still-live anchors (Connect
   origin seed, CDaGI union+asserts, Planning collectors, E-K5-PAD,
   parse-layer K1/K6 sites).
3. The owner's RANKING across S2/S3/S5/S6 — STOP if unranked and
   load-bearing; S3 carries the two explicit owner STOPs that MUST be
   answered before any implementation there.
