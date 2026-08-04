# K1 — Multi-`@key` repetition (the multi-adornment surface lift): Part A, grounded pseudocode of the landed reality

**Status:** Part A ONLY — a faithful pseudocode transcription of what is *already
landed* at tip `4d67c721` (branch `keyed-instances`), plus a one-paragraph K1
scope note. The diffs are a LATER pass by a different agent; this document does
NOT design the change beyond §A5.

**Authoring date:** 2026-08-04. Single-pass, dated, file/function/line exact.
All line numbers are cited **tip-exact** (transcribed from a fresh scan this
session — the critique panel will refute-verify against the code; §VERIFICATION
LEDGER enumerates every citation).

---

## Header note — the two ratifications this session (2026-08-04, owner)

1. **K1 ranked first.** Multi-`@key` repetition (`@key(A) @key(B)` on one decl →
   N keyed stores, the multi-adornment surface lift) is the next slice.
   **Multi-`@key` semantics = TOTAL BIJECTION:** every SIP-inferred adornment
   must have a matching declared `@key` set and vice versa — an **order-free
   set-of-sets equality** between the declared key-sets and the inferred bound
   sets. Not "all adornments share one declared set" (that is the current landed
   single-set check, which K1 replaces) and not "some declared set covers each
   adornment" (a proper-subset relation would silently drop a declared key).

2. **K2 (RP-8 auto-sweep) stays STRICT.** The global `-demand` auto-sweep
   ("try auto-demand after the user-specified ones") remains a STRICT layer —
   an inadmissible forcing under the auto sweep is a *reject*, not a
   fence→skip. The best-effort variant is NOT adopted. (K2 is out of scope for
   K1 and is recorded here only to pin the ratified posture.)

**Normative record.** The design authority remains
`region-model-diffs.md` (RP-1..9, session-5 AMENDMENTS + the session-6 @DEMAND
section). This artifact is the K1 *working* authority. Seed lineage:
`key-pragma-landed-seed.md` Part 2 `DIFF-NEXT-K1`.

**Four seed-drift corrections folded in** (already adjudicated this session):
- `ConnectInsertsToSelects` lives in `lib/DataFlow/Connect.cpp` (not Build.cpp).
  It records the `insert_proxy → rel->declaration` correlation into the
  Build-scoped `proxy_view_to_decl` map that Demand.cpp reads once (§A2).
- The recursive-content / mutual-recursion fences live in the **ControlFlow
  `Program::Build` nested pre-pass** (`lib/ControlFlow/Build/Build.cpp`), NOT in
  `Demand.cpp` Loop 1 (§A3). `demand_recursive_content_1` is additionally caught
  UPSTREAM by the plain-`-demand` body-walk; `demand_mutual_content_1` is the
  R-BODYWALK shadowed belt.
- `key_tc_witness` has **11 symlinked goldens** (not "all") — `.contract.opt` and
  `.behavioral.stdout` are its OWN real goldens (§A4).
- The three `demand__`-collision rejects do **not** fork advice on
  `pragma_activated` (recorded K6 rider; noted where relevant in §A2 — the
  `reject()` lambda's advice fork is a *separate* mechanism).

---

## A1. Parser — the `@key(K...)` pragma-tail state machine (as landed)

**File:** `lib/Parse/Parser.cpp`, function `ParseLocalExport`. The driving
`switch (state)` is at `Parser.cpp:414`, inside the sub-token `for` loop
(`for (next_pos = tok.NextPosition(); ReadNextSubToken(tok); ...)`).

### State variables (locals of `ParseLocalExport`)

Declared at `Parser.cpp:390-396` (peers of `int state = 0; ... Token name;
Token highlight; bool has_mutable_parameter = false;` at 374-402):

```
  Token key_pragma_tok;        // anchors the states-21/22 diagnostics
  bool key_expect_var = false; // in-arg-list sub-state (expecting a var vs a , / ))
```

**No-DisplayRange fact (K6-ii, CONFIRMED):** `key_pragma_tok` is a *function
local only*. It is never written into `local->`; the pragma's own source
position is NOT persisted onto `ParsedDeclarationImpl`. The only surviving
storage is the resolved `std::vector<unsigned>` column-index set (§storage
below). There is no source-range accessor for the `@key` pragma on the public
`ParsedDeclaration` API.

### `case 8:` — the pragma tail; the `kPragmaKey` branch (`Parser.cpp:784-802`)

State 8 is the post-parameter-list pragma tail (reached after the `)` closes the
parameter list). The `@key` arm:

```
  case 8:
    if (Lexeme::kPragmaKey == lexeme) {
      // *** THE SECOND-@key REJECT that K1 LIFTS *** (Parser.cpp:789-797)
      if (!local->instance_key_param_indices.empty()) {
        error << "Unexpected second '" << tok << "' pragma on "
              << local->KindName() << " '" << local->name << "/"
              << local->parameters.Size()
              << "'; multiple instance keys (one per adornment) are not yet "
              << "supported";
        return;
      }
      key_pragma_tok = tok;
      state = 21;            // → the argument-list sub-machine
      continue;
    } else if (Lexeme::kPragmaPerfInline == lexeme) { ... @inline ... }
      else if (Lexeme::kPuncPeriod == lexeme) { local->last_tok = tok; state = 9; }
      else if (Lexeme::kPragmaDebugHighlight == lexeme) { ... state = 8; }
      else if (Lexeme::kPuncColon || Lexeme::kPuncColonHyphen) { ... embedded clause ... }
      else { "Unexpected tokens before the terminating period ..." ; state = 10; }
```

The whole `case 8:` closes at `Parser.cpp:874`. The **second-`@key` reject** at
`Parser.cpp:789-797` is the single most important K1 target in the parser: it
GUARDS against `@key(A) @key(B)` today and must become the N-pragma accumulation
path. Its exact string (segments concatenated):
`"Unexpected second '" << tok << "' pragma on " << KindName << " '" << name << "/" << arity << "'; multiple instance keys (one per adornment) are not yet supported"`.

### States 21 and 22 — the `@key( ... )` argument list (`Parser.cpp:890-984`)

Parameters are already bound (post-paren position), so each key column resolves
IMMEDIATELY to a parameter index — no deferred resolve. A pragma left open at
EOF exits the loop mid-state and the `state != 9` gate at `Parser.cpp:988` draws
the generic incomplete-declaration diagnostic (no bespoke EOF arm).

```
  case 21:  // After `@key`: expect the argument list's `(`.  (Parser.cpp:891-903)
    if (Lexeme::kPuncOpenParen == lexeme) { key_expect_var = true; state = 22; continue; }
    else  // Parser.cpp:903-905
      error << "Expected '(' after '" << key_pragma_tok << "' on "
            << KindName << " '" << name << "', but got '" << tok << "' instead";
      return;

  case 22:  // Inside `@key( ... )`.  (Parser.cpp:906-983)
    if (Lexeme::kIdentifierVariable == lexeme) {
      if (!key_expect_var)  // Parser.cpp:913-915
        error << "Expected ',' or ')' in the instance key of "
              << KindName << " '" << name << "', but got '" << tok << "' instead";
        return;
      // IMMEDIATE resolution: scan local->parameters for a name-id match.
      resolved_index = ~0u;
      for (param : local->parameters)
        if (param->name.IdentifierId() == tok.IdentifierId()) { resolved_index = param->index; break; }
      if (resolved_index == ~0u)  // Parser.cpp:929-932  (unknown column)
        error << "Unknown key column '" << tok << "' in the instance key of "
              << KindName << " '" << name << "/" << arity
              << "'; every instance key column must name a declared parameter";
        return;
      for (prev : local->instance_key_param_indices)  // Parser.cpp:938-940 (duplicate)
        if (prev == resolved_index)
          error << "Duplicate column '" << tok << "' in the instance key of "
                << KindName << " '" << name
                << "'; an instance key is a duplicate-free ordered column set";
          return;
      local->instance_key_param_indices.push_back(resolved_index);
      key_expect_var = false;
      continue;

    } else if (Lexeme::kIdentifierUnnamedVariable == lexeme) {  // Parser.cpp:950-952
      error << "Instance key columns of " << KindName << " '" << name
            << "' must be named; wildcard/anonymous variables ('" << tok
            << "') are not permitted";
      return;

    } else if (Lexeme::kPuncComma == lexeme) {
      if (key_expect_var)  // Parser.cpp:958-960  *** SAYS "demand key" — see finding F-1 ***
        error << "Expected named variable (capitalized identifier) in the "
              << "demand key of " << KindName << " '" << name
              << "', but got '" << tok << "' instead";
        return;
      key_expect_var = true; continue;

    } else if (Lexeme::kPuncCloseParen == lexeme) {
      if (local->instance_key_param_indices.empty() || key_expect_var)  // Parser.cpp:969-971
        error << "The instance key of " << KindName << " '" << name
              << "' must list at least one named column and may not end with a "
              << "trailing comma";
        return;
      key_expect_var = false;
      state = 8;   // BACK to the pragma tail (period / other pragmas)
      continue;

    } else {  // Parser.cpp:980-982
      error << "Expected ')' to close the instance key of "
            << KindName << " '" << name << "', but got '" << tok << "' instead";
      return;
    }
```

**The state-8 ↔ state-22 round-trip is the K1 hook:** a closing `)` returns to
state 8, where a *second* `@key` currently hits the `:789` reject. Lifting that
reject re-enters states 21/22 and appends the next key-set.

### Retired-bracket redirect (`case 1:`, `Parser.cpp:429-451`)

A `[` before the parameter list draws the pointed redirect (diag at
`Parser.cpp:440`): `"Declared instance keys are written as '@key(Col, ...)' after
the parameter list of " << introducer_tok << " '" << name << "', not as a bracket
before it"`. The bracket lexemes remain parser-unconsumed; `rel[Bound](Free)`
stays design-doc/dump notation.

### Storage

**Field** — `lib/Parse/Parse.h:381-386` (in `ParsedDeclarationImpl`):

```
  std::vector<unsigned> instance_key_param_indices;
```

This is the ONLY `@key`-related field on the impl (grep-complete over the file:
only lines 381-385 comment + 386 field). It holds parameter indices in written
column order. **The doc comment is STALE** — it still describes the retired
`name[K...]` bracket surface, not the landed `@key(K...)` pragma (finding F-2).
Empty means "no `@key`".

**Public accessors** — declared at `include/drlojekyll/Parse/Parse.h:441-447`,
defined at `lib/Parse/Parse.cpp:851-858`:

```
  bool HasInstanceKey() const noexcept;               // → !indices.empty()
  const std::vector<unsigned> &InstanceKey() const noexcept;  // → indices
```

Same stale-bracket comment (F-2). Both are single-key-set shaped today —
`InstanceKey()` returns ONE vector. **K1 must widen this shape** (§A5).

### Formatter round-trip (`lib/Parse/Format.cpp:98-123`)

`operator<<(OutputStream&, ParsedDeclaration)` re-emits the pragma in written
order so the parse round-trip is byte-faithful:

```
  if (decl.HasInstanceKey()) {
    auto demand_sep = " @key(";              // *** local still named demand_sep (F-3) ***
    for (param_index : decl.InstanceKey())
      os << demand_sep << decl.NthParameter(param_index).Name();
      demand_sep = ", ";
    os << ")";
  }
```

The local variable name `demand_sep` is a pre-rename cosmetic leftover (F-3),
no functional effect. K1 must loop this block over N key-sets.

---

## A2. Demand pass — activation, R-1BOUND, the two-phase loop, decl resolution, Step 2b

**File:** `lib/DataFlow/Demand.cpp`, function `QueryImpl::ApplyDemandTransform`
(signature at `Demand.cpp:386`). Run at the post-`ConnectInsertsToSelects` slot
in `Build.cpp`. `proxy_view_to_decl` (the Connect-recorded
`insert_proxy → declaration` map, minted in `lib/DataFlow/Connect.cpp`) is passed
in as an argument.

### Activation gate (RP-6, `Demand.cpp:386-451`)

```
  bool ApplyDemandTransform(module, log, demand_mode, demand_retract,
                            suppress_demand, proxy_view_to_decl):

    if (suppress_demand) return true;   // bin/Oracle: DEMAND-BLIND, overrides
                                        // BOTH flag and pragma (Demand.cpp:394-396)

    // Scan the PARSED module for @key-bearing decls (the durable carrier — a
    // #local's flows are proxied out of `relations` by Connect). Dedup by
    // decl Id across sub-modules, decl order.  (Demand.cpp:410-435)
    demand_key_decls = []
    seen_decl_ids = {}
    for sub_module in ParsedModuleIterator(module):
      for l in sub_module.Locals():   d = ParsedDeclaration(l)
        if d.HasInstanceKey() && seen_decl_ids.insert(d.Id()): demand_key_decls.push(d)
      for e in sub_module.Exports():  d = ParsedDeclaration(e)
        if d.HasInstanceKey() && seen_decl_ids.insert(d.Id()): demand_key_decls.push(d)

    pragma_activated = !demand_key_decls.empty()         // Demand.cpp:437
    if (!demand_mode && !pragma_activated) return true;  // containment gate (Demand.cpp:438-440)

    if (module.DemandMessagesFabricated())               // G2 at-most-once (Demand.cpp:442-450)
      log << "Internal error: the demand transform was re-entered ..."; return false;

    // The reject() advice-fork lambda (Demand.cpp:445-451):
    reject = (what) -> log << what
        << (pragma_activated ? "; fix or remove the @key pragma"
                             : "; recompile without -demand"); return false;
```

**K6 rider:** the `reject()` advice fork keys on `pragma_activated`. The three
`demand__`-collision rejects (in `lib/Parse/Demand.cpp`) do NOT participate in
this fork — they are a separate mechanism and their advice does not vary on
`pragma_activated`. K1 does not touch that.

### `bound_queries` build + no-bound / R-1BOUND rejects

```
  bound_queries = [REL* for every bound #query decl]        // Demand.cpp:457-469

  if (bound_queries.empty()):                               // Demand.cpp:471-484
    if (pragma_activated):
      d = demand_key_decls[0]
      log(d.SpellingRange) << "'" << d.NameAsString()
          << "' declares a demand key but no bound #query exists to seed "
          << "demand; remove the @key pragma or add a bound query"; return false
    return true                                             // benign no-op flag-only

  // *** R-1BOUND *** (Demand.cpp:486-490)
  if (bound_queries.size() > 1u):
    return reject("Multiple demanded (bound) queries are not yet supported "
                  "under -demand")
```

**R-1BOUND** is the load-bearing invariant for both §A3's `front()`-only decl
consult AND Step 2b's `plan.size()`-is-forcing-count reasoning: with one bound
query name, all forcings/adornments share the ONE demanded relation `p`
(one `p_merge`). The realization reject at `:471-484` and this no-bound-query
reject BOTH say `"declares a demand key"` — see finding F-1 (the "demand key" vs
"instance key" wording is inconsistent across the surface, not confined to
Parser.cpp:959).

### The two-phase per-adornment loop (D3.a.3)

**`PerAdornment` plan-entry struct** (`Demand.cpp:504-516`) — one
traced-but-not-yet-minted adornment; Loop 1 fills it (Steps 1b+2+3, NO minting),
Loop 2 consumes it (Steps 5-10):

```
  struct PerAdornment {
    ParsedDeclaration redecl;             // the declared binding pattern
    vector<unsigned> bound_indices;       // Step-1 (redecl-derived)
    vector<unsigned> p_bound;             // adornment as p-column positions
    TUPLE *q_read; VIEW *q_consumer; MERGE *p_merge;   // Step-2 outputs
    vector<GuardSite> sites;              // Step-3 outputs
    vector<TUPLE *> pushdown_reads;
  };
  unordered_set<VIEW*> known_consumers;          // ADV-3 pass-level union
  vector<PerAdornment> plan;
  unordered_set<string> seen_variants;           // mirror Build.cpp:678
```

**Loop 1 (Phase 1)** — `Demand.cpp:522-846`. Header + `seen_variants` dedup
(`Demand.cpp:522-532`); the dedup key is `redecl.BindingPattern()` string
(`Demand.cpp:529`):

```
  for redecl in q_decl.UniqueRedeclarations():             // Demand.cpp:527
    binding = redecl.BindingPattern()
    if !seen_variants.insert(binding): continue            // Demand.cpp:529-531
    ... Steps 1b + 2 + 3 (fence bodies not extracted) ...
    // Loop-1 tail (Demand.cpp:836-846):
    known_consumers.insert(q_consumer); + each site.consumer
    assert(plan.empty() || p_merge == plan.front().p_merge) // ONE relation p
    plan.push_back(PerAdornment{redecl, bound_indices, p_bound, q_read,
                                q_consumer, p_merge, sites, pushdown_reads})
```

**Post-Loop-1 decl resolution + RP-6 realization check** (`Demand.cpp:848-877`):

```
  p_decl_it = proxy_view_to_decl.find(plan.front().p_merge)    // Demand.cpp:855
  if (miss): fprintf(stderr, "T1-DECL-MISS: ..."); abort()     // Demand.cpp:856-861
  p_demanded_decl = p_decl_it->second

  // RP-6 realization: every @key decl must BE the demanded target p.
  for d in demand_key_decls:                                   // Demand.cpp:868-876
    if (d.Id() != p_demanded_decl.Id()):
      log(d.SpellingRange) << "'" << d.NameAsString()
          << "' declares a demand key but is not the demanded relation ('"
          << p_demanded_decl.NameAsString()
          << "' is); remove the @key pragma"; return false
```

The map is Build-scoped and pre-Optimize-valid HERE and nowhere later (VIEW*
keys dangle past Optimize). A miss is a broken invariant (loud abort, never
name-guess).

### Step 2b — V-DECLARED-KEY as landed (`Demand.cpp:879-920`) — THE K1 CORE

Sits between decl resolution and Step 4. `demand_forcings` is still empty here
(populated only in Loop 2), so `plan.size()` IS the forcing count.

```
  if (p_demanded_decl.HasInstanceKey()):

    // ---- (K1-TARGET-1) ADJ-R3-A STRICT single-forcing scope ----  Demand.cpp:891-901
    if (2u <= plan.size()):
      log(p_demanded_decl.SpellingRange())
          << "An instance key on " << KindName << " '" << NameAsString()
          << "' is only supported when it is demanded under a single query "
          << "adornment; fix or remove the @key pragma"; return false

    // ---- (K1-TARGET-2) per-adornment single-set reconciliation ----  Demand.cpp:903-919
    declared     = p_demanded_decl.InstanceKey()           // ONE vector today
    declared_set = set(declared)
    for a in plan:
      inferred = set(a.p_bound)
      if (inferred != declared_set):
        log(p_demanded_decl.SpellingRange())
            << "Declared instance key of " << KindName << " '" << NameAsString()
            << "' disagrees with the demanded binding pattern; fix or remove "
            << "the @key pragma"; return false
```

**Which lines K1 replaces (flagged):** *(Part-B correction: the ranges below
bundle the leading comments; the EXECUTABLE lines are `:893-900` (strict) and
`:906-919` (reconciliation) — see Part B DIFF-K1-2.)*
- `Demand.cpp:891-901` — the STRICT single-forcing reject. K1 DELETES this: N
  adornments become supported.
- `Demand.cpp:903-919` — the single-declared-set reconciliation loop (every
  adornment must equal the ONE `declared_set`). K1 REPLACES this with a
  **set-of-sets total bijection**: `{ InstanceKey()[j] as sets } == { a.p_bound
  as sets }` order-free (multiset-of-sets equality; see §A5 for the open
  duplicate-key-set sub-question). Under the current single-set code, even with
  the `:891` reject lifted, `bf` (`{A}`) and `fb` (`{B}`) would each fail the
  `inferred != declared_set` test against a single declared set — so BOTH
  targets must change together.

### Step 4 — stray-consumer accounting (`Demand.cpp:922-951`)

ONCE, between the loops, on the PRE-MINT graph. Every reader of `p_merge` must be
traced; every consumer of a reader must be a guard-site consumer. Untraced ⇒
reject (two distinct strings, `Demand.cpp:936-939` and `:942-946`). Both
adornments demand the SAME `p_merge` (asserted in Loop-1 tail), so the union is
checked once before any guard mint (ADV-3). K1 does not change Step 4's shape but
must keep it correct under N declared key-sets (the stray-consumer union is
adornment-count agnostic already).

**Ordering (normative):** Loop 1 → decl resolution → RP-6 realization → **Step 2b**
→ Step 4 → Loop 2.

### Loop 2 (Phase 2) + registries

Header `Demand.cpp:973-981`; closes `Demand.cpp:1319`. One mint per
adornment/forcing. Per-forcing `forcing_index`/`first_annotation` at
`Demand.cpp:1173-1174`. The `RecognizedSubgraph` register (8b) at
`Demand.cpp:1255-1270`; the `QueryDemandForcing` register (Step 10) at
`Demand.cpp:1310-1317` with `.query = ParsedQuery::From(a.redecl)`.

`RecognizedSubgraph` struct — `include/drlojekyll/DataFlow/Query.h:1023-1041`:
one entry per FORCING; carries `forcing_index`, `demanded_view` (p's post-Connect
MERGE), `key_cols` (bound α positions), `pub_view`, `guard_annotation_indices`,
and `demanded_decl` (the Tier-1 mint-time parse-identity snapshot). Append order
= forcing order; this registry is the keyed-instance census recount source.

`QueryDemandForcing` struct — `include/drlojekyll/DataFlow/Query.h:962-980`;
storage `lib/DataFlow/Query.h:1211` (`std::vector<QueryDemandForcing>
demand_forcings;`); accessor `Demand.cpp:296-299`. "BindingPattern-keyed" is
CONCEPTUAL — the container is a flat `std::vector` appended once per adornment in
forcing order; the distinctness is enforced by the `seen_variants` dedup upstream
(`Demand.cpp:529`), not by a literal map (finding note, not a contradiction).

---

## A3. ControlFlow — the RP-9 admissibility fork + nested lowering shape

**File:** `lib/ControlFlow/Build/Build.cpp`, `Program::Build(const
FrozenRegionalProgram&, ErrorLog&, unsigned first_id, PassPolicy&, bool
demand_instance)` (signature `Build.cpp:1332-1335`).

### Per-forcing admissibility flags + the strict/pragma fork (`Build.cpp:1439-1557`)

The feature-gap fences are computed from LIVE guard JOINs (the CSE-migrating
`GuardAnnotationIndex` stamp), never a stored `RecognizedSubgraph` handle:

```
  any_forcing = false; all_forcings_admissible = true;

  fguards = {}   // forcing_index → [(QueryView, annotation_index)]
  for v in query.ForEachView():
    ai = v.GuardAnnotationIndex(); if (ai == kNoGuardAnnotation) continue
    fguards[annots[ai].forcing_index].push({v, ai})

  for (fi, entries) in fguards:
    any_forcing = true
    recursive_content = false; cyclic_demand = false
    for (v, ai) in entries:
      if (!v.IsJoin()) continue
      jl = JoinedViews(v); if (jl.size() < 2) continue
      if (annots[ai].role == kBody):                 // FENCE (i, ADJ-C2) recursive-CONTENT
        in = jl[1]
        if (in.InductionGroupId() || ViewSelfReachable(in)): recursive_content = true
        for p in in.Predecessors(): if (p.InductionGroupId()): recursive_content = true
      if (ViewSelfReachable(jl[0])): cyclic_demand = true    // FENCE (i) cyclic-demand
    if (cyclic_demand || recursive_content): all_forcings_admissible = false

    if (demand_instance):        // STRICT arm — the -demand-instance FLAG
      if (cyclic_demand):
        log << "Recursive demand relations are not yet supported under -demand-instance"
      else if (recursive_content):
        log << "Demanded subgraphs with recursive (induction-owned) content are "
               "not yet supported under -demand-instance (a keyed-instance feature gap)"

  if (num_errors != log.Size()): return nullopt
```

Note the fences live HERE (ControlFlow pre-pass), not in Demand.cpp — the folded
seed-drift correction. (FENCE (ii) mid-stream monotone edge-add is gone — R-a2
band-(a2) handles it. FENCE (iii) diff-input was lifted at D3.a.2.)

### `effective_demand_instance` — the RP-9 fallback (`Build.cpp:1525-1538`)

```
  bool effective_demand_instance = demand_instance;          // separate local
  if (!demand_instance && any_forcing && all_forcings_admissible):
    subgraphs = query.RecognizedSubgraphs()
    if (!subgraphs.empty() && subgraphs[0].demanded_decl.HasInstanceKey()):   // *** front()/[0] ONLY ***
      effective_demand_instance = true
  ...
  context.demand_instance_enabled = effective_demand_instance   // NEVER from demand_instance directly
```

**`front()`-only decl consultation — IS it N-forcing-correct under R-1BOUND?**
YES, by the load-bearing invariant made explicit in the code's own comment
(`Build.cpp:1525-1531`): with R-1BOUND, all forcings share the ONE demanded
relation `p`, hence the ONE `demanded_decl`, hence the ONE `HasInstanceKey()`
bit. So `subgraphs[0]`'s pragma bit correctly drives every forcing —
admissibility is all-or-nothing (`all_forcings_admissible` already ANDs across
forcings; any inadmissible forcing sends the WHOLE program to the flat arm).
**K1 caveat:** this `[0]`-only read is correct *only while* R-1BOUND holds and
all `demanded_decl`s coincide. K1 keeps one demanded relation `p` with N
adornments, so `demanded_decl` is still shared and `[0]` stays correct. If a
future slice ever lets different forcings key different decls, `[0]` alone would
silently ignore a differently-pragma'd decl on forcing index ≥ 1 — record this as
a K1-adjacent hazard, not a K1 defect.

Strict-arm invariant: `demand_instance` (the flag param) is untouched by the
pragma arm; `context.demand_instance_enabled` is fed from
`effective_demand_instance`.

### Nested lowering per-forcing shape (`lib/Rel/Rel.cpp`)

Gate (`Rel.cpp:2062-2069`): `if (context.demand_instance_enabled)
BuildSubgraphInstanceOps(flow, impl, context, query, scc_map);`.

`BuildSubgraphInstanceOps` (`Rel.cpp:1038-1054`): `LiveRecognition lr =
ResolveLiveRecognition(impl, query)` computed ONCE (shared across forcings), then
looped over `query.RecognizedSubgraphs()` — ONE `RecognizedSubgraph rs` per
forcing, indexed per-iteration by `lr.by_forcing.find(rs.forcing_index)`. Fully
dead forcings (all guards eliminated) are ABA-safe skipped. `sid =
flow.instances.size()` at loop-body entry (`Rel.cpp:1080`) — per-forcing
sequential id. Each live forcing mints **exactly one `kSubgraphInstantiate` op**
(`Rel.cpp:1112-1161`, plus `flow.instances.push_back` at `Rel.cpp:1109`). So
**N forcings → N `kSubgraphInstantiate` ops → N distinct instance stores/pubs.**
`demand_multi_adorn_witness` is the landed proof: `kSubgraphInstantiate=2`, two
stores sharing one pub.

### V-INST-SOLE re-key (`CheckInstanceSolePub`, `Rel.cpp:5002-5023`)

Re-keyed on `(pub_table, forcing_index)` (D3.a.3 O1) so N forcings can share one
pub while each `(pub, forcing)` pair still has EXACTLY one deriver:

```
  map<pair<uintptr_t /*pub_table*/, unsigned /*forcing_index*/>, unsigned> inst_per_pub
  for op in flow.ops:
    if (op.kind == kSubgraphInstantiate && op.table_op_table):
      ++inst_per_pub[{ptr(op.table_op_table), op.forcing_index}]
  for kv in inst_per_pub:
    if (kv.second != 1): ValidatorFail("V-INST-SOLE: a (published table, forcing) "
        "pair has more than one SUBGRAPH_INSTANTIATE deriver")
```

Declared `Rel.h:1237-1255`; called `Rel.cpp:4386`. Always-on (fprintf+abort,
survives NDEBUG). This re-key is exactly what makes K1's shared-pub × N-store
shape legal — K1 needs no further V-INST-SOLE change.

---

## A4. Test surface (as landed)

### `key_multi_adorn_1` — the STRICT single-forcing reject (all-4-modes, FLAGLESS)

`tests/OptDiff/cases/key_multi_adorn_1.dr` (verbatim):

```
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

One `@key(A)` pragma on a relation demanded under TWO adornments (`bf`+`fb`).
Rejects at Step 2b `Demand.cpp:891-901` (the STRICT single-forcing scope). NO
`.drflags` sidecar — FLAGLESS via RP-6 activation (only `.dr` + `.main.cpp` in
the case dir). runall.sh entry: the `|`-joined all-4-modes-diagnostic case-arm at
`tests/OptDiff/runall.sh:498` (arm body `runall.sh:496-503`, `for mode in opt
nodf nocf none; do expect_diagnostic $mode; done`). No dedicated header comment.
**K1 flips this reject→golden** (§A5).

### `demand_multi_adorn_witness` — the mono flagship (the K1 answer twin)

`tests/OptDiff/cases/demand_multi_adorn_witness.dr` (verbatim):

```
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

Same graph as `key_multi_adorn_1` but with NO `@key` pragma and driven under
`-demand`. Inventory:
- `.drflags` = `-demand` (bare, flat/mono flagship; no trailing newline)
- `.eqgate` = `-demand -demand-instance` (re-drives nested; N=2 disjoint stores)
- `.irgold` = `region opt / region nodf / region nocf / region none`
- Goldens: FOUR REAL `.region.<mode>.golden` (opt/nodf/nocf/none, 864 B each) +
  ONE REAL `.stdout` (76 B). All REAL, no symlinks. NO `.batches`, hence no
  oracle/monotone/behavioral/contract goldens.

Driver shape (`demand_multi_adorn_witness.main.cpp`): two lambda probes —
`probe_bf(a, expect)` calls `q_bf(db, log, functors, a)`, drains + sorts +
compares (out-neighbors-by-A); `probe_fb(b, expect)` symmetric (in-neighbors-by-B).
Explicit cursor-contract comment (drain fully before next entry point; sort every
keyed drain — order unspecified). Graph has ≥2 components; probes at A=10/B=11
exercise "the OTHER component" as a mis-keyed-store leak check.
`emit("bf",1,{2,3})`, `emit("bf",2,{4})`, `emit("bf",10,{11})`,
`emit("fb",4,{2})`, `emit("fb",2,{1})`, `emit("fb",11,{10})`, `emit("fb",3,{1})`.

**Nested-arm check is the LIVE `.eqgate`**, not a blessed nested golden:
`run_eqgate` (`runall.sh:445-494`) re-compiles under `flags_of($mode)
-demand-instance` (`.drflags`'s `-demand` already inside `flags_of`, then
`-demand-instance` appended) with the SAME driver in all 4 modes and byte-compares
each mode's stdout against the ONE committed `.stdout` golden
(`NESTED-GOLDEN-DIVERGE` on mismatch). flat==golden (diffrun) ∧ nested==golden
(eqgate) ⇒ flat==nested transitively.

### `demand_multi_adorn_allfree_1` — the all-free-sibling fence (all-4-modes-diagnostic)

`tests/OptDiff/cases/demand_multi_adorn_allfree_1.dr` (verbatim tail):

```
#query q(bound u64 A, free u64 B).
#query q(free u64 A, free u64 B).
q(A, B) : rel(A, B).
```

`.drflags` = `-demand`. A query name carrying a BOUND adornment AND an all-free
sibling: the all-free cursor would read the demand-GUARDED pub and silently
UNDER-answer. Rejected by the demand pass (the per-adornment
`bound_indices.empty()` arm). runall.sh: header narrative `runall.sh:32-34` + the
`:498` case-arm. An all-free-ONLY query name is demand-inert and compiles (never
enters the per-adornment loop); the fence fires only on the bound+all-free MIX.
**K1 must decide whether/how the allfree fence interacts** (§A5 open sub-question)
— K1 lowers a bound+bound repetition, not bound+allfree, so the allfree fence
should remain a distinct diagnostic, but the diff pass must confirm the
`@key`-pragma'd allfree case draws the right reject.

### The symlink-twin golden pattern (the `key_tc_witness` 11-symlink exemplar)

`key_tc_witness` is the activation-equivalence twin of `demand_tc_witness`
(pragma-activated compile == `-demand`-activated compile, byte-for-byte). **11
symlinked goldens** (each a surface claimed byte-identical to the `-demand` twin):

```
key_tc_witness.df.opt.golden       -> demand_tc_witness.df.opt.golden
key_tc_witness.h.opt.golden        -> demand_tc_witness.h.opt.golden
key_tc_witness.ir.opt.golden       -> demand_tc_witness.ir.opt.golden
key_tc_witness.monotone.stdout     -> demand_tc_witness.monotone.stdout
key_tc_witness.oracle.stdout       -> demand_tc_witness.oracle.stdout
key_tc_witness.region.nocf.golden  -> demand_tc_witness.region.nocf.golden
key_tc_witness.region.nodf.golden  -> demand_tc_witness.region.nodf.golden
key_tc_witness.region.none.golden  -> demand_tc_witness.region.none.golden
key_tc_witness.region.opt.golden   -> demand_tc_witness.region.opt.golden
key_tc_witness.rel.opt.golden      -> demand_tc_witness.rel.opt.golden
key_tc_witness.stdout              -> demand_tc_witness.stdout
```

TWO OWN/REAL goldens: `key_tc_witness.contract.opt.golden` (1261 B) and
`key_tc_witness.behavioral.stdout` (257 B) — the CBF header embeds the case name,
so these cannot be shared. **Bless-through-symlink hazard:** `runall.sh --bless`
writing through a symlink would corrupt the twin's golden. Per CLAUDE.md: "NEVER
bless its symlinked surfaces directly." (Compare `key_neighborhood_witness`,
which symlinks ONLY `.stdout` → `demand_neighborhood_mono_witness.stdout` and
keeps its own real `.rel.opt.golden` — a lighter twin.) K1's `@key`-pragma'd
witness (§A5) will follow this symlink-twin pattern against
`demand_multi_adorn_witness`.

### `key_mismatch_1` / `key_undemanded_1` — the verbatim reject shapes

`tests/OptDiff/cases/key_mismatch_1.dr`:

```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To) @key(To).
path(F, T) : edge_2(F, T).
path(F, T) : path(F, M), edge_2(M, T).
#query reachable_from(bound u64 From, free u64 To) : path(From, To).
```

V-DECLARED-KEY (all-4-modes, FLAGLESS): declared `@key(To)` disagrees with the
SIP-inferred `{From}`. Rejects at the Step 2b set-reconciliation loop
(`Demand.cpp:903-919`). RP-3 STABLE HARD reject. **K1 must preserve this** —
under the set-of-sets bijection, a single-adornment `@key(To)` vs inferred
`{From}` is still a bijection failure ({{To}} ≠ {{From}}).

`tests/OptDiff/cases/key_undemanded_1.dr`:

```
#message edge_2(u64 From, u64 To).
#local foo(u64 X, u64 Y) @key(X).
foo(X, Y) : edge_2(X, Y).
#local p(u64 A, u64 B).
p(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B) : p(A, B).
```

The RP-6 realization reject (all-4-modes, FLAGLESS): `foo` declares a key but the
bound query demands `p`, not `foo`. Rejects at the RP-6 realization loop
(`Demand.cpp:868-876`). **K1 preserves this** unchanged (realization is
orthogonal to key-set count).

### Corpus counts (tip): `tests/OptDiff/cases/*.dr` = 200; `tests/OptDiff/rejects/*.dr` = 46.

---

## A5. K1 SCOPE NOTE (what the diffs pass must produce — NOT a design)

The K1 diff pass must: (1) **Parser lift** — turn the single
`instance_key_param_indices` vector into a *list of key-sets* (one per `@key`
pragma), which means deleting/repurposing the second-`@key` reject at
`Parser.cpp:789-797` so a closing `)` returning to state 8 can accept another
`@key(...)` and append a new set, widening the `ParsedDeclarationImpl` storage
(§A1) plus its accessors (`HasInstanceKey`/`InstanceKey` → an N-set shape) and the
`Format.cpp:98-123` round-trip (loop over N sets); (2) **Step 2b set-of-sets total
bijection** — replace BOTH the STRICT single-forcing reject (`Demand.cpp:891-901`)
AND the single-declared-set reconciliation loop (`Demand.cpp:903-919`) with an
order-free equality between `{declared key-sets}` and `{inferred a.p_bound sets}`
for `@key`-bearing decls, keeping `key_mismatch_1` (single-adornment bijection
failure) and the RP-6 realization reject (`Demand.cpp:868-876`) intact; (3)
**Witness path** — add a pragma'd flagless twin of `demand_multi_adorn_witness`
(two `@key` pragmas, `@key(A)` + `@key(B)`, over the same bf/fb graph),
eqgate-style with the symlink-twin golden pattern (§A4), and flip
`key_multi_adorn_1` reject→golden (removing it from the `runall.sh:498`
all-4-modes-diagnostic arm and giving it a real `.stdout`, or replacing it with a
new supported-shape case). **Open sub-questions the diff pass must settle:** (a)
**duplicate declared key-sets across pragmas** — `@key(A) @key(A)` should reject
(a set-of-sets bijection wants distinct declared sets; is this a parse reject or a
Step-2b reject, and with what string?); (b) **exact diagnostic wording** for a
bijection failure that names WHICH declared set has no inferred match and vice
versa (the current single-set string "disagrees with the demanded binding
pattern" is inadequate for N sets); (c) **allfree-fence interaction** — confirm a
bound+allfree mix with `@key` pragmas still draws the all-free-sibling fence
(`demand_multi_adorn_allfree_1` shape) and is NOT swallowed by the new bijection
path; (d) **the `[0]`-only `demanded_decl` read** at `Build.cpp:1532-1538` stays
correct only while all N forcings share one `demanded_decl` (true under R-1BOUND +
one demanded relation) — the diff pass should assert/document that invariant
rather than silently rely on it; (e) **the "demand key" vs "instance key" wording
drift** (F-1) — decide whether K1 normalizes the leftover strings while it is
already editing these sites, or leaves them for a separate cleanup.

---

## VERIFICATION LEDGER (every file:line cited — all tip-exact, from the session extracts)

**Parser (`lib/Parse/Parser.cpp`):**
- `:390-396` — `key_pragma_tok` / `key_expect_var` state-var declarations.
- `:374-402` — enclosing local-var block; `:414` — the driving `switch (state)`.
- `:429-451` — `case 1:` retired-bracket redirect; `:440` — its diagnostic string.
- `:784-802` — `case 8:` `kPragmaKey` branch; `:789-797` — **second-`@key` reject (K1 lift target)**; `:874` — end of `case 8`.
- `:890-984` — states 21/22; `:891-903` (case 21), `:903-905` no-paren reject.
- `:906-983` (case 22): `:913-915` id-where-comma, `:929-932` unknown column, `:938-940` duplicate column, `:950-952` anonymous var, `:958-960` comma-when-var (**says "demand key"**, F-1), `:969-971` empty/trailing comma, `:980-982` unexpected token.
- `:988-1001` — post-loop `state != 9` incomplete-decl gate.

**Parse storage / accessors / formatter:**
- `lib/Parse/Parse.h:381-386` — `instance_key_param_indices` field (stale bracket comment, F-2).
- `include/drlojekyll/Parse/Parse.h:441-447` — `HasInstanceKey`/`InstanceKey` decls (stale comment, F-2).
- `lib/Parse/Parse.cpp:851-858` — accessor definitions.
- `lib/Parse/Format.cpp:98-123` — round-trip formatter (`demand_sep` local, F-3).

**Demand (`lib/DataFlow/Demand.cpp`):**
- `:386` — `ApplyDemandTransform` signature; `:386-451` — activation gate + `reject()` lambda; `:394-396` `suppress_demand`; `:410-435` decl scan; `:437` `pragma_activated`; `:438-440` containment gate; `:442-450` G2; `:445-451` `reject()` advice fork.
- `:457-469` — `bound_queries` build; `:471-484` — no-bound pragma reject; `:486-490` — **R-1BOUND**.
- `:504-516` — `PerAdornment` struct; `:522-532` — Loop-1 header + `seen_variants`; `:529` — BindingPattern dedup key; `:836-846` — Loop-1 tail.
- `:848-877` — decl resolution + RP-6 realization (`:855` map find, `:856-861` T1-DECL-MISS abort, `:868-876` realization reject).
- `:879-920` — **Step 2b** (executable block `:887-920`); `:893-900` — **STRICT single-forcing reject (K1-TARGET-1)**; `:906-919` — **single-set reconciliation (K1-TARGET-2)**. *(Part A originally cited `:891-901`/`:903-919` = block-with-comment; corrected to executable lines.)*
- `:922-951` — Step 4 stray-consumer (`:936-939`, `:942-946` reject strings).
- `:973-981` — Loop-2 header; `:1173-1174` — forcing_index/first_annotation; `:1255-1270` — RecognizedSubgraph register (8b); `:1310-1317` — QueryDemandForcing register; `:1319` — Loop-2 close.
- `:296-299` — `DemandForcings()` accessor.

**Contract emitter (`lib/DataFlow/Format.cpp`) — Part-A omission, added in Part B:**
- `:1728-1751` — the `-contract-out` declared-key reconciliation block; `:1733`
  `HasInstanceKey()` + `decl.Id()` dedup; `:1736-1737` `declared-key rel=... declared=(`;
  `:1739` `InstanceKey()` iterate; `:1743` `) inferred=(`; `:1745` `rs.key_cols`.
  (K1 design-question-3 target — DIFF-K1-3.)

**Query IR (`include/drlojekyll/DataFlow/Query.h`, `lib/DataFlow/Query.h`):**
- `include/.../Query.h:1023-1041` — `RecognizedSubgraph` struct.
- `include/.../Query.h:962-980` — `QueryDemandForcing` struct.
- `lib/DataFlow/Query.h:1211` — `demand_forcings` storage member.

**ControlFlow (`lib/ControlFlow/Build/Build.cpp`):**
- `:1332-1335` — `Program::Build(FrozenRegionalProgram&, ...)` signature.
- `:1439-1557` — nested pre-pass admissibility fences; `:1525-1531` — RP-9 all-or-nothing comment; `:1525-1538` (esp. `:1532-1538`) — `effective_demand_instance`, `subgraphs[0]`-only read.

**Rel (`lib/Rel/Rel.cpp`, `lib/Rel/Rel.h`):**
- `Rel.cpp:2062-2069` — `demand_instance_enabled` gate → `BuildSubgraphInstanceOps`.
- `Rel.cpp:1038-1054` — `BuildSubgraphInstanceOps` header + per-forcing loop; `:1080` — `sid`; `:1109` — `flow.instances.push_back`; `:1112-1161` — the `kSubgraphInstantiate` mint.
- `Rel.cpp:5002-5023` — `CheckInstanceSolePub` (V-INST-SOLE re-key); `Rel.cpp:4386` — its call; `Rel.h:1237-1255` — its declaration/doc.

**Tests (`tests/OptDiff/`):**
- `cases/key_multi_adorn_1.dr` (+ absence of `.drflags`).
- `runall.sh:496-503` — all-4-modes-diagnostic case-arm; `:498` — the `|`-joined names line; `:194-206` — `flags_of` / `.drflags` append; `:445-494` — `run_eqgate`; `:17-96` — header expectations; `:32-34` — allfree narrative.
- `cases/demand_multi_adorn_witness.dr` + `.drflags` (`-demand`) + `.eqgate` (`-demand -demand-instance`) + `.irgold`; goldens: 4× `.region.<mode>.golden` + `.stdout` (all REAL).
- `cases/demand_multi_adorn_allfree_1.dr` + `.drflags` (`-demand`).
- `key_tc_witness` — 11 symlinked goldens (df/h/ir/monotone/oracle/region.{nocf,nodf,none,opt}/rel/stdout) + 2 OWN (contract.opt, behavioral.stdout); `key_neighborhood_witness` — 1 symlink (stdout) + own rel.opt.
- `cases/key_mismatch_1.dr`; `cases/key_undemanded_1.dr`.
- Counts: `cases/*.dr` = 200; `rejects/*.dr` = 46.

---

## Findings (contradictions surfaced across the extracts — reported, not smoothed)

- **F-1 ("demand key" vs "instance key" wording drift, scope wider than the
  parser extract claimed).** The parser extract asserts `Parser.cpp:958-959` is
  "the sole diagnostic in the whole surface that doesn't say 'instance key'."
  That is true only *within Parser.cpp*. In `Demand.cpp` the "demand key" wording
  ALSO appears: the no-bound-query reject (`:471-484`, "declares a demand key")
  and the RP-6 realization reject (`:868-876`, "declares a demand key"), while
  Step 2b and the `reject()` lambda say "@key pragma" / "instance key". So the
  drift spans the parser AND the demand pass; the surface mixes at least three
  spellings ("instance key", "demand key", "@key pragma"). Recorded as a K1
  open sub-question (§A5-e), not smoothed.
- **F-2 (stale storage doc comments).** Both `lib/Parse/Parse.h:381-385` and
  `include/drlojekyll/Parse/Parse.h:441-447` still document the *retired*
  `name[K...]` bracket surface ("in the WRITTEN bracket order", "unless the
  declaration was written with a bracket") rather than the landed `@key(K...)`
  pragma. The field/accessor semantics are current; only the prose is stale.
- **F-3 (pre-rename cosmetic leftover).** `lib/Parse/Format.cpp` names its
  separator local `demand_sep` (pre-`@demand`→`@key` era), functionally inert.
- **F-4 (registry keying — terminology, not a code contradiction).** CLAUDE.md
  calls `QueryDemandForcing` "BindingPattern-keyed", but the container is a flat
  `std::vector<QueryDemandForcing>` appended in forcing order; distinctness is
  enforced by the upstream `seen_variants` dedup on `redecl.BindingPattern()`
  (`Demand.cpp:529`), not by any map. Conceptual key, not a literal container —
  noted so the diff pass does not go hunting for a `std::map` that isn't there.

---

## Part B — the K1 diffs (2026-08-04, session 7, opus-authored)

**Status:** the dated diff pass over Part A's pseudocode, authored against a
fresh re-read of every cited site (Parser.cpp, Parse.h ×2, Parse.cpp, the two
Format.cpp emitters, Demand.cpp, Build.cpp, the contract emitter, runall.sh, the
rejects corpus, and the two witness goldens). All Part-A line numbers were
refute-verified; the corrections are folded into the "Part-A corrections" note
below and cited tip-exact here. This part implements RP-10 (TOTAL BIJECTION) +
RP-8-strict-stays and discharges the six orchestrator adjudications
(ADJ-K1-A..E) recorded at the end.

**Diff/hunk convention:** matches `region-model-diffs.md` HUNK R3a-2/-3/-4 —
dated hunks, ` ```diff ` before/after at Part A's grain, exact diagnostic
strings, a per-hunk golden-delta claim, and an obligations line.

### Overview — five hunks

| hunk | site(s) | what |
| --- | --- | --- |
| DIFF-K1-1 | `Parser.cpp` state 8/22, `Parse.h` ×2, `Parse.cpp`, `Parse/Format.cpp` | storage → list-of-sets; delete second-`@key` reject; per-set parse accumulation; ADJ-K1-A parse-time dup-set reject; formatter loop over N sets; F-2/F-3 fixes |
| DIFF-K1-2 | `Demand.cpp:887-920` (Step 2b) | delete the STRICT single-forcing reject; replace the single-set loop with the RP-10 set-of-sets total bijection; two named diagnostic arms |
| DIFF-K1-3 | `DataFlow/Format.cpp:1728-1751` | `-contract-out` declared-key line: one line per declared set, each paired with its matched inferred adornment |
| DIFF-K1-4 | `Build.cpp:1533-1538` | ADJ-K1-D intent-communicating assert (all forcings share one `demanded_decl`) |
| DIFF-K1-5 | `tests/OptDiff/` | witnesses (new golden + 2 new diagnostics + 1 reshaped reject), header comments, runall.sh diagnostic-list edits |

---

### DIFF-K1-1 (parser + storage + formatter) — 2026-08-04

**Design decision 1 (storage + accessor surface).** The single
`std::vector<unsigned> instance_key_param_indices` becomes a **list of sets**:
`std::vector<std::vector<unsigned>> instance_key_param_index_sets` (each inner
vector one `@key` pragma's columns, in written order; the outer vector in
pragma-written order). **Accessor surface:**
- `HasInstanceKey()` stays, semantics UNCHANGED (`!sets.empty()` = *any* set
  declared).
- The single-vector `InstanceKey()` is **DELETED, not kept**, and replaced by
  `const std::vector<std::vector<unsigned>> &InstanceKeys() const noexcept`.

**Caller census (grep-complete, tip-exact — every site that must change):**
`InstanceKey()` (the single-vector accessor) has exactly THREE real call sites,
and ALL THREE are rewritten by K1 anyway — so keeping the old single-set
accessor helps NO caller (SMALLER-NOT-LARGER; a dead accessor left behind would
be the only single-set reader in the tree):
- `Demand.cpp:906` — Step 2b reconciliation → rewritten by DIFF-K1-2.
- `Parse/Format.cpp:118` — round-trip formatter → rewritten below.
- `DataFlow/Format.cpp:1739` — contract-out → rewritten by DIFF-K1-3.

`HasInstanceKey()` callers stay (predicate unchanged): `Demand.cpp:415,421,887`,
`DataFlow/Format.cpp:1733`, `Parse/Format.cpp:116`, `Build.cpp:1535`. The field
itself is touched directly only in `Parser.cpp` (accumulation: `:790,935,944,967`)
and `Parse.cpp:853,857` (the two accessor bodies).

**Storage field (`lib/Parse/Parse.h:381-386`)** — widen + fix the stale bracket
comment (F-2):

```diff
- // The optional declared region key `name[K...]` (DIFF-R3 R3a): parameter
- // indices into `parameters`, in the WRITTEN bracket order (the order is a
- // parse-layer capture only — a DIFF-R5 arrangement hint; R3a reconciles
- // the SET). Empty means "no bracket" (an empty bracket rejects at parse).
- // Populated only for `#local`/`#export` declarations.
- std::vector<unsigned> instance_key_param_indices;
+ // The declared instance key(s) `@key(K...)` (RP-5/RP-9; RP-10 multi-set).
+ // One inner vector per `@key` pragma — parameter indices into `parameters`,
+ // in written column order — and the outer vector in pragma-written order.
+ // Each set is duplicate-free (parse-enforced) and distinct from every other
+ // set on the decl (ADJ-K1-A parse-time dup-set reject). Empty ⇒ no `@key`.
+ // Populated only for `#local`/`#export` declarations.
+ std::vector<std::vector<unsigned>> instance_key_param_index_sets;
```

**Accessor declarations (`include/drlojekyll/Parse/Parse.h:441-447`)** — fix the
stale comment (F-2), swap `InstanceKey`→`InstanceKeys`:

```diff
- // The optional declared region key `name[K...]` (DIFF-R3 R3a; `#local` /
- // `#export` only). `InstanceKey()` returns parameter indices in the WRITTEN
- // bracket order; the SET is the logical region key (the order is an inert
- // DIFF-R5 arrangement hint). Empty unless the declaration was written
- // with a bracket.
- bool HasInstanceKey(void) const noexcept;
- const std::vector<unsigned> &InstanceKey(void) const noexcept;
+ // The declared instance key(s) `@key(K...)` (RP-5/RP-9; RP-10 multi-set;
+ // `#local` / `#export` only). `HasInstanceKey()` is true iff ANY `@key` set
+ // was declared. `InstanceKeys()` returns the list of key-sets (one inner
+ // vector per `@key` pragma, each a duplicate-free column-index set in written
+ // order); the SET-of-SETS is the logical multi-adornment key. Empty list ⇒
+ // no `@key`.
+ bool HasInstanceKey(void) const noexcept;
+ const std::vector<std::vector<unsigned>> &InstanceKeys(void) const noexcept;
```

**Accessor bodies (`lib/Parse/Parse.cpp:851-858`):**

```diff
- bool ParsedDeclaration::HasInstanceKey(void) const noexcept {
-   return !impl->instance_key_param_indices.empty();
- }
- const std::vector<unsigned> &ParsedDeclaration::InstanceKey(void) const noexcept {
-   return impl->instance_key_param_indices;
- }
+ bool ParsedDeclaration::HasInstanceKey(void) const noexcept {
+   return !impl->instance_key_param_index_sets.empty();
+ }
+ const std::vector<std::vector<unsigned>> &
+ ParsedDeclaration::InstanceKeys(void) const noexcept {
+   return impl->instance_key_param_index_sets;
+ }
```

**Parser — `case 8:` second-`@key` reject DELETED (`Parser.cpp:785-802`).** The
closing `)` of state 22 already returns to state 8 (Part A §A1); deleting the
`:790-798` guard lets a *second* `@key` re-enter states 21/22 and open a new
set. A new function-local `std::vector<unsigned> key_cur_set;` is the in-progress
set (peer of `key_pragma_tok`/`key_expect_var` at `Parser.cpp:395-396`), cleared
after each set closes:

```diff
  case 8:
    if (Lexeme::kPragmaKey == lexeme) {
-     // A SECOND `@key` is a clean not-yet-supported reject (RP-5:
-     // repetition is the reserved multi-adornment lift path ...).
-     if (!local->instance_key_param_indices.empty()) {
-       context->error_log.Append(scope_range, tok_range)
-           << "Unexpected second '" << tok << "' pragma on "
-           << local->KindName() << " '" << local->name << "/"
-           << local->parameters.Size()
-           << "'; multiple instance keys (one per adornment) are not yet "
-           << "supported";
-       return;
-     }
+     // RP-10: a second `@key` OPENS a new instance-key set. `key_cur_set` is
+     // empty here (cleared at the prior set's `)` close, or never filled).
      key_pragma_tok = tok;
      state = 21;
      continue;
```

**Parser — state 22 accumulation redirected to `key_cur_set`
(`Parser.cpp:909-984`).** The per-column dup check and the empty/trailing check
scope to the CURRENT set, and the `)` arm does the ADJ-K1-A dup-SET check +
append + clear. Redirects (three edits) then the close arm:

```diff
  case 22:  // Inside `@key( ... )`.
    if (Lexeme::kIdentifierVariable == lexeme) {
      ... resolve `resolved_index` against local->parameters (UNCHANGED) ...
-     for (unsigned prev : local->instance_key_param_indices) {
+     for (unsigned prev : key_cur_set) {           // dup COLUMN within THIS set
        if (prev == resolved_index) {
          ... "Duplicate column '...' in the instance key ..." (UNCHANGED) ...
          return;
        }
      }
-     local->instance_key_param_indices.push_back(resolved_index);
+     key_cur_set.push_back(resolved_index);
      key_expect_var = false;
      continue;
    ...
    } else if (Lexeme::kPuncCloseParen == lexeme) {
-     if (local->instance_key_param_indices.empty() || key_expect_var) {
+     if (key_cur_set.empty() || key_expect_var) {
        ... "must list at least one named column and may not end with a
            trailing comma" (UNCHANGED) ...
        return;
      }
+     // ADJ-K1-A: a set-based dup check against every already-stored set
+     // (order-free — {A} vs {A} and {A,B} vs {B,A} both match). Duplicate
+     // declared sets are the single-adornment ambiguity reborn (RP-10).
+     {
+       std::vector<unsigned> canon(key_cur_set);
+       std::sort(canon.begin(), canon.end());
+       for (const std::vector<unsigned> &prev_set :
+            local->instance_key_param_index_sets) {
+         std::vector<unsigned> prev_canon(prev_set);
+         std::sort(prev_canon.begin(), prev_canon.end());
+         if (prev_canon == canon) {
+           context->error_log.Append(scope_range, key_pragma_tok.SpellingRange())
+               << "Duplicate instance key on " << local->KindName() << " '"
+               << local->name << "'; this '@key' pragma declares the same "
+               << "column set as an earlier '@key' pragma — each instance key "
+               << "must be a distinct column set (one per query adornment)";
+           return;
+         }
+       }
+     }
+     local->instance_key_param_index_sets.push_back(std::move(key_cur_set));
+     key_cur_set.clear();          // ready for a possible next `@key`
      key_expect_var = false;
      state = 8;   // Back to the pragma tail (period / other pragmas).
      continue;
    }
```

The ADJ-K1-A **exact diagnostic string** (segments concatenated):
`"Duplicate instance key on " << KindName << " '" << name << "'; this '@key' pragma declares the same column set as an earlier '@key' pragma — each instance key must be a distinct column set (one per query adornment)"`
anchored at `key_pragma_tok.SpellingRange()` (the offending pragma token).

**Parser — where the dup-set check sits (design decision 1, settled):** at the
`)` close of state 22, on the just-completed `key_cur_set`, BEFORE the append —
NOT at the state-8 second-`@key` entry (where the columns aren't parsed yet) and
NOT in Demand (a malformed *surface* must reject at parse, `-demand`-independent,
so it is a genuine all-4-modes / both-mode-extremes reject like the sibling
dup-COLUMN and empty-set rejects). This is why `reject_key_double_1` (DIFF-K1-5)
is a rejects/-corpus parse case, not a diagnostic demand case.

**Formatter round-trip (`lib/Parse/Format.cpp:114-123`)** — loop over N sets;
rename `demand_sep` (F-3):

```diff
  if (decl.HasInstanceKey()) {
-   auto demand_sep = " @key(";
-   for (unsigned param_index : decl.InstanceKey()) {
-     os << demand_sep << decl.NthParameter(param_index).Name();
-     demand_sep = ", ";
-   }
-   os << ")";
+   for (const std::vector<unsigned> &key_set : decl.InstanceKeys()) {
+     auto key_sep = " @key(";
+     for (unsigned param_index : key_set) {
+       os << key_sep << decl.NthParameter(param_index).Name();
+       key_sep = ", ";
+     }
+     os << ")";
+   }
  }
```

Round-trip: `@key(A) @key(B)` re-emits `@key(A) @key(B)` (written order
preserved on both axes). For N=1 the byte output is IDENTICAL to today
(one ` @key(...)` group).

**Golden-delta claim (DIFF-K1-1):** NONE of the 246 existing goldens change.
The pragma-free corpus never constructs a non-empty `instance_key_param_index_sets`
(the field is empty ⇒ `HasInstanceKey()` false ⇒ the formatter/contract blocks are
skipped) and the accessor rename is a compile-time-only change with no runtime
output. The single-`@key` cases re-emit byte-identically (N=1 formatter loop ≡ the
old single loop). `-dump-parse`/decl-formatter round-trips are unaffected for N=1.

**Obligations (DIFF-K1-1).** DISCHARGED: parser termination unchanged (state 22
still has an explicit edge for every token class + the `)` arm always either
rejects or transitions to state 8; `key_cur_set` is bounded by the parameter
count). ADJ-K1-A dup-set reject is `-demand`-independent (fires during parse) →
a both-mode-extremes rejects/-corpus witness. F-2 (two stale comments) + F-3
(`demand_sep`) FIXED by this hunk (ADJ-K1-C).

---

### DIFF-K1-2 (Step 2b — the RP-10 set-of-sets total bijection) — 2026-08-04

**Site (tip-exact, refute-verified):** `lib/DataFlow/Demand.cpp`, Step 2b is the
`if (p_demanded_decl.HasInstanceKey()) { ... }` block at `:887-920`. Within it:
the STRICT single-forcing reject is the `if (2u <= plan.size())` at `:893-900`
(K1-TARGET-1); the single-declared-set reconciliation is `:906-919`
(K1-TARGET-2). *(Part-A correction: Part A cited these as `:891-901` and
`:903-919` — those ranges bundle the leading comments; the executable lines are
`:893-900` and `:906-919`.)*

**Design decision 2 (multiset vs set — SETTLED to set-of-sets).** Both sides are
duplicate-free when Step 2b runs, so plain **set-of-sets equality is exactly a
bijection** — no multiplicity bookkeeping:
- *Declared side:* two `@key` pragmas declaring the same column set are rejected
  at PARSE by ADJ-K1-A (DIFF-K1-1), so `InstanceKeys()` holds distinct sets here.
- *Inferred side:* `plan` is built under the `seen_variants` dedup on
  `redecl.BindingPattern()` (`Demand.cpp:530`); distinct binding patterns over a
  fixed-arity query name yield distinct bound-position sets, hence distinct
  `a.p_bound` sets. And even in the pathological case where two adornments mapped
  to the same inferred set, the bijection is decided by *existence* checks
  (`d ∈ inferred`, `i ∈ declared`), which are duplicate-insensitive — a repeated
  inferred set would be checked twice with the same verdict. So set-of-sets is
  sound on both sides.

```diff
  if (p_demanded_decl.HasInstanceKey()) {
-   // ADJ-R3-A STRICT single-forcing scope, checked BEFORE reconciliation ...
-   if (2u <= plan.size()) {
-     log.Append(p_demanded_decl.SpellingRange())
-         << "An instance key on " << p_demanded_decl.KindName() << " '"
-         << p_demanded_decl.NameAsString() << "' is only supported when it "
-         << "is demanded under a single query adornment; fix or remove the "
-         << "@key pragma";
-     return false;
-   }
-   // Set-reconciliation: declared SET == the SIP-inferred bound set ...
-   const std::vector<unsigned> &declared = p_demanded_decl.InstanceKey();
-   const std::unordered_set<unsigned> declared_set(declared.begin(),
-                                                   declared.end());
-   for (const PerAdornment &a : plan) {
-     const std::unordered_set<unsigned> inferred(a.p_bound.begin(),
-                                                 a.p_bound.end());
-     if (inferred != declared_set) {
-       log.Append(p_demanded_decl.SpellingRange())
-           << "Declared instance key of " << p_demanded_decl.KindName() << " '"
-           << p_demanded_decl.NameAsString() << "' disagrees with the "
-           << "demanded binding pattern; fix or remove the @key pragma";
-       return false;
-     }
-   }
+   // RP-10 TOTAL BIJECTION (V-DECLARED-KEY, multi-set): the declared
+   // set-of-sets must EQUAL the SIP-inferred set-of-sets, order-free. Both
+   // sides are duplicate-free here (declared by the ADJ-K1-A parse dup-set
+   // reject; inferred by the seen_variants BindingPattern dedup), so
+   // set-of-sets equality IS a bijection. `plan.size()` is the forcing count
+   // (R-1BOUND: one bound query name; demand_forcings still empty here).
+   //
+   // Canonical form: each set sorted into a std::vector<unsigned>; membership
+   // via std::set<std::vector<unsigned>>. A key-set is rendered by column name
+   // for the diagnostics (RES-6: anchored at the decl, no flag/pragma suffix
+   // beyond the fix advice).
+   auto canon = [](std::vector<unsigned> v) {
+     std::sort(v.begin(), v.end());
+     return v;
+   };
+   auto names = [&](const std::vector<unsigned> &s) {
+     std::string out; auto sep = "";
+     for (unsigned pos : s) {
+       out += sep; out += p_demanded_decl.NthParameter(pos).NameAsString();
+       sep = ", ";
+     }
+     return out;
+   };
+
+   std::set<std::vector<unsigned>> declared_sets;
+   for (const std::vector<unsigned> &s : p_demanded_decl.InstanceKeys()) {
+     declared_sets.insert(canon(s));
+   }
+   std::set<std::vector<unsigned>> inferred_sets;
+   for (const PerAdornment &a : plan) {
+     inferred_sets.insert(canon(a.p_bound));
+   }
+
+   // Arm A — a declared @key set with no matching demanded adornment
+   // (over-declaration; witness key_over_adorn_1).
+   for (const std::vector<unsigned> &d : declared_sets) {
+     if (!inferred_sets.count(d)) {
+       log.Append(p_demanded_decl.SpellingRange())
+           << "Declared instance key (" << names(d) << ") on "
+           << p_demanded_decl.KindName() << " '"
+           << p_demanded_decl.NameAsString() << "' has no matching demanded "
+           << "query adornment; fix or remove the @key pragma";
+       return false;
+     }
+   }
+   // Arm B — a demanded adornment with no matching @key set (partial
+   // declaration; witness key_multi_adorn_1, repurposed).
+   for (const std::vector<unsigned> &i : inferred_sets) {
+     if (!declared_sets.count(i)) {
+       log.Append(p_demanded_decl.SpellingRange())
+           << "The demanded query adornment binding (" << names(i) << ") on "
+           << p_demanded_decl.KindName() << " '"
+           << p_demanded_decl.NameAsString() << "' has no matching @key "
+           << "instance key; declare @key(" << names(i) << ") or remove the "
+           << "@key pragma";
+       return false;
+     }
+   }
  }
```

**Exact diagnostic strings** (both keep the "fix or remove the @key pragma"
family advice tail; both anchored at `p_demanded_decl.SpellingRange()`):
- Arm A (declared surplus): `"Declared instance key (" << names(d) << ") on " << KindName << " '" << name << "' has no matching demanded query adornment; fix or remove the @key pragma"`.
- Arm B (inferred surplus): `"The demanded query adornment binding (" << names(i) << ") on " << KindName << " '" << name << "' has no matching @key instance key; declare @key(" << names(i) << ") or remove the @key pragma"`.

**N=1 compatibility argument (design decision 2, verified against the code).**
With one declared set `D` and one adornment `P`:
- *Old* code: `plan.size()==1` ⇒ the `:893` strict reject never fires; then
  rejects iff `set(P) != set(D)`.
- *New* code: `declared_sets={canon(D)}`, `inferred_sets={canon(P)}`. Arm A
  rejects iff `canon(D) ∉ {canon(P)}` i.e. `D≠P` (as sets). Arm B rejects iff
  `canon(P) ∉ {canon(D)}` i.e. `P≠D`. So the new check rejects **iff `D≠P`** and
  accepts **iff `D==P`** — the SAME accept/reject decision as old, for every
  single-`@key` program.

Concrete predictions:
- `key_mismatch_1` (`@key(To)` vs inferred `{From}`): Arm A fires
  (`{To} ∉ {{From}}`) → **still rejects, all 4 modes**. Its diagnostic TEXT
  changes from "Declared instance key of #local 'path' disagrees with the
  demanded binding pattern; ..." to "Declared instance key (To) on #local 'path'
  has no matching demanded query adornment; ...". Reject text is UNPINNED
  (all-4-modes-diagnostic arm byte-compares only rc + cross-mode agreement, not
  message text), so this reword is safe — **predicted explicitly**.
- `key_tc_witness` / `key_neighborhood_witness` (`@key(From)` matching inferred
  `{From}`): both arms pass → **still accept, byte-identically** (no mint change;
  their `.rel`/`.stdout`/`.contract` surfaces are untouched).
- `key_multi_adorn_1` (`@key(A)` vs inferred `{A},{B}`): the `:893` strict reject
  is GONE; Arm A passes (`{A}∈{{A},{B}}`); Arm B fires on `{B}`
  (`{B}∉{{A}}`) → **still rejects, all 4 modes**, class MOVED strict→bijection
  (ADJ-K1-B). Its runall.sh entry is unchanged.

**Wording (ADJ-K1-C).** Both rewritten strings say "instance key" (never "demand
key"). The leftover "demand key" sites (`Demand.cpp:472` no-bound reject,
`Demand.cpp:872` RP-6 realization, `Parser.cpp:958` comma-when-var) are OUT of K1
scope — recorded in the RIDER list.

**Golden-delta claim (DIFF-K1-2):** no golden BYTES change. The only affected
existing cases are diagnostics whose text is unpinned (`key_mismatch_1`,
`key_multi_adorn_1`) — rc + cross-mode agreement preserved. `key_tc_witness`'s
real `.contract.opt.golden` is handled in DIFF-K1-3 (predicted unchanged).

**Obligations (DIFF-K1-2).** DISCHARGED: O-R3.1 answer-identity for N=1 (the
compatibility argument above — same accept/reject, no mint change on accept).
Fence-first order preserved: Step 2b still sits AFTER Loop-1's per-adornment
fences (the allfree fence at `:540-550`, all Step-3 fences) and after RP-6
realization, so a fence always pre-empts the bijection (see DIFF-K1-5's
`key_multi_adorn_allfree_1`).

---

### DIFF-K1-3 (`-contract-out` declared-key line, N sets) — 2026-08-04

**Site (Part-A GAP filled):** `lib/DataFlow/Format.cpp:1728-1751` — the
declared-key reconciliation block. *(Part A's VERIFICATION LEDGER did not cite
this emitter; design-question 3 requires it. Recorded as a Part-A omission, not a
wrong claim.)* Today it iterates `RecognizedSubgraphs()`, dedups by
`decl.Id()`, and emits ONE `declared-key rel=... declared=(...) inferred=(...)`
line per demanded_decl, rendering `decl.InstanceKey()` (the flat vector) for
`declared=` and `rs.key_cols` for `inferred=`.

**Two problems under N sets:** (a) `InstanceKey()` is deleted (accessor shape
change) — the emitter MUST change even to preserve N=1 bytes; (b) the
`decl.Id()` dedup collapses N adornments (N RecognizedSubgraphs sharing one
`demanded_decl`) to a single line, printing only the first forcing's inferred
set — a lost-information bug for K1.

**Design decision 3 (SETTLED):** emit **one line per declared set, in written
(pragma) order**, each paired with the inferred adornment that equals it. On a
compiled program the bijection has already passed (Step 2b), so every declared
set has exactly one matching inferred adornment among `RecognizedSubgraphs()`
`key_cols` — the pairing is total. Written-pragma order is deterministic w.r.t.
source (independent of the forcing-append order), and coincides with forcing
order for N=1.

```diff
  {
    std::unordered_set<uint64_t> seen_decls;
    for (const RecognizedSubgraph &rs : qc.query.RecognizedSubgraphs()) {
      const ParsedDeclaration decl = rs.demanded_decl;
      if (!decl.HasInstanceKey() || !seen_decls.insert(decl.Id()).second) {
        continue;
      }
-     os << "declared-key rel=" << decl.NameAsString() << " declared=(";
-     auto sep = "";
-     for (unsigned pi : decl.InstanceKey()) {
-       os << sep << decl.NthParameter(pi).NameAsString();
-       sep = ", ";
-     }
-     os << ") inferred=(";
-     sep = "";
-     for (unsigned pi : rs.key_cols) {
-       os << sep << decl.NthParameter(pi).NameAsString();
-       sep = ", ";
-     }
-     os << ")\n";
+     // One line per declared @key set (pragma-written order), paired with the
+     // inferred adornment (some RecognizedSubgraph's key_cols) whose SET equals
+     // it. Post-Step-2b bijection guarantees the pairing is total.
+     auto set_eq = [](std::vector<unsigned> a, std::vector<unsigned> b) {
+       std::sort(a.begin(), a.end()); std::sort(b.begin(), b.end());
+       return a == b;
+     };
+     for (const std::vector<unsigned> &dset : decl.InstanceKeys()) {
+       const std::vector<unsigned> *inferred = nullptr;
+       for (const RecognizedSubgraph &rs2 : qc.query.RecognizedSubgraphs()) {
+         if (rs2.demanded_decl.Id() == decl.Id() &&
+             set_eq(rs2.key_cols, dset)) {
+           inferred = &rs2.key_cols;
+           break;
+         }
+       }
+       assert(inferred && "K1: declared @key set has no matching forcing "
+                          "(Step 2b bijection guarantees a match)");
+       os << "declared-key rel=" << decl.NameAsString() << " declared=(";
+       auto sep = "";
+       for (unsigned pi : dset) {
+         os << sep << decl.NthParameter(pi).NameAsString();
+         sep = ", ";
+       }
+       os << ") inferred=(";
+       sep = "";
+       for (unsigned pi : *inferred) {
+         os << sep << decl.NthParameter(pi).NameAsString();
+         sep = ", ";
+       }
+       os << ")\n";
+     }
    }
  }
```

**N=1 byte-identity (verified against the real golden).** For
`key_tc_witness` the block emits (golden line 43, tip):
`declared-key rel=path declared=(From) inferred=(From)`. New code: `InstanceKeys()`
= `[{From}]`; the single declared set `{From}` pairs with the one forcing whose
`key_cols` = `[From]`; `declared=` renders `dset` in written order `(From)`,
`inferred=` renders `key_cols` `(From)`. **Byte-identical line.** The census
lines (golden 44-45) are unaffected (`contracts.size()` and role counts are
independent of the declared-key block). So
`key_tc_witness.contract.opt.golden` is **predicted byte-unchanged** (its 11
symlinked surfaces are equally untouched). *(N=2 has no existing golden — the
new `key_multi_adorn_witness` carries no `.contract` golden, DIFF-K1-5.)*

**Golden-delta claim (DIFF-K1-3):** `key_tc_witness.contract.opt.golden`
byte-unchanged (only real declared-key golden in the tree). No other golden
carries a `declared-key` line.

**Obligations (DIFF-K1-3).** DISCHARGED: N=1 byte-identity (verified line 43).
Determinism: written-pragma outer order + the inner match are both
source-deterministic; the inner `RecognizedSubgraphs()` re-scan is O(N²) in the
per-decl set count (N ≤ arity, trivial).

---

### DIFF-K1-4 (ControlFlow — ADJ-K1-D intent-communicating assert) — 2026-08-04

**Site:** `lib/ControlFlow/Build/Build.cpp:1533-1538` — the RP-9 fallback arm.
*(Part-A correction: Part A cited the `subgraphs[0]` read as "1532-1538"; the
executable `subgraphs[0].demanded_decl.HasInstanceKey()` is at `:1535`,
`effective_demand_instance` declared at `:1532`.)* The `subgraphs[0]`-only read
is correct ONLY while every `RecognizedSubgraph` shares one `demanded_decl`
(R-1BOUND + one demanded relation `p`). ADJ-K1-D upgrades the load-bearing
comment to a positive, intent-communicating assert:

```diff
  bool effective_demand_instance = demand_instance;
  if (!demand_instance && any_forcing && all_forcings_admissible) {
    const auto &subgraphs = query.RecognizedSubgraphs();
    if (!subgraphs.empty() && subgraphs[0].demanded_decl.HasInstanceKey()) {
+     // INVARIANT (R-1BOUND + one demanded relation p): EVERY RecognizedSubgraph
+     // shares the ONE demanded_decl, so subgraphs[0]'s pragma bit correctly
+     // drives every forcing. State it positively rather than trust it silently
+     // — if a future slice keys distinct decls per forcing, [0] would ignore a
+     // differently-pragma'd decl at index >= 1 and this fires first.
+     for (const RecognizedSubgraph &rs : subgraphs) {
+       assert(rs.demanded_decl.Id() == subgraphs[0].demanded_decl.Id() &&
+              "K1: all forcings must share one demanded_decl (R-1BOUND)");
+     }
      effective_demand_instance = true;
    }
  }
```

**Assert form (settled):** a debug `assert(... && "message")` (the idiom the
surrounding pre-pass already uses), not a named fprintf+abort validator — the
invariant is structural, cheap (N ≤ arity), and localized, and this is not a
delta-graph V-* validator. Panel may promote to fprintf+abort if NDEBUG coverage
is wanted (OPEN-Q4).

**Golden-delta claim (DIFF-K1-4):** none — an assert adds no emission. The guard
condition (`subgraphs[0].demanded_decl.HasInstanceKey()`) is unchanged, so
`effective_demand_instance` selection is bit-identical for every case (the
`key_multi_adorn_witness` nested selection still fires; `key_tc_witness`'s
recursive flat fallback still fires).

**Obligations (DIFF-K1-4).** DISCHARGED: the `[0]`-only read is now an asserted
invariant, not a silent reliance (design-question 4 / A5-d). K6-i cross-decl
consistency stays OUT of scope (ADJ-K1-E) — the assert would trip loudly if that
ever changes without the read being generalized.

---

### DIFF-K1-5 (witnesses + runall.sh + rejects) — 2026-08-04

**Design decision 5 (witness completeness under the evil-monkey rule).** The
minimal-but-complete witness set, each with a one-line necessity argument:

1. **`key_multi_adorn_witness` (NEW golden case — the flagless multi-store
   flagship).** The `demand_multi_adorn_witness` bf/fb graph with `@key(A)
   @key(B)` and NO `.drflags`. *Necessity:* the only positive proof that two
   `@key` pragmas SELECT N=2 disjoint keyed stores flaglessly (RP-9 fallback)
   AND answer identically to the flat/mono twin. Inventory (follows the
   `key_neighborhood_witness` precedent, NOT the `key_tc_witness` 11-symlink
   precedent — justified below):
   - `.dr`:
     ```
     #message edge_2(u64 A, u64 B).
     #local rel(u64 A, u64 B) @key(A) @key(B).
     rel(A, B) : edge_2(A, B).
     #query q(bound u64 A, free u64 B).
     #query q(free u64 A, bound u64 B).
     q(A, B) : rel(A, B).
     ```
   - `.main.cpp`: byte-copy of `demand_multi_adorn_witness.main.cpp` (same
     generated ABI `q_bf`/`q_fb`, same probes/asserts).
   - `.irgold` = `rel opt` (one line — the `key_neighborhood_witness` shape).
   - `goldens/key_multi_adorn_witness.stdout` → **symlink** to
     `demand_multi_adorn_witness.stdout` (answer identity: flagless-nested ==
     the `-demand` twin's answer).
   - `goldens/key_multi_adorn_witness.rel.opt.golden` — **own REAL** golden,
     pinning `kSubgraphInstantiate=2` (+ `kInstanceSeal=2`) in the census line
     (two stores, one pub).
   - NO `.drflags` (flagless), NO `.eqgate`, NO `.batches`.

   **Region-golden decision (ADJ-K1-B — SETTLED to the precedent = none).**
   `demand_multi_adorn_witness` carries 4 `.region.<mode>` goldens, but those
   are from its FLAT `-demand` compile (its `.irgold` is `region {opt,nodf,nocf,
   none}`, no `-demand-instance`). `key_multi_adorn_witness`'s pragma compile
   lowers NESTED (RP-9 fallback on the non-recursive bf/fb shape), so its region
   output DIVERGES from the twin's flat region goldens — symlinking them (the
   `key_tc_witness` trick) would be INVALID here. `key_tc_witness` CAN symlink
   region goldens only because its demanded relation is recursive TC, where BOTH
   the pragma and `-demand` compiles fall back FLAT (identical). So
   `key_multi_adorn_witness` follows `key_neighborhood_witness` (the other nested
   flagless witness): the `.rel.opt.golden` is the load-bearing pin of the nested
   shape, `.stdout` symlink proves the answer, and region goldens are omitted (no
   correct symlink target exists, and an own set of 4 would be pure maintenance
   with no new coverage over the rel census). **Verified reason, not a default.**

   runall.sh: no case-arm edit needed — it falls through to the default
   `run_vs_golden`/`.irgold` handling (a normal golden case).

2. **`key_multi_adorn_1` (REPURPOSED diagnostic — class moves).** The `.dr` is
   UNCHANGED (`@key(A)` on the bf/fb graph); ADD a header comment recording the
   new class. *Necessity:* the inferred-surplus (partial-declaration) arm B of
   the bijection. Stays in the `runall.sh:498` all-4-modes-diagnostic list
   (unchanged — ADJ-K1-B). New header comment (the `.dr` currently has none):
   ```
   ; RP-10 partial declaration: `@key(A)` declares only {A} while `q` is
   ; demanded under bf AND fb (inferred {A},{B}); the demanded adornment {B}
   ; has no matching @key -> Step 2b bijection Arm B reject (all 4 modes).
   ; (Pre-K1 this rejected at the STRICT single-forcing scope; K1 moves the
   ; class to the RP-10 bijection but keeps it an all-4-modes diagnostic.)
   ```

3. **`key_over_adorn_1` (NEW diagnostic — the declared-surplus arm).**
   *Necessity:* arm A of the bijection (a declared `@key` set with NO inferred
   match) — the mirror of `key_multi_adorn_1`; without it the declared-surplus
   reject is untested (evil-monkey both-arms rule). `.dr`:
   ```
   ; RP-10 over-declaration: two @key sets {A},{B} but `q` is demanded under a
   ; SINGLE adornment bf (inferred {A}); declared {B} has no matching demanded
   ; adornment -> Step 2b bijection Arm A reject (all 4 modes). FLAGLESS.
   #message edge_2(u64 A, u64 B).
   #local rel(u64 A, u64 B) @key(A) @key(B).
   rel(A, B) : edge_2(A, B).
   #query q(bound u64 A, free u64 B).
   q(A, B) : rel(A, B).
   ```
   FLAGLESS (RP-6 activation), no `.drflags`. runall.sh: ADD `key_over_adorn_1`
   to the `:498` all-4-modes-diagnostic `|`-list.

4. **`key_multi_adorn_allfree_1` (NEW diagnostic — the allfree × pragma fence,
   closes A5-c).** *Necessity:* proves the allfree-sibling fence
   (`Demand.cpp:540-550`, in Loop-1 Step-1) still WINS under pragma-activation
   and is NOT swallowed by the new bijection — the fence is UPSTREAM of Step 2b
   (Part A §A3 ordering: Loop 1 → decl resolution → RP-6 → Step 2b), so a
   bound+allfree mix rejects at the fence (via the `reject()` lambda, now with
   the "; fix or remove the @key pragma" advice fork under `pragma_activated`)
   before the bijection is ever reached. This is a genuinely NEW code path
   (allfree fence × `pragma_activated` advice fork) and directly answers the A5-c
   open sub-question. `.dr`:
   ```
   ; A5-c: the allfree-sibling fence is UPSTREAM of the RP-10 bijection (Loop 1
   ; Step 1 vs Step 2b), so it fires FIRST even with an @key pragma. `q` under
   ; bf + ff: the ff sibling would read the demand-guarded pub -> reject with
   ; the @key advice fork. Bijection never reached. FLAGLESS, all 4 modes.
   #message edge_2(u64 A, u64 B).
   #local rel(u64 A, u64 B) @key(A).
   rel(A, B) : edge_2(A, B).
   #query q(bound u64 A, free u64 B).
   #query q(free u64 A, free u64 B).
   q(A, B) : rel(A, B).
   ```
   FLAGLESS, no `.drflags`. runall.sh: ADD `key_multi_adorn_allfree_1` to the
   `:498` list. *(This is the single marginal witness — it re-proves an unchanged
   upstream fence under a new activation path; retained because it is the exact
   A5-c question and the advice-fork × fence combination is otherwise untested.)*

5. **`reject_key_double_1.dr` (RESHAPED — the ADJ-K1-A dup-set pin).** Today it
   pins the second-`@key` PARSE reject that K1 DELETES:
   `#local foo(u64 A, u64 B) @key(A) @key(B).` (grep-confirmed the sole
   rejects/-corpus case with two `@key` pragmas). After K1 that file would PARSE
   (two distinct sets {A},{B}) and reject later at the no-bound-query arm — a
   changed class that no longer pins what the header claims. Reshape it to pin
   the ADJ-K1-A dup-set reject instead:
   ```
   ; RP-10 / ADJ-K1-A: two @key pragmas declaring the SAME column set are a
   ; parse-time reject (each instance key must be a distinct set, one per
   ; adornment). SET-based, order-free. Rejects during PARSE, before demand.
   #message b_1(u64 X, u64 Y).
   #local foo(u64 A, u64 B) @key(A) @key(A).
   foo(A, B) : b_1(A, B).
   ```
   Header class = **duplicate instance key set**. Still a should-FAIL case
   exiting 1 CLEANLY in both mode extremes (parse reject is mode-independent). No
   query needed (parse rejects first). rejects/ count stays 46 (reshape, not
   add).

**Cases NOT touched (compatibility, explicit):** `key_mismatch_1` (single-adorn
bijection reject preserved), `key_undemanded_1` (RP-6 realization, orthogonal),
`key_tc_witness` / `key_neighborhood_witness` (single-adorn accept, byte-
identical), `key_wildcard_1`/`key_anon_1`/`key_dup_1`/`key_unknown_1`/
`key_fenced_1` (parse/fence rejects on single sets, unaffected),
`demand_multi_adorn_witness` / `demand_multi_adorn_allfree_1` /
`demand_multi_adorn_1` (the `-demand`-flag family, no pragma) — all unchanged.

**runall.sh delta (exact):** the `:498` `|`-joined all-4-modes-diagnostic arm
gains `key_over_adorn_1` and `key_multi_adorn_allfree_1`; `key_multi_adorn_1`
stays (comment-only `.dr` change). No `expect_diagnostic`/`run_vs_golden`
structure changes; `key_multi_adorn_witness` needs no arm (default golden path).

**Case/reject counts after K1:** `cases/*.dr` 200 → 203 (+`key_multi_adorn_witness`,
+`key_over_adorn_1`, +`key_multi_adorn_allfree_1`); `rejects/*.dr` stays 46
(`reject_key_double_1` reshaped in place).

---

### Whole-K1 golden-delta table (the predict-then-verify seed)

Every file the implementation touches, with the byte-identity claim for the
existing 246-case suite:

| file | change | existing-golden effect |
| --- | --- | --- |
| `lib/Parse/Parse.h` | field → `vector<vector<unsigned>>` + comment (F-2) | none (compile-time) |
| `include/drlojekyll/Parse/Parse.h` | `InstanceKey`→`InstanceKeys` decl + comment (F-2) | none (compile-time) |
| `lib/Parse/Parse.cpp` | accessor bodies | none |
| `lib/Parse/Parser.cpp` | delete `:790-798`; per-set accumulation; ADJ-K1-A dup-set reject | none for accepted programs; a NEW parse reject only for dup-set input |
| `lib/Parse/Format.cpp` | N-set loop; `demand_sep`→`key_sep` (F-3) | none (N=1 byte-identical) |
| `lib/DataFlow/Demand.cpp` | Step 2b bijection (DIFF-K1-2) | none (N=1 same accept/reject; texts unpinned) |
| `lib/DataFlow/Format.cpp` | contract declared-key N-set (DIFF-K1-3) | `key_tc_witness.contract.opt.golden` byte-UNCHANGED (verified) |
| `lib/ControlFlow/Build/Build.cpp` | ADJ-K1-D assert | none |
| `tests/OptDiff/cases/key_multi_adorn_witness.*` | NEW (`.dr`/`.main.cpp`/`.irgold`) | new case |
| `tests/OptDiff/goldens/key_multi_adorn_witness.{stdout,rel.opt.golden}` | NEW (symlink + real) | new goldens |
| `tests/OptDiff/cases/key_multi_adorn_1.dr` | header comment only | rc unchanged (class moves, stays diagnostic) |
| `tests/OptDiff/cases/key_over_adorn_1.*` | NEW diagnostic | new case |
| `tests/OptDiff/cases/key_multi_adorn_allfree_1.*` | NEW diagnostic | new case |
| `tests/OptDiff/rejects/reject_key_double_1.dr` | reshape `@key(A) @key(B)`→`@key(A) @key(A)` | reject class changes (parse-2nd → parse-dup-set); still exit-1 clean |
| `tests/OptDiff/runall.sh` | +2 names in the `:498` diagnostic list | none |

**Existing goldens predicted to change: NONE** (pragma-free by the containment
gate; single-`@key` by the N=1 compatibility argument;
`key_tc_witness.contract.opt.golden` verified byte-identical against line 43).
**Cases changing CLASS:** exactly `key_multi_adorn_1` (strict→bijection, stays
diagnostic) and `reject_key_double_1` (parse-2nd→parse-dup-set, stays a reject).
Any hunk that could not argue byte-identity would say so here — none apply; all
runtime-observable behavior on the untouched corpus is invariant.

---

### ADJ record (the six orchestrator adjudications, as implemented)

- **ADJ-K1-A** — duplicate declared key-sets reject at PARSE TIME, at the state-22
  `)` close, SET-based (order-free) against every stored set. String drafted in
  DIFF-K1-1. Pinned by the reshaped `reject_key_double_1`.
- **ADJ-K1-B** — `key_multi_adorn_1` stays an all-4-modes diagnostic; class moves
  to the RP-10 bijection Arm B (partial declaration); header comment added
  (DIFF-K1-5). Golden path = the NEW flagless `key_multi_adorn_witness`
  (stdout symlink + own `.rel.opt.golden` pinning `kSubgraphInstantiate=2`).
  **Region goldens: NONE** — settled to the `key_neighborhood_witness` precedent
  with a verified reason (the pragma compile lowers NESTED and so DIVERGES from
  the twin's FLAT region goldens; no correct symlink target exists — unlike
  `key_tc_witness`, whose recursive shape falls back flat on both compiles).
- **ADJ-K1-C** — F-1 wording drift OUT of scope except the two Step 2b strings K1
  rewrites (both say "instance key"). Leftover sites recorded in the RIDER list.
  F-2 (two stale comments) fixed by the DIFF-K1-1 storage/accessor rewrite; F-3
  (`demand_sep`→`key_sep`) fixed by the DIFF-K1-1 formatter rewrite.
- **ADJ-K1-D** — the `subgraphs[0]`-only `demanded_decl` read at `Build.cpp:1535`
  gains an intent-communicating `assert` (positive statement: all forcings share
  one `demanded_decl`), DIFF-K1-4.
- **ADJ-K1-E** — cross-REDECLARATION `@key` consistency (K6-i) stays OUT of K1;
  recorded obligation, unchanged. The ADJ-K1-D assert would trip if a future
  slice broke the shared-decl invariant without generalizing the read.

### RIDER list (recorded, deliberately NOT done in K1)

- **F-1 leftover "demand key" strings** (ADJ-K1-C): `Demand.cpp:472` (no-bound
  reject), `Demand.cpp:872` (RP-6 realization reject), `Parser.cpp:958`
  (comma-when-var). Left as a separate wording-cleanup slice; K1 touches only the
  Step 2b strings.
- **K6-i cross-redeclaration `@key` consistency** (ADJ-K1-E): unchecked; recorded.
- **N-pragma + no-bound-query interaction:** after K1, `@key(A) @key(B)` on an
  UNQUERIED relation parses then rejects at the no-bound-query arm
  (`Demand.cpp:471-484`) — covered transitively by the existing single-`@key`
  `reject_key_no_bound_query_1`; no dedicated N-pragma no-query witness added.
- **The `[0]`-only read K1-adjacent hazard** (Part A §A3): still relies on
  R-1BOUND + one demanded relation; the ADJ-K1-D assert makes the reliance loud
  rather than silent, but does not lift R-1BOUND.

### OPEN QUESTIONS FOR PANEL (not settleable with code evidence alone)

1. **`key_multi_adorn_witness.main.cpp` — copy vs share.** I specify a byte-copy
   of `demand_multi_adorn_witness.main.cpp` (the ABI `q_bf`/`q_fb` + probes are
   identical). The corpus has no driver-sharing idiom; is a copy acceptable, or
   should the harness gain a shared-driver mechanism (out of K1 scope, but the
   copy is the only duplicated artifact K1 introduces)?
2. **`key_multi_adorn_allfree_1` necessity.** It re-proves an UNCHANGED upstream
   fence under a new activation path (allfree × `pragma_activated` advice fork).
   Kept under the evil-monkey rule + it is the literal A5-c question. Panel may
   judge it redundant with `demand_multi_adorn_allfree_1` (the `-demand` twin) +
   the flagless-advice coverage from other `key_*` rejects, and drop it.
3. **`key_over_adorn_1` — new case vs fold into `key_multi_adorn_1`.** The
   declared-surplus (Arm A) and inferred-surplus (Arm B) arms are distinct code
   paths; I add a dedicated `key_over_adorn_1` for Arm A. Panel may prefer a
   single case exercising one arm and trust symmetry — but both-arms coverage is
   the evil-monkey default.
4. **ADJ-K1-D assert strength.** Debug `assert` (matching the surrounding
   pre-pass) vs an always-on fprintf+abort validator (NDEBUG coverage, matching
   the V-* family). I chose `assert`; the invariant is structural + cheap, so
   fprintf+abort is defensible if the panel wants NDEBUG protection.
5. **Contract declared-key output order (N ≥ 2).** I chose written-pragma order
   (source-deterministic, N=1-identical). Forcing-append order is the alternative
   (matches today's iteration). No existing N≥2 golden disambiguates; the panel
   picks the canonical order for the first multi-set contract golden (none is
   authored in K1 — `key_multi_adorn_witness` has no `.batches`/`.contract`).
6. **Whether K1 should add an N≥2 `.contract` golden.** K1 pins the multi-store
   shape via `.rel.opt` (`kSubgraphInstantiate=2`) only. A `.contract` golden
   over `key_multi_adorn_witness` would byte-fix the two-line declared-key output
   (design decision 3) directly — but it needs a `.batches` sidecar the witness
   otherwise doesn't carry. Deferred as a panel call.

---

## PANEL RECORD — the K1 adversarial panel (2026-08-04, 4 lenses, 12 findings)

Four lenses (correctness, testability, necessity, termination) ran over Parts A
and B; every finding was refute-verified against the code. Outcome: **5
confirmed** (one after the orchestrator confirmed a finding whose refuter agent
died), **4 downgraded to notes**, **3 refuted**. The table below records finding
→ verdict → resolution. Resolutions ADJ-K1-F..J are authored in the
"ADJUDICATED RESOLUTIONS" section; they SUPERSEDE the conflicting Part-B hunk
text per the "Part B AMENDMENTS" section (hunks are not rewritten in place).

| # | lens / verdict | finding | resolution |
| --- | --- | --- | --- |
| 1 | correctness — **CONFIRMED should-fix** | The reshaped `reject_key_double_1` (no `#query`) does NOT guard ADJ-K1-A: it rejects via the demand no-bound arm whether or not the parse dup-set check exists, and Step 2b PROVABLY cannot catch duplicate declared sets (its `std::set` dedups `{A},{A}` → `{{A}}`, which then matches a single inferred `{A}`). Dropping ADJ-K1-A would silently ACCEPT `@key(A) @key(A)` + a matching bound query. | **ADJ-K1-F**: the reshaped case gains a bound query matching `{A}` so removing the parse check flips it reject→accept(rc=0) = LOST CHECK. |
| 2 | correctness — **CONFIRMED should-fix** (orchestrator-verdicted; refuter agent died) | `runall.sh --bless` uses plain `cp`, which writes THROUGH destination symlinks (verified `runall.sh:111-168`, no `-P`/`readlink` guard). Blessing a symlink-twin witness silently overwrites the TWIN's real golden — masking exactly the divergence the witness exists to catch. Pre-existing hazard (`key_tc`/`key_neighborhood` already symlink), WIDENED by K1's `key_multi_adorn_witness.stdout` symlink. | **ADJ-K1-G**: DIFF-K1-5 gains a `runall.sh` bless-loop hunk — a `bless_copy` helper guards EVERY golden write: symlink dest ⇒ never `cp`; byte-compare instead (`cmp -s`) — identical ⇒ `skipped … (symlink, byte-identical)`, divergent ⇒ `BLESS-REFUSED …` + `exit 1`. Mechanizes the never-bless-through-symlink rule suite-wide. |
| 3 | correctness **CONFIRMED note** + testability/necessity **DOWNGRADED notes** | The N≥2 declared-key contract output is unpinned by any golden (the sole declared-key golden `key_tc_witness.contract.opt.golden` is N=1). Refuter mechanism correction: contract goldens ride the `.irgold` sidecar (`contract opt`), NOT `.batches`. | **ADJ-K1-H**: `key_multi_adorn_witness.irgold` becomes `rel opt` + `contract opt`; the witness gains its OWN `.contract.opt.golden` pinning BOTH declared-key lines — which also LOCKS the OQ5 written-pragma order by bytes. (No `.batches` needed — supersedes OQ6's "deferred".) |
| 4 | correctness+necessity — **DOWNGRADED notes** (DIFF-K1-3 emitter shape) | The nullable match-pointer + debug `assert` + unconditional deref (an NDEBUG deref of null if the bijection invariant ever weakens), and the O(N²) rescan. | **ADJ-K1-I**: the emitter's no-match arm becomes an always-on `fprintf`+`abort` belt (unreachable post-Step-2b; message names the relation + declared set), killing the nullable deref. Written-pragma order KEPT (N=1 byte-compat proven). O(N²) ACCEPTED with a one-line note (N ≤ arity, tiny; complexity is inherent to the pairing requirement). |
| 5 | correctness **CONFIRMED note** + testability **CONFIRMED note** | `key_multi_adorn_1.dr`'s existing header still calls `@key(A) @key(B)` the RESERVED-unsupported lift path — contradictory once K1 lands. And the two bijection arms are not discriminated by any pinned text. | **ADJ-K1-J**: DIFF-K1-5 rewrites `key_multi_adorn_1`'s header to name its NEW class (RP-10 bijection, inferred-surplus **arm B** — the partial-declaration negative witness); `key_over_adorn_1`'s header names **arm A** (declared-surplus). Text stays UNPINNED per house policy (header comments are the class authority). |
| 6 | testability — **DOWNGRADED note** | The multi-adornment family has NO definitional referee (`.batches` absent on the flat twin ⇒ oracle/RefInterp/behavioral never run; the driver probes only non-empty groups). | Recorded as rider **R-K1-BATCHES** (follow-on: add the `.batches` family to `demand_multi_adorn_witness` and symlink the pragma twin's oracle/monotone goldens). NOT K1-blocking — K1 is a surface slice; answer identity is refereed by the `.stdout` symlink + driver asserts across all 4 modes. |
| 7 | termination — **note, verdicted** | `kSubgraphInstantiate=2` is pinned only in the opt-mode `.rel` golden. | ACCEPTED — house `.irgold` policy; behavioral coverage spans all modes via the `.stdout`. Recorded, no action. |
| 8 | (finding) "reshape drops N-pragma→parse-OK→no-bound coverage" | The reshape of `reject_key_double_1` removes the N-pragma-parses-then-no-bound path. | RESOLVED by ADJ-K1-F itself (the dup case now carries a bound query); the no-bound arm keeps its OWN dedicated case **`reject_key_no_bound_query_1`** — VERIFIED present at `tests/OptDiff/rejects/reject_key_no_bound_query_1.dr` (`#local foo(u64 A) @key(A).` + `#query qf(free u64 A) : foo(A).`, the RP-6 no-seed reject). |

**Three REFUTED findings** (refuted-with-evidence; see panel transcript):
- **R1 — refuted.** "The N=1 formatter loop changes bytes vs the old single loop."
  Refuted: the N=1 `InstanceKeys()` loop emits exactly one ` @key(...)` group,
  byte-identical to the old single `InstanceKey()` loop (DIFF-K1-1 golden-delta
  claim holds). *refuted — see panel transcript.*
- **R2 — refuted.** "Deleting the single-vector `InstanceKey()` breaks a live
  caller." Refuted: the caller census is grep-complete (3 sites, all rewritten by
  K1); no fourth reader exists. *refuted — see panel transcript.*
- **R3 — refuted.** "The `subgraphs[0]`-only `demanded_decl` read at
  `Build.cpp:1535` mis-selects under N adornments." Refuted: R-1BOUND + one
  demanded relation `p` make all `RecognizedSubgraph`s share one `demanded_decl`;
  the ADJ-K1-D assert now states this positively. *refuted — see panel
  transcript.*

---

## ADJUDICATED RESOLUTIONS (2026-08-04, post-panel)

**ADJ-K1-F — `reject_key_double_1` gains a matching bound query.** The reshaped
case (DIFF-K1-5 item 5) is `@key(A) @key(A)` on `foo(u64 A, u64 B)`. Per finding
1 it must ALSO carry a bound query matching `{A}` so the case actually guards the
ADJ-K1-A parse dup-set check: WITH the check ⇒ parse reject; WITHOUT it ⇒ the two
identical sets collapse in `std::set`, match the single inferred `{A}`, the
bijection passes, and the program ACCEPTS (rc=0) = a LOST CHECK. The added line
is `#query q(bound u64 A, free u64 B) : foo(A, B).` (see Part C3 for the full
reshaped file). Still a should-FAIL rejects/-corpus case exiting 1 cleanly in
both mode extremes (the parse reject pre-empts demand; the bound query only
matters to the counterfactual where the parse check is removed).

**ADJ-K1-G — the `runall.sh` bless-loop symlink guard.** DIFF-K1-5 gains a
`bless_copy` helper wrapping EVERY golden `cp` in the `--bless` loop (six write
sites: `.stdout`, `.oracle.stdout`, `.monotone.stdout`, `.behavioral.stdout`, the
`.irgold` `$surface.$mode.golden` loop, and the flat `kvindex_1.stdout`). Rule:
if the destination is a symlink (`[ -L ]`), never `cp` — byte-compare (`cmp -s`):
identical ⇒ `skipped $label (symlink, byte-identical)` (not counted), divergent ⇒
`BLESS-REFUSED $label: golden is a symlink and bytes diverge` + `exit 1`. Full
hunk in Part C4.

**ADJ-K1-H — `key_multi_adorn_witness` gains a `.contract.opt.golden`.** Its
`.irgold` becomes two lines — `rel opt` + `contract opt` — and it ships its OWN
real `.contract.opt.golden` (Part C2, 44 lines) pinning both declared-key lines.
This settles OQ5 (written-pragma order byte-locked) and OQ6 (yes, add the
contract golden — no `.batches` needed, since contract goldens ride `.irgold`,
not `.batches`). SUPERSEDES DIFF-K1-5 item 1's `.irgold = rel opt` and the
open-question deferrals.

**ADJ-K1-I — the DIFF-K1-3 no-match arm is an always-on belt.** The emitter's
"declared set has no matching forcing" arm (currently a debug `assert(inferred
&& …)` with an unconditional `*inferred` deref) becomes an always-on
`fprintf`+`abort` belt naming the relation + declared column set, killing the
nullable-deref-under-NDEBUG hazard. The O(N²) inner rescan is accepted with a
one-line note (N ≤ arity; the pairing requirement makes it inherent). Written-
pragma order is kept (N=1 byte-compat proven in DIFF-K1-3). SUPERSEDES the
DIFF-K1-3 hunk's `assert` line (see Part B AMENDMENTS).

**ADJ-K1-J — header comments name the bijection arms.** `key_multi_adorn_1`'s
header names its NEW class (RP-10 bijection, **inferred-surplus arm B** — partial
declaration); `key_over_adorn_1`'s header names **declared-surplus arm A**. Text
stays unpinned (house policy: the header comment is the class authority, not a
byte-goldened line). The existing DIFF-K1-5 header drafts already carry the
Arm A / Arm B labels; ADJ-K1-J confirms them and additionally requires
`key_multi_adorn_1.dr`'s pre-K1 "RESERVED-unsupported lift path" prose be
replaced (finding 5).

### OQ dispositions (from Part B "OPEN QUESTIONS FOR PANEL")

- **OQ1 — byte-copy the driver: UNANIMOUS.** `key_multi_adorn_witness.main.cpp`
  is a byte-copy of `demand_multi_adorn_witness.main.cpp` (same generated ABI
  `q_bf`/`q_fb`, same probes/asserts). The corpus has no shared-driver idiom; a
  copy is the accepted precedent (a harness shared-driver mechanism is out of K1
  scope).
- **OQ2 — KEEP `key_multi_adorn_allfree_1`: 3-of-4 (evil-monkey).** The allfree ×
  `pragma_activated` advice-fork path is otherwise untested and is the literal
  A5-c question; retained.
- **OQ3 — KEEP `key_over_adorn_1`: UNANIMOUS.** Arm A (declared surplus) and Arm B
  (inferred surplus) are distinct code paths; both-arms coverage is the
  evil-monkey default.
- **OQ4 — the `Build.cpp` guard stays a debug `assert`** (house style in that
  nested pre-pass); the EMITTER-side assert is SUPERSEDED by ADJ-K1-I's always-on
  belt. (The two are different sites: DIFF-K1-4's structural invariant is a
  localized debug assert; DIFF-K1-3's emitter belt guards a
  runtime-reachable-if-invariant-weakens deref.)
- **OQ5 — written-pragma order, now BYTE-LOCKED via ADJ-K1-H's contract golden.**
- **OQ6 — YES, add the N≥2 `.contract` golden, via `.irgold` `contract opt`**
  (mechanism corrected: no `.batches` sidecar required).

### RIDER additions

- **R-K1-BATCHES** (finding 6): add a `.batches` definitional-referee family to
  `demand_multi_adorn_witness` and symlink the pragma twin's oracle/monotone
  goldens. Follow-on; NOT K1-blocking.

---

## Part B AMENDMENTS (2026-08-04, post-panel) — these SUPERSEDE the conflicting hunk text

Per house style, the Part-B hunks above are NOT rewritten in place; the following
dated amendments override them where they conflict with the panel resolutions and
the Part-C byte predictions.

1. **DIFF-K1-3 emitter no-match arm (ADJ-K1-I).** The `assert(inferred && "K1:
   declared @key set has no matching forcing …")` line, and the subsequent
   unconditional `*inferred` deref, are SUPERSEDED by an always-on belt:
   ```cpp
       if (!inferred) {
         fprintf(stderr, "V-DECLARED-KEY-PAIR: declared @key set on '%s' "
                 "has no matching demanded forcing (Step 2b bijection "
                 "guarantees a match)\n", decl.NameAsString().c_str());
         abort();
       }
   ```
   placed where the `assert` was (before the `declared=(`/`inferred=(` render).
   The belt is unreachable on any compiled program (Step 2b's bijection already
   passed) and survives NDEBUG. The `*inferred` deref that follows is now
   null-safe. O(N²) inner rescan accepted (N ≤ arity).

2. **DIFF-K1-5 item 1 `.irgold` (ADJ-K1-H).** `key_multi_adorn_witness.irgold` is
   TWO lines — `rel opt` and `contract opt` — not the single `rel opt` line the
   hunk specifies. The witness additionally ships an OWN real
   `goldens/key_multi_adorn_witness.contract.opt.golden` (Part C2). The
   whole-K1 golden-delta table row for the witness's `.irgold`/goldens is updated
   accordingly (Part C6).

3. **DIFF-K1-5 item 5 reshaped `reject_key_double_1` (ADJ-K1-F).** The file gains
   `#query q(bound u64 A, free u64 B) : foo(A, B).`; the hunk's "No query needed
   (parse rejects first)" note is SUPERSEDED — the query is required so the case
   guards the parse dup-set check counterfactually (finding 1). Full text in
   Part C3.

4. **DIFF-K1-5 item 2/3 headers (ADJ-K1-J).** `key_multi_adorn_1.dr`'s pre-K1
   header prose ("the RESERVED multi-adornment lift path") must be replaced by the
   RP-10 arm-B (partial-declaration) class text; `key_over_adorn_1`'s header names
   arm A. (The DIFF-K1-5 header drafts already match; this amendment records that
   the OLD `key_multi_adorn_1.dr` header is a hard delete, not an addition.)

5. **OPEN QUESTIONS 5 & 6 are SETTLED** by ADJ-K1-H (written-pragma order
   byte-locked; contract golden added via `.irgold contract opt`, no `.batches`).
   OQ1/OQ2/OQ3/OQ4 dispositions recorded above.

6. **DIFF-K1-5 gains the ADJ-K1-G bless-loop hunk** (Part C4) — a new addition,
   not a conflict.

**No other Part-B claim conflicts with the Part-C byte computation.** In
particular the Part-B case-count arithmetic (200 → 203; rejects stay 46) is
CONFIRMED by Part C6 — the panel's framing "200 → 204" is a mis-count; the correct
delta is +3 cases (see C6).

---

## Part C — desired output states (byte-exact, 2026-08-04)

Predict-then-verify: the bytes below are computed from the ACTUAL emitters'
formatting rules (`lib/Rel/Format.cpp`'s `.rel` dump, `lib/DataFlow/Format.cpp`'s
contract dump at `:1728-1751`), not guessed. Any implementation must reproduce
them exactly.

### C1 — `key_multi_adorn_witness.rel.opt.golden` (predicted whole-file)

**Whole-file identity claim.** The predicted golden is BYTE-IDENTICAL to the tip
collection at `scratchpad/d-dumps/kmaw-pred.rel` (76 lines), produced this session
by `drlojekyll cases/demand_multi_adorn_witness.dr -demand -demand-instance
-rel-out`. The K1 flagless-pragma compile of `key_multi_adorn_witness.dr`
(`@key(A) @key(B)`, no `.drflags`) lowers NESTED via the RP-9 fallback and is
byte-for-byte the same `.rel` as the `-demand -demand-instance` compile of the
pragma-free twin — the **`key_neighborhood_witness` equivalence pattern**
(flagless-pragma compile == flag compile). The pragma changes only the *activation
path* (RP-6 containment gate + RP-9 fallback), never the DR-flow graph, so every
op/dep/census byte coincides.

First 5 lines:
```
rel

instances:
  DRInstance i#0 forcing=q key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[A] row_cols=[B]
  DRInstance i#1 forcing=q key=%table:19 pub=%table:4 input=%table:11 store=I#1 key_cols=[B] row_cols=[A]
```

Last 5 lines (72–76):
```
  op.7 -> op.5 WAW epoch
  op.8 -> op.4 WAW epoch
  op.9 -> op.6 WAW epoch

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=3 kNegateGate=0 kPivotAssemble=0 kIngestFold=3 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=2 kInstanceDeath=0 kInstanceSeal=2 kEagerForward=3 kEagerInsert=0 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=0 kEagerProduct=0 kIngestLoop=0 kJoinEmit=0 kProductEmit=0
```

Load-bearing census tokens: `kSubgraphInstantiate=2` (two disjoint keyed stores,
`i#0` keyed `[A]`/`i#1` keyed `[B]`) sharing ONE pub `%table:4` (`pub=%table:4` on
both `DRInstance` lines and both `kSubgraphInstantiate` ops) + `kInstanceSeal=2` +
`kIngestFold=3` (edge_2, demand__q_bf, demand__q_fb) + `kInstanceDeath=0`
(monotone demand). This is the positive proof that `@key(A) @key(B)` SELECTS N=2
stores flaglessly.

### C2 — `key_multi_adorn_witness.contract.opt.golden` (predicted whole-file, 44 lines)

**Derivation.** The tip flag-compile contract dump of the pragma-free twin is
`scratchpad/d-dumps/kmaw-pred.contract` (42 lines, NO declared-key lines — the
source is pragma-free ⇒ `HasInstanceKey()` false ⇒ the `:1728-1751` block emits
nothing). The pragma'd source has an IDENTICAL dataflow graph (equivalence
pattern), so contract lines 1–40 (through `insert ^insert.18`) and the census
(final 2 lines) are byte-identical to the twin's. The ONLY delta: the
`:1728-1751` declared-key block now fires because `decl.HasInstanceKey()` is true.
Per DIFF-K1-3 + ADJ-K1-I, it emits **one line per declared set in written-pragma
order**, each paired (by set-equality) with the forcing whose `key_cols` equals
it. `InstanceKeys()` = `[{A},{B}]`; `{A}`→`key_cols=[A]`, `{B}`→`key_cols=[B]`.
Rendering rule (verified against emitter `:1736-1749` and golden
`key_tc_witness.contract.opt.golden:43` `declared-key rel=path declared=(From)
inferred=(From)`): `"declared-key rel=" << NameAsString() << " declared=(" <<
names(dset) << ") inferred=(" << names(inferred) << ")\n"`, names joined by
`", "`. The block sits AFTER the last view (`insert`) and BEFORE the census —
exactly where line 43 sits in the key_tc golden.

The two new lines (verbatim):
```
declared-key rel=rel declared=(A) inferred=(A)
declared-key rel=rel declared=(B) inferred=(B)
```

Full predicted 44-line file:
```
contracts

select ^select.0 (A:u64, B:u64)
  role=n/a key=(A,B)
select ^select.1 (c3:u64)
  role=n/a key=(c3)
select ^select.2 (c4:u64)
  role=n/a key=(c4)
tuple ^tuple.3 (A:u64, B:u64)
  role=distinct key=(A,B)
tuple ^tuple.4 (A:u64, B:u64)
  role=member key=(A,B)
tuple ^tuple.5 (c9:u64)
  role=member key=(c9)
tuple ^tuple.6 (A:u64, B:u64)
  role=member key=(A,B)
tuple ^tuple.7 (c12:u64)
  role=member key=(c12)
tuple ^tuple.8 (A:u64, B:u64)
  role=member key=(A,B)
tuple ^tuple.9 (A:u64, B:u64)
  role=member key=(A,B)
tuple ^tuple.10 (A:u64, B:u64)
  role=member key=(A,B)
tuple ^tuple.11 (A:u64, B:u64)
  role=member key=(A,B)
join ^join.12 (A:u64, B:u64)
  role=n/a key=(A,B)
join ^join.13 (A:u64, B:u64)
  role=n/a key=(A,B)
join ^join.14 (B:u64, A:u64)
  role=n/a key=(B,A)
join ^join.15 (B:u64, A:u64)
  role=n/a key=(B,A)
merge ^merge.16 (A:u64, B:u64)
  role=n/a key=(A,B)
merge ^merge.17 (A:u64, B:u64)
  role=n/a key=(A,B)
insert ^insert.18 (A:u64, B:u64)
  role=n/a key=(A,B)
declared-key rel=rel declared=(A) inferred=(A)
declared-key rel=rel declared=(B) inferred=(B)
census: views=19 contracts=19 role{distinct=1 member=8 na=10}
        agg_input_key_ok=0 collapse_error=0
```

(The census `contracts=19`/`role{…}` counts are independent of the declared-key
block — unchanged from the twin's 42-line dump.)

### C3 — new/changed case files (full text)

**`tests/OptDiff/cases/key_multi_adorn_witness.dr`** (NEW, flagless — the
pragma'd variant of `demand_multi_adorn_witness.dr`):
```
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A) @key(B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

**`tests/OptDiff/cases/key_multi_adorn_witness.irgold`** (NEW — ADJ-K1-H, two
lines):
```
rel opt
contract opt
```

**`tests/OptDiff/cases/key_multi_adorn_witness.main.cpp`** (NEW — OQ1 byte-copy):
a verbatim byte-copy of `tests/OptDiff/cases/demand_multi_adorn_witness.main.cpp`
(same generated ABI `q_bf`/`q_fb`, the two `probe_bf`/`probe_fb` lambdas, the
same `emit("bf",…)`/`emit("fb",…)` batch, the cursor-contract comment). No edit —
the pragma'd program exposes the identical public ABI, so the driver is
identical. (The only duplicated artifact K1 introduces; OQ1 accepts the copy.)

**`tests/OptDiff/goldens/key_multi_adorn_witness.stdout`** (NEW — symlink):
`ln -s demand_multi_adorn_witness.stdout
goldens/key_multi_adorn_witness.stdout` (answer identity: flagless-nested ==
the `-demand` twin's answer). This symlink is exactly why ADJ-K1-G's bless-guard
is required.

**`tests/OptDiff/goldens/key_multi_adorn_witness.rel.opt.golden`** (NEW — own
REAL golden): the 76-line file from C1.

**`tests/OptDiff/goldens/key_multi_adorn_witness.contract.opt.golden`** (NEW —
own REAL golden, ADJ-K1-H): the 44-line file from C2.

**`tests/OptDiff/cases/key_multi_adorn_1.dr`** (CHANGED — header comment only;
ADJ-K1-J. The graph is UNCHANGED; the pre-K1 header is HARD-DELETED and replaced):
```
; RP-10 partial declaration (bijection Arm B, inferred-surplus): `@key(A)`
; declares only {A} while `q` is demanded under bf AND fb (inferred {A},{B});
; the demanded adornment {B} has no matching @key -> Step 2b bijection Arm B
; reject (all 4 modes). Pre-K1 this rejected at the STRICT single-forcing
; scope; K1 moves the class to the RP-10 bijection but keeps it an
; all-4-modes diagnostic. FLAGLESS (RP-6 activation).
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, bound u64 B).
q(A, B) : rel(A, B).
```

**`tests/OptDiff/cases/key_over_adorn_1.dr`** (NEW diagnostic — Arm A,
declared-surplus; ADJ-K1-J):
```
; RP-10 over-declaration (bijection Arm A, declared-surplus): two @key sets
; {A},{B} but `q` is demanded under a SINGLE adornment bf (inferred {A});
; declared {B} has no matching demanded adornment -> Step 2b bijection Arm A
; reject (all 4 modes). FLAGLESS (RP-6 activation).
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A) @key(B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
q(A, B) : rel(A, B).
```

**`tests/OptDiff/cases/key_multi_adorn_allfree_1.dr`** (NEW diagnostic — allfree ×
pragma fence, closes A5-c):
```
; A5-c: the allfree-sibling fence is UPSTREAM of the RP-10 bijection (Loop 1
; Step 1 vs Step 2b), so it fires FIRST even with an @key pragma. `q` under
; bf + ff: the ff sibling would read the demand-guarded pub -> reject with the
; @key advice fork. Bijection never reached. FLAGLESS, all 4 modes.
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B) @key(A).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).
#query q(free u64 A, free u64 B).
q(A, B) : rel(A, B).
```

**`tests/OptDiff/rejects/reject_key_double_1.dr`** (RESHAPED — ADJ-K1-A dup-set
pin WITH the ADJ-K1-F bound query. The pre-K1 `@key(A) @key(B)` second-pragma
content is HARD-DELETED):
```
; RP-10 / ADJ-K1-A: two @key pragmas declaring the SAME column set are a
; parse-time reject (each instance key must be a distinct set, one per
; adornment). SET-based, order-free. Rejects during PARSE, before demand.
; The bound query below matches {A}: it exists ONLY so that removing the
; ADJ-K1-A parse check would flip this reject->accept (a LOST CHECK), i.e.
; the query makes the case actually GUARD the dup-set check (finding 1).
#message b_1(u64 X, u64 Y).
#local foo(u64 A, u64 B) @key(A) @key(A).
foo(A, B) : b_1(A, B).
#query q(bound u64 A, free u64 B) : foo(A, B).
```
Header class = **duplicate instance key set**. Still a should-FAIL case exiting 1
cleanly in both mode extremes (the parse reject pre-empts demand). `rejects/`
count stays 46 (reshape in place, not an add).

### C4 — `runall.sh` deltas (unified-diff-style hunks)

**Hunk C4-a — the diagnostic-list line (`runall.sh:498`).** Add
`key_over_adorn_1` and `key_multi_adorn_allfree_1` to the `|`-joined
all-4-modes-diagnostic alternation (`key_multi_adorn_1` STAYS — comment-only `.dr`
change; `demand_multi_adorn_allfree_1` is the distinct `-demand`-flag twin,
already present):
```diff
-    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|negate_never_diff_1|nonascii_1|truncated_decl_1|demand_multi_adorn_1|demand_multi_adorn_allfree_1|demand_cyclic_1|demand_recursive_content_1|product_in_scc_diff_1|demand_agg_body_1|demand_kv_body_1|demand_config_agg_body_1|demand_mutual_content_1|demand_two_queries_1|key_wildcard_1|key_anon_1|key_dup_1|key_unknown_1|key_mismatch_1|key_multi_adorn_1|key_fenced_1|key_undemanded_1)
+    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|negate_never_diff_1|nonascii_1|truncated_decl_1|demand_multi_adorn_1|demand_multi_adorn_allfree_1|demand_cyclic_1|demand_recursive_content_1|product_in_scc_diff_1|demand_agg_body_1|demand_kv_body_1|demand_config_agg_body_1|demand_mutual_content_1|demand_two_queries_1|key_wildcard_1|key_anon_1|key_dup_1|key_unknown_1|key_mismatch_1|key_multi_adorn_1|key_over_adorn_1|key_multi_adorn_allfree_1|key_fenced_1|key_undemanded_1)
```
(`key_multi_adorn_witness` needs NO case-arm — it falls through to the default
`run_vs_golden`/`.irgold` handling as a normal golden case.)

**Hunk C4-b — the ADJ-K1-G bless-loop symlink guard (`runall.sh:110-166`).**
Introduce a `bless_copy` helper immediately after `mkdir -p "$HERE/goldens"` and
route EVERY golden `cp` through it. The helper guards all six write sites; `n` is
incremented only on a real write (`&&`), so skipped symlinks do not inflate the
count. `set -u` is on (not `set -e`), so a `return 1` skip does not abort, while
`exit 1` on divergence does:
```diff
   mkdir -p "$HERE/goldens"
+  # ADJ-K1-G: never bless THROUGH a symlink. A symlink golden is a twin-
+  # equivalence claim (key_tc_witness, key_neighborhood_witness,
+  # key_multi_adorn_witness.stdout, ...); a plain `cp` writes through it and
+  # silently corrupts the TWIN's real golden, masking the very divergence the
+  # witness exists to catch. $1=produced src  $2=golden dest  $3=label.
+  bless_copy() {
+    if [ -L "$2" ]; then
+      if cmp -s "$1" "$2"; then
+        echo "skipped $3 (symlink, byte-identical)"
+      else
+        echo "BLESS-REFUSED $3: golden is a symlink and bytes diverge"
+        exit 1
+      fi
+      return 1
+    fi
+    cp "$1" "$2"
+    echo "blessed $3"
+    return 0
+  }
   n=0
   for d in "$WORKROOT"/*/; do
     name=$(basename "$d")
     echo "$name" | grep -qE "$FILTER" || continue
     src="$d$name.opt/stdout"
     if [ -f "$src" ]; then
-      cp "$src" "$HERE/goldens/$name.stdout"
-      echo "blessed $name"
-      n=$((n + 1))
+      bless_copy "$src" "$HERE/goldens/$name.stdout" "$name" && n=$((n + 1))
     fi
     osrc="$d$name.oracle/stdout"
     if [ -f "$osrc" ]; then
-      cp "$osrc" "$HERE/goldens/$name.oracle.stdout"
-      echo "blessed $name.oracle"
-      n=$((n + 1))
+      bless_copy "$osrc" "$HERE/goldens/$name.oracle.stdout" "$name.oracle" \
+        && n=$((n + 1))
     fi
     msrc="$d$name.monotone/stdout"
     if [ -f "$msrc" ]; then
-      cp "$msrc" "$HERE/goldens/$name.monotone.stdout"
-      echo "blessed $name.monotone"
-      n=$((n + 1))
+      bless_copy "$msrc" "$HERE/goldens/$name.monotone.stdout" "$name.monotone" \
+        && n=$((n + 1))
     fi
     bsrc="$d$name.refinterp/behavioral.opt"
     if [ -f "$bsrc" ]; then
-      cp "$bsrc" "$HERE/goldens/$name.behavioral.stdout"
-      echo "blessed $name.behavioral"
-      n=$((n + 1))
+      bless_copy "$bsrc" "$HERE/goldens/$name.behavioral.stdout" \
+        "$name.behavioral" && n=$((n + 1))
     fi
     ...
       isrc="$d$name.irgold/$surface.$mode.out"
       if [ ! -f "$isrc" ]; then
         echo "FATAL: $name.irgold pins '$surface $mode' but $isrc is missing"
         exit 1
       fi
-      cp "$isrc" "$HERE/goldens/$name.$surface.$mode.golden"
-      echo "blessed $name.$surface.$mode"
-      n=$((n + 1))
+      bless_copy "$isrc" "$HERE/goldens/$name.$surface.$mode.golden" \
+        "$name.$surface.$mode" && n=$((n + 1))
     done < "$HERE/cases/$name.irgold"
     ...
   done
   if [ -f "$WORKROOT/kvindex_1.opt/stdout" ] \
       && echo kvindex_1 | grep -qE "$FILTER"; then
-    cp "$WORKROOT/kvindex_1.opt/stdout" "$HERE/goldens/kvindex_1.stdout"
-    echo "blessed kvindex_1"
-    n=$((n + 1))
+    bless_copy "$WORKROOT/kvindex_1.opt/stdout" "$HERE/goldens/kvindex_1.stdout" \
+      "kvindex_1" && n=$((n + 1))
   fi
```
All six `cp` destinations in the bless loop are now guarded: `.stdout`,
`.oracle.stdout`, `.monotone.stdout`, `.behavioral.stdout`, the `.irgold`
`$surface.$mode.golden` write, and the flat `kvindex_1.stdout`. With this in
place, blessing `key_multi_adorn_witness` writes its real `.rel.opt`/`.contract.opt`
goldens but SKIPS (byte-verifies) its `.stdout` symlink — the twin's
`demand_multi_adorn_witness.stdout` can never be corrupted through it.

### C5 — determinism note (why every predicted byte is mode/platform-stable)

- **Written-pragma order** (the `declared-key` line order in C2, and the
  formatter round-trip) comes from `instance_key_param_index_sets`, a
  `std::vector<std::vector<unsigned>>` appended in parse order — a source-order
  vector, not a hash container. Iteration order is the source text order, stable
  across platforms and optimization modes.
- **The `.rel` dump (C1)** is already suite-refereed deterministic (id-ordered
  textual dump; the R-final SET oracle proves cross-mode/permutation stability).
  The pragma compile reuses the identical DR-flow graph, so its bytes inherit that
  determinism.
- **The contract emitter (C2)** iterates the `contracts` storage in view-id order
  (unchanged), then the declared-key block iterates `InstanceKeys()` (parse-order
  vector) with an inner set-equality match into `RecognizedSubgraphs()` (a
  forcing-order vector); both are deterministic sequences. `NthParameter(pi)`
  renders the DECLARED column name (source identifier), not an internal id.
- **Cross-mode identity:** all four optimization modes produce the SAME `.rel` and
  `.contract` bytes for these cases (the `.irgold` pins opt-mode; the other modes
  agree by the golden-master cross-mode contract). The `.stdout` symlink answer is
  refereed across all 4 modes by the diffrun/eqgate machinery.

### C6 — final predicted suite shape

**Case count: 200 → 203** (NOT 204 — the panel framing's "204" mis-counts). The
recount:
- **+`key_multi_adorn_witness`** (NEW golden case) — +1
- **+`key_over_adorn_1`** (NEW diagnostic) — +1
- **+`key_multi_adorn_allfree_1`** (NEW diagnostic) — +1
- `key_multi_adorn_1` — REPURPOSED in place (header comment only), NOT a new case
  — +0
- `reject_key_double_1` — RESHAPED in the `rejects/` corpus, NOT a `cases/` add —
  +0
⇒ `cases/*.dr` = **203**. `rejects/*.dr` stays **46** (reshape in place).

**New expected-diagnostic entries** (`runall.sh:498` alternation): `key_over_adorn_1`,
`key_multi_adorn_allfree_1` (both all-4-modes-diagnostic, FLAGLESS). `key_multi_adorn_1`
stays in the list (class moved strict→bijection Arm B, still all-4-modes-diagnostic).

**NO existing golden bytes change**, on two independent arguments:
1. **Containment gate** — the pragma-free corpus never constructs a non-empty
   `instance_key_param_index_sets`, so `HasInstanceKey()` is false ⇒ the
   formatter, the contract declared-key block, and the demand pragma-activation
   are all skipped ⇒ byte-identical output (the `-demand`-flag family included).
2. **N=1 compatibility** — for every single-`@key` case (`key_tc_witness`,
   `key_neighborhood_witness`, `key_mismatch_1`, `key_undemanded_1`, …) the
   list-of-sets storage holds one set; the N=1 formatter loop emits one
   ` @key(...)` group (byte-identical to the old single loop); the Step-2b
   bijection accepts iff `D==P` (same accept/reject as the old single-set check);
   and the contract declared-key line renders `declared=(…) inferred=(…)`
   identically (verified against `key_tc_witness.contract.opt.golden:43`).

**Cases changing CLASS (not bytes):** exactly `key_multi_adorn_1`
(strict→bijection Arm B, stays diagnostic) and `reject_key_double_1`
(parse-second-pragma → parse-dup-set, stays a reject). Diagnostic text is
UNPINNED, so both class moves are byte-safe under the all-4-modes-diagnostic
rc+cross-mode contract.
