# Keyed-instance rewrite — deepened P7/P8/P9 (physical-layer) diffs

Session 14 (2026-08-06, branch `keyed-instances`, tip `46a404d4`). This is the
**physical-layer sibling** of `keyed-rewrite-reconstruction-diffs.md` (which
deepened P1 + the logical/residual reconstruction phases P2–P6). It deepens the
three physical-realization phases into operational diffs at implementer (hunk)
grain, formulates their design-goal diffs, and states DISCRIMINATING (structural,
never answer-only) exit gates.

Authority chain: `INDEX.md` → `next-session-prompt.md` (semantic authority: `@key`
is a relation-local ORDERED access-path declaration; a query is ONE binding
source; honoring a path is a semantic capability guarantee, NOT a physical
promise) → `session-14-whole-program-seed.md` (§2 four-authority target, §3
P4/P5/P7–P9, §4 tasks) → `keyed-rewrite-reconstruction-diffs.md` (§2 D4
label==emission, §3 P4 ProgramTableScanRegion reuse, §3 P5 BindingStateSchema /
BindingEdge / MaterializePrefixChain, §4 design-goal table) → **this file** (P7–P9
deepened) → `keyed-rewrite-ir-desired-states.md` (§5 `.rel` census, §6/§7
no-baseline carriers). The three sonnet reader first-cut maps this file builds on
are folded in verbatim-where-cited.

**State: docs only. Suite 251 PASS unchanged; nothing blessed; no production code
touched.** Phase 1 remains OWNER-GATED and destructive; P7–P9 sit strictly AFTER
P1–P6 in the roadmap.

### §0 Where P7–P9 sit, and which authority each touches

P1–P6 delete the demand machinery and rebuild the LOGICAL evaluation model
(RegionalFact / RequestEdge / RuleActivationEdge / BindingStateSchema). At the P6
tail the compiler answers correctly with `@key` inert (P1 full materialization) or
honestly key-filtered (P4 `FullScanFilter`), but no phase has yet re-provided a
physical structure faster than a scan. P7–P9 re-provide **physical capability**
without touching the logical answer. The four authorities established at P2–P6
stay separate:

| Authority | Owner phase | P7–P9 relationship |
|---|---|---|
| logical fact (`RegionalFactId` / `member_key`) | P2/P3 | UNTOUCHED — P7–P9 never change what a fact is |
| residual specialization (`BindingStateId` values) | P3/P5 | UNTOUCHED — P8 navigates it, never redefines it |
| logical access path (`DeclaredAccessPath`, ordered) | P5 seeds, **P9 extends** | **P9 feeds it** (additive inference) |
| physical structure (`AccessPlan` / `PhysicalAccessStructure`) | **P7 introduces, P8 extends** | **P7 maps logical→physical; P8 adds the trie** |

The load-bearing invariant across all three: **inference (P9) picks NO physical
structure; planning (P7) picks NO logical path.** P9 emits ordered field
sequences; P7 maps each to a structure; P8 adds one new structure. Answer identity
holds at every step because `FullScanFilter` is always a correct realization of
any logical path (`next-session-prompt.md:106-108`).

---

## §1 P7 — physical access planning (introduce the `AccessPlan` domain)

**Touches:** the physical-structure authority (new). **Reuses:** `EmitScan`
(`Database.cpp:3334-3426`), `ProgramTableScanRegion` (`Program.h:1097-1140`),
`GetOrCreateIndex` (`Data.cpp:348-372`). **New codegen: NONE** — the emission molds
already exist as the three `EmitScan` arms.

### §1.1 The domain — its own Rel domain, NOT the join `Lowering` enum (goal 1)

The single most load-bearing P7 correction: `AccessPlan` is a NEW typed-id domain,
disjoint from the advisory join `Lowering` enum (`kFullScan`/`kSectionWalk`,
`Rel.cpp:2432-2433`). The join `Lowering` badge stays a DR-only, unrefereed
advisory (F33); `AccessPlan` is the physical-structure authority with an
emission-fidelity referee. They are two different objects that happen to name
similar shapes; never fold one into the other.

```
# NEW domain (peer of the Identity.h typed-id battery). plan_kind field default = kUnplanned.
AccessPlan = variant {                     # the plan CHOSEN for one AccessRequirement
    kUnplanned,                             #  DEFAULT — the two LEGACY mints (Join.cpp:254, Build.h:469) leave
                                            #    it here; the §1.5 emission belt SKIPS it (⊗ B-P7, s15 R2).
    kFullScanFilter,                        #  <-> index=nullopt, partial/zero bound (EmitScan full-scan+filter :3392)
    kFullKeyExactProbe,                     #  <-> index=nullopt, ALL cols bound -> member.Find (keyed_probe :3387;
                                            #    ⊗ M-P7-exactkey: the all-col index is codegen-dead, so Find NOT idx)
    kFullKeyHashLookup(HashArrangement),    #  <-> index=Some, STRICT-subset bound (EmitScan keyed_chain :3381)
    # kTriePrefixWalk(TriePrefixIndex)      #  RESERVED — enters ONLY at P8, iff caps admit it (§2)
}

PhysicalAccessStructure = variant {
    HashArrangement {
        table          : DataTable          # ctx.model_table_of(rel) == view_to_model->table (Join.cpp:249-251)
        key_columns    : vector<ColIndex>   # positional == DataIndex::KeyColumns() col.Index()
        exact_full_key : bool               # true => keyed_probe (Find); false => keyed_chain (First/Next)
    }
    # TriePrefixIndex { table, ordered_prefix : vector<ColIndex> }   # P8 backing; no runtime today
}
```

### §1.2 CodegenPlanCapabilities — a compile-time SET enumerating the real EmitScan arms

```
CodegenPlanCapabilities = {                 # a compile-time constant SET, NOT a runtime flag
    kFullScanFilter,                         # ALWAYS in — the EmitScan full-scan+filter arm (Database.cpp:3392-3418)
    kFullKeyExactProbe,                      # in — the keyed_probe member.Find arm exists (:3387; all-cols-bound)
    kFullKeyHashLookup,                      # in — the keyed_chain First/Next arm exists (:3381; strict-subset bound)
    # kTriePrefixWalk                        # OUT until P8 lands the ordered trie + TrieRange arm (§2; Table.h all-hash)
}
# kUnplanned is NOT a cap — it is the ProgramTableScanRegionImpl default that SelectAccessPlan NEVER returns; the
#   legacy join/crossover mints (Join.cpp:254, Build.h:469) carry it, and the §1.5 emission belt skips it.
# D4 INVARIANT: every kind in this set maps to EXACTLY ONE ProgramTableScanRegion shape.
#   The set literally enumerates the EmitScan arms that EXIST. Growing codegen grows the set.
#   SelectAccessPlan may NEVER return a kind outside this set (would be a plan EmitScan cannot emit).
```

### §1.3 SelectAccessPlan — real dispatch (replaces the P4 stub `return kFullScanFilter`)

```
SelectAccessPlan(req : AccessRequirement, caps) -> AccessPlan:
    # req from P3/P5: req.relation, req.bound_field_positions (decl-ordinal, order-FREE SET
    #   drawn from the terminal BindingStateSchema field set), req.body.
    table = ctx.model_table_of(req.relation)          # Join.cpp:249-251 pattern
    bound = req.bound_field_positions

    if bound.empty():                                 # unkeyed rescan
        return kFullScanFilter

    if |bound| == table.Columns().size():             # ALL columns bound
        return kFullKeyExactProbe                      # -> member.Find (⊗ M-P7-exactkey: index=nullopt, NOT an idx)

    if kFullKeyHashLookup in caps and HasOrCanMintIndexOn(table, bound):   # STRICT subset
        return kFullKeyHashLookup(HashArrangement{table, sorted(bound)})   # -> First/Next, keyed_chain (sorted: H-P7)

    # kTriePrefixWalk arm inserts HERE at P8 (statically dead until kTriePrefixWalk in caps).

    return kFullScanFilter                            # HONEST FALLBACK — always in caps
# NO cost model: the specializations are correctness-preserving, the fallback always legal.
#   Mirrors today's ONLY selection logic (Join.cpp always GetOrCreateIndex on pivots).
```

`HasOrCanMintIndexOn` is `GetOrCreateIndex`'s dedup logic hoisted to a predicate:
because `GetOrCreateIndex(impl, cols)` `SortAndUnique`s `cols` (`Data.cpp:350`),
"can mint" is always true for any bound set today — the P7 arm is unconditional
once `bound` is a strict-and-nonempty subset. (P8's ordered variant is what makes
this discriminating.)

### §1.4 LowerAccessRequirement — mint a ProgramTableScanRegion (reuse, no new arm)

```
LowerAccessRequirement(req, ctx):                     # Rel/ControlFlow build, NOT codegen
    plan  = SelectAccessPlan(req, ctx.caps)
    table = ctx.model_table_of(req.relation)
    # H-P7-inkey-order (folded, VERIFIED s15 R1): bound_field_positions is an order-FREE SET, but
    #   EmitScan's keyed_chain (Database.cpp:3381) trusts in_vars in KeyColumns (ASCENDING) order
    #   (Data.cpp:350 SortAndUnique; the Key struct is laid out ascending). Join.cpp:268-278 and the
    #   #query cursor factory (Database.cpp:1820-1824) BOTH rebuild key_exprs by ITERATING KeyColumns;
    #   EmitScan does NOT reorder. So SORT bound_cols ascending HERE and build in_vars in that order,
    #   else First({...}) is mis-keyed and returns empty. (A mis-ordered [B,A] key silently under-answers.)
    bound_cols = sorted(req.bound_field_positions)    # ASCENDING -> in_cols (IndexedColumns), aligned to KeyColumns
    bound_vars = [ctx.VarFor(f) for f in bound_cols]  # -> in_vars (InputVariables); D1 terminal BindingStateId
    scan = ctx.program.CreateTableScanRegion(parent)  # == Join.cpp:254-256 pattern
    scan.table = table
    scan.plan_kind = plan.kind                        # NEW field on ProgramTableScanRegionImpl (Program.h:1632,
                                                      #   beside `index`) — see §1.5 for the kUnplanned/CSE contract
    switch plan.kind:                                 # EXHAUSTIVE: -Wswitch + assert(unreachable)
      case kFullScanFilter:     scan.index = nullopt                                  # => EmitScan else-arm
      case kFullKeyExactProbe:  scan.index = table.GetOrCreateIndex(impl, ALL_COLUMNS) # => keyed_probe (member.Find)
                                # ⊗ VERIFIED s15 (Database.cpp:3379-3380): keyed_probe REQUIRES region.Index()==Some.
                                #   The all-col index is codegen-DEAD (excluded from index_member @501 because
                                #   ValueColumns().empty()), so keyed_chain's `index_member.contains()` fails and
                                #   control FALLS to keyed_probe, which emits member.Find (the table's built-in hash).
                                #   So exact-probe carries index=Some (the dead all-col index) — NOT nullopt: an
                                #   index=nullopt here would fall to the else full-scan arm (a real miscompile).
                                #   The dead index is a pre-existing fiction (certification 3); killing it (M-P7-exactkey
                                #   Option B: relax keyed_probe to fire with index=nullopt) is a follow-on codegen
                                #   honesty cleanup, NOT a P7 prerequisite. P7 REUSES EmitScan byte-unchanged.
      case kFullKeyHashLookup:  scan.index = table.GetOrCreateIndex(impl, plan.key_columns)  # => keyed_chain
      # case kTriePrefixWalk:   unreachable at P7 (not in caps); P8 sets scan.index = an ORDERED trie index
      #                         (GetOrCreateOrderedIndex, kind=kTriePrefix) + emits the TrieRange arm — §2.2
    scan.in_cols = bound_cols
    scan.in_vars = bound_vars
    for col in table.columns: scan.out_vars.Create(...)   # ONE VAR per column (Join.cpp:268-270).
                                                          #   MANDATORY: bind_outputs (:3345-3356) emits
                                                          #   one `const auto v = r.<field>` per OutputVariable;
                                                          #   an empty out_vars => an empty body => wrong answer.
    scan.body = LowerRegion(req.body)
    return scan
# plan_kind is EXCLUDED from ProgramTableScanRegionImpl::Hash/Equals/MergeEqual (the S5' mint_tag precedent).
#   VERIFIED (s15 R2): Hash (Program.h:1611-1626) mixes the index ptr @1617; Equals (:1632-1681) gates on exact
#   table.get()/index.get() ptr equality @1642-1643; MergeEqual runs only AFTER Equals. A plan_kind NOT in
#   Hash/Equals is silently folded by MergeEqual — which is SAFE here precisely because EmitScan is label-blind
#   (grep -c lowering Database.cpp = 0): two Equals-equal scans have identical (table,index,in_vars) and emit the
#   IDENTICAL arm regardless of which plan_kind survives the merge. So the §1.5 belt tolerates the survivor. (This
#   supersedes the L-P7-plankind-hasheq "unspecified" flag with a proof, not a punt.) plan_kind is NOT a pure
#   function of index-presence — a kUnplanned join scan and a P7 kFullKeyHashLookup scan can BOTH have
#   index=Some-in-index_member — but since emission reads only (index, in_vars) and never plan_kind, a merge of
#   two Equals-equal scans emits one honest keyed_chain either way; whichever plan_kind survives, the belt passes.
```

### §1.5 The D4 Option-1 → Option-2 TRANSITION — the headline P7 correction

At P4 the AccessPlan-kind → region-kind map was INJECTIVE (kFullScanFilter is the
sole kind, `index=nullopt`), so D4 Option 1 held: label==emission was a
compile-time invariant of the `LowerAccessRequirement` switch, and V-PLAN-HONEST
was redundant. **P7 breaks injectivity.** Both `kFullScanFilter` (index=nullopt)
and `kFullKeyHashLookup` (index=Some) now lower to the SAME region kind
(`ProgramTableScanRegion`), discriminated by codegen ONLY via
`region.Index().has_value()`. `EmitScan` is label-blind (`grep -c lowering
Database.cpp = 0`), so a DR-tail belt can only assert `label ∈ caps`, never `label
== emission`. This is exactly the D4-predicted trigger.

**⊗ B-P7 correction (folded; the s14 critique premise itself CORRECTED at s15 R2).** The belt
CANNOT sit unconditionally at `EmitScan`'s head. `EmitScan` serves ALL `ProgramTableScanRegion`s,
and — VERIFIED s15 R2 — there are **TWO** pre-existing mint sites (the s14 critique's "sole mint
site" claim was WRONG on `sole`):

1. `Join.cpp:254-266` — join-pivot scan, `index` ALWAYS Some (pivot_cols non-empty, `:247`).
2. `Build.h:469-475` `BuildMaybeScanPartial` — the negation / on-cycle `@product` crossover
   partial-index scan. `index=Some` when the caller supplies ≥1 bound column (`in_col_indices`
   non-empty, `:462-465`); **`index=None` when ZERO bound columns — a genuine full-table-scan +
   TUPLECMP filter.** LIVE at 3 `Stratum.cpp` sites (`:1033` join-neg, `:1215` negate-fold, `:1333`
   product crossover; its own comment `:1332` says "Zero bound columns: full scan").

Neither site sets `plan_kind`. So an unconditional bijection assert with a `kFullScanFilter` default
fires `true==false` on every join, AND site 2's `index=None` arm is a PRE-EXISTING `index=nullopt`
scan that is NOT a P7 `FullScanFilter`. The fix is a `kUnplanned` sentinel default that the belt
**skips** — it keeps P7 a pure addition and needs zero edits at either legacy mint (the alternative,
threading `plan_kind` at every mint, would mean editing `Build.h:469` in BOTH its arms).

```
# D4 Option 2 (REQUIRED at P7): move V-PLAN-HONEST to the EMISSION site, branching on a plan_kind
#   threaded onto ProgramTableScanRegionImpl (the ONE point codegen stops being label-blind). NEVER a
#   DR-IR tail belt. plan_kind default = kUnplanned (the legacy join/crossover mints leave it there).
EmitScan(region):                                     # Database.cpp:3334, at the arm-selection head
    if region.plan_kind != kUnplanned:                # SKIP the pre-existing join/crossover scans (2 mints, R2)
        assert( region.plan_kind == kFullScanFilter    => (arm is full_scan_filter) ) # index=nullopt, NumRows loop
        assert( region.plan_kind == kFullKeyExactProbe => (arm is keyed_probe) )       # member.Find (index=Some but DEAD)
        assert( region.plan_kind == kFullKeyHashLookup => (arm is keyed_chain) )       # First/Next over a real idx
        assert( region.plan_kind == kTriePrefixWalk    => (arm is TrieRange) )         # P8; statically dead at P7
        # NB: the belt is per-kind ARM implications, NOT an Index()==nullopt biconditional — kFullKeyExactProbe
        #   has index=Some (the dead all-col index, ⊗ s15) yet is NOT a hash lookup. Arm-identity is the honest key.
    ... existing three-arm dispatch unchanged (P8 adds the TrieRange arm when kTriePrefixWalk enters caps) ...
# plan_kind is inert to answer semantics — a pure emission-fidelity witness. Codegen still SELECTS the
#   arm from Index() (§2), NOT from plan_kind; the belt only CHECKS the honest agreement.
#   The bijection is NO LONGER a strict `==` on (kFullScanFilter <=> index==nullopt): kFullKeyExactProbe
#   ALSO has index==nullopt (it Finds on the table, the all-col index being codegen-dead, M-P7-exactkey).
#   So the belt is a per-kind IMPLICATION set, not one biconditional — the R2/critique refinement.
```

F33 corollary (flagged, out of P7 scope but coupled): the surviving join-pivot
`kSectionWalk` label (`Rel.cpp:2432-2433`) sits in the identical epistemic
position — an advisory badge `EmitJoin` never reads. Apply the same Option-2 belt
to the join lowering site, or it stays unrefereed. P7 does not fix it; P7 pins the
pattern the join site should later adopt.

### §1.6 P7 exit gate — DISCRIMINATING (structural / loop-shape)

Answer-equality is a LOST CHECK: the P1 full-materialization baseline and the P4
honest `FullScanFilter` both already answer right, so a no-op P7 (always
`kFullScanFilter`) passes any answer test. Every pin is structural.

```
(1) CURSOR-SHAPE — THE load-bearing query-path discriminator (⊗ M-P7-loopshape, VERIFIED s15 R1 in the
      real generated header). The P1 baseline #query cursor (Database.cpp:1735-1829) keys on `pos`/`id`
      inside a `<name>_cursor` STRUCT; a P7 region read keys on the REGION cursor `s<id>` (cursor="s"+id,
      Database.cpp:3346). `pos` vs `s<id>` is the real distinguisher. This is PRIMARY because loop-shape
      (pin 2) does NOT discriminate on the query path — see the scoping note below.
      ⊗ PRECONDITION (critique p7-query-path-cursor-shape-scope): pin (1) discriminates ONLY IF P7 actually
        lowers the bound-#query READ to a ProgramTableScanRegion (the `s<id>` emitter). If P7 leaves the
        hand-emitted `<name>_cursor` factory (Database.cpp:1735) in place and only re-plans INTERIOR reads,
        `s<id>` never appears on the query path and BOTH pins collapse there. So the P7 deliverable MUST
        include relocating the bound-query answer to a region scan (P3/P4 RoutedResult over a
        ProgramTableScanRegion), OR pin (1) applies only to the interior path and the query path is left to
        the P3 RequestEdge/RoutedResult structural pins. State which in the P4/P7 landing.
(2) LOOP-SHAPE — the row-visit-count discriminator, VALID ONLY for the INTERIOR / rule-body path (no query
      cursor exists there):
      kFullKeyExactProbe        => "if (const uint32_t s<id> = <table>.Find(<bound...>);"      # visits <=1 row
      kFullKeyHashLookup chain  => "for (uint32_t s<id> = <idx>.First(<bound...>); ... = <idx>.Next"  # visits matches
      kFullScanFilter           => "for (uint32_t s<id>=0; s<id> < <table>.NumRows()" + "if (r<id>.<key>==<bound>"  # ALL rows
    ⊗ SCOPING (M-P7-loopshape): on the QUERY path this is NON-discriminating — an index-bearing relation's
      baseline #query cursor ALREADY emits `.First/.Next` (VERIFIED s15: key_neighborhood_witness's header
      shows `db.idx_41.First({Start})` / `pos = db.idx_41.Next(id)` because `neighborhood` carries an index).
      So a no-op P7 and a real P7 emit the SAME First/Next on the query path; only pin (1) cursor-shape
      distinguishes them. Loop-shape survives as a discriminator ONLY for the interior/rule-body scan.
(3) HONESTY belts (the §1.5 per-kind implication asserts, always-on, survive NDEBUG):
      plan_kind != kUnplanned  =>  (its implication from §1.5 holds);
      IndexedColumns().size() == InputVariables().size()  (the existing EmitScan :3405 assert).
(4) F33 COROLLARY pin: assert the join-pivot kSectionWalk site is EITHER refereed by the same
      belt OR explicitly recorded as an accepted advisory-only badge (no silent third state).
```

---

## §2 P8 — lazy shared tries / COLT / Free Join

**Touches:** the physical-structure authority (adds `TriePrefixIndex`) + physically
realizes the P5 partial-binding DAG (goal 6). Grounding legend: **[REUSE]** cited
real code; **[NEW]** greenfield.

### §2.0 The intra-relation prefix seek is P7, NOT P8 (⊗ s15 R5 + the s15 critique cluster)

The s15 grounding of the `.Range` residue first cleaved P8 into a cheap "P8a prefix-sibling `Index`" and
a genuinely-new "P8b ordered trie." The s15 refuter panel then verified that **P8a was redundant** — an
intra-relation strict-subset (prefix) seek is ALREADY delivered by **P7 `kFullKeyHashLookup`**: `EmitScan`
`keyed_chain` (`Database.cpp:3381`) fires when `index_member.contains(idx) && |in_vars| ==
|idx.KeyColumns()|`, so a `GetOrCreateIndex(bound_subset)` over ANY strict subset `S` mints an ordinary
non-dead index (`ValueColumns()` non-empty, so it IS in `index_member`), `keyed_chain` fires, and it emits
`idx.First(S-values)/Next` — enumerating exactly the rows matching the partial bind, no whole-table scan.
The critique CONFIRMED (findings `p8a-p7-redundant-duplicate-index`, `p8a-kind-tag-self-contradiction`,
`p8a-prefixindex-breaks-cse-safety-proof`) that a SEPARATE `region.PrefixIndex()` field + `keyed_prefix`
arm would (a) break the plan_kind CSE-safety proof (a THIRD emission-selecting input not folded into
Hash/Equals), (b) be unreachable (no plan kind / caps admission), and (c) mint a duplicate kHash index
aliasing the P7 one. So **there is no P8a.** The intra-relation prefix seek is P7:

- **P7 `kFullKeyHashLookup` delivers the intra-relation prefix seek.** Prefix SHARING falls out of
  `GetOrCreateIndex`'s order-free dedup for free: `@key(A)` and `@key(A,B)` share the `{A}` index; `[A,B]`
  and `[B,A]` share the `{A,B}` full index and each read mints its own bound-subset index on demand. This
  satisfies acceptance test 17 (no whole-table scan for a bound subset) and tests 8/9's convergence WITHOUT
  any new structure. `region.Index()` (already in Hash/Equals, `:1617,1642-1643`) is the CSE-correct
  emission input — no new field, no CSE hole.

- **P8 is PURELY the cross-relation ordered trie / COLT / Free Join (GENUINELY NEW).** A hash index gives
  an O(1) SEEK on ONE relation's subset but NOT ordered ENUMERATION and NOT a prefix SHARED ACROSS
  relations — which is what the §2.5 `InducedOrdering` Free Join variable-ordering spine needs (lock-step
  multi-relation descent). THIS is the real greenfield: interior nodes interned on `BindingStateId`
  (§2.1), the ordered `.Range` subtree DFS (§2.2, distinct from hash First/Next — it walks an ORDERED
  range, not one exact-key chain), the multi-depth `Clear()`+rebuild emission (absent today, R3), the
  `kind:{kHash,kTriePrefix}` tag so an ordered `[A,B]` index does NOT alias a hash `{A,B}` (§2.3), the
  `EmitJoin` lock-step rewrite (M-P8-freejoin), and its own `AccessPlan` kind `kTriePrefixWalk` gated OUT
  of caps until the trie lands. **~100% GREENFIELD applies to P8 (all of it is the ordered trie).**

The rest of §2 describes the P8 ordered trie. Where earlier text said "P8a," read "already P7
kFullKeyHashLookup"; where it said "P8b," read "P8."

### §2.1 LazyPathTrie — navigates the canonical DiffTable, never owns rows

```
# [NEW] runtime structure; peer of Index<Key> (Table.h:748), NOT a replacement.
# One LazyPathTrie per (canonical relation, KeyPathId) — the P5 declared/inferred path.
# Contrast the P1-deleted InstanceStore, which OWNED per-iid double-buffered row COPIES
#   (InstanceStore.h:19,335-336). The trie owns only interior nodes + terminal row-id chains;
#   rows stay in the DiffTable. No double-buffer: kInI/InNew already carry frozen/current
#   (Table.h:385-395), so P8 re-provides prefix NAVIGATION, not snapshot storage.

# ⊗ B-P8 (folded): the runtime trie node is INTERNED on BindingStateId = (schema, sorted value-map) in a
#   region-global pool — NOT the value-free schema. VERIFIED (s15 R3) that this is the reconciliation of BOTH
#   B-P8 (convergence) AND H-P8-schema-retrieval (value-partitioned retrieval): a (schema, sorted-values) node
#   is value-partitioned (so TrieFirst(full_key).head returns ONE value's rows, honoring full-key-EXACT,
#   Table.h:791-803) AND convergent (two value maps that sort-equal share ONE node — the P5 endpoint contract).
#   F11's "node keys on SCHEMA, never the value-bearing id" is a P5 COMPILE-LATTICE rule (the schema DAG /
#   trie SPINE), NOT the P8b runtime-navigation rule. The runtime node is a BindingStateId.
struct TrieNode:                            # the P8 cross-relation ordered trie (the intra-rel seek is P7, §2.0)
    id     : BindingStateId                # (schema, sorted value-map) — the region-global intern KEY (⊗ B-P8)
    schema : BindingStateSchemaId          # [REUSE-P5] the ORDER-FREE field SET this node's id projects to;
                                           #   the SCHEMA SPINE (interior shape) is what P5 renders (§10.1).
    edges  : map<added_field_id -> TrieNode>   # ORDER-SIGNIFICANT hop; the child is the INTERN of id ++ {f=v}
    head   : uint32_t = kNoRow             # terminal only: DiffTable row-id chain head

struct LazyPathTrie:
    pool : map<BindingStateId -> TrieNode>  # ⊗ B-P8: the REGION-GLOBAL intern pool — the node OWNER. edges
                                           #   only CACHE pool lookups; two paths reaching one BindingStateId
                                           #   get the SAME node object (convergence). NOT a per-node local map.
    root : intern(BindingStateId(region_inst, EmptySchema, {}))   # the empty-binding node
    next : Vec<uint32_t>                   # [REUSE] per-row id chain, byte-identical to Index::next
                                           #   (Table.h:766-773): shares the DiffTable row-id space;
                                           #   readers filter liveness via DiffTable membership preds.

# LAZY MINT-ON-VISIT: a node exists iff a declared path materialized it (P5 MaterializePrefixChain,
#   eager, compile-time) OR a runtime bind visited it. NEVER the power set.
intern(trie, bsid):                        # ⊗ B-P8: the ONE node constructor — canonical by BindingStateId
    if bsid not in trie.pool:
        trie.pool[bsid] = TrieNode(id=bsid, schema=bsid.schema)
    return trie.pool[bsid]
Navigate(trie, ordered_bound_fields, values):     # [NEW] — INTERNS on canonical id, so [A,B] & [B,A] CONVERGE
    node = trie.root
    for (f, v) in zip(ordered_bound_fields, values):          # ORDER-SIGNIFICANT traversal...
        child_bsid = BindingStateId(node.id.region_inst,      # ...but the child id is canonical:
                                    node.schema.fields ∪ {f}, # order-FREE field set
                                    sorted(node.id.bindings ++ {f=v}))   # SORTED value-map (⊗ B-P8)
        child = intern(trie, child_bsid)                      # region-global pool: convergent AND value-partitioned
        node.edges[f] = child                                 # edges only CACHE the interned node
        node = child
    return node
# CONVERGENCE (test 9): [A,B] visits root ->{A=a}->{A,B=a,b}; [B,A] visits root ->{B=b}->{A,B=a,b}. The two
#   terminal child_bsid values are ({A,B}, sorted{A=a,B=b}) — IDENTICAL — so intern() returns the SAME node.
#   The interior entries {A=a} vs {B=b} are legitimately distinct (different prefixes). TrieFirst(full_key)
#   is value-PARTITIONED (the node id carries the values), honoring full-key-EXACT (H-P8-schema-retrieval).

LinkRow(trie, row_id, row):                # [REUSE] at the fold's added_row site; mirrors Index::Add
    node = Navigate(trie, trie.path.ordered_fields, project(row, trie.path))
    grow(trie.next, row_id)
    trie.next[row_id] = node.head; node.head = row_id         # prepend, id-order-in (Table.h:764-773)
```

### §2.2 The P8 ORDERED-range read — the ONE genuinely-new terminal (⊗ s15 R5 + critique)

Two terminal shapes; only the second is new.

**Whole-key exact — already P7, NOT new.** An intra-relation strict-subset seek is `kFullKeyHashLookup`
(§2.0): `GetOrCreateIndex(bound_subset).First(values)/Next` over the matching row-id chain, emitted by the
EXISTING `keyed_chain` arm (`Database.cpp:3381-3386`). No new field, no new arm, no CSE hole (the index is
already in Hash/Equals). This is where the s15-first-draft "P8a prefix sibling `Index`" lives — folded
into P7 by the critique (§2.0).

**Ordered range — the ONE genuinely-new P8 codegen surface.** A hash `First/Next` enumerates ONE exact-key
chain; a Free Join / COLT descent needs to seek to an interior prefix node and enumerate its ORDERED
subtree (all extensions of the prefix, IN key order, so a sibling relation's cursor can advance in
lock-step). That is not expressible by the hash `Index` — it needs the ordered `TrieNode` (§2.1, interned
on `BindingStateId`) and a new `EmitAccessPlan` branch:

```
# TrieRange seeks to the interned prefix node, then DFS-enumerates the ordered subtree (interior edges in
#   key order; chain-walk each terminal head), filtering liveness via DiffTable InI/InNew/Present (:385-424).
TrieRange(trie, bound_prefix, values):     # [NEW] — the ordered range, NOT a hash First/Next
    node = Navigate(trie, bound_prefix.ordered_fields, values)   # §2.1 — region-global intern (⊗ B-P8)
    yield from SubtreeRows(node)                                 # ordered DFS; liveness re-tested by caller
RangeFirst/RangeNext lower this as a cursor pair (the EmitAccessPlan branch), the ordered analog of
    keyed_chain's First/Next; codegen mold is :3381-3386 but over the trie, not a hash Index.
# F20 COMPLETENESS: an UNBOUND read (bound.empty()) routes to kFullScanFilter (WHOLE DiffTable == complete
#   answer), NEVER a prefix-active subset — the ActiveSubset-trap guard (reconstruction-diffs.md:488).
```

This arm carries its OWN `AccessPlan` kind `kTriePrefixWalk`, admitted to `CodegenPlanCapabilities` ONLY
once the runtime trie + `TrieRange` land (§1.2 / §6#3). Until then it is statically dead and an
intra-relation prefix read is served by P7 `kFullKeyHashLookup` (correct, just unordered). **Compaction**
of the trie reuses the generic `table.Indices()` rebuild (`Database.cpp:3009-3034`, `Table.h:610-613`) —
VERIFIED s15 R3 that node identity (schema+sorted-values) is compaction-invariant; only the multi-depth
re-`Add` EMISSION is new (single-level today).

### §2.3 GetOrCreateIndex — the order-free → ordered hook (the Data.cpp:350 change)

```
# ⊗ H-P8-dedup-kind (folded, VERIFIED s15 R3): GetOrCreateIndex's dedup is kind-BLIND — Data.cpp:350
#   SortAndUnique + :357 ColumnSpec + :358-362 string-equality is the WHOLE identity test, and DataIndexImpl
#   (Program.h:91-106: id/column_spec/columns/mapped_columns/table) carries NO kind field. So TODAY an ordered
#   @key(A,B) prefix/trie request and a join-pivot HASH request over the same column SET return the SAME
#   TABLEINDEX object. §2.3 therefore CANNOT leave GetOrCreateIndex untouched.
# CHANGE (two parts):
#  (1) add `kind : {kHash, kTriePrefix}` to DataIndexImpl (Program.h:100, right after column_spec) and re-key
#      the dedup to (column_spec, kind) in Data.cpp:358-362 for BOTH minters. Thread `kind` through all 8 call
#      sites (Data.cpp:205 FillDataModel; Build.h:464; Join.cpp:264/400; Build.cpp:560/590/610/1705) — every
#      existing caller passes kind=kHash (BYTE-IDENTICAL: dedup key gains a constant column, so nothing moves).
#  (2) the ORDERED P8 trie is the ONLY kTriePrefix minter (order IS identity, [A,B] != [B,A]):
GetOrCreateOrderedIndex(impl, ordered_cols, kind=kTriePrefix):   # peer of GetOrCreateIndex, P8 trie only
    spec = ordered_column_spec(ordered_cols)                     # NO SortAndUnique — order IS identity
    return dedup_by(spec, kind)                                  # [A,B] and [B,A] are DISTINCT ordered indexes
# ⊗ CRITIQUE (p8a-kind-tag-self-contradiction, RESOLVED): there is NO separate "P8a prefix" hash minter with a
#   kTriePrefix tag — the intra-relation prefix seek is P7 kFullKeyHashLookup, whose GetOrCreateIndex(subset)
#   stays kind=kHash and SHARES with join-pivot hash indexes over the same set (test 14: no duplicated fact
#   index). The `kind` tag exists PURELY so an ordered kTriePrefix "0:1" index does not alias a hash "0:1"; it
#   is read only by EmitScan's arm selection + the §1.5 belt, never by answer logic.
```

### §2.4 How one TrieNode realizes the P5 partial-binding DAG (goal 6)

```
# P5 (reconstruction-diffs.md:461-471) is a COMPILE-TIME lattice: BindingStateSchema nodes
#   (order-free field sets) linked by order-significant BindingEdges, lazy MaterializePrefixChain,
#   CONVERGENT endpoints ([A,B] & [B,A] -> one {A,B} schema). P8 is its PHYSICAL realization:
#     P5 BindingStateSchema           <->  TrieNode.schema             (one-to-one)
#     P5 BindingEdge(parent,f,child)  <->  TrieNode.edges[f] hop
#     P5 convergence (one {A,B})      <->  merged terminal TrieNode    ([A,B],[B,A] -> ONE physical leaf)
#     P5 MaterializePrefixChain       <->  eager LinkRow along declared paths (compile-time)
#     P5 runtime BindingStateId       <->  a live Navigate() cursor position (F11: node keys on SCHEMA;
#                                          the CURSOR carries values — never store values in a node)
#     P5 F8 exit gate                 IS   the trie shape: @key(A,B,C) => root->{A}->{A,B}->{A,B,C}
#                                          exists; {A,C}/{B}/{C} ABSENT (mint-on-visit + declared-only).
```

### §2.5 InducedOrdering — Free Join / COLT (replace two order-free hash indexes with one ordered trie)

```
# [NEW] the compile-time choice of WHICH ordered path a trie realizes, so a multi-way join reuses
#   ONE trie prefix across relations (COLT = column-oriented lazy trie; Free Join = the trie IS the
#   join's variable-ordering spine). Refutes the "cubic self-join" premise (CLAUDE.md group_ids note):
#   the self-join already lowers to one table + two hash indexes + a pivot loop; InducedOrdering
#   replaces the two ORDER-FREE hash indexes (Data.cpp:350 SortAndUnique) with ONE ordered trie whose
#   prefix both pivot legs share.
InducedOrdering(join_region):              # ControlFlow build, beside Join.cpp:264/400
    order = topo_merge([ path.ordered_fields for rel in join_region.relations
                                             for path in AccessPaths(rel) ])   # [REUSE-P9 shape]
    for rel in join_region.relations:
        rel.access = TriePrefixIndex(rel.table, KeyPathId(order ∩ rel.fields))
    return FreeJoinSpine(order)            # lock-step multi-trie descent, not nested First/Next pivot loops
# CONSTRAINT (goal 4+7): InducedOrdering runs at the SAME site as the P6 joint fixpoint's routing.
#   It MUST NOT reorder fields in a way that breaks a co-recursive component's CommonPreservedPrefix
#   (reconstruction-diffs.md P6.3). InducedOrdering is a physical spine over an ALREADY-CHOSEN logical
#   routing; it never changes which fields flow (that is P6.2 SymbolicField unification).
# COLT LAZINESS: an interior node's subtree materializes only when a probe descends (Navigate
#   mint-on-visit). A cold prefix costs nothing — the "lazy" in LazyPathTrie. Matches InstanceStore's
#   demand-gated materialization WITHOUT its per-key row copies.
```

**⊗ M-cross-inducedordering — the STRUCTURAL belt, DEEPENED to operational grain (s15 R6, the residue
§6#6).** The §4 goals-4+7 belt was answer-equality ("published surface byte-identical with InducedOrdering
on vs off") — a LOST CHECK: a no-op InducedOrdering trivially passes `off ≡ off`. The belt must instead
ABORT on the ONE failure mode — a physical reorder that silently reroutes which `SymbolicField` crosses a
`RuleActivationEdge`. Grounded (s15 R6) in P6.2 (`reconstruction-diffs.md:516-521`, `RuleRoutingProjection`
+ `PromoteSharedSymbolicField` under UNANIMOUS producer agreement — shared spelling never promotes,
`next-session-prompt.md:201-205`) and P6.3 (`:539-546`, `CommonPreservedPrefix` over the PROMOTED classes,
computed AFTER the P6.2 fixpoint, F28):

```
V-INDUCEDORDER-SYMBOLIC-CONSISTENT(join_region, scc):   # freeze tail, peer of V-DECLARED-PATH-PRESERVED;
    # Runs once per join_region INSIDE a recursive_component (P6.1 SCC membership), AT the InducedOrdering
    # site (beside Join.cpp:264/400) so it sees the ACTUAL chosen order, not a re-derivation that could
    # silently diverge from what was emitted. Survives NDEBUG.
    chosen_order = join_region.induced_order            # §2.5 output — the physical spine
    classes      = symbolic_field_classes               # P6.2 post-fixpoint union-find (post-Promote)

    # (1) PERMUTATION CHECK — InducedOrdering may reshuffle WITHIN a class-equivalence, never RELABEL a
    #     field into a different class than P6.2 assigned it:
    for rel in join_region.relations:
        assert multiset(classes[f] for f in chosen_order if f in rel.fields)
            == multiset(classes[f] for f in rel.fields)      # ABORT: chosen_order changed a field's class
            # (e.g. treats p's K and q's X as one physical slot when P6.2 proved they are NOT unified —
            #  exactly a routing-changing reorder, indistinguishable from miscompile WITHOUT this assert).

    # (2) ACTIVATION-EDGE CROSSING CHECK — every field the SCC's RRE carries across stays in-class + placed:
    for edge in ActivationEdgesOf(scc):                 # P6.4 DeriveActivationEdge edges
        (rule, src_rel, dst_rel) = edge
        for (f_src, f_dst) in RuleRoutingProjection(rule):
            assert classes[f_src] == classes[f_dst]                          # P6.2 invariant re-affirmed
            assert PositionInOrder(chosen_order, f_src, src_rel) is not None  # both endpoints of the
            assert PositionInOrder(chosen_order, f_dst, dst_rel) is not None  #   crossing field resolve

# SECOND PIN (never answer-equality): a SHARED trie prefix IS emitted — the spine node/index count (§2.7),
#   not the published surface. Carrier: the co-recursive p(K,X)@key(K)/q(X,K)@key(X) DIFFERENT-KEY program
#   (routing (p,K)->(q,X)->(p,K); at tip it REJECTS via the mutual-recursion body-walk fence, so it is a
#   pure P6 target with NO baseline — CommonPreservedPrefix is empty, forcing JointFixpoint, and a
#   FusedFixpoint mis-selection here would fail pin (1) at the K!=X class boundary).
```

### §2.6 Reuse-vs-new ledger for the DataIndex/EmitScan seam

| Concern | Verdict | Ground |
|---|---|---|
| Whole-key `.First/.Next` chain | **[REUSE]** verbatim | `Table.h:804-824`, `Database.cpp:3381-3386` |
| Per-row `next[]` id chain, id-order-in | **[REUSE]** = `Index::Add`/`Next` | `Table.h:766-773,821` |
| Clear + rebuild-under-key-projection on compaction | **[REUSE]** the existing contract | `Table.h:611-613,780-787` |
| Row liveness via membership preds (no re-check) | **[REUSE]** | `Table.h:385-424,791-803` |
| Intra-relation prefix seek (bound subset) | **[REUSE]** — it is P7 `kFullKeyHashLookup`: `GetOrCreateIndex(subset)` + `keyed_chain` First/Next; NOT a new P8 structure (§2.0) | `Data.cpp:348-372`, `Database.cpp:3381-3386` |
| Prefix SHARING (`@key(A)` ⊂ `@key(A,B)`) + `[A,B]`/`[B,A]` full-key convergence | **[REUSE]** — falls out of `GetOrCreateIndex` order-free dedup | `Data.cpp:350,357-362` |
| P8 `TriePrefixIndex` ordered `column_spec` (order IS identity) | **[NEW]** — `GetOrCreateIndex` is order-free (`SortAndUnique`); needs the ordered variant + `kind` tag | `Data.cpp:348-372` |
| P8 ordered `.Range` subtree DFS (seek to prefix node, enumerate ordered subtree) | **[NEW]** — a hash `First/Next` walks ONE exact chain, not an ordered range; needs an `EmitAccessPlan` branch | `Database.cpp:3392-3418` |
| P8 interior shared-prefix node storage + multi-depth rebuild + `BindingStateId` interning | **[NEW]** — no nested structure exists; the rebuild loop is single-level (R3) | grep-confirmed empty, `Database.cpp:3017-3032` |
| P8 trie compaction | **[REUSE]** — generic `table.Indices()` rebuild; node id (schema+values) compaction-invariant | `Database.cpp:3009-3034`, `Table.h:610-613` |
| Keyed leaf cache (from deleted InstanceStore) | **[NEW]** — trie leaves over the canonical DiffTable (no double-buffer: kInI/InNew carry frozen/current) | `InstanceStore.h:108-133`, `Table.h:385-395` |

### §2.7 P8 exit gate — DISCRIMINATING (node-count / prefix-sharing, never answer)

```
(1) PREFIX-SHARING count — SCOPED to the COMPILE-TIME schema spine (⊗ M-P8-nodecount + critique
      p8a-3-not-4-pin-nondiscriminating: a DataIndex count is a LOST CHECK — the all-column {A,B} index
      ALWAYS exists (Data.cpp:205) and the {A}/{B} indexes pre-exist if the rel is join-pivoted, so a NO-OP
      passes "3 DataIndexes"; and 4 order-free hash siblings are structurally impossible (SortAndUnique)).
      Pin instead the P5 -region-out binding-schema block (§2.4 makes TrieNode<->BindingStateSchema
      one-to-one): for @key(A,B) + @key(B,A), EXACTLY ONE `binding-schema fields={A,B}` (S3) reachable by TWO
      `binding-edge`s, both `origin=declared` -> compile-time schema nodes {A},{B},{A,B} = 3, not 4. This is a
      COMPILE-TIME spine count (before any EvaluateEpoch), NOT a runtime node or DataIndex count.
(2) F20 COMPLETENESS: an UNBOUND read routes to kFullScanFilter, enumerating the WHOLE DiffTable == the
      complete answer (never a prefix-active subset). The ActiveSubset-trap guard. (P7: bound.empty() never
      mints an index seek; P8 trie: TrieRange(root) == full scan, count-equality assert.)
(3) DECLARED-ORIGIN spine (a shape pin, not answer-equality): a declared path renders `origin=declared`
      binding-schema nodes at COMPILE time; a `origin=visited` runtime node is NOT counted here (folds
      reconstruction-diffs §5.6 M7's declared-vs-visited origin tag into the render).
(4) D4/F33 belt (from §1.5, extended): once kTriePrefixWalk enters caps, the EmitScan plan_kind assert MUST
      have a live ordered-trie arm — assert( plan_kind==kTriePrefixWalk => arm is TrieRange ).
```

---

## §3 P9 — access-path inference (additive, logical-only)

**Touches:** the logical-access-path authority ONLY. P9 ADDS inferred ordered paths
into `DeclaredAccessPathSet`; it never picks a physical structure (that is P7) and
never overrides a declared `@key` (goal 1). It is the direct replacement for the
DELETED `V-DECLARED-KEY` bijection (`Demand.cpp:892-960`): equality mandate →
additive union.

### §3.1 InferAccessPaths — three sources, additive union

**⊗ B-P9 / H-P9-frozen-graph (folded; the s15 gating DECISION, EMPIRICALLY CONFIRMED).** P9 runs
**PRE-Optimize, at the `Build.cpp:2601` slot** the deleted demand pass occupied — NOT at Regional
freeze. The predict-then-verify was run this session: `demand_tc_witness` `.df` shows **56 tuple + 11
merge (nodf) → 37 tuple + 10 merge (opt)** — Optimize DELETES 19 forwarding TUPLEs + 1 single-source
MERGE, so the frozen (post-Optimize) graph does NOT present the chain `LiftBoundColumnsToRelation`
descends. Two independent code facts (s15 R4) seal it: (a) the `out_to_in` ORDER is
canonicalization-DISCLAIMED (`Query.h:834-838` TODO "I don't think the ordering invariant is
maintained through canonicalization"); (b) the `p_bound` SET is recovered by the very MERGE/TUPLE chain
Optimize deletes (`Demand.cpp:585-598,768-773` vs `Tuple.cpp:106-125`). So the walk MUST run
pre-Optimize; the ORDER signal is the **#query adornment parameter ordinal** (parse-stable,
Optimize-invariant, `Demand.cpp:817-828`), NOT `SortedPredecessors` (which is file-static ControlFlow,
unreachable from Regional, and B-P9-refuted). `out_to_in` stays a keyed `.find()` (`Demand.cpp:781`),
never iterated for order.

The pass deposits its result into a **`QueryImpl` MEMBER** — `impl->inferred_access_paths`, keyed by
`(relation_decl_id, PathSourceKey)` — computed at the Build tail. **⊗ CRITIQUE (satellite-lifetime,
CORRECTED):** the first draft cited `proxy_view_to_decl` as the precedent, but that map is Build-SCOPED and
"destroyed at return" (`Build.cpp:2578-2582`) with VIEW* keys that dangle after Optimize — it CANNOT reach
`FrozenRegionalProgram::Build` (a separate `Program::Build` phase, `Main.cpp:92` vs `:76`). The correct
precedent is `row_contracts` / `RecognizedSubgraphs` — surviving `QueryImpl` members keyed by
Optimize-stable PARSE identity (`relation_decl_id`), which Regional already friend-reads. `inferred_access_paths`
uses the same parse-identity keying so it survives Optimize AND the DataFlow→Regional handoff.
RAZOR (Avoid-false-start last bullet): this READS the graph, never MUTATES it, and deposits an IMMUTABLE
member — Regional/P2 reads the member, NOT a post-Optimize re-recognition of a mutated graph, so it stays on
the right side of "don't let DataFlow mutation + post-opt recognition remain the authority" (critique-certified).

```
# Runs PRE-Optimize (Build.cpp:2601 slot), over the pre-canonicalization Query graph + parsed decls.
#   Purely LOGICAL + read-only: reads graph shape, emits ordered field sequences into a satellite.
#   Chooses NO physical structure. P2/Regional later folds the satellite into schema.access_paths.
InferAccessPaths(impl) -> populates impl->inferred_access_paths:   # a QueryImpl MEMBER (⊗ satellite fix)
    for rel in impl.module.relations_with_a_decl:
        declared = DeclaredPathsOf(rel)                    # from @key pragmas — the invariant baseline
        inferred = OrderedPathSet()                        # unordered-unique by (ordered_fields, source)

        # Each OrderedPath carries a PathSourceKey so the union never drops non-adornment paths (⊗ CRITIQUE
        #   source23-adornment-key-undefined): kQueryAdorn(binding_pattern) | kJoinPivot(join_id) | kContextual(ctx_id).
        # SOURCE 1 — query adornments (a query is ONE binding source; next-session-prompt.md:96-99).
        for adorn in QueryAdornmentsProjectingInto(rel):   # dedup by BindingPattern (Demand.cpp:531-542)
            p_bound = LiftBoundColumnsToRelation(adorn, rel)   # §3.2 — PRE-Optimize walk (chain intact)
            if p_bound.nonempty():                             # ORDER = adornment param ordinal
                inferred.add(OrderedPath(p_bound, source=kQueryAdorn(adorn.BindingPattern())))

        # SOURCE 2 — join pivots. The SET is out_to_in's image (Query.h:832, keyed .find()); the ORDER is
        #   the relation's own decl-column ordinal (a DataFlow-visible, Optimize-invariant signal), NOT the
        #   ControlFlow SortedPredecessors probe order (B-P9-refuted: file-static, layer+authority violation).
        for join in JoinsReading(rel):                     # pre-Optimize graph
            pivot_cols = PivotColumnsOfRelInJoin(join, rel)         # out_to_in image (keyed find, §3.3)
            ordered = OrderByRelationDeclOrdinal(pivot_cols, rel)   # §3.3 — decl-ordinal, deterministic
            if ordered.nonempty(): inferred.add(OrderedPath(ordered, source=kJoinPivot(join.Id())))

        # SOURCE 3 — contextual (inherited) bindings (next-session-prompt.md:180-190).
        for ctx in InheritedBindingFieldsTouching(rel):
            for base in (declared ++ inferred):
                inferred.add(OrderedPath(ctx.ordered_fields ++ base.ordered_fields,
                                         source=kContextual(ctx.Id()), base=base.KeyPathId()))   # base back-ref (L-P9-split-source3)

        # ADDITIVE UNION — declared authoritative, ordered equality (NOT set equality).
        for p in inferred:
            if not declared.contains_exact_ordered(p):     # [A,B] != [B,A]: both survive
                impl.inferred_access_paths[(rel.decl.Id(), p.source)].add(p, provenance=kInferred)
    # Regional reads impl->inferred_access_paths; AssertDeclaredPathsPreserved runs at freeze (§3.4).
```

### §3.2 LiftBoundColumnsToRelation — first operational hunk (grounds Demand.cpp:556-643)

The SIP Step-2 projection lift is DELETED with `Demand.cpp` at P1, but its ALGORITHM is the seed: trace
a query's bound input columns through its post-Connect `MERGE`/forwarding-`TUPLE`s to the full-width read
of `rel`, recording the arrival positions. **⊗ H-P9-frozen-graph (folded):** this walk runs over the
**PRE-Optimize** graph (§3.1) — the forwarding TUPLEs (`Demand.cpp:768-773`) and single-source MERGE
(`Demand.cpp:585-598`) it descends are still present there (VERIFIED s15: they collapse only under
Optimize — the 19-tuple / 1-merge delta on `demand_tc_witness`). The former blocking prerequisite is
RESOLVED by the pass position, not by re-expressing the walk over post-Optimize invariants.

```
LiftBoundColumnsToRelation(adorn, rel):            # Demand.cpp:556-643, run PRE-Optimize (chain intact)
    bound_indices = [i for i,p in enumerate(adorn.Parameters()) if p.Binding()==kBound]  # Demand.cpp:537-542
    if bound_indices.empty(): return []            # all-free adornment is demand-inert (Demand.cpp:543-553)
    p_merge = FullWidthReaderMergeOf(rel)          # the single-member AsMerge the query projects into
    p_bound = []
    for i in bound_indices:                         # OUTER loop in ADORNMENT ORDER — this IS the order signal
        in_col = adorn.OutputColumnAt(i)
        while not IsFullWidthReaderOf(view_of(in_col), p_merge):   # descend forwarding TUPLEs/MERGE
            in_col = view_of(in_col).input_columns[in_col.Index()] # Demand.cpp:768-773 forwarding rule
        p_bound.append(in_col.Index())             # the arrival POSITION in rel — the lifted bound field
    # ORDER = decl parameter order of the bound params (the query's stated calling convention, adornment
    #   ordinal — Optimize-INVARIANT, s15 R4), NOT a key mandate. The old From-preservation reject
    #   (Demand.cpp:746-750, in_col.Index()!=pos) becomes the ORDER signal here, never an error.
    return [SymbolicFieldOf(rel, pos) for pos in p_bound]
```

### §3.3 OrderByRelationDeclOrdinal — first operational hunk (⊗ B-P9: DataFlow-visible order, no SortedPredecessors)

```
# ⊗ B-P9 (folded): the pivot-path ORDER is the relation's own decl-column ordinal — a DataFlow-visible,
#   Optimize-invariant signal reachable from the pre-Optimize graph. NOT SortedPredecessors (file-static
#   ControlFlow Join.cpp:142, unreachable from the inference pass, planning/logical authority-collapse) and
#   NOT out_to_in ITERATION order (canonicalization-disclaimed, Query.h:834-838). This also MOOTS the old
#   L-P9-boundat-noop min-vs-last defect — SortedPredecessors is gone entirely.
OrderByRelationDeclOrdinal(pivot_cols, rel):       # pivot_cols = out_to_in image (a keyed .find(), NOT iterated)
    return sorted(pivot_cols, key = lambda c: c.RelDeclOrdinal(rel))   # deterministic, parse-stable
    # The SIP "sideways" reject (Demand.cpp:746-750,801-805) becomes CLASSIFICATION (which field chains to
    #   which), never an error. NEGATE/AGG (Demand.cpp:703-706) simply TERMINATE a path here, never reject.
    # NOTE: decl-ordinal is a CONSERVATIVE order signal — it names a legal path, not necessarily the join's
    #   physically-best probe order (that is P7/P8's job, and P7 reads NO inference artifact — goal 1). An
    #   inferred path is a capability hint; the honest FullScanFilter realizes any order (next-session:106-108).
```

### §3.4 V-DECLARED-PATH-PRESERVED — the anti-regression referee

```
AssertDeclaredPathsPreserved(schema, declared_baseline):   # freeze tail, peer of V-REGION-CENSUS, survives NDEBUG
    for d in declared_baseline:
        matches = schema.access_paths.filter(p => p.relation==d.relation
                                              && p.ordered_fields==d.ordered_fields   # EXACT ordered eq
                                              && p.provenance==kDeclared)             # not demoted
        if matches.size() != 1:
            ABORT("V-DECLARED-PATH-PRESERVED: declared @key path (", names(d.ordered_fields), ") on '",
                  d.relation, "' was dropped/reordered/demoted by inference")
    # An inferred path MAY duplicate a declared endpoint at a DIFFERENT order — legal, additive; it never
    #   rewrites the declared entry's KeyPathId or ordered_fields.
```

### §3.5 The `-contract-out` render — provenance decision

CLAUDE.md's landed format is a single line per declared set carrying both halves:
`declared-key rel=… declared=(…) inferred=(…)`. That single-line format was the SIP
bijection's shape — `inferred=` was the SIP-inferred `p_bound` the declared set had
to EQUAL. Post-P9 the two halves are no longer paired by equality (declared and
inferred merely UNION), so a single line implying a pairing is misleading.

**⊗ H-P9-render-anchor (folded): the render MOVES DataFlow→Regional.** The current single-line renderer
(`DataFlow/Format.cpp:1745-1798`) iterates `qc.query.RecognizedSubgraphs()` (`:1747`) — a P1-DELETED
side table — and lives in a DataFlow dump that cannot see where P9 stores paths (the inference satellite
threaded into Regional `schema.access_paths`). So `-contract-out` becomes a **Regional/typed dump sourced
from `schema.access_paths` (provenance-tagged)**, composing with P2's "formatting derives from typed
records" mandate. It is NOT an in-place edit of the deleted `Format.cpp` block.

**DECISION: SPLIT into distinct provenance lines** (new Regional render target; F21 order-free):

```
declared-key rel=path declared=(From)              # provenance=kDeclared — ALWAYS present, even with no match
inferred-key rel=path inferred=(From, To)          # provenance=kInferred — additive, zero-or-more
```

Rationale: (a) a declared path with NO matching inference must still render (the
anti-regression witness) — the old single line would have printed an empty
`inferred=()` implying failure, or been dropped; (b) N declared × M inferred is
naturally N+M lines, not an N×M cross-pairing; (c) each line carries ONE
provenance, matching the `provenance` field on the path — no bridge. Ordering:
declared lines first, KeyPathId-sorted (F21: reordering pragmas → byte-identical);
inferred lines after, sorted by ordered_fields. (Owner scrutiny flag: this changes
the committed render shape vs the CLAUDE.md single-line description — the split is
the recommendation, but it is a render-golden-moving choice, so it is called out
for adjudication in §6.)

### §3.6 P9 exit gate — DISCRIMINATING (provenance survival, never answer)

```
(1) ANTI-REGRESSION (what the deleted bijection would have rejected): a declared @key path with NO
      matching query adornment / join pivot SURVIVES verbatim — one `declared-key` line, provenance
      kDeclared. The old V-DECLARED-KEY bijection ABORTED this exact case; P9 must keep it.
(2) ADDITIVE-NOT-MERGE: an inferred path duplicating a declared endpoint at a DIFFERENT order is ADDED
      (a distinct `inferred-key` line), never merged into the declared entry. [A,B] declared +
      [B,A] inferred => two lines, two KeyPathIds.
(3) F21 ORDER-FREE: reordering two @key pragmas => byte-identical `-contract-out` (KeyPathId-sorted render).
(4) SOURCE ISOLATION: an inferred path NEVER carries a physical structure token — `-contract-out` shows
      logical field sequences only; `AccessPlan`/`PhysicalAccessStructure` appear ONLY in `.rel`/codegen
      (P7). A physical token leaking into the contract render is a layer-violation finding.
```

---

## §4 Design-goal diffs (physical layer)

| # | Goal | Resolving hunks (this doc) | The load-bearing correction |
|---|------|-----------------------------|------------------------------|
| 1 | Four-authority separation (AccessPlan its own domain) | §1.1 AccessPlan domain; §1.5 Option-2 belt; §3.4/§3.6 P9 logical-only | AccessPlan is a NEW Rel domain, DISJOINT from the join `Lowering` enum; inference feeds LOGICAL only |
| 2 | Order-significant paths + order-free identity | §2.3 ordered `column_spec` GetOrCreateOrderedIndex; §2.4 TrieNode schema order-free / edge ordered | the trie PHYSICALLY realizes it: order-free node (schema=field set), order-significant edge |
| 5 | Honest non-scan emission | §1.5 V-PLAN-HONEST at the EMISSION site (kUnplanned-skipped, per-kind implications); §1.6 cursor-shape pin | at P7 the kind→region map STOPS being injective; the belt MOVES to EmitScan on a threaded plan_kind (excluded from Hash/Equals; safe because emission is label-blind) |
| 6 | Partial-binding DAG physically realized | intra-rel seek = P7 `kFullKeyHashLookup` (§2.0, convergence FREE via order-free dedup); cross-rel ordered spine = P8 interned trie (§2.1+§2.4) | P7 already delivers prefix seek + sharing; P8 trie interns nodes on BindingStateId (schema+sorted values), convergence ⇔ shared node |

**Goals 3, 4+7 stay sound (P7–P9 must not break them):**

- **Goal 3 (RequestEdge vs RuleActivationEdge).** The physical layer is
  EDGE-AGNOSTIC. P7 plans access to a relation's model table; P8 navigates rows;
  P9 infers field orders. None reads or mints a RequestEdge or RuleActivationEdge —
  those stay the P3/P6 ownership/derivation authorities untouched. No hunk in this
  doc constructs an edge.
- **Goals 4+7 (co-recursive key flow + rooted reachability).** The one hazard is
  §2.5 InducedOrdering running at the join site inside a recursive component. The
  constraint (stated in §2.5): InducedOrdering is a physical spine over an
  ALREADY-CHOSEN logical routing (P6.2 SymbolicField unification / P6.3
  CommonPreservedPrefix); it may pick a trie prefix but must NOT reorder fields in
  a way that changes which fields flow across the joint fixpoint. Exit belt (⊗
  M-cross-inducedordering, s15 R6 — the answer-equality "on vs off" belt was a LOST
  CHECK, off≡off vacuous): the STRUCTURAL `V-INDUCEDORDER-SYMBOLIC-CONSISTENT` assert
  (§2.5) — chosen_order is a permutation consistent with the P6.2 class multiset per
  relation, and every RuleActivationEdge's crossing field stays in-class + placed; a
  routing-changing reorder ABORTS. Rooted-reachability
  liveness (P6.6) is unaffected — the trie is a navigation cache over the canonical
  DiffTable, whose membership preds (`kInI`/`InNew`/`Present`) remain the sole
  liveness authority; a trie leaf never asserts presence.

---

## §5 The discriminating-exit-gate philosophy for the physical layer

Answer-equality is a LOST CHECK for every P7–P9 pin. The P1 full-materialization
baseline answers correctly with `@key` inert; the P4 honest `FullScanFilter`
answers correctly key-filtered. So a NO-OP P7 (always `kFullScanFilter`), a NO-OP
P8 (never mint a trie), and a NO-OP P9 (infer nothing) all pass any answer test.
**Every physical-layer pin MUST be structural / loop-shape / node-count / provenance
— never "the output matched".** The strongest per-phase structural belt:

| Phase | Strongest structural belt | Why it discriminates a no-op |
|---|---|---|
| P7 | **loop-shape grep** (§1.6.1): `Find`/`First`+`Next` vs `NumRows`+`if` | a no-op emits the NumRows loop; a real plan on a strict-subset bound emits First/Next — row-visit count differs |
| P7 | **plan_kind ⇔ Index() bijection assert** (§1.5) | a label that disagrees with the emitted arm aborts at compile |
| P8 | **prefix-shared node count** (§2.7.1): `[A,B]`&`[B,A]` ⇒ 3 interior nodes, not 4 | a per-path (non-shared) trie mints 4; the shared trie mints 3 — a pure structural count |
| P8 | **TrieRange(root) == full scan** (§2.7.2, F20) | guards the ActiveSubset trap that answer tests on a rooted query would MISS |
| P9 | **declared path with no inference SURVIVES** (§3.6.1) | exactly what the deleted bijection rejected; a regressed P9 drops the line while the answer is unchanged |
| P9 | **additive-not-merge** (§3.6.2): distinct KeyPathId per order | a merging P9 collapses `[A,B]`/`[B,A]` to one line; the answer is identical |

Cross-layer belt: **source isolation** (§3.6.4) — a physical token in
`-contract-out`, or a logical path token in `.rel`/codegen, is a layer-violation
finding regardless of answer correctness.

---

## §6 Open owner calls / least-grounded residue

1. **The intra-relation prefix seek is NOT a residue** (⊗ s15 R5 + critique): it is P7
   `kFullKeyHashLookup` (§2.0) — an ordinary hash `Index` over the bound subset + `keyed_chain` First/Next.
   The genuinely-new P8 surface is ONLY the ORDERED `.Range` (seek to a prefix node, DFS-enumerate the
   ordered subtree — distinct from a hash exact-key chain; §2.2) + the `EmitJoin` Free Join rewrite.
2. **The P8 ordered-trie runtime structure** (§2.1) is greenfield — no nested/sorted store exists (grep). Its
   compaction reuses the `Table.h:610-613` Clear+rebuild contract (VERIFIED s15 R3: node identity =
   schema+sorted-values is compaction-invariant, only the row-membership chain rebuilds — no NEW
   Table/CompactDead invariant), but the interior-node storage, mint-on-visit lifecycle, the multi-depth
   rebuild EMISSION (Database.cpp:3017-3032 is single-level today), and convergent-endpoint interning on
   `BindingStateId` (§2.1 `Navigate`/`intern`) are all new and unpinned by any golden.
3. **Whether `kTriePrefixWalk` ever enters `CodegenPlanCapabilities`** (§1.2) is a
   P8 owner gate: it MUST stay out until the runtime trie + `TrieRange` codegen
   land, or `SelectAccessPlan` can pick a plan `EmitScan` cannot emit, breaking
   label==emission. The `AccessPlan` variant reserves the arm; caps admission is
   the discrete P8→P7 handshake.
4. **The `-contract-out` render split** (§3.5) moves a committed golden shape (the
   CLAUDE.md single `declared-key … declared=(…) inferred=(…)` line → separate
   `declared-key`/`inferred-key` lines). Recommended, but owner-adjudicated
   because it re-blesses every `.contract` golden carrying a declared key
   (`key_multi_adorn_witness`, `key_tc_witness`'s real contract goldens).
5. **`LiftBoundColumnsToRelation` / `OrderByRelationDeclOrdinal`** (§3.2/§3.3) — **RESOLVED at s15
   (R4 + predict-then-verify).** The walk runs PRE-Optimize (Build.cpp:2601 slot), where the
   forwarding-TUPLE/single-source-MERGE chain is intact (the `demand_tc_witness` `.df` collapse — 19
   TUPLEs + 1 MERGE — was OBSERVED this session), so the H-P9-frozen-graph verification debt is closed by
   pass POSITION, not by re-expressing the walk. Order = adornment ordinal / rel-decl ordinal; the pass
   deposits a Build-scoped satellite Regional reads (§3.1).
6. **InducedOrdering vs the P6 joint fixpoint** (§2.5): **MECHANIZED at s15 (R6)** as the structural
   `V-INDUCEDORDER-SYMBOLIC-CONSISTENT` belt tied to P6.2's SymbolicField classes — a routing-changing
   reorder ABORTS (§2.5). Carrier is the co-recursive `p@key(K)/q@key(X)` (rejects at tip; a pure P6
   target). This replaces the answer-equality "on vs off" belt (a LOST CHECK).

---

## §7 Session-14 critique amendments (NOW FOLDED into §1–§3 at s15 — this section is the historical ledger)

**s15 STATUS: the three BLOCKING corrections + the two least-grounded residues are ABSORBED into §1–§3
as primary text** (marked `⊗ …(folded)` in place). §7 below is retained as the s14 provenance record.
The s15 grounding (WF `p7p9-ground-s15`) additionally CORRECTED two of the corrections and confirmed a
third empirically — carry these:

- **B-P7 was WRONG on "sole mint site"** (s15 R2): there are TWO `ProgramTableScanRegion` mints —
  `Join.cpp:254-266` (index=Some) AND `Build.h:469-475` `BuildMaybeScanPartial` (index=Some OR
  **index=None** full-scans, 3 live `Stratum.cpp` crossover sites). This STRENGTHENS the `kUnplanned`
  choice (the belt must skip a pre-existing `index=nullopt` scan that is NOT a P7 FullScanFilter) and
  adds the plan_kind/CSE proof (excluded from Hash/Equals; safe because emission is label-blind). §1.4/§1.5.
- **B-P9 SETTLED → Arm A (pre-Optimize inference), EMPIRICALLY** (s15 R4 + predict-then-verify): the
  `demand_tc_witness` `.df` collapses 19 forwarding TUPLEs + 1 single-source MERGE under Optimize, so
  the walk MUST run at the `Build.cpp:2601` slot and deposit a Build-scoped satellite; order = adornment
  ordinal (not `SortedPredecessors`, not iterated `out_to_in`). Resolves H-P9-frozen-graph by POSITION. §3.1-§3.3.
- **The `.Range` residue is MUCH cheaper than "a new trie"** (s15 R5): the intra-relation prefix seek
  reduces to a prefix-projected sibling `Index<PrefixKey>` reusing `First`/`Next` — P8 SPLITS into a cheap
  the intra-relation prefix seek (which the s15 critique folded into P7 kFullKeyHashLookup — convergence
  FREE via order-free dedup, no new structure) and the genuinely-new cross-relation ordered trie (where
  B-P8 node-interning on BindingStateId applies). §2.0/§2.2.
- **InducedOrdering belt** now the structural `V-INDUCEDORDER-SYMBOLIC-CONSISTENT` (s15 R6), not
  answer-equality. §2.5.

An independent opus refuter panel (`keyed-rewrite-p7p9-critique.md`, 15 survivors + 18
certifications) attacked §1–§6 against real code. The direction held; the amendments below are
REQUIRED. Each is verified at tip (file:line in the critique doc). `⊗` = a correction to the text above.

### §7.1 BLOCKING (each forces one real design correction)

```diff
⊗ B-P7 (§1.5) — the V-PLAN-HONEST belt at EmitScan's head ABORTS every join scan. EmitScan serves
    ALL ProgramTableScanRegions, and the PRE-EXISTING join-pivot mint site (Join.cpp:254-266) sets
    index=Some but never plan_kind; a kFullScanFilter default makes the bijection assert true==false.
-   [§1.5 as written: thread plan_kind only at LowerAccessRequirement]
+   plan_kind MUST be initialized at EVERY ProgramTableScanRegion mint site (add
+   plan_kind=kFullKeyHashLookup at Join.cpp:262), OR introduce a kUnplanned sentinel DEFAULT that the
+   §1.5 belt explicitly SKIPS (recommended — it keeps P7 a pure addition and needs no join-site edit),
+   OR scope the belt to LowerAccessRequirement-origin regions. State this in §1.4 AND §1.5.

⊗ B-P8 (§2.1/§2.4/§2.7.1) — the trie as written is VALUE-keyed (edges on (added_field_id,
    prefix_values_hash), children in a per-parent LOCAL map), so [A,B] and [B,A] with concrete values
    reach TWO physical leaves — refuting the convergence + "3-not-4" pin.
-   [§2.1 edges local map + §2.4 one-to-one TrieNode<->BindingStateSchema + §2.7.1 shared-{A,B}-leaf]
+   INTERN nodes in a region-global pool keyed by the canonical BindingStateId = (schema, sorted
+   value-map). This is ALSO the H-P8-schema-retrieval fix: a node keyed on (schema, sorted values)
+   is value-partitioned (so TrieFirst(full_key).head returns ONE value's rows, honoring the
+   full-key-EXACT no-recheck contract) AND convergent (two value maps that sort-equal share ONE node,
+   the P5 convergence contract). F11's "node keys on SCHEMA never the value-bearing id" is a P5
+   COMPILE-LATTICE rule (the schema DAG), NOT a P8 runtime-navigation rule — the runtime trie node
+   is a BindingStateId. Restate §2.4 as: P5 BindingStateSchema <-> the trie's SCHEMA SPINE (interior
+   shape); P5 runtime BindingStateId <-> a physical trie node. TrieFirst is a MUTATING seek (Navigate
+   mints), NOT the const Index::First — only TrieNext reuses Index::Next verbatim (certification 6).

⊗ B-P9 (§3.1 SOURCE 2 / §3.3) — the inferred-path ORDER must NOT derive from SortedPredecessors: it is
    file-STATIC in lib/ControlFlow/Build/Join.cpp:142 (Regional freeze provably cannot call it — the
    layering is ControlFlow->Regional->DataFlow), does not EXIST at freeze time, and collapses the
    planning/logical authorities (goal 1: inference reads NO planning artifact).
-   [§3.3 OrderPivotsByInducedBinding via SortedPredecessors index_in]
+   Derive the order from a DataFlow-visible signal reachable from Regional over the FROZEN Query graph:
+   the pivot columns' canonical field order (out_to_in image, Query.h:832) projected through decl
+   parameter order / SIP arrival order. §3.3 becomes a pure function of out_to_in + the relation's decl
+   ordinal; drop "induced by the chosen probe order" entirely. (This also moots L-P9-boundat-noop.)
```

### §7.2 HIGH

```diff
⊗ H-P7-inkey-order (§1.4) — bound_field_positions is an order-FREE SET but EmitScan's keyed_chain trusts
+   input_vars in KeyColumns (ascending) order (Data.cpp:350 SortAndUnique; Key struct Database.cpp:1102).
+   §1.4 MUST sort bound_cols ascending and build in_vars in that same order (mirror Join.cpp:268-278 /
+   the cursor factory Database.cpp:1820-1824 which rebuild key_exprs by iterating KeyColumns). Add a belt.
⊗ H-P8-dedup-kind-blind (§2.3) — GetOrCreateIndex dedups ONLY on column_spec (Data.cpp:357), kind-blind,
+   so an ordered @key(A,B) spec "0:1" ALIASES a join hash SortAndUnique{0,1}="0:1". §2.3 CANNOT leave
+   GetOrCreateIndex untouched: add a `kind:{kHash,kTriePrefix}` field to DataIndexImpl (Program.h:100)
+   and dedup on (column_spec, kind) in BOTH minters.
⊗ H-P9-frozen-graph (§3.2, §6.5) — UPGRADE §6.5 from "verification debt" to a BLOCKING PREREQUISITE.
+   ApplyDemandTransform (Build.cpp:2601) runs BEFORE Optimize (Build.cpp:2622); TUPLE canonicalization
+   (Tuple.cpp:60-125) DELETES the forwarding TUPLEs (Demand.cpp:768-773) and single-source MERGE
+   (Demand.cpp:585-598) the §3.2 walk descends/anchors on. RESOLUTION: either run inference over the
+   pre-Optimize graph (as the deleted pass did — the cleaner arm), OR rewrite §3.2 against post-Optimize
+   invariants (readers pull from as-far-up-as-possible; no forwarding chain). Predict-then-verify against
+   a real post-Optimize .df dump before trusting the descent.
⊗ H-P9-render-anchor (§3.5) — DataFlow/Format.cpp:1745-1798 reads the P1-DELETED RecognizedSubgraphs and
+   is a DataFlow dump that cannot see Regional schema.access_paths. RE-TARGET the declared/inferred split
+   render to a Regional/typed dump sourced from schema.access_paths (provenance-tagged); -contract-out
+   MOVES DataFlow->Regional (composes with the P2 "formatting derives from typed records" mandate).
```

### §7.3 MEDIUM / LOW (folded)

```
M-P7-loopshape (§1.6.1/§5): the loop-shape belt is NON-discriminating for the QUERY path — the baseline
  #query cursor already emits .First/.Next when an index exists (verified in key_neighborhood_witness's
  generated header: db.idx_41.First({Start})). §1.6.2 cursor-shape (pos vs s<id>) is THE query-path belt;
  §1.6.1 stays valid only for the interior/rule-body path. State this scoping.
M-P7-exactkey-fiction (§1.1/§1.3): exact_full_key uses member.Find on the TABLE, not an index; the
  all-column GetOrCreateIndex is dead (index_member excludes ValueColumns().empty(), Database.cpp:501).
  Represent exact-full-key as its OWN plan kind (kFullKeyExactProbe) emitting Find with index=nullopt;
  relax §1.5 to allow that kind with Index()==nullopt. (OUTCOME is correct today — certification 3.)
M-P8-freejoin-no-emitter (§2.5): FreeJoinSpine has NO codegen consumer (joins emit nested First/Next,
  Database.cpp:3169-3179). Add the EmitJoin lock-step rewrite to §6 as a GATING deliverable; gate
  BuildLazyOrdering behind a caps handshake like kTriePrefixWalk (else it is a label!=emission violation).
M-P8-nodecount (§2.7.1/§5): "3 not 4" is runtime-unstable (mint-on-visit) with no dump. SCOPE it to the
  EAGER MaterializePrefixChain schema spine (declared-origin, before any EvaluateEpoch); fold the
  reconstruction-diffs §5.6 M7 declared-vs-visited origin tag into TrieNode; OBSERVE it via the P5
  -region-out binding-schema block (§2.4's one-to-one makes that surface reusable). Do NOT present a
  runtime count as a golden.
M-cross-inducedordering (§4 goals 4+7): REPLACE the on/off answer-equality belt with a STRUCTURAL pin —
  assert InducedOrdering's chosen order is a permutation consistent with the P6.2 SymbolicField classes
  (a reorder that changes a co-recursive class ABORTS) + pin a shared trie prefix is emitted. Mechanizes §6#6.
L-P7-plankind-hasheq (§1.5): state plan_kind is EXCLUDED from Hash/Equals/MergeEqual (the mint_tag S5′
  precedent); the §1.5 assert tolerates whichever plan_kind survives a CSE merge (inert to emission).
L-P9-split-source3 (§3.5): the split drops SOURCE 3's base->derived pairing. Add a base back-reference
  token on contextual inferred-key lines, OR scope the "strict superset" claim to union sources (SOURCES 1/2).
L-P7-accessplan-arms (§1.1): next-session-prompt names a 5-arm AccessPlan; §1.1 collapses to 3. Either
  keep the 5-arm taxonomy (each structure carries its own refereed label) OR document the subsumption and
  how the §1.5 belt discriminates Enumerate/BuildLazyOrdering via the DataIndex.kind tag (H-P8 adds it).
L-cross-varfor-drift (§1.4): reconcile ctx.VarFor(f) (P7) vs ctx.VarFor(f,v) (reconstruction-diffs P4)
  in ONE place — cite the same M5 interner / D1 pull-forward; EmitScan's keyed arms need the bound VALUES.
```

### §7.4 What HELD (load-bearing certifications — do not re-litigate)

The §1.6.2 cursor-shape discriminator; the three-arm EmitScan taxonomy; the trie is NOT a second
fact owner (navigates the canonical DiffTable, liveness via membership preds); P9 SOURCE 1 treats a
query adornment as ONE inferred source not the key; §3.4 V-DECLARED-PATH-PRESERVED is a real
provenance-survival anti-regression belt; GetOrCreateIndex order-free grounds why an ordered trie is
the discriminating structure; the `-contract` goldens that move are exactly key_multi_adorn_witness +
key_tc_witness. Full list: `keyed-rewrite-p7p9-critique.md` Certifications 1–18.

### §7.5 Net status

Direction sound. The three blocking corrections are: thread `plan_kind` at every scan mint (or a
skipped sentinel); intern trie nodes on `BindingStateId` (schema + sorted values — value-partitioned
AND convergent); re-source the inferred-path order from a DataFlow signal. None invalidates the
four-authority target, AccessPlan-as-its-own-domain, or the ProgramTableScanRegion/EmitScan reuse.
The two strongest structural belts (P7 cursor-shape, P9 V-DECLARED-PATH-PRESERVED) survive intact.
