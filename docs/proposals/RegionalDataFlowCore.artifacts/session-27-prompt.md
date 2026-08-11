# Session 27 charter — post-P6.3-detection: pick the next cut (P6.4–P6.6 runtime eval / P8 / P9)

You are resuming the **keyed-instance greenfield rewrite** on branch `keyed-instances` (Dr.
Lojekyll, the `hyde` C++ Datalog compiler). **P1–P7 + P7b + P7c + P6.3-detection ARE LANDED**
(OptDiff **SUITE: PASS (226)**, ctest **5/5**). Session 26 was an [OWNER STOP]: the owner picked the
**P6.3 fusion-DETECTION spike** from a go/no-go over P9/neither, and it landed. It is the FIRST cut of
the P6.3–P6.6 runtime-eval arc, taken **DETECT-ONLY** (compile-time, dump-only, **codegen
BYTE-UNCHANGED**): a `-region-out` classification of each recursive component FUSABLE vs JOINT. **P9
(access-path inference) was grounded the same session and DEFERRED** by the owner (consumer-less at
landing). **E-71 was found already discharged** at tip. No pre-ranked next cut — the [OWNER STOP] below.

## Read first (resume authority, in order)
1. **`p6.3-fusion-detection-grounding.md`** — the LANDED P6.3-detection record: §1 the cut, §2 why
   sound + honestly-bounded value, §3 the diff at hunk grain, §4 IR desired states, §5 the 18 moved
   goldens, §6 the exit gate, §8 the refuter verdict PROCEED + folded amendments. The method exemplar
   for a detect-only compile-time cut.
2. **`session-26-whole-program-seed.md`** — the grounded whole-program pseudocode (POST-P7c). Still
   accurate EXCEPT: `ClassifyFusableComponents` now runs in `FrozenRegionalProgram::Build` after
   `PromoteSharedSymbolicField` (`Planning.cpp:760`, before census), populating the two new P6.3 fields
   on `RecursiveComponent` (`Fusion fusion`, `std::vector<uint32_t> binding_prefix`) and rendering a
   gated `fused-fixpoint`/`joint-fixpoint` block in `-region-out` (after the P6.2 shared-field block,
   `Format.cpp`). Re-verify anchors at tip — the pipeline drifts each session.
3. **`p9-inference-grounding.md`** — the DEFERRED P9 design (verdict SOUND-BUT-SPECULATIVE). Its
   RECOMMENDED formulation is **clause-source-at-freeze** (the P6.2 pattern), NOT the seed doc's
   deleted pre-Optimize DataFlow walk. Load-bearing open finding: P9 is **consumer-less** — the
   four-authority firewall bars `SelectAccessPlan` from reading it, so it needs an explicit
   firewall-relaxation decision to earn a reader before it delivers anything but observability.
4. **memory `regional-dataflow-core-epoch`** (P6.3 banner at the head) + **`greenfield-rewrite-motivation`**.
5. For the runtime layers: **`keyed-rewrite-reconstruction-diffs.md §P6.3–P6.6`** (the
   `PlanRecursiveComponent`/`EvaluateEpoch` pseudocode — **P6.3 DETECTION is now LANDED**; P6.4
   activation / P6.5 joint fixpoint + DRed / P6.6 rooted-reachability retire remain, the B0/F1
   two-pass fix is the load-bearing correction) + memory `demand-cost-model` /
   `mobius-differential-dataflow` / `free-termination-paper`. For the physical layers:
   **`keyed-rewrite-p7p9-diffs.md §2 (P8)`** (partly stale — trust the whole-program seed §2).

## The [OWNER STOP] — pick the next cut, then DOCS-ONLY grounding until execution is green-lit
No pre-ranked next cut. CONFIRM with the owner first:

- **P6.4–P6.6 — the rest of runtime evaluation (the biggest, toward replacing M3, the FIRST M3
  divergence).** P6.3 detection now PINS which components are fusable; P6.4 lights up the
  `RuleActivationEdge` across recursive components, P6.5 is the joint signed-frontier rooted-reachability
  + semi-naive fixpoint (with the B0/F1 two-pass DRed — pass (A) within-epoch support-loss independent of
  root liveness, pass (B) rooted worklist), P6.6 is per-fact DRed + drain-before-retire. LARGE; must be
  sub-sliced. The natural first slice: **P6.4 activation-edge DETECTION/derivation** (still compile-time
  or a minimal runtime seam), consuming P6.3's `fusion`/`binding_prefix`. This is the first cut that will
  actually MOVE codegen/runtime and diverge from M3 — the gate stops being purely structural.
- **P8 — the cross-relation ordered TRIE (Free Join / COLT; heaviest).** NEW runtime range/trie
  structure (none exists; all-hash today), `BindingStateId` node interning, ordered `.Range` subtree
  DFS. NB the intra-relation prefix seek is ALREADY P7 — P8 is PURELY the cross-relation ordered case.
- **P9 — access-path inference (grounded, DEFERRED).** Ready to execute (clause-source-at-freeze, ~6
  hunks, 2 existing region goldens + 1 new witness) BUT consumer-less until a firewall-relaxation
  decision. Only pick this if the owner wants the observability surface / to bundle it with the firewall
  decision that gives it a reader.
- **Smaller.** None known on-theme (E-71 discharged; P7c was total for the seek path — do not invent a
  P7d re-check retire).

Then run the grounding loop below **DOCS-ONLY**; present the design + exit gate and STOP for the
execution go/no-go. On green-light, execute as one coherent commit (SUITE PASS, ctest 5/5; goldens
re-blessed after review; codegen byte-stable for every non-affected program — UNLESS the cut is P6.4+
which deliberately moves codegen, in which case the gate is a NEW emitted evaluation path cross-checked
against the M3 oracle/behavioral goldens, which MUST stay byte-identical). Then update CLAUDE.md +
memory + write the session-28 seed.

## Method — the grounding loop, run via WORKFLOWS with opus + sonnet (the loop that landed P2–P6.3)
Thin orchestrator; sequential single-phase workflows; watch the `(await parallel(...)).filter(...)`
precedence. DOCS-ONLY until green-lit.
1. **As-is pseudocode** — start from the whole-program seed §1 + the P6.3 delta above; **re-verify every
   anchor at tip** (sonnet).
2. **Design-goal diffs** at hunk grain (opus); each with a DISCRIMINATING STRUCTURAL exit gate (or, for
   P6.4+, a new-emitted-path gate cross-checked against the M3 oracle).
3. **Critique adversarially** (opus refuter panel) — **VERIFY EMPIRICALLY**: for a codegen-moving cut,
   an in-tree-then-revert OR throwaway-worktree spike (apply, build, run the FULL suite, measure the true
   golden move + answer-invariance). P6.3's predict-then-verify matched the carriers EXACTLY before bless
   (`corecursion_1`→`(A)`, `two_inductions`→joint, `tc_nonlinear_diff`→`(From,To)`, `key_corecursion_1`→
   `(K)`, `recursion` nodf/none→5 self-loops) — the clean template.
4. **IR desired states** (predict-then-verify, STRUCTURAL pins); decide exactly which goldens MOVE.

Model tiering (memory `subagent-model-tiering`): **sonnet** = anchor re-verification, baseline dumps;
**opus** = design diffs, refuter panel, IR desired-states. Keep the orchestrator thin.

## Gotchas (carried)
- clangd diagnostics in this repo are NOISE (no include paths) — trust the real build only.
- macOS bash 3.2 / zsh word-splitting: use `${=var}` when a var holds multiple CLI args. `.dr` ASCII-only.
- Run builds/suite SILENT on success; the full OptDiff suite takes ~3–4 min — run it BACKGROUNDED and
  await. Never run bench concurrently with the suite.
- `runall.sh --bless <workroot> [filter]` takes NO jobs arg (it PROMOTES from an existing workroot; run
  the suite into that workroot FIRST). Bless ONLY after reviewing the delta; never through a symlink
  golden; never to make a red case green. A `SUITE: FAIL` whose ONLY divergences are the predicted
  additive-suffix region goldens is the expected pre-bless state (P6.3's 18 were exactly that).
- P6.3 detect-only invariant to preserve: the `RecursiveComponent::fusion`/`binding_prefix` fields are
  NEVER hashed and NEVER read by codegen. If P6.4 makes a consumer read them, that consumer is where M3
  divergence begins — gate it against the oracle/behavioral goldens, and re-check the F16 join-multiplicity
  laxness named in `Regional.h`'s `binding_prefix` comment (an ordinal is marked preserved on ANY in-cycle
  identity route, without promotion's single-source guard).
