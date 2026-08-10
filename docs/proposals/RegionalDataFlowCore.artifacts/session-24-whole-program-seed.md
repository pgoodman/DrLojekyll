# Session 24 whole-program seed — POST-P7 (the keyed-instances namesake arc is closed)

> Cold-start START-HERE for session 24. Written at the P7 close (branch `keyed-instances`,
> OptDiff SUITE PASS 226, ctest 5/5). The compile pipeline and freeze/codegen anchors are
> in `p7-grounding-seed.md` §1 (still accurate for the freeze SELECT + codegen READ path);
> this seed gives the POST-P7 status and frames the next-cut decision. clangd is noise.

---

## §0 STATUS

- **P1–P7 LANDED.** The frozen Regional layer (`lib/Regional`) is a COMPILE-TIME observer
  over the retained M3 full-materialization backend, and it now steers codegen at TWO
  points: P4's `AccessPlan` select (bound+free → withhold index) and **P7's partial-key
  hash SEEK** (bound+free with a bound subset → `Index::First/Next`). The `keyed-instances`
  namesake — a bound `#query` over a keyed relation lowering to an index seek — is DONE.
- **The four authorities are all live:** the logical fact id (`RegionalFactId`), the
  order-free binding schema (`BindingStateId`/P5), the ordered logical access path
  (`DeclaredAccessPath`/P5), and the PHYSICAL `AccessPlan` (P4/P7). P6.1 populated
  `recursive_components`; P6.2 populated `rules`/`inherited_symbolic_fields` (clause-source
  routing + `SymbolicFieldId` promotion). All compile-time; codegen honest.
- **What still does NOT run in the greenfield backend:** actual runtime EVALUATION
  (fusion, cyclic activation, joint fixpoint, per-fact DRed) — the M3 backend does it. That
  is the P6.3–P6.6 cut. Every post-P1 gate is STRUCTURAL, never answer-equality (the M3
  baseline answers correctly).

## §1 WHAT P7 SETTLED (do not re-litigate)

- `SelectAccessPlan` (`RegionInstance.h`) dispatches: all-bound → `kFullKeyHashLookup`
  (`.Find`); bound+free with a non-empty bound subset → `kPartialKeyHashSeek`; no bound
  cols → `kFullScanFilter`. Enum is `{kUnplanned=0, kFullScanFilter, kFullKeyHashLookup,
  kPartialKeyHashSeek}`; `plan` ∉ Hash/Equals, `RequestPortRecord` never hashed.
- SCOPE = **Q1a raw bound subset**: the seek is NOT gated on `@key`; the P5↔physical
  firewall (`Regional.h:92`) stays UP; `@key` is a witness of intent. UNFENCED over
  recursive-owned relations (reads the settled table; `GetOrCreateIndex` shares/mints a
  non-colliding secondary index).
- **NEW codegen surface: NONE.** The seek reuses `EmitQueryFriends`' existing `via_index`
  First/Next arm + `Build.cpp`'s existing `GetOrCreateIndex(col_indices)` — driven by the
  selector alone (`withhold_index = plan==kFullScanFilter`).
- **P7 is #query-path ONLY.** The interior/join `EmitScan` path (`Database.cpp:2778`) is
  NOT plan-driven; `plan_kind` is NOT on `ProgramTableScanRegion`; V-PLAN-HONEST lives at
  the two query-entry-point sites, not EmitScan. That is the deferred P7b.

## §2 THE NEXT-CUT DECISION (the [OWNER STOP])

Three genuinely different directions — see `session-24-prompt.md` for the framing:

| Cut | Character | Size | Touches |
|---|---|---|---|
| **P7b** interior/join plan-driven scans | continuation of the AccessPlan authority; thread `plan_kind` on `ProgramTableScanRegion`, move V-PLAN-HONEST to EmitScan with the `kUnplanned` skip (the deferred B-P7/D4) | SMALL | compile-time + codegen; structural gates |
| **P6.3–P6.6** runtime evaluation | the compile-time model does REAL work; the first divergence from M3 | LARGE (sub-slice; a compile-time P6.3 fusion-detection spike is the low-risk entry) | codegen + runtime |
| **P8 / P9** ordered trie / path inference | a NEW runtime range/trie structure (none exists — all-hash) + inference | HEAVY | runtime + Regional |

Recommendation is the OWNER's. P7b is the smallest on-theme continuation; P6.3 (or its
detection spike) is the highest-value toward the real backend; P8/P9 are the heaviest.

## §3 METHOD

Grounding loop via workflows (opus design + sonnet anchors), THEN a throwaway-worktree
empirical spike for any change with a measurable blast radius (it caught P7's stale unit
expectation and confirmed the Q1a golden move), THEN owner-gated execution. Structural
gates only. Authorities: `p7-execution-grounding.md`, `p7-grounding-seed.md` §1,
`keyed-rewrite-p7p9-diffs.md` §2/§3 (P8/P9), `reconstruction-diffs.md` §3-P6.3..P6.6.
