I have decisive evidence. Verdict below.

---

# REFUTER #3 (scope-shadow-faithfulness) — Findings

Grounded against tip + `docs/proposals/InstanceFlow.md` §6/§7.3/§7.6/§8/§16/§17. I confirm the byte-identity *core* (grove built post-Optimize, pure read, `det_seq` stable, `row_contracts` precedent) genuinely holds — but the **faithfulness** half fails hard: the flat family model the design picks is a third model matching neither §6's type system nor §16's dump grammar, and the corpus (`transitive_closure`) proves it cannot be deferred to Phase D.

---

## F1 — BLOCKING — the single monolithic `FlatFamily` is untypeable under §6 `scc_ownership`; per-node `scc` is explicitly forbidden

**§6 verbatim** (`InstanceFlow.md:524-528`):
> "Every sum above is a real tagged variant. **A family does not have nullable `probe`, `interest`, `scc`, or `arrangement` collaborators.**"

and `FamilyTemplate.scc_ownership: Acyclic | WholeQueryScc(QuerySccId)` (`:503`) is a **per-family, singular** typed field.

The design (§3 data model + R3) ships **one** `FlatFamily{ id=if#0 }` mirroring the *whole* DAG and adds `std::optional<QuerySccId> scc` **to `FamilyNode`**. That is precisely the nullable-`scc` collaborator §6 outlaws, moved onto the wrong object.

**Concrete witness — `transitive_closure` (IF4(c)):** the single flat family contains SCC-interior origins `{merge.11, tuple.1, tuple.2, tuple.3, join.10}` *and* acyclic downstream `{tuple.6, insert.13, tuple.4, merge.12, insert.15}`. Such a family is **neither** `Acyclic` **nor** `WholeQueryScc(_)` — the §6 variant cannot be assigned. §7.6 ("A family never contains half of a recursive SCC") is satisfied, but the converse obligation — a family is *wholly* one SCC *or* wholly acyclic — is violated the moment a family holds an SCC **plus** its acyclic tail.

The faithful flat baseline must therefore **already split at SCC boundaries into ≥2 families at empty context** (≥1 `WholeQueryScc`, ≥1 `Acyclic`). This is not a Phase-D concern — it is forced by the §6 type on day one.

**FIX:** the flat baseline is a *set* of empty-context families (§6 `families: ordered map FamilyId -> FamilyTemplate`, not a lone `FlatFamily`), partitioned by SCC-condensation; each family typed `Acyclic|WholeQueryScc`. Drop `FamilyNode.scc`. `-instanceflow-out` renders N `family if#k` blocks now, so no re-baseline at Phase D.

---

## F2 — BLOCKING — the design's dump grammar is not a subset of §16; every `-instanceflow-out` golden re-bases at Phase D

The task's whole point is stable goldens. But the design's §4 grammar structurally diverges from the **canonical** §16 dump (`InstanceFlow.md:1508-1522`):

| §16 canonical | design §4 | drift |
|---|---|---|
| `family if#3 root=u#41 context=(…) activation=…` | `family if#0 context=empty` | **no `root=`** (§6 `FamilyTemplate.root_use` omitted) |
| `covers u#41 domain=all authority=ea#7` **inside** the family block | top-level `coverage` block, line `u#0 use q#0->q#9 role=copied col=0 …` | coverage relocated + different line shape |
| `node if#3.0 origin=q#22 join role=root residual=(A,B)` | `node if#0.0 origin=q#0 select residual=(From,To)` | **no `role=`** token (§6 `FamilyNode.occurrence: OccurrenceRole`, `:501`, omitted from the struct) |

When Phase D lands, families multiply and gain `root=`, nodes gain `role=`, coverage moves inside the family — **every node/family/coverage line in every `-instanceflow-out` golden moves.** `join_1`, `merge_2`, `transitive_closure` goldens all churn. That is exactly the rework the owner's "will not need rework when Phase B/D land" gate forbids.

**FIX:** emit the §16 grammar *now* — `family if#k root=u#N`, per-node `role=<occurrence>`, coverage nested under its family. Populate `FamilyTemplate.root_use` (available: the root `OriginUseId` per F4) and `FamilyNode.occurrence` from the producer's `InputColumnRole` at the emission site.

---

## F3 — MAJOR — the `gInstanceFlowTailPass` function-pointer hook makes the "always-on" validators conditionally-off, contradicting §17

**§17 verbatim** (`:1577-1578`):
> "Run validators **unconditionally** in compiler builds used for correctness, not only when a dump or optimization flag is enabled."

The design's own §1 concedes the hook is "`nullptr` in a DataFlow-only link (tests that don't pull InstanceFlow)." I confirmed **four separate `Query::Build` call sites** exist beyond the CLI: `bin/Oracle/Main.cpp`, `bin/RefHarness/Main.cpp`, `bin/RefInterp/Main.cpp`, plus `bin/drlojekyll/Main.cpp` (grep result above). `bin/Oracle` is a **correctness-gate** binary (drlojekyll-oracle, runs on every `.batches` case). Under the hook design, unless each of those mains + every future test main calls `WireInstanceFlow()`, the "always-on" validators and the grove silently **do not run** there — the validators are on exactly where someone remembered to wire them. That is the opposite of §17's unconditional contract, and a latent hole: a builder bug caught only in the CLI path, green everywhere else.

Separately, the wiring `if (gInstanceFlowTailPass && !gInstanceFlowTailPass(...)) return std::nullopt;` conflates two failure modes the design never reconciles: §5 says violations `fprintf+abort` (survive NDEBUG), yet the bool-false path turns a *green* compile *red* (returns `nullopt`). If a validator ever returns false instead of aborting on a corpus case, that is a green→red regression, not byte-identity.

**FIX:** don't use a wired-nullable hook for an always-on invariant. Either fold the build+validate into `lib/DataFlow` (call directly at the tail, unconditional in every `Query::Build` caller — see F5), or make the pass a hard link dependency of every binary that builds a Query. Make validators abort-only; delete the `return std::nullopt` path (it is dead if they always abort, live-and-dangerous if they don't).

---

## F4 — MAJOR — `CandidateSeed` collapses §7.3's typed payloads; pinning `seeds` goldens now commits a vocabulary §7.3/§5.1 later change

§7.3 (`:626-632`) defines seeds with **structured** payloads:
```
BoundaryBinding(root_use, bound_columns)
JoinPivot(join_origin, equality_classes)
AggregateGroup(aggregate_origin, group_columns)
```

The design's struct is `CandidateSeed { Kind; QueryOriginId origin; std::vector<uint32_t> columns; }` and the dump pins `seed join-pivot origin=q#10 columns=(X)` / `seed boundary-binding query=reachable_from columns=(From)`. Two commitments contradict the doc:

1. **BoundaryBinding is keyed on `root_use` (an `OriginUseId`), not a `QueryOriginId`+name.** The design stores `origin: QueryOriginId` and renders `query=reachable_from` (a name). Phase D's `build_grove` (`:669-717`) iterates seeds whose root *is* a `root_use` and calls `certify_one_backward_growth`. The seed golden `query=reachable_from` must become `root=u#N` → golden moves.

2. **JoinPivot's payload is `equality_classes` (cross-view column sets from `NthInputPivotSet`), not the single output-pivot ordinal.** The design records `columns=(X)`. Worse, IF4(a) Stress #1 shows `join_1`'s pivot is bound to a *literal* (`A=1`), which §5.1 `EqualityClassProof`/`ContextTransfer::Bind` has **no vocabulary for** — so `columns=(X)` on a constant-bound pivot is not merely lossy, it records a class §5.1 cannot express. When Phase D grows this seed it must reshape the payload → golden moves.

Because the design deliberately dumps + goldens `seeds`, this is committed shape, not inert scratch.

**FIX:** either (a) record seeds in the §7.3 typed shape now (BoundaryBinding→`root=u#N`; JoinPivot→rendered equality classes) so Phase D reads them unchanged, or (b) **do not emit `seeds` in the goldened dump this slice** (compute-and-validate only), so no seed vocabulary is pinned before §5.1's literal-operand question is resolved.

---

## F5 — MAJOR — the separate-library layering rationale is internally contradicted by the `unique_ptr` incomplete-type storage

§1 argues for a distinct `lib/InstanceFlow` target with **only** `InstanceFlow→DataFlow`, storing the grove as `std::unique_ptr<InstanceFlowProgram>` (incomplete fwd-decl) on `QueryImpl`. But `QueryImpl::~QueryImpl` is **out-of-line** (`lib/DataFlow/Query.cpp:24`), and destroying a `unique_ptr<Incomplete>` requires the **complete** type at the dtor's TU. So `lib/DataFlow/Query.cpp` must `#include` the InstanceFlow header — a compile-time **`DataFlow→InstanceFlow` edge**, exactly the dependency the separate library was chosen to avoid. §1 even admits this ("simplest: give `QueryImpl` an out-of-line dtor that `#include`s the InstanceFlow header") without noticing it refutes the layering premise. IF2's alternative (by-value `InstanceFlowGrove instance_flow;` in `Query.h`, "mirror `row_contracts` exactly") is *worse* — it needs the complete type in the **header**, pulling the edge into everything that includes `Query.h`.

Net: for a slice with **no** Rel consumer, the separate library buys only a wire-once global, a nullable always-on hole (F3), and a layering edge it can't actually break. `row_contracts` — the cited precedent — is a plain by-value member in `lib/DataFlow` with no library split.

**FIX:** fold Phase A/B.1-B.2 into `lib/DataFlow` (by-value member beside `row_contracts`, direct unconditional tail call — dissolves F3 too). Split out `lib/InstanceFlow` only when Phase C actually introduces the `Rel→InstanceFlow` reader. Accept the Phase-C move as cheap, localized debt; it is strictly smaller than the hook+incomplete-type scaffolding proposed now.

---

## F6 — MINOR (concede + residual) — codegen byte-identity of the *core* holds; the residual is unproven validator-quiescence

Refutation attempts that **fail** (design survives here): the grove is built at the post-`row_contracts` tail (`Build.cpp:2654+`), after `Optimize`/CSE/`FinalizeColumnIDs`; `det_seq` is stamped once at `IdentifyInductions` (`Build.cpp:2633`) and not re-stamped before the slot (IF1 verified); the pass is a pure read (no `impl` graph mutation); the new `QueryImpl` member + `friend` + tag struct touch no existing `operator<<`. So `-df`/`-contract`/`-origin`/`.rel`/`.ir`/codegen goldens do not move from *storage or graph perturbation*. The `row_contracts` precedent (`Query.h:1234-1240`: "NOT present during Optimize... CSE/canonicalization have nothing to preserve") confirms the shape is safe.

**Residual risk (not refuted, must be discharged empirically):** the byte-identity claim is contingent on **no V-IF-\* validator firing on any corpus case** — a fire is `abort()`, i.e. a crash, not byte-identity. The design asserts flat reductions ("bijection onto uses/sites") without a corpus run. IF4(a)'s orphan origins (`compare.14/15`), the unsat `never` arm, and constant-ref edges are exactly the shapes most likely to trip a naïve bijection belt. Must run the full OptDiff suite with the validators live *before* claiming observer status.

---

## F7 — MINOR — `DerivationSiteId = per-live-INSERT` is faithful for the flat slice (concede R2)

Contra the design's own R2 worry: for the flat slice this is sound and total. §8.4's partition is per `(DerivationSiteId, CoverageDomain)`; `transitive_closure`'s `tc` gets 2 sites (insert.13/14) into 1 LC, two authorities over `domain=All` on **different** sites → no §8.4 overlap (`:895-899`). One-writer-per-site holds by construction. The Phase-D per-rule concern doesn't move any flat golden. No change needed this slice.

---

## Verdict

**The flat slice does NOT survive as framed.** The codegen byte-identity core is sound (F6), but the design is **not a faithful §6/§16 subset**: it invents a single rootless, role-less, per-node-`scc` family that (F1) is untypeable under §6's `scc_ownership` and outlawed by §6's "no nullable `scc`", and (F2) prints a non-§16 dump grammar — both **forced to re-base at Phase D**, on corpus cases (`transitive_closure`) that make the split non-deferrable. Reframe the baseline as SCC-partitioned empty-context families rendered in §16 grammar with `root_use`/`OccurrenceRole` populated (F1+F2), fold into `lib/DataFlow` to make validators genuinely always-on (F3+F5), and either type-match or drop the `seeds` dump (F4). With those, the slice becomes the true forward-compatible observer the owner asked for.