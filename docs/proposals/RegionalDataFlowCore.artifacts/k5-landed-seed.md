# The post-K5 architecture — the session-8 close seed (2026-08-04)

Purpose: ground session 9 at the session-8 close (K5 Tier-2 origin
provenance LANDED, impl 81385a92). SEED discipline: single pass,
orchestrator-authored from full session context — session 9 MUST
fleet-re-verify every anchor before building (functions/files exact, line
numbers approximate). ONE-AUTHORITY RULE: the normative design record stays
`region-model-diffs.md` (RP-1..RP-10 + THE SUBGRAPH-AUTHORITY FRAMING); the
K5 working authority is `k5-provenance.md` (Parts A-E + ADJUDICATIONS + the
17-finding panel record — Part A's replacement-site census and Part E's
byte-diff prediction method are reusable instruments); `k6-landed-seed.md`
Part 1 remains the whole-program pseudocode EXCEPT as amended below.

---

## Part 1 — the pipeline DELTA (on k6-landed-seed.md Part 1)

```
ConnectInsertsToSelects(log, proxy_view_to_decl)     # Connect.cpp
  ...
  proxy_view_to_decl.emplace(insert_proxy, rel->declaration)   # T1 stamp
+ if !decl.IsInline() && !decl.IsQuery():                      # K5 SEED
+   assert(insert_proxy->origin_decls.empty())                 # seed-once
+   insert_proxy->origin_decls = { rel->declaration }
  rel->inserts.Clear()

QueryViewImpl::CopyDifferentialAndGroupIdsTo(that)   # View.cpp — THE choke point
  that->group_ids += this->group_ids; sort
+ that->origin_decls ∪= this->origin_decls           # K5 UNION: loser→survivor,
+   # sort+unique by decl.Id(), NO clear-on-this (a SET, not a counted
+   # scalar — contrast guard_annotation_index's clear). Tigerstyle pair:
+   # is_sorted AND adjacent_find==end (is_sorted alone tolerates adjacent
+   # equals under a strict comparator — the referee-pass fix).
  that->can_receive_deletions |= ...                 # lockstep — the
    # differentialness-migration invariant that makes Tier-2 support= sound

Query::Build tail (Build.cpp, #ifndef NDEBUG)
+ K5 conservation belt (RESCOPED): every RecognizedSubgraph.demanded_decl
+   origin-reachable at a live view. Redundant-with-Tier-1 defense-in-depth
+   (owner overruled the panel's delete — Tigerstyle standing directive).

FrozenRegionalProgram::Build (Planning.cpp)
  contracts := CollectContractInserts ++ CollectDemandInteriorDecls
+            ++ CollectOriginInteriorDecls            # K5 TIER-2: the residue
+   # outside insert-named ∪ Tier-1, ascending decl.Id() (= STRING-POOL
+   # INTERN-OFFSET order, NOT declaration order — `edge` suffix-aliases
+   # `add_edge`, so tc_nonlinear_diff pins E1=edge/E2=tc); member-key
+   # AllFields positional; support= ResolveOriginSupport (OR over live
+   # carriers' CanReceiveDeletions, loud-abort + DEBUG agreement assert);
+   # negative-space asserts (never inline/query/insert-named/Tier-1).
  census.row_contracts += CollectOriginInteriorDecls.size()   # same-collector

Main.cpp
+ -origin-out <PATH>: advisory per-view dump (`origin-sets` header, rows
+   ordered (min decl.Id(), det_seq), decls in stored Id order), drained
+   right after Query::Build, NEVER goldened; run_irgold smoke-produces it.
```

REFEREE STACK at tip: **250 = 203 golden cases + 47 rejects** (unchanged
counts; tc_nonlinear_diff GAINED an `.irgold` — the `.region` golden family
is now 20 = 5 cases × 4 modes); ctest 7/7; eqgate 6. E-K5-PAD (normative
render fact): the region emitter's member-key column width is a PER-DUMP
MAX (`Regional/Format.cpp` Pad), so a new longest member-key RE-PADS
existing lines — region-golden predictions must compute padding, never
assume byte-additivity (join_1's E0/E1 gained 3 interior spaces at K5).
The K5 predict-then-verify record: all 12 golden deltas matched Part E's
predictions byte-for-byte on first implementation.

## Part 2 — the path forward (DIFF-NEXT re-ranked post-K5)

- **S2 (K4, key-subset covering-array fuzz arm).** Unchanged from
  k6-landed-seed; the oracle must model the K6-widened reject surface
  (identical-or-absent + revived F31 checks). Small, independent.
- **S3 (K3, the Stage-C ownership flip — DESIGN-ONLY until the STOPs).**
  Unchanged: blocked on D2.6 (region-vs-store reader handles), the
  §6-vs-§11 routing rule, and the DIFF-R1 15-amendment fold. K5's origin
  sets are now AVAILABLE INPUT for Stage-C demand-area member naming (the
  consumer B5.2 anticipated).
- **S4 (residual riders, any ranking).**
  (i) request-port `declared` badge (unchanged);
  (ii) the behavioral PLAIN-vs-.drflags contradiction is now CONFIRMED
  CODE FACT (session-8 anchor fleet: `flags_of` runall.sh:206-218 appends
  `.drflags` to the behavioral compile at :405-407 against the comments at
  :92-93/:367-368 and CLAUDE.md; NINE non-diagnostic demand cases build
  their behavioral binaries demand-ON) — reconcile comment vs code and
  re-adjudicate what those goldens pin;
  (iii) the missing oracle-vs-behavioral cross-family byte check (unchanged);
  (iv) K2 best-effort re-opens only on explicit owner call;
  (v) NEW — the K5-D8 riders: the `has_one_insert` dead branch
  (Connect.cpp:14-17 Swap-before-read, always-MERGE; K5-immune, fix is
  one line, feeds K4), the Merge.cpp:296-343-vs-Join.cpp:266-280 CDaGI
  guard-tuple inconsistency (pre-existing under-migration), D2.9(β)
  proxy-role-at-mint (deferred, re-opens on a named dump consumer);
  (vi) NEW — bless verbosity: `--bless` prints "blessed" for
  byte-identical non-symlink files (9 of 21 at the K5 bless were no-op
  copies); a skip-line for identical regular files would make bless
  output census-honest (bless_copy already does this for symlinks).
- **RETIRED:** S1/K5 (landed); ADJ-R3-C (dropped as moot post-RP-6 —
  every accepted @key'd relation is demanded, `key_undemanded_1` +
  unseeded close the routes; re-opens only with a non-reject activation
  route, e.g. K2 best-effort).

## Part 3 — what session 9 verifies before building

1. Baselines: clean tree at the session-8 close tip (pushed); rebuild;
   `SUITE: PASS (250)`; ctest 7/7.
2. Fleet re-verify Part 1's K5 anchors: the Connect seed guard + assert,
   the View.cpp union + Tigerstyle assert pair, CollectOriginInteriorDecls/
   ResolveOriginSupport (Planning.cpp), the Build-tail conservation belt,
   the -origin-out plumbing + run_irgold smoke line, the 20-file .region
   golden census, E-K5-PAD against Regional/Format.cpp.
3. The owner's RANKING across S2/S3/S4 — STOP if unranked and
   load-bearing; S3 still carries the two owner STOPs.
