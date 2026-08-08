# Next-session prompt — keyed-instance rewrite: pseudocode, diffs, critique, IR desired-states

Continue in the Dr. Lojekyll repo on branch `keyed-instances`. Re-check the
branch tip and worktree first — the Phase-0 changes below may still be
uncommitted, or may have been committed since.

## Where things stand (session 11 close)

- The authority chain is `RegionalDataFlowCore.artifacts/INDEX.md` →
  `next-session-prompt.md` (the semantic + roadmap authority: `@key` is a
  relation-local ordered access path, NOT a query adornment; a 10-phase
  greenfield rewrite that DELETES the demand machinery) →
  `keyed-rewrite-pseudocode-seed.md` (NEW this session: whole-program
  pseudocode of the current pipeline + the target model + P0–P9 as diffs) →
  `regional-arch-pseudocode.md` for line-level current-state depth.
- **Phase 0 partially landed** (working tree; verify committed-or-not):
  reject `@key` on `#query` (a targeted witness in `lib/Parse/Query.cpp`
  state 6) and full-context redeclaration consistency (`lib/Parse/Parser.cpp`
  — the canonical first-key-bearing decl, not the immediate-previous one).
  New/changed rejects: `reject_key_on_query_1.dr`, `reject_key_redecl_1.dr`.
  Verified green: ctest 7/7, OptDiff `SUITE: PASS (251 cases)`.
- **Owner decision (session 11): "Safe Phase 0 only."** The destructive
  Phase 1 cut (delete the entire demand authority; keyed/demand evaluation is
  then non-functional until Phase 4) is OWNER-GATED. Do NOT begin it without
  an explicit go-ahead. Phase 0 item 4 (order-significant paths) is entangled
  with Phase 1 (the demand bijection at `Demand.cpp:905` sorts) and waits for
  the cut.
- Memory (`regional-dataflow-core-epoch.md`, `MEMORY.md`) records this state.

## This session's task: ground the rewrite before cutting

The goal is NOT to start the destructive implementation. It is to produce the
rigorous, critiqued design grounding the owner needs to green-light Phase 1+
with confidence — pseudocode, diffs on that pseudocode, adversarial critique,
and the desired IR output states. Treat `keyed-rewrite-pseudocode-seed.md` as
a SEED to deepen and correct, not a finished artifact.

Deliverables:

1. **Deepen the whole-program pseudocode.** For each subsystem (parser/AST,
   `Query::Build` + demand transform, `Stratify`/row-contracts, Rel lowering,
   ControlFlow build, CodeGen, Runtime `InstanceStore`, `FrozenRegionalProgram`),
   produce faithful pseudocode of the CURRENT algorithm at implementer grain,
   with re-verified `file:line` anchors. Correct any drift in the seed's §1.
   Also render the TARGET authority model (§2) and evaluation contract (§3) as
   operational pseudocode (turn the typed records into algorithms).

2. **Formulate the diffs for the key design goals.** For each phase P1–P9,
   express the change as a hunk-grained diff on the pseudocode from (1):
   what is deleted, what is added, what invariant it establishes, and its exit
   gate. Explicitly resolve the design goals from `next-session-prompt.md`
   "Authority model" / "Non-negotiable language semantics": four-authority
   separation, order-significant paths with order-free binding identity,
   RequestEdge vs RuleActivationEdge, rooted-reachability liveness, honest
   `FullScanFilter`, partial-binding DAG, co-recursive key flow.

3. **Critique the diffs adversarially.** For every phase diff, run a critique
   that tries to REFUTE it against the real code and the retained invariants
   in `RegionalDataFlowCore.md` (member identity, exact request ownership,
   caller-qualified results, effect/epoch invariants — see the INDEX
   supersession matrix). Surface soundness gaps, ordering hazards, deletion
   obligations missed, and tests that would silently pass. Rank findings.

4. **Author the desired IR output states.** For each IR surface — `.df` /
   `.contract`, `-region-out` (+ DOT twin), `.rel`, the generated header, and
   the generated C++ — specify the DESIRED post-cut output for a small set of
   carrier programs (reuse the `key_*`/`demand_*` datasets per the
   `next-session-prompt.md` "Test migration" list). Do the same
   deepen→diff→critique loop on these desired states (predict-then-verify
   discipline: state the target bytes/shape, mark what each phase changes).

## How to do the work (workflows + model tiering)

Use the Workflow tool for the fan-out/critique structure — the owner has
opted into multi-agent orchestration for this grounding. Suggested shape
(adapt freely; several sequential single-phase workflows, staying in the loop
between them, beats one mega-workflow):

- **Understand (fan-out, sonnet):** one reader per subsystem extracting
  current-algorithm pseudocode + re-verified anchors. Sonnet is right for this
  mechanical extraction/census work. Merge into the deepened §1.
- **Design + diff (opus):** author the P1–P9 pseudocode diffs and the target
  model/evaluation pseudocode. This is judgment work — use opus.
- **Critique (adversarial panel, opus):** per-phase refuters, each prompted to
  break the diff against real code + retained invariants; majority-refute
  kills or flags a diff. Verify findings against the codebase, don't just
  assert them.
- **IR desired-states (opus author, sonnet carrier-dump extraction):** author
  the desired `.df`/`.contract`/`-region-out`/`.rel`/header/C++ states; use
  sonnet to pull current carrier dumps as the diff baseline.

Keep the orchestrator thin: subagents return distilled pseudocode/findings,
not raw file dumps. Front-load cheap artifacts (dump the current IRs once,
embed verbatim in agent prompts). Land the results as updates to
`keyed-rewrite-pseudocode-seed.md` and new desired-state docs alongside the
existing `*-desired-states.md` family.

## Guardrails

- Re-derive every cited anchor against the current tip before trusting it;
  anchors drift.
- Do NOT edit production code toward Phase 1 without an explicit owner
  go-ahead. Phase 0 item 4 and the demand deletion are owner-gated.
- Do not bless any golden. If you compile carriers for baseline dumps, that is
  read-only; the suite must stay `SUITE: PASS`.
- Update memory at session close with what landed and the next ranked step.
- Prefer silent-on-success builds/tests (capture to file, surface only on
  failure); tokens matter.
```
