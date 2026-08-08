# Session 13 prompt — keyed-instance rewrite: amend, deepen the reconstruction phases, re-critique, IR desired-states

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. Re-check the branch
tip and worktree first: the s11 Phase-0 parser witnesses may still be uncommitted, and
the session-12 grounding docs were added (docs only — no production code was touched;
suite was `SUITE: PASS (251 cases)` and nothing was blessed).

## Where things stand (session 12 close)

Session 12 was a GROUNDING session (design/critique, executed with workflows +
opus/sonnet tiering). It produced four artifacts and a consolidated backbone. Read in
this order:

1. `RegionalDataFlowCore.artifacts/INDEX.md` (read-order + supersession matrix)
2. `next-session-prompt.md` — the SEMANTIC authority: `@key` is a relation-local,
   ordered access-path declaration, NOT a query adornment / demand opt-in. A 10-phase
   greenfield rewrite (P1 DELETES the demand machinery).
3. **`keyed-rewrite-whole-program.md`** — the CONSOLIDATED backbone: whole-program
   pseudocode (current §1), the four-authority target + EvaluateEpoch (§2), and the
   AMENDED path forward (§3) with the critique folded in + the open decisions D1–D4.
   START HERE for the actionable state.
4. `keyed-rewrite-current-pseudocode.md` — deepened §1 (8 subsystems), the RE-VERIFIED
   anchor table (§1.0, supersedes the seed §5), the drift ledger (§1.1).
5. `keyed-rewrite-pseudocode-seed.md` — §2/§3 are OPERATIONAL target pseudocode; §4 is
   the P0–P9 seed.
6. `keyed-rewrite-phase-diffs.md` — the P1–P9 hunk diffs (P1.1–P1.7, P6.1–P6.6). Its P1
   inventory is SUPERSEDED IN PART by `keyed-rewrite-whole-program.md` §3 (a banner says so).
7. `keyed-rewrite-critique.md` — 33 surviving findings (6 blocking / 12 high) + 7
   refuted, each verified against real code with a concrete `Fix:`.
8. `keyed-rewrite-ir-desired-states.md` — desired post-cut IR for the carriers
   (baselines verified; post-cut predicted).

**Key facts to carry:** the critique proved the P1 deletion inventory is INCOMPLETE
(the atomic cut would not compile — GuardAnnotation CSE-migration in
View/Join/IdentityJoin/Link.cpp; `kSectionWalk`'s live join consumer at Rel.cpp:2433;
ControlFlow DR-vocab consumers; IsDemandMessage's 3 callers). Two blocking soundness/
reintro gaps sit in the reconstruction phases: P4 re-provides `FullScanFilter` emission
only in prose, and P6.5 lacks OVERDELETE→REDERIVE. The corrected inventory + amendments
are in `keyed-rewrite-whole-program.md` §3, with four open owner decisions D1–D4.

**Phase 1 remains OWNER-GATED and destructive.** Do NOT begin it without an explicit
go-ahead. This session is grounding, not implementation.

## This session's task

Continue the deepen → diff → critique → IR-desired-states loop, now weighted toward the
RECONSTRUCTION phases (P2–P6), which the critique showed are far less concrete than P1.

1. **Enact the §3 amendments into a corrected diff set.** Fold the critique's blocking
   and high `Fix:` amendments into `keyed-rewrite-phase-diffs.md` (or a successor) so the
   P1 inventory is compile-clean and P2–P6 carry their corrections. Resolve the four open
   decisions D1–D4 (P4/P5 ordering; recursive_components producer; P6.5 deletion
   semantics; P7 honesty-seam location) with a recommendation + rationale each — or, where
   they are genuine owner calls, frame them crisply for an `AskUserQuestion`.

2. **Deepen the reconstruction-phase pseudocode.** For P2–P6, turn the target typed
   records (`RegionTemplate`/`RelationSchema`/`RequestEdge`/`RuleActivationEdge`/
   `FactDerivation`/`BindingState`/`AccessPlan`) into OPERATIONAL algorithms at
   implementer grain, grounded in the real code they replace (re-verify anchors at tip —
   they drift). Show how each reconstruction phase re-provides what P1 deleted, and how
   the four authorities stay separate. This is the same "build out pseudocode of the
   architecture/algorithms" work session 12 did for §1, now for §2/§3's realization.

3. **Formulate the diffs for the key design goals** on that deepened pseudocode, at hunk
   grain with exit gates — especially the goals the critique found under-resolved: honest
   `FullScanFilter` as a REAL emission (goal 5), the partial-binding DAG with order-free
   schema identity (goals 2+6), co-recursive key flow with rooted-reachability + a real
   DRed deletion path (goals 4+7), RequestEdge-vs-RuleActivationEdge kept distinct
   (goal 3).

4. **Critique the diffs adversarially.** Re-run the refute panel against the real code
   and the retained `RegionalDataFlowCore.md` invariants (member identity, exact request
   ownership, caller-qualified results, effect/epoch invariants, drain-before-retire).
   Verify findings against the codebase — do not merely assert them. Rank survivors;
   record refuted diffs as certifications.

5. **Author the desired IR output states** for the reconstruction phases, extending
   `keyed-rewrite-ir-desired-states.md`. Predict-then-verify: state the target
   bytes/shape for `.df`/`.contract`/`-region-out`(+DOT)/`.rel`/generated header/
   generated C++, mark what each phase changes, and add STRUCTURAL pins (not just
   answer-equality — the critique showed answer-equality is a lost check post-P1; the
   `.rel` census multiset is the strongest anti-silent-pass belt). AUTHOR the two carriers
   that have no baseline: the co-recursive `p(K,X)@key(K) / q(X,K)@key(X)` program (P6) and
   the `@key(A) @key(A,B)` / `@key(A,B) @key(B,A)` convergence probes (P5).

## How to do the work (workflows + model tiering)

Use the **Workflow tool** for the fan-out/critique structure — the owner has opted into
multi-agent orchestration for this grounding. Several sequential single-phase workflows,
staying in the loop between them, beats one mega-workflow. Suggested shape (adapt freely):

- **Deepen (fan-out, sonnet):** one reader per reconstruction subsystem / target record,
  extracting the current code it replaces + re-verified anchors + a first-cut operational
  pseudocode. Sonnet is right for the mechanical census/extraction.
- **Diff + amend (opus):** author the corrected/deepened P2–P6 diffs and resolve D1–D4.
  This is judgment work — use opus, effort high.
- **Critique (adversarial panel, opus):** per-phase refuters + a cross-phase completeness
  critic, each prompted to BREAK the diff against real code + retained invariants; verify,
  don't assert; rank.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** sonnet pulls the
  current carrier dumps as the diff baseline (read-only compiles into a scratch dir);
  opus authors the desired states + the two new carriers.

Keep the orchestrator thin: subagents return distilled pseudocode/findings, not raw file
dumps. Front-load cheap artifacts (dump the current carrier IRs once into scratch; embed
paths in agent prompts). Land results as updates to the existing docs (or clearly-named
successors) and keep `keyed-rewrite-whole-program.md` as the single backbone (update its
§3 as amendments land).

## Guardrails

- Re-derive every cited anchor against the current tip before trusting it; anchors drift
  (the s12 anchor table in `keyed-rewrite-current-pseudocode.md` §1.0 is a starting point,
  not gospel — re-verify).
- Do NOT edit production code toward Phase 1 without an explicit owner go-ahead. The
  destructive demand deletion and P0-item-4 (order-significant paths) are owner-gated.
  (P0-item-4's parser half at `Parser.cpp:1477` could land ahead of P1; its
  `Demand.cpp:902` half must wait for the cut — flag this, don't act on it unprompted.)
- Do not bless any golden. If you compile carriers for baseline dumps, that is read-only;
  the suite must stay `SUITE: PASS`.
- Prefer silent-on-success builds/tests (capture to file, surface only on failure);
  tokens matter.
- Update memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) at session close with
  what landed and the next ranked step.
