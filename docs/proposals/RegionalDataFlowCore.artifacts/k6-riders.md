# K6-riders — the one-authority artifact

**Slice**: K6-riders (seven small, independent follow-ons to the DIFF-R3
`@key` surface and the K1 multi-adornment landing).
**Repo tip at authoring**: `3bf87d56` (all extracts below are tip-exact
against this commit; each cited `file:line` is marked TIP-EXACT and must be
re-anchored if the tree moves before a hunk lands).
**Author agent session**: session 7 continuation, 2026-08-04.

## Owner ratification (2026-08-04, session 7 continuation)

- **Scope**: ALL SEVEN riders are in scope for this artifact (Part A grounded
  reality + Part B mini-diffs). Landing order is the panel's call; the diffs
  are mutually independent except where noted (DIFF-K6-3 storage is a
  pre-req the DIFF-K6-1/DIFF-K6-2 wording work does *not* depend on).
- **Cross-redeclaration `@key` semantics** (settles the CLAUDE.md "cross-
  REDECLARATION @key consistency is unchecked" recorded obligation):
  **IDENTICAL-OR-ABSENT**. A redeclaration that carries `@key` MUST declare
  the identical set-of-sets (order-free, per-set and across-sets) as every
  other pragma-bearing redeclaration of the same name+arity; a
  pragma-free redeclaration inherits the sibling's key SILENTLY (no error,
  no obligation to restate). Any divergence between two pragma-bearing
  redeclarations is a HARD REJECT.

The seven hunks, in this order: **DIFF-K6-1** ADVICE-FORK · **DIFF-K6-2**
WORDING NORMALIZATION (F-1) · **DIFF-K6-3** PRAGMA DisplayRange ·
**DIFF-K6-4** CROSS-REDECLARATION CONSISTENCY · **DIFF-K6-5** R-K1-BATCHES ·
**DIFF-K6-6** `KEY(_MissingVar)` WART · **DIFF-K6-7** DOT RIDERS.

Global golden-delta prediction (argued per hunk in Part B): **DIFF-K6-1..4
and DIFF-K6-6..7 change ZERO existing goldens; DIFF-K6-5 ADDS goldens
only.**

---

# PART A — grounded reality per rider

## A1. ADVICE-FORK (the demand__-collision advice mismatch)

`QueryImpl::ApplyDemandTransform` (`lib/DataFlow/Demand.cpp`, single
definition opening at `:388`, TIP-EXACT) computes one local
`const bool pragma_activated = !demand_key_decls.empty();` at `:429`
(TIP-EXACT) and closes over it in a generic `reject()` lambda at
`:447-453` (TIP-EXACT) whose advice suffix already forks correctly:

```cpp
447:  const auto reject = [&](const char *what) -> bool {
448:    log.Append(module.SpellingRange())
449:        << what
450:        << (pragma_activated ? "; fix or remove the @key pragma"
451:                             : "; recompile without -demand");
452:    return false;
453:  };
```

The comment at `:444-446` states the invariant: for a pragma-activated
compile, "recompile without -demand" is the NEC-2 silent lie — dropping the
flag does not deactivate an explicit `@key`.

Three OTHER rejects — the `demand__` prefix-collision guards inside Loop 2
(`for (PerAdornment &a : plan)`), at `:1042-1049`, `:1053-1059`, and
`:1064-1070` (TIP-EXACT) — hardcode the suffix
`"; rename it or recompile without -demand"` UNCONDITIONALLY and never
consult `pragma_activated`, even though they sit in the SAME function body
where `pragma_activated` is already in scope (confirmed: `:429` is a local
`const bool` in `ApplyDemandTransform`, Loop 2 is later in the same body;
no threading needed). This is precisely the NEC-2 silent-lie the `reject()`
comment calls out, un-fixed on three sibling paths. The fix is mechanical:
splice the same ternary onto each of the three suffixes.

## A2. WORDING DRIFT (F-1) — "demand key" vs "instance key"

The landed surface names the mechanism the **instance key** (`@key`
pragma). Diagnostic strings drifted: a census of user-facing
`log.Append`/`error_log.Append` chains (extract §2, all TIP-EXACT):

- `lib/DataFlow/Demand.cpp` — four "@key pragma" sites (`:450`, `:480-481`,
  `:874`/`:876`, `:931`/`:934`, `:946-947`): CONSISTENT with the surface
  ("@key pragma" is the correct name); `:480-481` says "declares a demand
  **key**" (describes the *act*, borderline, see below).
- `lib/Parse/Parser.cpp` — the `@key(...)` parse state machine (states
  21/22) says "instance key" at `:447`, `:911`, `:927`/`:930`, `:936`/`:938`,
  `:967`, `:986`, `:1003` — EXCEPT the lone outlier at **`:956-957`**
  (TIP-EXACT), the `kPuncComma`-with-`key_expect_var` arm:
  `"Expected named variable (capitalized identifier) in the "` /
  `"demand key of "` — the ONE "demand key" verbatim inside an otherwise
  all-"instance key" machine.

Comment-only occurrences (Format.cpp, Parse.cpp/.h, Database.cpp, Rel.*,
Program.h, Procedure.cpp — extract §2 list) are NOT diagnostic strings and
are OUT of scope for the F-1 wording fix (they are internal vocabulary, not
user-facing).

The wording fix is a single normalization at `Parser.cpp:957`. The
`Demand.cpp:480-481` "declares a demand key" is a judgment call: it names
the *act of declaring* rather than the mechanism; the panel may either
leave it (defensible) or normalize to "declares an instance key" for total
consistency. This artifact recommends normalizing it too (one extra token
swap, zero risk) and treats it as the second site of DIFF-K6-2.

**Golden-safety fact** (verified this session, tip 3bf87d56):
`grep -rln "demand key" tests/OptDiff/goldens/` → nothing;
`grep -rln "demand key" tests/OptDiff/cases/` → nothing. No golden and no
case-header comment quotes the old string. Diagnostic text is unpinned by
policy (rejects corpus pins the expected CLASS in the case header comment,
never the text).

## A3. PRAGMA DisplayRange (tight diagnostic anchoring for `@key`)

Today the four Step-2b / V-DECLARED-KEY-family rejects in Demand.cpp anchor
on `ParsedDeclaration::SpellingRange()` — never a pragma-tight range
(extract §3, all TIP-EXACT):

- `:479` — `d.SpellingRange()`, `d = demand_key_decls[0]`: the unseeded
  reject (a `@key` with no bound `#query`).
- `:873` — `d.SpellingRange()`, `d` over `demand_key_decls`: the RP-6
  realization reject (`@key` on a relation that is not the demanded target).
- `:930` — `p_demanded_decl.SpellingRange()`: **Arm A**, a declared key set
  with no matching query adornment (over-declaration).
- `:942` — `p_demanded_decl.SpellingRange()`: **Arm B**, a demanded
  adornment with no matching `@key` set (partial declaration — the surplus
  is INFERRED, not declared).

`ParsedDeclaration::SpellingRange()` (`lib/Parse/Parse.cpp:732-745`,
TIP-EXACT) spans the WHOLE declaration (`#local rel(...) @key(...).`) because
`impl->last_tok` is the closing `.` (set at Parser.cpp:826/850), after any
`@key`. So a multi-`@key` decl with one bad set carets the entire line, not
the offending set.

The parser CAPTURES but DISCARDS the material a tight range needs. The
per-set opening token lives in the local `key_pragma_tok`
(`Parser.cpp:397` decl, `:797` set on entry to state 21) and is used
transiently for the ADJ-K1-A dup-set reject at `Parser.cpp:985`
(`key_pragma_tok.SpellingRange()`, TIP-EXACT) — but never stored on the
impl. The closing `)` is `tok` at the `kPuncCloseParen` arm (state 22,
`Parser.cpp:964`, TIP-EXACT). Storage today is only
`std::vector<std::vector<unsigned>> instance_key_param_index_sets`
(`lib/Parse/Parse.h:387`, TIP-EXACT) — index sets, no ranges. The two
`DisplayRange`-member precedents in the file are both SINGULAR
(`ParsedAggregateImpl::spelling_range` at `Parse.h:223`;
`ParsedParameterImpl::opt_mutable_range` at `Parse.h:250`) — there is no
existing per-repeated-pragma range-vector pattern, so `@key`'s N-set case
is a NEW (but trivial) storage shape: a `std::vector<DisplayRange>` parallel
to `instance_key_param_index_sets`.

The **formatter is confirmed range-inert**: `lib/Parse/Format.cpp:114-125`
(TIP-EXACT) reads only `decl.HasInstanceKey()`, `decl.InstanceKeys()`, and
`NthParameter(i).Name()` — pure param-index data, zero token/range reads.
Adding a parallel range vector cannot perturb the round-trip.

## A4. CROSS-REDECLARATION CONSISTENCY

`instance_key_param_index_sets` is PER-IMPL (`Parse.h:381-387`, TIP-EXACT):
each textual `#local`/`#export` occurrence has its OWN sets; a redecl
without `@key` is simply EMPTY (no inheritance today). `DeclarationContext`
(`Parse.h:42-81`) carries NO `@key` field. This is the exact mechanism of
the "unchecked" obligation.

Redeclarations LINK via a shared `DeclarationContext` (`std::shared_ptr`),
keyed by name+arity in `ParserImpl::AddDecl<T>`
(`lib/Parse/Parser.h:219-273`, TIP-EXACT) — NOT via the
`first_redecl`/`next_redecl` fields (`Parse.h:370-371`), which are
functor-only (populated in `lib/Parse/Functor.cpp:751/754/756`). The redecl
branch (`:260`) constructs the second `ParsedDeclarationImpl` overload
(`Parse.cpp:169-179`) passing the first decl's `context`.
`UniqueRedeclarations()` (`Parse.cpp:952`) is a `USED_RANGE` over
`context->unique_redeclarations`.

All arity/type/binding reconciliation across redecls happens in
`ParserImpl::FinalizeDeclAndCheckConsistency`
(`lib/Parse/Parser.cpp:1465-1706`, TIP-EXACT), called after each decl
finishes parsing, with both `prev_decl` and `decl` in hand plus
`prev_decl_range`/`scope_range` for anchoring. It checks binding attributes
(`:1624-1641`), externally-visible names (`:1643-1662`), mutable-merge
(`:1664-1675`), parameter type (`:1677-1688`), and inline attribute
(`:1692-1703`) — and returns `true` at `:1705` with NO comparison of
`instance_key_param_index_sets` anywhere in the body (confirmed by full
read). The `@first`-attribute check (`:1525-1547`) and `@differential`
check (`:1549-1570`) are the same-shaped precedents to imitate.

The order-free set-of-sets canonicalization already exists in
Demand.cpp:902-905/917-920 (sort each set, compare) — reusable here.

## A5. R-K1-BATCHES (referee coverage for the multi-adornment flagships)

Neither `demand_multi_adorn_witness` nor `key_multi_adorn_witness` has a
`.batches`/`.probes` sidecar today (extract §2; verified this session:
`demand_multi_adorn_witness` owns only `region.{opt,nodf,nocf,none}` +
`stdout` goldens; `key_multi_adorn_witness` owns real `contract.opt` +
`rel.opt` goldens and a symlinked `stdout`). So neither runs the oracle,
monotone-projection, or I0 behavioral referees — the exact gap this rider
fills.

The `.batches` grammar (`bin/Oracle/Main.cpp:64-77`, TIP-EXACT) is
line-oriented `batch … end` blocks of `("+"|"-") msgname value*`; one block
= one epoch. `.probes` (`bin/RefInterp/Main.cpp:1388-1412`, TIP-EXACT) is
`<name>_<bindings> <bound-value>*` per line, matched against
`q.NameAsString() + "_" + BindingPattern()` — so `q_bf` and `q_fb` are
distinct, correctly-resolved tokens.

Both referee tools are demand-BLIND and multi-adornment-NATIVE:
- **Oracle** (`bin/Oracle/Main.cpp`) evaluates the plain graph with
  `suppress_demand=true` (`:735-753`, TIP-EXACT — a `@key` pragma must NOT
  activate the transform in the answer referee) and dumps every named
  relation's FULL extension via `DumpRelations` (`:2369-2409`). It has NO
  probe/binding mechanism (`main()` at `:2655-2712` takes only
  `<dr> <batches> [--project-monotone]`); it dumps the ONE full `q`
  relation once, which trivially subsumes both bf/fb probes. No special
  multi-adornment handling needed or present.
- **RefInterp/RefHarness** iterate `module.Queries()` (every `#query`
  redeclaration, un-deduped). RefInterp matches `.probes` tokens against
  `q_<bindings>` (`:1577-1578`); RefHarness emits one `drive_<ident>` per
  adornment (`ident = name + "_" + bindings`, `:256-294`, `:896-973`) and a
  per-adornment `RunProbe` arm (`:975-1005`), with an
  `if constexpr (requires { q_bf(...); })` overload probe (`:919-933`) that
  correctly resolves the forcing-augmented query signatures. The compiled
  frozen ABI (confirmed this session against a `-demand` gen/) exposes
  `q_bf`/`q_fb` cursors over ONE backing table `q_4` through two indexes +
  two injectors — N=2 disjoint stores, one pub, exactly as the header
  claims.

The CBF header embeds the case name (`bin/RefInterp/Main.cpp:1445-1453`;
RefHarness bakes the same, `:1052`), so a demand-twin and key-twin
behavioral golden differ ONLY on that one header line — hence the key-twin's
`.behavioral.stdout` must be a REAL golden, never a symlink.

The `key_tc_witness` symlink-vs-real precedent (extract §5, verified this
session): `stdout`/`oracle`/`monotone`/`df`/`rel`/`ir`/`h`/`region` are
SYMLINKS to `demand_tc_witness`'s; `contract.opt` + `behavioral.stdout` are
REAL (the CBF header embeds the case name). `key_multi_adorn_witness`
already follows the shape (`stdout` symlink; `contract.opt`/`rel.opt` real);
this rider extends it to `oracle`/`monotone` (symlinks) + `behavioral`
(real). The `bless_copy` guard (`runall.sh:110-133`, TIP-EXACT) REFUSES to
bless through a symlink and already names `key_multi_adorn_witness.stdout`
as an anticipated twin — the extension is the intended pattern.

## A6. `KEY(_MissingVar)` WART (dataflow DOT render)

The `-dot-out` dataflow twin (`lib/DataFlow/Format.cpp`,
`operator<<(OutputStream&, Query)`) renders `role=`/`KEY(...)` annotations
inside `do_table`. At `Format.cpp:129` (TIP-EXACT) it streams
`os << col.Variable();` directly — a `std::optional<ParsedVariable>` — and
the optional's `operator<<` (`lib/Parse/Format.cpp:24-31`, TIP-EXACT) prints
`_MissingVar` on `nullopt`. `QueryColumn::Variable()`
(`lib/DataFlow/Query.cpp:589-591`) returns `impl->var`, nullopt for a
fabricated demand column (minted by `FabricateDemandMessage`/
`FabricateDemandLocal`, no source-clause variable). So any
`@key`/demand-interior view's `KEY(...)` render prints `_MissingVar`. The
analogous plain-column render in `do_col` (`Format.cpp:157-169`, line 162)
has the same direct-stream bug.

The SAME file already codifies the correct idiom — the `-df-out`/`-ir-out`
`name_tok` lambda (`Format.cpp:887-901`, TIP-EXACT): guard `c.Variable()`,
else emit `c<id>`, with the explicit comment "Never `_MissingVar`".

**Golden-safety fact** (verified this session, tip 3bf87d56):
`grep -rln "_MissingVar" tests/OptDiff/goldens/` → nothing. The DOT surface
is advisory and never goldened (CLAUDE.md IR-observability directive). The
fix cannot touch a golden.

## A7. DOT RIDERS

**(a) region-DOT declared badge.** The `-region-dot-out` twin
(`lib/Regional/Format.cpp`, `operator<<(OutputStream&, FrozenRegionalDOT)`,
`:156-184`, TIP-EXACT) renders one `cluster_region_0` with `port_p*`,
`internal_*`, `proot_*`, `contract_e*` nodes as `label="…"` strings. The
pragma bit is REACHABLE at both mint sites (extract, DOT §2, TIP-EXACT):
- Row-contract mints (`lib/Regional/Planning.cpp:477-519` insert-derived,
  `:528-536` Tier-1 demand-interior) each bind a `ParsedDeclaration decl` —
  `decl.HasInstanceKey()` (`include/drlojekyll/Parse/Parse.h:447-448`) is
  directly callable.
- Request-port mint (`Planning.cpp:352-364`) binds `fdecl` (the `#query`);
  reaching the pragma needs a join against `query.RecognizedSubgraphs()` on
  `forcing_index == fi` to get `demanded_decl.HasInstanceKey()` — the same
  correlation `CollectDemandInteriorDecls`/`ResolveInteriorSupport` already
  do.

`RegionalContract` (`include/drlojekyll/Regional/Regional.h:108-113`) and
`RegionalPort` (`Regional.h:82-92`) have no badge field today — a `bool
declared_key` (or similar) is the additive.

**(b) NEW `-rel-dot-out` DR-IR DOT twin.** No DOT twin of the Rel/DR-IR flow
graph exists. The `-rel-out` TEXT emitter (`lib/Rel/Format.cpp`,
`EmitDRFlow`, `:392-1153`, TIP-EXACT) is the content model: header token
`rel` (`:405`), id-ordered vecs (`:409-424`, `def=[op.N]`/`use=[op.N]`),
branches/joins (`:440-474`), ops walked in the CHECKED LINEARIZATION
`pinned_order` (`:708-1062`), rounds substrate (`:1064-1081`), sorted deps
(`:1083-1110`), census tail (`:1112-1152`). Payload type is `DRFlowGraph`
(`lib/Rel/Rel.h:896-937`). The sink pattern to imitate (`Format.cpp:1155-
1168`, TIP-EXACT): a file-static `gRelDumpStream`, `SetRelDumpStream`
(public on `include/drlojekyll/ControlFlow/Format.h:17`), and
`DumpRelIfEnabled` PRE-guarded, drained from `Stratum.cpp` INSIDE
`Program::Build` (the DR graph exists only there — unlike the top-level
`-region-dot-out` drain).

Main.cpp wiring to clone: the globals block (`:57-64`), the install-before-
build `SetRelDumpStream(gRelStream)` (`:94-98`), the `-rel-out` arg arm
(`:419-434`), the `rel_out` unique_ptr local (`:319`), the help line
(`:239`). No shared DOT helper exists (`grep` for `DotEscape`/`namespace
dot`/`class Dot` → nothing); every emitter hand-rolls literals
(`DataFlow/Format.cpp:29-39` HTML-table locals; `Regional/Format.cpp` plain
`label="…"`). The cluster-per-stratum idiom to copy is
`DataFlow/Format.cpp:56-75` (cluster per `v.Stratum()`); for DR-IR the key
is `DROpStratum(flow, op)` per op (or `flow.rounds[].scc_group` for the
round substrate). No label text needs quote-escaping (ids/enum-spellings
only). Keep it MINIMAL: a faithful graph render (vecs + ops as nodes,
def/use edges), census-free, advisory, never goldened.

---

# PART B — dated mini-diffs

House convention per hunk: before/after pseudocode, exact diagnostic
strings, per-hunk golden-delta claim. Dated 2026-08-04.

## DIFF-K6-1 — ADVICE-FORK

**File**: `lib/DataFlow/Demand.cpp`, three sites in Loop 2 (`:1042-1049`,
`:1053-1059`, `:1064-1070`, TIP-EXACT). `pragma_activated` (`:429`) is
already in scope.

**Before** (all three, static suffix — shown for the first):
```cpp
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand declarations for '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it or recompile without -demand";
```

**After** (splice the `reject()` ternary from `:450-451` onto each suffix):
```cpp
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand declarations for '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it"
        << (pragma_activated ? "; fix or remove the @key pragma"
                             : " or recompile without -demand");
```

Site 2 (`:1053-1059`), body `"Cannot fabricate the demand message '"`,
gets the identical tail transform; site 3 (`:1064-1070`), body
`"Cannot fabricate the demand relation '" … "_local'"`, likewise.

**Exact new strings** (pragma-activated arm), by site:
- `"…prefix; rename it; fix or remove the @key pragma"` (declarations)
- `"…prefix; rename it; fix or remove the @key pragma"` (message)
- `"…prefix; rename it; fix or remove the @key pragma"` (relation)

The flag arm is byte-identical to today
(`"…prefix; rename it or recompile without -demand"`). Note the subtle
split: today's suffix is `"; rename it or recompile without -demand"` as one
token; the after-form splits at `"; rename it"` + the ternary so the
pragma arm reads `"; rename it; fix or remove the @key pragma"` and the flag
arm reproduces the original byte-for-byte via `" or recompile without
-demand"`. (A `reject(std::string)` overload is the alternative but these
build a `std::string` via `<<` onto a mutable prefix — the inline ternary is
the lower-friction change, matching `reject()`'s own shape.)

**Golden-delta claim: ZERO existing goldens.** These are compile-collision
reject paths (a `demand__`-prefix user collision), reachable only when
fabrication would collide — no golden `.stdout` case exercises them (all
corpus cases compile cleanly). The rejects corpus does not pin diagnostic
TEXT (expected class lives in the case header). The flag-arm bytes are
preserved exactly, so even a hypothetical future text-pinning reject case on
the flag path is safe.

## DIFF-K6-2 — WORDING NORMALIZATION (F-1)

**File**: `lib/Parse/Parser.cpp:956-957` (TIP-EXACT), the lone "demand key"
outlier; optionally `lib/DataFlow/Demand.cpp:480-481` (the "declares a
demand key" act-phrasing).

**Before** (`Parser.cpp:956-957`):
```cpp
            context->error_log.Append(scope_range, tok_range)
                << "Expected named variable (capitalized identifier) in the "
                << "demand key of " << local->KindName() << " '"
                << local->name << "', but got '" << tok << "' instead";
```

**After**:
```cpp
            context->error_log.Append(scope_range, tok_range)
                << "Expected named variable (capitalized identifier) in the "
                << "instance key of " << local->KindName() << " '"
                << local->name << "', but got '" << tok << "' instead";
```

**Exact string change**: `"demand key of "` → `"instance key of "` (one
token). This makes all thirteen Parser.cpp `@key`-machine sites say
"instance key" uniformly.

Optional second site (`Demand.cpp:480-481`, recommended): before
`"' declares a demand key but no "`, after
`"' declares an instance key but no "`. (Mirror at `:874` if the panel wants
total uniformity: `"' declares a demand key but is not "` →
`"' declares an instance key but is not "`. The extract flags `:874` as
"@key pragma"-family; re-read at implementation — if it says "demand key" it
joins this hunk, if "@key pragma" it is already fine.)

**Golden-delta claim: ZERO existing goldens.** Verified this session
(tip 3bf87d56): `grep -rln "demand key" tests/OptDiff/goldens/` → nothing;
`grep -rln "demand key" tests/OptDiff/cases/` → nothing. No golden and no
case-header comment quotes the string. Diagnostic text is unpinned by
policy. **CHECK RESULT: clean — no golden or header references the old
string.**

## DIFF-K6-3 — PRAGMA DisplayRange

**Files**: `lib/Parse/Parse.h` (storage), `lib/Parse/Parser.cpp` (capture),
`lib/Parse/Parse.cpp` + `include/drlojekyll/Parse/Parse.h` (accessor),
`lib/DataFlow/Demand.cpp` (re-anchor).

**Storage (Parse.h, after `:387`)** — parallel range vector:
```cpp
  std::vector<std::vector<unsigned>> instance_key_param_index_sets;
  // Per-set spelling range: `@key(` token .. the closing `)`.NextPosition().
  // Parallel to instance_key_param_index_sets (same index = same set).
  std::vector<DisplayRange> instance_key_ranges;
```

**Capture (Parser.cpp, the `kPuncCloseParen` arm, at the push at `:995`)**:
```cpp
          // Before:
          local->instance_key_param_index_sets.push_back(std::move(key_cur_set));
          // After (add, immediately before the push, using tok = the ')' ):
          local->instance_key_ranges.push_back(
              DisplayRange(key_pragma_tok.Position(), tok.NextPosition()));
          local->instance_key_param_index_sets.push_back(std::move(key_cur_set));
```
`key_pragma_tok` holds this set's own `@key` token (`:797`, overwritten only
on the NEXT state-21 entry, which is after this push); `tok` is the closing
`)`. Idiom mirrors the `mutable(func)` range at `Parser.cpp:593-594`
(TIP-EXACT).

**Accessor (mirror `InstanceKeys()`)** — `include/drlojekyll/Parse/Parse.h`
adds `const std::vector<DisplayRange> &InstanceKeyRanges(void) const
noexcept;`; `lib/Parse/Parse.cpp` (after `:859`) forwards
`return impl->instance_key_ranges;`.

**Re-anchor (Demand.cpp)** — which arm anchors to which range:
- **Unseeded reject** (`:479`, `d = demand_key_decls[0]`): a `@key` exists
  but no bound query. Anchor to the RELEVANT pragma's range —
  `d.InstanceKeyRanges()[k]` for the offending set `k` (single-forcing
  scope: `k = 0` today). Before `d.SpellingRange()` → after the set range.
- **RP-6 realization reject** (`:873`, `d` over `demand_key_decls`): the
  `@key` is on a non-demanded relation. Anchor to that decl's pragma range
  (`d.InstanceKeyRanges()[k]`).
- **Arm A** (`:930`, over-declaration — a DECLARED key set with no matching
  adornment): the surplus is a DECLARED set, so it HAS a pragma range —
  anchor to `p_demanded_decl.InstanceKeyRanges()[surplus_set_index]`.
- **Arm B** (`:942`, partial declaration — a demanded adornment with no
  matching `@key` set): the surplus is INFERRED (`p_bound` from SIP), NOT
  declared, so there is NO pragma to anchor. **Arm B stays
  decl-anchored** (`p_demanded_decl.SpellingRange()`, unchanged) — the
  caret correctly points at the whole decl because the fix ("add
  `@key(...)`") is a decl-level addition, not a correction to an existing
  set.

**Formatter unaffected — CONFIRMED** from extract §4: `Format.cpp:114-125`
reads only `HasInstanceKey()`/`InstanceKeys()`/`NthParameter().Name()`, zero
range reads. The new vector is a pure additive, format-inert rider, exactly
like `opt_mutable_range`/`spelling_range`.

**Golden-delta claim: ZERO existing goldens.** The rejects corpus is
textless AND goldenless — a reject exits rc=1 with unpinned diagnostic text
AND unpinned caret RANGE (the rejects lane byte-compares nothing; it checks
exit code only). No golden pins a DisplayRange. Re-anchoring changes only
the caret location in human-facing diagnostics, which nothing compares.
The storage/accessor additions are formatter-inert (confirmed) and touched
by no golden emitter.

## DIFF-K6-4 — CROSS-REDECLARATION CONSISTENCY

**File**: `lib/Parse/Parser.cpp`, `FinalizeDeclAndCheckConsistency`
(`:1465-1706`, TIP-EXACT). Insert a new check block shaped like the
`@first`-attribute check (`:1525-1547`), using the order-free set-of-sets
canonicalization already in Demand.cpp:902-905.

**Before**: no `instance_key_param_index_sets` comparison exists anywhere in
`:1465-1706` (confirmed by full read); the function returns `true` at
`:1705`.

**After** (new block, placed with the other attribute checks, both
`prev_decl` and `decl` in hand):
```cpp
  // IDENTICAL-OR-ABSENT (owner-ratified 2026-08-04): a redeclaration
  // carrying `@key` must declare the identical set-of-sets (order-free) as
  // the prior; a pragma-free redecl inherits silently (empty ⇒ no claim).
  {
    const auto &pk = prev_decl->instance_key_param_index_sets;
    const auto &ck = decl->instance_key_param_index_sets;
    // Only a divergence between two NON-EMPTY declarations is an error;
    // an empty side inherits (no obligation to restate).
    if (!pk.empty() && !ck.empty() && !SameKeySetOfSets(pk, ck)) {
      auto err = context->error_log.Append(scope_range, decl_key_range);
      err << "Instance key declared here differs from a previous "
          << "redeclaration of " << decl->KindName() << " '" << decl->name
          << "'; every `@key`-bearing redeclaration must declare the "
          << "identical column set(s), or omit `@key` to inherit";
      auto note = err.Note(prev_decl_range, prev_key_range);
      note << "Previous instance key is declared here";
      RemoveDecl(decl);
      return false;
    }
  }
```
`SameKeySetOfSets` is the order-free comparator (sort each inner set, sort
the outer collection of canonicalized sets, compare) — extract the existing
Demand.cpp:902-905/917-920 canonicalization into a small free helper reused
by both. `decl_key_range`/`prev_key_range` come from DIFF-K6-3's
`InstanceKeyRanges()` when present (falls back to
`decl->ParsedRange()`/`prev_decl_range` if DIFF-K6-3 has not landed — this
hunk does NOT hard-depend on DIFF-K6-3; it degrades to decl-anchored).

**Exact diagnostic string**:
`"Instance key declared here differs from a previous redeclaration of <kind> '<name>'; every `@key`-bearing redeclaration must declare the identical column set(s), or omit `@key` to inherit"`
plus note `"Previous instance key is declared here"`.

**Interaction with dedup/UniqueRedeclarations**: the check runs at
`FinalizeDeclAndCheckConsistency` time — per redeclaration as it is parsed,
against the FIRST decl (`prev_decl`), BEFORE any `UniqueRedeclarations()`
dedup (`Parse.cpp:952`) matters. This is the correct site: dedup collapses
binding-pattern-identical redecls, but `@key` consistency must be checked
across ALL textual occurrences regardless of dedup, and the pairwise
prev-vs-current check at parse time covers the full chain transitively (each
new redecl reconciles against the established one).

**Coverage** — divergence pinned by a rejects-corpus case; positive
(identical + absent) coverage:
- **Divergence (reject)**: `tests/OptDiff/rejects/reject_key_redecl_1.dr` —
  driverless, goldenless, must exit 1 cleanly. Shape:
  ```
  ; expect: instance-key redeclaration divergence (IDENTICAL-OR-ABSENT).
  #local rel(u64 A, u64 B) @key(A).
  #local rel(u64 A, u64 B) @key(B).
  rel(A, B) : edge_2(A, B).
  #message edge_2(u64 A, u64 B).
  #query rel(bound u64 A, free u64 B).
  ```
  (Two pragma-bearing redecls declaring `{A}` vs `{B}` — a hard reject.)
- **Positive (identical + absent)**: **DECISION — fold into
  `key_multi_adorn_witness`, gated on a byte-identity verification;
  otherwise a dedicated minimal witness.** The minimal fold adds a
  pragma-free redeclaration line to `key_multi_adorn_witness.dr`:
  ```
  #local rel(u64 A, u64 B) @key(A) @key(B).
  #local rel(u64 A, u64 B).           ; absent redecl — inherits silently
  ```
  exercising the ABSENT-inherit arm; an identical-redecl variant restates
  `@key(A) @key(B)` on the second line, exercising the IDENTICAL arm. The
  fold adds NO new golden IFF the added redeclaration leaves the relation's
  df/rel/contract/stdout byte-identical (a redecl shares the
  `DeclarationContext`, adds no relation/row/contract). **This byte-identity
  is NOT proven by the extracts and MUST be verified at implementation** — a
  redeclaration could perturb the formatter or df dump. If it perturbs any
  of `key_multi_adorn_witness`'s three real/symlinked goldens, split the
  positive coverage to a dedicated `key_redecl_witness` case instead (see
  OPEN QUESTIONS). This artifact recommends the fold as the minimal path and
  flags the verification as blocking.

**Golden-delta claim: ZERO existing goldens** (with the flagged caveat). The
reject case is goldenless (rejects lane). The positive fold is designed to
leave `key_multi_adorn_witness`'s existing goldens byte-identical —
CONTINGENT on the verification above; if it fails, the positive coverage
moves to a new case (which ADDS goldens but changes none). The
`FinalizeDeclAndCheckConsistency` insertion fires only on genuine
divergence, which no existing corpus case contains (no corpus program has
two pragma-bearing redecls of one relation), so no existing golden flips.

## DIFF-K6-5 — R-K1-BATCHES

**Files added** (inputs, not goldens):
`tests/OptDiff/cases/demand_multi_adorn_witness.batches`,
`tests/OptDiff/cases/demand_multi_adorn_witness.probes`, and the twin's
`.batches`/`.probes` as **symlinks** to the demand twin's (byte-identical
message stream + probe list; the `key_tc_witness` precedent has
`.batches`/`.probes` as byte-identical copies of `demand_tc_witness`'s —
extract §2 confirms "`cases/key_tc_witness.batches`/`.probes` are
byte-identical copies, `diff` rc=0". This artifact uses symlinks to match
the `bless_copy` symlink discipline; a plain copy is equally valid and is
what `key_tc_witness` did — panel's call, see OPEN QUESTIONS).

**`.batches` content** (single epoch, mirrors the driver's single
`edge_2_2` call with edges `{1,2},{1,3},{2,4},{10,11},{11,12}`):
```
# demand_multi_adorn_witness -- the oracle's input batch (the SAME edge
# stream the driver sends). One epoch. The oracle evaluates the plain
# (undemanded) program; `q` is the full (A,B) copy of `edge_2`, subsuming
# both bf/fb probe answers by inspection.
batch
+ edge_2 1 2
+ edge_2 1 3
+ edge_2 2 4
+ edge_2 10 11
+ edge_2 11 12
end
```

**`.probes` content** (one line per probed adornment token, driver order,
mirroring the seven `emit(...)` calls):
```
# probes mirrored from demand_multi_adorn_witness.main.cpp
q_bf 1
q_bf 2
q_bf 10
q_fb 4
q_fb 2
q_fb 11
q_fb 3
```
Format per `bin/RefInterp/Main.cpp:1388-1412` + RefHarness
`d.ident = d.name + "_" + d.bindings` (`:278`): token 0 =
`"<name>_<bindings>"`, remaining tokens the bound values. `q_bf`/`q_fb` are
distinct, natively resolved by both tools (multi-adornment is native — A5).

**Referee goldens GAINED** (GENERATED-then-reviewed — definitional outputs,
NEVER hand-predicted):
- `goldens/demand_multi_adorn_witness.oracle.stdout` — REAL, generated by
  `bin/Oracle` (full `q` extension + all named relations).
- `goldens/demand_multi_adorn_witness.monotone.stdout` — REAL, generated by
  `bin/Oracle --project-monotone` (monotone graph is identical here — no
  differential input).
- `goldens/demand_multi_adorn_witness.behavioral.stdout` — REAL, generated
  by the I0 pipeline (interp CBF == opt-mode behavioral binary).
- `goldens/key_multi_adorn_witness.oracle.stdout` → **SYMLINK** to the
  demand twin's (oracle is demand-blind, `suppress_demand=true`; the
  `.batches` are byte-identical; answers identical).
- `goldens/key_multi_adorn_witness.monotone.stdout` → **SYMLINK** to the
  demand twin's (same reasoning).
- `goldens/key_multi_adorn_witness.behavioral.stdout` → **REAL, own golden**
  (NOT a symlink) — the CBF header line embeds `case_name`
  (`key_multi_adorn_witness` ≠ `demand_multi_adorn_witness`), so bytes
  diverge on that one line even though the CBF body is byte-identical.

**Exact bless/creation sequence (honoring `bless_copy`, runall.sh:110-133)**:
1. Author the demand twin's `.batches` + `.probes` (real files). Create the
   key twin's `.batches`/`.probes` as symlinks (or copies) to them.
2. Run the suite once to PRODUCE the referee outputs into `$WORKROOT`
   (oracle/monotone/refinterp will report GOLDEN-MISSING /
   BEHAVIORAL-MISSING on first run — expected).
3. **Review the produced outputs** (definitional — read them, confirm the
   full `q` extension matches the driver's asserted bf/fb sets, confirm the
   CBF transcript). Do NOT hand-predict.
4. Bless the demand twin's three REAL goldens:
   `runall.sh --bless <workroot> demand_multi_adorn_witness`. `bless_copy`
   writes real files (`oracle`/`monotone`/`behavioral`).
5. Create the key twin's symlinks explicitly (`ln -s` in `goldens/`) for
   `oracle`/`monotone` — do NOT bless them (bless would try to write through
   the symlink; `bless_copy` REFUSES that, `runall.sh:115-118`, and it
   already anticipates the `key_multi_adorn_witness` twin family).
6. Bless the key twin's REAL `behavioral.stdout`:
   `runall.sh --bless <workroot> key_multi_adorn_witness` — `bless_copy`
   writes the real behavioral golden; the `oracle`/`monotone` symlinks are
   `skipped … (symlink, byte-identical)` and `stdout` (already a symlink) is
   skipped.
7. Re-run the full suite; it must end `SUITE: PASS` with the new
   oracle/monotone/refinterp `OK` lines for both witnesses.

**Golden-delta claim: ADDS goldens only.** Six new golden entries (three
real for the demand twin, one real + two symlinks for the key twin) plus
input sidecars. No EXISTING golden is touched — the witnesses' current
`stdout`/`region`/`contract`/`rel` goldens are inputs to unrelated referees
and are unchanged.

## DIFF-K6-6 — `KEY(_MissingVar)` WART

**File**: `lib/DataFlow/Format.cpp:129` (the `KEY(...)` render) and `:162`
(the `do_col` plain-column render), TIP-EXACT.

**Before** (`:129`, inside the `KEY(…)` member-key loop):
```cpp
        if (col.Id() == field.v) {
          if (col.IsConstantOrConstantRef()) {
            os << "k" << field.v;
          } else {
            os << col.Variable();          // streams _MissingVar on nullopt
          }
          named = true;
          break;
        }
```

**After** (guard the optional, matching the `name_tok` idiom at `:887-901`):
```cpp
        if (col.Id() == field.v) {
          if (col.IsConstantOrConstantRef()) {
            os << "k" << field.v;
          } else if (auto var = col.Variable()) {
            os << *var;
          } else {
            os << "f" << field.v;          // fabricated demand column
          }
          named = true;
          break;
        }
```
Using `f<field.v>` matches the existing `!named` fallback arm two lines
down (`os << "f" << field.v;`), keeping the render vocabulary consistent for
fabricated columns. (`c<id>` is the `name_tok` fallback and equally valid —
panel's call; `f<field.v>` is chosen here for local consistency with the
sibling arm.)

**Before** (`:162`, `do_col`):
```cpp
    os << col.Variable();          // << ":" << *(col.Index());
```
**After**:
```cpp
    if (auto var = col.Variable()) {
      os << *var;
    } else {
      os << "c" << col.Id();
    }
```

**Golden-delta claim: ZERO existing goldens.** Verified this session
(tip 3bf87d56): `grep -rln "_MissingVar" tests/OptDiff/goldens/` → nothing.
**CHECK RESULT: no committed golden contains `_MissingVar`.** The DOT
surface is advisory and never goldened (CLAUDE.md). The fix is a pure
render-label improvement with no compared surface.

## DIFF-K6-7 — DOT RIDERS

### (a) region-DOT declared badge (render-only)

**Files**: `include/drlojekyll/Regional/Regional.h` (struct field),
`lib/Regional/Planning.cpp` (populate), `lib/Regional/Format.cpp`
(render).

**Struct** (`Regional.h:108-113`, `RegionalContract`): add `bool
declared_key = false;` (and optionally the same on `RegionalPort`,
`:82-92`, for the request-port badge).

**Populate** (`Planning.cpp`): both contract loops bind `decl` —
```cpp
  contract.support_text = ...;
  contract.declared_key = decl.HasInstanceKey();   // NEW, insert-derived (:477-519)
```
```cpp
  contract.support_text = ...;
  contract.declared_key = decl.HasInstanceKey();   // NEW, Tier-1 interior (:528-536)
```
Request-port badge (`:352-364`) needs the `RecognizedSubgraphs()` join on
`forcing_index == fi` to reach `demanded_decl.HasInstanceKey()` (optional —
the contract badge alone is the minimal deliverable; the port badge is a
follow-on if the panel wants it).

**Render** (`Format.cpp:180-183`, the `contract_e*` label):
```cpp
  // Before:
  os << "contract_e" << contract.edge_index << " [label=\"E"
     << contract.edge_index << " rel=" << contract.rel_name
     << " member-key=" << contract.member_key_text
     << " support=" << contract.support_text << "\"];\n";
  // After (append the badge only when set):
  os << "contract_e" << contract.edge_index << " [label=\"E"
     << contract.edge_index << " rel=" << contract.rel_name
     << " member-key=" << contract.member_key_text
     << " support=" << contract.support_text
     << (contract.declared_key ? " declared-key" : "")
     << "\"];\n";
```

**Golden-delta claim: ZERO existing goldens.** The `-region-dot-out` twin is
advisory and never goldened (CLAUDE.md; `Regional/Format.cpp` DOT comment).
The badge renders ONLY in the DOT label. **CHECK/CAVEAT**: the new struct
field is populated in `Planning.cpp` but must be READ only by the DOT
emitter — the `-region-out` TEXT emitter (the 16 `.region.<mode>` goldens)
must NOT render `declared_key`. Confirm at implementation that the text
emitter's contract line is untouched (it is a separate function from the
DOT `operator<<`); the field default `= false` and the DOT-only render keep
the text goldens byte-identical. Flagged in the ledger.

### (b) NEW `-rel-dot-out` DR-IR DOT twin

**Files**: `lib/Rel/Format.cpp` (new emitter + sink pair),
`include/drlojekyll/ControlFlow/Format.h` (declare the sink),
`lib/Rel/Rel.h` (internal re-decl if the existing `Set*`/`Dump*` pattern
requires it), `lib/ControlFlow/Build/Stratum.cpp` (drain call site),
`bin/drlojekyll/Main.cpp` (flag wiring).

**Sink pair** (`Format.cpp`, beside `:1155-1168`, imitating
`SetRelDumpStream`/`DumpRelIfEnabled`):
```cpp
static OutputStream *gRelDotDumpStream = nullptr;
}  // namespace
void SetRelDotDumpStream(OutputStream *stream) {
  gRelDotDumpStream = stream;
}
void DumpRelDotIfEnabled(const DRFlowGraph &flow) {
  if (gRelDotDumpStream) {          // PRE-guard, same as DumpRelIfEnabled
    EmitDRFlowDOT(*gRelDotDumpStream, flow);
    gRelDotDumpStream->Flush();
  }
}
```
Declare `void SetRelDotDumpStream(OutputStream *stream);` on
`include/drlojekyll/ControlFlow/Format.h:17` beside `SetRelDumpStream`.
Drain `DumpRelDotIfEnabled(flow)` at the SAME `Stratum.cpp` site
`DumpRelIfEnabled(flow)` fires (inside `Program::Build`).

**Emitter** (`EmitDRFlowDOT`, MINIMAL — faithful graph, census-free):
```cpp
static void EmitDRFlowDOT(OutputStream &os, const DRFlowGraph &flow) {
  os << "digraph {\n"
     << "node [shape=box font=courier];\n";

  // Cluster per DR stratum (idiom from DataFlow/Format.cpp:56-75).
  std::map<unsigned, std::vector<unsigned>> stratum_ops;
  for (unsigned pi = 0u; pi < flow.pinned_order.size(); ++pi) {
    const unsigned oi = flow.pinned_order[pi];
    stratum_ops[DROpStratum(flow, flow.ops[oi])].push_back(oi);
  }
  for (const auto &[stratum, ops] : stratum_ops) {
    os << "subgraph cluster_stratum_" << stratum << " {\n"
       << "label=\"stratum " << stratum << "\";\n"
       << "style=\"rounded,dashed\";\n";
    for (unsigned oi : ops) {
      os << "op" << oi << " [label=\"op." << oi << " "
         << DROpKindName(flow.ops[oi].kind) << "\"];\n";
    }
    os << "}\n";
  }

  // Vecs (id-ordered by mint, DataFlow twin's id-order discipline).
  for (unsigned vi = 0u; vi < flow.vecs.size(); ++vi) {
    os << "vec" << vi << " [shape=ellipse label=\"vec." << vi << "\"];\n";
  }

  // def/use edges: op -> vec (def), vec -> op (use).
  for (unsigned vi = 0u; vi < flow.vecs.size(); ++vi) {
    for (unsigned oi : flow.vecs[vi].def) os << "op" << oi << " -> vec" << vi << ";\n";
    for (unsigned oi : flow.vecs[vi].use) os << "vec" << vi << " -> op" << oi << ";\n";
  }
  os << "}\n";
}
```
(Field names `flow.vecs[vi].def/.use`, `DROpKindName`, `DROpStratum` are the
text-emitter's own accessors per extract DOT §3 — re-confirm exact spellings
against `Rel.h`/`Format.cpp` at implementation; the shape is the deliverable,
census-free, no analysis, no dep-sorting.)

**Main.cpp wiring** (clone the `-rel-out` arm, `:419-434`, and the globals
`:57-64` + install `:94-98` + help `:239` + local `:319`):
```cpp
// global (beside gRelStream):
static OutputStream *gRelDotStream = nullptr;
// install-before-build (beside SetRelDumpStream(gRelStream)):
SetRelDotDumpStream(gRelDotStream);
// arg arm:
} else if (!strcmp(argv[i], "--rel-dot-out") ||
           !strcmp(argv[i], "-rel-dot-out")) {
  ++i;
  if (i >= argc) {
    error_log.Append() << "Command-line argument '" << argv[i - 1]
                       << "' must be followed by a file path for "
                       << "Rel IR DOT output";
  } else {
    rel_dot_out.reset(new hyde::FileStream(display_manager, argv[i]));
    if (!rel_dot_out->fs.is_open()) {
      error_log.Append() << "Unable to open '" << argv[i]
                         << "' for Rel IR DOT output";
    }
    hyde::gRelDotStream = &(rel_dot_out->os);
  }
// help line (beside :239):
//   -rel-dot-out <PATH>  Emit the Rel (DR-IR) flow graph as GraphViz DOT to PATH.
// local (beside rel_out, :319):
std::unique_ptr<hyde::FileStream> rel_dot_out;
```
The install-before-build + `Set*`/`Dump*IfEnabled` pattern is MANDATORY (the
DR graph exists only inside `Program::Build`, like `-rel-out`, unlike the
top-level `-region-dot-out` drain).

**Golden-delta claim: ZERO existing goldens.** Brand-new advisory sink,
never goldened (CLAUDE.md IR-observability directive: new DOT surfaces get a
digraph twin, advisory, never goldened). Additive Main.cpp/help/sink wiring
touches no compared surface.

---

# VERIFICATION LEDGER

| Hunk | Claim | Evidence (this session, tip 3bf87d56) | Residual verify-at-impl |
|---|---|---|---|
| K6-1 | 0 existing goldens | reject-only paths; rejects lane pins no text; flag-arm bytes preserved | Re-anchor the three `file:line` (Demand.cpp :1042/:1053/:1064) if tree moved |
| K6-2 | 0 existing goldens | `grep "demand key"` → 0 goldens, 0 case headers | Confirm Demand.cpp:874 wording family (extract ambiguous: "@key pragma" vs "demand key") |
| K6-3 | 0 existing goldens | rejects lane compares exit code only, no caret range pinned; formatter range-inert (Format.cpp:114-125 confirmed) | Confirm `key_pragma_tok` holds set-N's token at set-N's `)` (extract says yes, :797/:964) |
| K6-4 | 0 existing goldens (contingent) | reject case goldenless; no corpus program has 2 pragma-bearing redecls | **BLOCKING**: verify the absent-inherit fold leaves `key_multi_adorn_witness` df/rel/contract/stdout byte-identical; else split to a dedicated positive witness |
| K6-5 | ADDS goldens only | neither witness has batches/oracle/monotone/behavioral today (ls confirmed) | Definitional outputs GENERATED-then-reviewed; never hand-predict; honor `bless_copy` symlink refusal |
| K6-6 | 0 existing goldens | `grep "_MissingVar"` → 0 goldens; DOT never goldened | none |
| K6-7 | 0 existing goldens | region-DOT + rel-dot advisory, never goldened | **CHECK**: confirm `-region-out` TEXT emitter does NOT render `declared_key` (16 `.region.<mode>` goldens must stay byte-identical); confirm `flow.vecs[].def/.use`/`DROpKindName`/`DROpStratum` spellings |

Global suite gate (any hunk that touches compiler code): the full
`runall.sh` sweep must end `SUITE: PASS` and the 6-member eqgate family must
stay green (re-derive `ls tests/OptDiff/cases/*.eqgate | wc -l` = 6, verified
this session — do not propagate the constant).

---

# OPEN QUESTIONS FOR PANEL

1. **K6-4 positive-coverage placement (blocking decision).** Fold the
   absent-inherit + identical-redecl shapes into `key_multi_adorn_witness.dr`
   (a pragma-free / identical second redeclaration), CONTINGENT on verifying
   byte-identity of its three existing goldens — or spend a dedicated
   `key_redecl_witness` case? This artifact recommends the fold with a
   blocking byte-identity check; the panel should ratify the fallback
   (dedicated case) trigger. Unknown from extracts: whether a redeclaration
   perturbs the df/formatter/contract dump of a shared-context relation.

2. **K6-2 scope of "demand key".** Normalize ONLY `Parser.cpp:957` (the
   unambiguous outlier), or also `Demand.cpp:480-481` (and `:874`?) where the
   phrasing "declares a demand key" names the ACT of declaring rather than
   the mechanism? Recommendation: normalize all for uniformity (zero risk,
   verified golden-safe), but `:480-481`/`:874` is a judgment call the panel
   owns. Requires re-reading `:874` to settle its wording family.

3. **K6-3 storage shape.** A parallel `std::vector<DisplayRange>
   instance_key_ranges` (this artifact's choice, mirrors the flat
   `instance_key_param_index_sets`) vs a fused
   `std::vector<std::pair<std::vector<unsigned>, DisplayRange>>`? The parallel
   vector keeps the existing accessor/formatter untouched (they read the
   index-set vector unchanged); the fused form is one allocation but churns
   `InstanceKeys()`. Recommendation: parallel vector. Also: should
   `InstanceKeyRanges()` be public API (needed by Demand.cpp across the
   lib boundary) or is there a narrower seam?

4. **K6-3 Arm-B anchoring.** Confirmed decision: Arm B (inferred surplus)
   stays decl-anchored because there is no pragma to point at. Panel confirm
   this is the desired UX (caret at whole decl, message "declare `@key(...)`")
   vs. some other anchor (e.g. the query adornment's range).

5. **K6-5 twin `.batches`/`.probes`: symlink vs copy?** The `key_tc_witness`
   precedent uses byte-identical COPIES (extract §2); this artifact proposed
   SYMLINKS to match the `bless_copy` symlink discipline. Copies are simpler
   and match precedent; symlinks make the twin-equivalence explicit but add a
   symlink the bless guard must tolerate. Recommendation: match precedent
   (copies) unless the panel prefers the explicit twin marker.

6. **K6-6 fabricated-column render token.** `f<field.v>` (matches the sibling
   `!named` arm) vs `c<id>` (matches the `name_tok` policy idiom)? Both are
   ungoldened. Minor; recommendation `f<field.v>` for local consistency.

7. **K6-7 DR-IR DOT node granularity for big programs.** The minimal emitter
   nodes every DROp and every DRVec with def/use edges — for a large program
   this is a dense graph. Keep it fully faithful (this artifact's choice,
   advisory tool, user opts in per-file), or add an elision knob (e.g.
   collapse vecs, or per-SCC subgraphs only)? Recommendation: keep minimal
   and faithful for v1; elision is a follow-on if it proves unreadable. Also:
   render the branches/joins/rounds substrate as nodes too, or ops+vecs only
   (this artifact's minimal choice)?

8. **K6-7 request-port badge.** Ship the contract-badge only (minimal), or
   also the request-port badge (needs the `RecognizedSubgraphs()` join)? The
   contract badge is the higher-signal surface; the port badge is optional.

---

# PANEL RECORD (K6-riders post-authoring critique panel — 2026-08-04, session 7 continuation)

**Convention (house style, per k1-multikey.md).** This section and the two that
follow are APPENDED, superseding the affected Part-B hunks — the original hunks
are NOT rewritten in place; where an AMENDMENT below supersedes a hunk it says so
by name, and the superseded hunk stands as authored for provenance.

**Re-anchor note.** All Part-A/Part-B line citations were authored against tip
`3bf87d56` and are still tip-exact EXCEPT where K1 (`e641be46`, folded into
`3bf87d56`) shifted the demand__-collision sites: the three DIFF-K6-1 rejects are
now at `Demand.cpp:1045-1047 / :1055-1057 / :1066-1068` (was `:1042/:1053/:1064`),
and the Step-2b arms are at `Demand.cpp:928-937` (Arm A) / `:940-949` (Arm B) — the
panel re-verified every load-bearing cite fresh this session.

**13 findings** (3 lenses — necessity / sufficiency / hazard — + 2 orchestrator
probes). Verdicts: **3 CONFIRMED must-fix**, **8 notes** (7 refuter-downgraded from
raw findings + 1 orchestrator-verdicted orphan), **2 refuted**.

## CONFIRMED must-fix — the DIFF-K6-4 cluster

The DIFF-K6-4 hunk as authored (Part B, `:473-569`) is REWORKED by ADJ-K6-A: what
it framed as a single "add the missing consistency check" is in truth THREE
interlocking live bugs, all panel-found and empirically proven this session against
`3bf87d56` (probe dumps in the scratchpad `k6probe/` dir).

- **F31 — a NEW numbered pre-existing live bug (panel-found; lldb-anchored +
  empirically proven).** `Parser.cpp:1511`
  `ParsedDeclarationImpl *const prev_decl = redecls[num_redecls - 1u];` ALIASES the
  CURRENT decl. Both `ParsedDeclarationImpl` ctors append `this` to
  `context->redeclarations` at construction (`Parse.cpp:166` and `:178`,
  `context->redeclarations.AddUse(this)` — a `WeakUseList`, `Parse.h:56`), and
  `FinalizeDeclAndCheckConsistency(decl)` runs AFTER the current decl is
  constructed, over `redecls = decl->context->redeclarations` (`Parser.cpp:1472`).
  So `num_redecls` counts the current decl and `redecls[num_redecls-1u]` IS `decl`
  itself: `prev_decl == decl`. EVERY redecl consistency check in the function
  (parameter type differs `:1677`, externally-visible parameter name differs
  `:1651`, `@first` `:1526`, inline `:1692`, mutable-merge `:1664`, and siblings) is
  therefore comparing a decl against itself and is DEAD at tip. Empirical proof
  (message made used so the "never published/received" guard does not mask):
  `#message foo(u64 A). #message foo(i32 A). #local r(u64 A). r(A):foo(A).`
  compiles rc=0 despite the `u64`/`i32` type divergence; the name-divergent twin
  (`foo(u64 A)` / `foo(u64 B)`) likewise compiles rc=0. **Record F31 for
  `tests/OptDiff/FINDINGS.md` at implementation** (repro = the two probes above,
  `k6probe/f31_type2.dr` / `f31_name2.dr`).

- **The artifact's proposed `reject_key_redecl_1` was a LOST CHECK (the ADJ-K1-F
  anti-pattern).** DIFF-K6-4's divergence witness (Part B `:531-540`) declared
  `#query rel(...)` over `#local rel(...)` — a CROSS-KIND collision rejected by
  `AddDecl` (`Parser.h`, "Cannot re-declare 'rel' as a local") long before any
  `@key` consistency check runs. Empirically: the shape rejects rc=1 at tip WITH
  the K6-4 check absent, so it is green→green under the change and would pin
  nothing. Reshaped by ADJ-K6-B.

- **F-K6-SHADOW — orchestrator probe (empirically proven at tip).** A pragma-free
  redeclaration FIRST + the `@key`-bearing decl SECOND compiles rc=0 with the
  pragma SILENTLY DROPPED. Probe `k6probe/shadow_first.dr`
  (`#local rel(u64 A,u64 B).` then `#local rel(u64 A,u64 B) @key(A).` then
  `q(A,B):rel(A,B).`, `#query q(bound A,free B)`) emits
  `kSubgraphInstantiate=0` (plain flat join web — pragma invisible), where the
  single-decl baseline emits `kSubgraphInstantiate=1`. Mechanism: per-impl storage
  (`instance_key_param_index_sets` on the impl, `Parse.h:387`) + the accessors
  `HasInstanceKey()`/`InstanceKeys()` reading `impl->` directly
  (`Parse.cpp:852-859`) + the activation gate's canonical-first resolution make a
  non-first pragma invisible. This is the **NEC-2 silent lie reachable by
  declaration ORDER** — a user's `@key` is silently ignored. The redecl-AFTER twin
  (`k6probe/shadow_after.dr`, `@key` first) is byte-inert
  (`kSubgraphInstantiate=1`) — the pragma survives only when it lands on the
  canonical-first impl.

## Notes (refuter-downgraded + the orphan)

- **K6-1 after-form strings (necessity lens).** The DIFF-K6-1 after-form (Part B
  `:330-347`) splits at `"; rename it"` and appends `"; fix or remove the @key
  pragma"`, yielding `"…prefix; rename it; fix or remove the @key pragma"` — a
  double-semicolon that reads awkwardly and does NOT match the Finding-1
  adjudication. Resolution (ADJ-K6-C): the pragma arm keeps `"rename it"` and the
  TAIL forks grammatically — `" or remove the @key pragma"` (pragma) /
  `" or recompile without -demand"` (flag), so the flag arm reproduces today's
  bytes exactly and the pragma arm reads `"…prefix; rename it or remove the @key
  pragma"`. Superseded strings in AMENDMENT A-K6-1.

- **K6-2 Part A mislabel (sufficiency lens).** Part A `:80` / the VERIFICATION
  LEDGER `:888` flag `Demand.cpp:874` as "@key pragma"-family and mark it
  ambiguous. Re-read: `:874` says `"' declares a demand key but is not "` — it IS
  drift, same as `:480` (`"' declares a demand key but no "`) and `Parser.cpp:957`
  (`"demand key of "`). Resolution: normalize ALL THREE sites (not one, not "maybe
  two"). AMENDMENT A-K6-2.

- **K6-3 Arm A `surplus_set_index` does not exist (necessity lens).** DIFF-K6-3's
  Arm-A re-anchor (Part B `:449-451`) writes
  `p_demanded_decl.InstanceKeyRanges()[surplus_set_index]`, but the landed Arm A
  (`Demand.cpp:928-937`) iterates a CANONICALIZED `std::set<std::vector<unsigned>>
  declared_sets` — its loop variable `d` is a sorted set VALUE, carrying no index
  into `InstanceKeys()`/`InstanceKeyRanges()`. There is no `surplus_set_index` to
  subscript. Resolution: restructure Arm A to an index-preserving iteration.
  AMENDMENT A-K6-3.

- **K6-5 A5 premise is FALSE for `.drflags` cases (sufficiency lens).** A5
  (Part A `:201-225`) and the run protocol assert the behavioral binary is "the
  PLAIN program … never `.drflags`". The runall.sh COMMENT says so
  (`:367-368`, `:399`, echoing CLAUDE.md), but the CODE contradicts it: the
  behavioral compile at `runall.sh:407` uses `$(flags_of "$bmode")`, and
  `flags_of` UNCONDITIONALLY appends the `.drflags` sidecar (`:214-216`). So the
  behavioral binary of a `.drflags` case IS demand-lowered. This is not a defect to
  fix in K6-5 — it is a precedent to honor: `demand_tc_witness` (`.batches` +
  `.drflags=-demand` + real `.behavioral.stdout`, all present at tip) already runs
  this exact path and passes. Resolution: correct A5 and design DIFF-K6-5 against
  the working `demand_tc_witness` shape (the witnesses' `.drflags=-demand`
  behavioral binaries are fine). AMENDMENT A-K6-5.

- **No oracle-vs-behavioral cross-family byte check exists (recorded note).** The
  only oracle cross-check in runall.sh is `oracle.stdout` vs the driver's `stdout`
  (`:283`); behavioral is checked against its own `.behavioral.stdout` + the interp
  CBF (`:441-446`). No lane byte-compares the oracle family against the behavioral
  family. Recorded, not actioned (the two referees answer different questions —
  definitional extension vs published-ABI transcript).

- **K6-7b emitter reads `.def`/`.use` but the DRVec fields are `.defs`/`.uses`
  (necessity lens).** The DIFF-K6-7b `EmitDRFlowDOT` pseudocode (Part B
  `:833-834`, `:839-842`) reads `flow.vecs[vi].def` / `.use`; the real DRVec
  members are `std::vector<unsigned> defs;` / `uses;` (`Rel.h:403-404`). Fix the
  spellings. The independent id-order walk (not reusing the text emitter's
  `pinned_order` traversal wholesale) is acceptable for an ADVISORY emitter;
  the alternative — factor a shared walk out of `EmitDRFlow` — is noted but not
  required for a never-goldened DOT twin. AMENDMENT A-K6-7b.

- **The `SameKeySetOfSets == TRUE` branch lacks a positive witness
  (orchestrator-verdicted orphan).** DIFF-K6-4's comparator has a FALSE-return path
  (divergence → reject, pinned by `reject_key_redecl_1`) and a TRUE-return path
  (both non-empty AND equal → NO reject). The TRUE path had no witness. Resolved by
  the ADJ-K6-B TRUE-branch decision (accepted gap + cheap exercise) below.

## Refuted (2)

- **"DIFF-K6-3 storage is a hard prerequisite for DIFF-K6-4."** REFUTED — DIFF-K6-4
  degrades cleanly to decl-anchored ranges when `InstanceKeyRanges()` is absent
  (the hunk's own `:513-514` fallback). The two are order-independent; only the
  caret tightness couples.

- **"Reviving the F31 checks (ADJ-K6-A(a)) will reject legitimate multi-adornment
  `#query` redeclarations."** REFUTED — the parameter-binding-attribute check
  (`Parser.cpp:1627-1641`) explicitly EXCLUDES `kFunctor`/`kQuery`
  (`prev_decl_kind != kFunctor && != kQuery`); the name/type checks compare
  parameter names + type kinds, which are identical across a query's adornments.
  Corpus grep (below) confirms zero green-case fallout.

---

# ADJUDICATED RESOLUTIONS (orchestrator, binding — 2026-08-04)

## ADJ-K6-A — the DIFF-K6-4 REWORK (supersedes Part B `:473-569`, ONE hunk, THREE parts)

DIFF-K6-4 is reissued as a single hunk with three tied parts. Part (a) revives a
dead machine; parts (b)/(c) build the new check on top of it. Landing (a) without
(b) leaves F-K6-SHADOW; landing (b) without (a) is impossible (the check needs a
real `prev_decl`).

### (a) FIX F31 — `prev_decl` must index the TRUE previous redecl.

**File**: `lib/Parse/Parser.cpp:1506-1513`.

```diff
   auto num_redecls = redecls.Size();
   if (1u >= num_redecls) {
     return true;
   }

-  ParsedDeclarationImpl *const prev_decl = redecls[num_redecls - 1u];
+  // F31 (2026-08-04): `redecls` already contains the CURRENT decl (both ctors
+  // AddUse(this) at construction, Parse.cpp:166/:178), so [-1] aliases `decl`
+  // itself and every consistency check below compared a decl to itself. The
+  // true previous redecl is [-2]; num_redecls >= 2 is guaranteed by the guard
+  // above.
+  assert(num_redecls >= 2u);
+  ParsedDeclarationImpl *const prev_decl = redecls[num_redecls - 2u];
   const ParsedDeclaration prev_decl_pub(prev_decl);
```

This REVIVES the type (`:1677`) / externally-visible-name (`:1651`) / `@first` /
inline / mutable-merge checks. **The suite is the referee for corpus fallout: any
newly-rejecting case is a FINDING to adjudicate (never fudge to green).**
Prediction: ZERO green-case fallout (see the corpus grep in the amendment
summary — the only same-name redecls are 6 multi-adornment `#query` that survive
the kQuery-excluded binding check with identical names/types, and 2 `#functor a`
redecls that already live in `rejects/`, where rc=1 is the expected outcome
regardless of WHICH check fires).

### (b) FIX F-K6-SHADOW — the accessors resolve through the redeclaration context.

**File**: `lib/Parse/Parse.cpp:851-859` (+ the K6-3 `InstanceKeyRanges()` twin).

Keep per-impl CAPTURE at parse (the parser writes each impl's own sets, unchanged);
make the READ resolve across the context's redeclaration list — first impl carrying
a non-empty set wins. Once ADJ-K6-A(c) enforces IDENTICAL-OR-ABSENT, "first
non-empty" is well-defined (all non-empty siblings are identical) and "which impl"
is moot for the absent-inherit sibling.

```diff
 bool ParsedDeclaration::HasInstanceKey(void) const noexcept {
-  return !impl->instance_key_param_index_sets.empty();
+  for (ParsedDeclarationImpl *redecl : impl->context->redeclarations) {
+    if (!redecl->instance_key_param_index_sets.empty()) {
+      return true;
+    }
+  }
+  return false;
 }

 const std::vector<std::vector<unsigned>> &
 ParsedDeclaration::InstanceKeys(void) const noexcept {
-  return impl->instance_key_param_index_sets;
+  for (ParsedDeclarationImpl *redecl : impl->context->redeclarations) {
+    if (!redecl->instance_key_param_index_sets.empty()) {
+      return redecl->instance_key_param_index_sets;
+    }
+  }
+  return impl->instance_key_param_index_sets;  // empty fallback (no @key sibling)
 }
```

`impl->context->redeclarations` is the `WeakUseList<ParsedDeclarationImpl>` both
ctors append to (`Parse.h:56`); it is iterable. **The activation gate needs NO
change** — it calls `HasInstanceKey()`/`InstanceKeys()`, which now see the shared
key regardless of declaration order. DIFF-K6-3's `InstanceKeyRanges()` accessor
gets the identical resolve-through-redeclarations body (return the first non-empty
sibling's parallel range vector).

### (c) IDENTICAL-OR-ABSENT at the revived check site.

**File**: `lib/Parse/Parser.cpp`, inside `FinalizeDeclAndCheckConsistency`, with
the other attribute checks (after `:1618`, both `prev_decl` — now the true previous
— and `decl` in hand). The canon idiom is LOCAL to `Parser.cpp` (the necessity
panel's point: no cross-library hoist of the Demand.cpp comparator; a small
file-static free helper here).

```cpp
  // IDENTICAL-OR-ABSENT (owner-ratified 2026-08-04): a redeclaration carrying
  // `@key` must declare the identical set-of-sets (order-free, per-set AND
  // across-sets) as the prior; a pragma-free redecl inherits silently (empty ⇒
  // no claim). Only a divergence between two NON-EMPTY declarations is an error.
  {
    const auto &pk = prev_decl->instance_key_param_index_sets;
    const auto &ck = decl->instance_key_param_index_sets;
    if (!pk.empty() && !ck.empty() && !SameKeySetOfSets(pk, ck)) {
      auto err = context->error_log.Append(scope_range, decl_key_range);
      err << "Instance key declared here differs from a previous redeclaration "
          << "of " << decl->context->kind_name << " '" << decl->name
          << "'; every `@key`-bearing redeclaration must declare the identical "
          << "column set(s), or omit `@key` to inherit";
      auto note = err.Note(prev_decl_range, prev_key_range);
      note << "Previous instance key is declared here";
      RemoveDecl(decl);
      return false;
    }
  }
```

`SameKeySetOfSets(pk, ck)` = the order-free set-of-sets comparator, a
`Parser.cpp`-local free helper (sort each inner set, collect into
`std::set<std::vector<unsigned>>`, compare) — the SAME canonicalization the landed
Step 2b uses at `Demand.cpp:902-905`, transcribed locally rather than hoisted.
`decl_key_range`/`prev_key_range` come from DIFF-K6-3's `InstanceKeyRanges()` when
present, else fall back to `decl_pub.SpellingRange()`/`prev_decl_range` — the hunk
does NOT hard-depend on DIFF-K6-3. (Use whatever the tip spelling of the kind-name
accessor is at implementation; `decl->KindName()` per the neighboring checks.)

## ADJ-K6-B — witnesses (binding)

- **`reject_key_redecl_1` RESHAPED** (supersedes Part B `:531-540`): a distinct
  query name, well-formed but for divergent keys, so ONLY the K6-4 check can reject
  it. Full text in AMENDMENT A-K6-B. Verified this session to compile **rc=0 at
  tip** (`k6probe/reject_reshaped.dr`, `kSubgraphInstantiate=1` — the FIRST redecl's
  `@key(A)` is silently taken, `@key(B)` shadowed) — so dropping the K6-4 check is a
  genuine LOST CHECK, and the reshaped case is a real reject-lane pin.

- **`key_multi_adorn_witness` gains a pragma-free redeclaration BEFORE the pragma'd
  decl** — the previously-BROKEN shadow shape. Post-fix (ADJ-K6-A(b)) it must
  compile BYTE-IDENTICAL to today's `key_multi_adorn_witness` goldens
  (`stdout` symlink + `contract.opt`/`rel.opt` real): order-independence +
  absent-inherit in one line. This is the ADJ-K1-F guard property: PRE-fix that
  shape silently drops the pragma (`kSubgraphInstantiate` flips to 0), which flips
  every golden of the witness — so a regression is loud. **The byte-identity is the
  blocking gate** (unchanged from the authored DIFF-K6-4 caveat): if the added
  redecl perturbs any of the witness's goldens, split the positive coverage to a
  dedicated `key_redecl_witness` case.

- **TRUE-branch decision (`SameKeySetOfSets == TRUE`) — ACCEPTED GAP + cheap
  exercise.** DECISION, from the code: do NOT fold a second identical-non-empty
  `@key(A) @key(B)` redeclaration into `key_multi_adorn_witness`. Rationale:
  (1) the mandated witness change is the pragma-FREE redecl, which hits the
  `!pk.empty() && !ck.empty()` short-circuit and structurally NEVER reaches
  `SameKeySetOfSets` — so it cannot cover the TRUE branch, and no reshaping of the
  absent-inherit witness will; (2) covering TRUE needs a SECOND pragma-bearing
  identical redecl, which incurs the very byte-identity risk the witness's goldens
  are the blocking gate for, AND dilutes the ADJ-K1-F shadow-guard role (a third
  redecl shifts which impl is canonical-first — the exact axis the shadow bug lives
  on); (3) the check is guarded by `num_redecls >= 2`, so single-`@key` cases never
  reach it, and the TRUE branch is the trivial no-op fall-through (predicate true ⇒
  condition false ⇒ control proceeds to `return true`) — nil risk surface versus
  the FALSE branch, which IS pinned by `reject_key_redecl_1`. **Cheap exercise
  (recommended, free):** shape `reject_key_redecl_1` with an identical MIDDLE redecl
  — `@key(A)`, `@key(A)`, `@key(B)` — so the pairwise check hits `SameKeySetOfSets
  == TRUE` on redecl 2 (no reject) en route to the divergence reject on redecl 3.
  The reject lane pins only rc=1, so this EXERCISES (does not positively PIN) the
  TRUE branch at zero golden cost. The positive PIN is recorded as an accepted gap.

## ADJ-K6-C — all other hunks amended per their notes

DIFF-K6-1 (strings), DIFF-K6-2 (three sites), DIFF-K6-3 (index-preserving Arm A),
DIFF-K6-5 (A5 correction), DIFF-K6-7b (`.defs`/`.uses` spellings) are amended as in
the AMENDMENTS section. DIFF-K6-6 and DIFF-K6-7a are unchanged (no finding
touched them). The K6-5 generated-then-reviewed referee protocol stands — no
hand-predicted referee bytes.

---

# AMENDMENTS (Part C-style desired-state deltas — only where the resolutions change them)

## A-K6-1 — DIFF-K6-1 corrected strings (supersedes Part B `:322-357`)

**File**: `lib/DataFlow/Demand.cpp`, three sites `:1045-1047`, `:1055-1057`,
`:1066-1068` (re-anchored from `:1042/:1053/:1064`). `pragma_activated` (`:437`) is
in scope.

**After** (site 1, declarations — the other two are the identical tail transform):

```cpp
    log.Append(q_decl.SpellingRange())
        << "Cannot fabricate the demand declarations for '" << base_name
        << "': a user declaration collides with the reserved demand__ "
        << "prefix; rename it"
        << (pragma_activated ? " or remove the @key pragma"
                             : " or recompile without -demand");
```

**Exact strings** (verbatim, both arms):
- flag arm (byte-identical to today): `"…prefix; rename it or recompile without -demand"`
- pragma arm: `"…prefix; rename it or remove the @key pragma"`

for all three site bodies (`"…demand declarations for '<name>'…"`,
`"…demand message '<name>'…"`, `"…demand relation '<name>_local'…"`). Note this
differs from the authored DIFF-K6-1: the fork is on the TAIL after `"rename it"`
(`" or remove the @key pragma"` / `" or recompile without -demand"`), NOT a
`"; fix or remove…"` splice — so the flag arm reproduces today's bytes exactly and
the pragma arm reads grammatically. **Golden-delta: ZERO** (reject-only paths; the
rejects lane pins no diagnostic text; flag-arm bytes preserved).

## A-K6-2 — DIFF-K6-2 all three drift sites (supersedes Part B `:367-406`)

Normalize "demand key" → "instance key" at ALL THREE user-facing sites (confirmed
this session):
- `lib/Parse/Parser.cpp:957`: `"demand key of "` → `"instance key of "`.
- `lib/DataFlow/Demand.cpp:480`: `"' declares a demand key but no "` →
  `"' declares an instance key but no "`.
- `lib/DataFlow/Demand.cpp:874`: `"' declares a demand key but is not "` →
  `"' declares an instance key but is not "` (Part A `:80` / ledger `:888`
  mislabeled this "@key pragma"-family — it IS drift). **Golden-delta: ZERO**
  (`grep "demand key" tests/OptDiff/{goldens,cases}` → nothing; diagnostic text
  unpinned by policy).

## A-K6-3 — DIFF-K6-3 index-preserving Arm A (supersedes Part B `:449-451`)

The landed Arm A (`Demand.cpp:928-937`) iterates canonicalized `declared_sets`
(`std::set<std::vector<unsigned>>`) — its element carries no index, so
`InstanceKeyRanges()[surplus_set_index]` cannot be written. Restructure Arm A to an
index-preserving iteration:

```cpp
    // Arm A — a declared @key set with no matching demanded adornment.
    for (unsigned j = 0u; j < p_demanded_decl.InstanceKeys().size(); ++j) {
      std::vector<unsigned> d = canon(p_demanded_decl.InstanceKeys()[j]);
      if (!inferred_sets.count(d)) {
        // K6-3: anchor to THIS set's pragma range, not the whole decl.
        log.Append(p_demanded_decl.InstanceKeyRanges()[j])
            << "Declared instance key (" << names(d) << ") on "
            << p_demanded_decl.KindName() << " '"
            << p_demanded_decl.NameAsString() << "' has no matching demanded "
            << "query adornment; fix or remove the @key pragma";
        return false;
      }
    }
```

Iterate `InstanceKeys()` by index `j`, canon element-wise, test membership against
the `inferred_sets` built from `a.p_bound`, and anchor `InstanceKeyRanges()[j]`
(parallel vector, same index = same set). Arm B stays decl-anchored (its surplus is
INFERRED — no pragma to point at), unchanged from Part B. **Golden-delta: ZERO**
(rejects lane compares exit code only; no caret range is pinned).

## A-K6-5 — DIFF-K6-5 A5 correction (supersedes the A5 "PLAIN never .drflags" premise)

The behavioral binary IS `.drflags`-compiled: `runall.sh:407` uses
`$(flags_of "$bmode")`, and `flags_of` appends the `.drflags` sidecar
(`:214-216`) — so a `.drflags=-demand` case's behavioral binary is demand-lowered
(the runall.sh comment at `:367-368` and CLAUDE.md's "PLAIN never .drflags" describe
INTENT the code does not implement; this is a standing discrepancy, recorded, NOT a
K6-5 obligation to reconcile). The precedent that makes DIFF-K6-5 sound:
`demand_tc_witness` — a `.batches` + `.probes` + `.drflags=-demand` case with a real
`.behavioral.stdout` golden, present and passing at tip — already runs this exact
demand-lowered behavioral path. Therefore DIFF-K6-5's `demand_multi_adorn_witness`
(`.drflags=-demand`) and `key_multi_adorn_witness` (flagless-pragma) behavioral
goldens are GENERATED-then-reviewed against the demand-lowered binary, exactly as
`demand_tc_witness`'s was — no design change beyond correcting the A5 wording. The
GENERATED-then-reviewed protocol and the `bless_copy` symlink discipline stand
unchanged. (Recorded: no oracle-vs-behavioral cross-family byte check exists — the
two goldens answer different questions.)

**SUPERSEDED by F32 (2026-08-05, session 9, owner-ratified):** this deferral's
basis was incomplete evidence. `demand_tc_witness` "present and passing" is one of
the five accidentally-agreeing MONOTONE-regime cases; the four DIFFERENTIAL-regime
demand cases' behavioral binaries disagreed with the demand-blind interpreter on
every run, and their `REFINTERP-DISAGREE` verdicts were silently dropped by the
suite aggregator's failure-token blacklist (F32 leg (b)) — invisible when A-K6-5
was written. The 2026-08-03 plain-compile adjudication is now IMPLEMENTED
(`mode_flags_of`), the four goldens re-blessed to the definitional CBF, and the
aggregator is a whitelist. See tests/OptDiff/FINDINGS.md F32. The parenthetical
"no oracle-vs-behavioral cross-family byte check exists" is also no longer true:
run_crossfamily (XFAM, landed the same session) byte-compares the oracle's
`--project-published` differential projection against the behavioral golden's
FINAL block for every `.batches` case.

## A-K6-7b — DIFF-K6-7b field spellings (supersedes Part B `:833-834`, `:839-842`)

DRVec def/use members are `defs` / `uses` (`Rel.h:403-404`), not `def` / `use`.
The `EmitDRFlowDOT` def/use-edge loop:

```cpp
  for (unsigned vi = 0u; vi < flow.vecs.size(); ++vi) {
    for (unsigned oi : flow.vecs[vi].defs) os << "op" << oi << " -> vec" << vi << ";\n";
    for (unsigned oi : flow.vecs[vi].uses) os << "vec" << vi << " -> op" << oi << ";\n";
  }
```

The independent id-order walk is acceptable for this advisory, never-goldened
emitter (factoring a shared walk out of `EmitDRFlow` is the recorded alternative,
not required). Re-confirm `DROpKindName`/`DROpStratum` spellings at implementation.
**Golden-delta: ZERO** (brand-new advisory sink).

## A-K6-B — reshaped witness texts (Part C desired-state)

**`tests/OptDiff/rejects/reject_key_redecl_1.dr`** (full text; message-first so
`edge_2` is forward-declared; verified rc=0 at tip = the check is load-bearing):

```
; expect: instance-key redeclaration divergence (IDENTICAL-OR-ABSENT, DIFF-K6-4).
; Two @key-bearing redecls of `rel` declaring {A} vs {B}. Distinct query name
; `q` so no cross-kind AddDecl collision masks the K6-4 check; compiles rc=0
; at tip (the check is absent), rejects rc=1 once DIFF-K6-4 lands.
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
#local rel(u64 A, u64 B) @key(B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
q(A, B) : rel(A, B).
```

Optional cheap TRUE-branch exercise (ADJ-K6-B): insert an identical middle redecl
`#local rel(u64 A, u64 B) @key(A).` between the two above so redecl-2-vs-1 hits
`SameKeySetOfSets == TRUE` (no reject) before redecl-3-vs-2 diverges — exercises,
does not pin, the TRUE branch at zero golden cost.

**`tests/OptDiff/cases/key_multi_adorn_witness.dr`** (amended head — a pragma-free
redecl BEFORE the pragma'd decl; the rest of the file unchanged; the added line
must leave all existing goldens byte-identical, the blocking gate):

```
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B).                 ; absent redecl — inherits @key silently
#local rel(u64 A, u64 B) @key(A) @key(B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

Post-ADJ-K6-A(b) this compiles byte-identical to today (order-independent
absent-inherit); pre-fix it silently drops the pragma
(`kSubgraphInstantiate` 2 → 0) and flips every witness golden — the loud-regression
guard. If byte-identity fails at implementation, split to a dedicated
`key_redecl_witness` (adds goldens, flips none).
