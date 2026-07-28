# D3.a.1 STAGE (b) — LANE b1: the retract channel + SET netting + `-demand-retract`

> Tip `95251825` (verified clean). Binding context honored, not re-litigated:
> d3a1-substrate.md (§5/§6/§7 — CO-ACTIVATION ruled, P-STORE stays
> `TableIsDifferential(pub)`, death gate keeps demand-keyed Rel.cpp:1139, do NOT
> fold the predicates; XC-3: d1 not independently landable), d3a-ruling-brief.md
> (OQ-RETRACT-POLICY: SET-demand + batch SET netting; the pinned three-way
> coupling), d3a-substrate.md §7:888-1001 (d1-d7), d3a0-design.md §3.
> Every anchor below re-verified at code THIS session unless marked [substrate].
> The whole slice lands as ONE commit (XC-3); b1's edits are one quarter of it.

---

## §0 HEADLINE DECISIONS

1. **`-demand-retract` implies `-demand`, is orthogonal to `-demand-instance`
   and to the 4 golden optimization modes, and stays OFF the PassPolicy
   registry** (a semantic flag, the `-demand`/`-demand-instance` precedent —
   Main.cpp:467-479 comment idiom carries over verbatim). `-demand-instance`
   does NOT imply `-demand-retract` (existing witnesses stay byte-identical),
   and `-demand-retract` does not imply `-demand-instance` (flat retraction is
   the eqgate's flat arm).
2. **ONE authority, one new bool threaded four hops:** Main.cpp
   `gDemandRetract` → `Query::Build(..., demand_retract)` →
   `QueryImpl::ApplyDemandTransform(..., demand_retract)` →
   `ParsedModule::FabricateDemandMessage(..., differential)` which, when true,
   stamps `message->differential_attribute =
   Token::Synthetic(Lexeme::kPragmaDifferential, DisplayRange())`. Everything
   downstream keys off `ParsedMessage::IsDifferential()` mechanically — zero
   further plumbing (co-activation, substrate §6/§7).
3. **The retract writer is a SECOND injector proc + a generated driver entry
   `<name>_<bindings>_retract(db, log, functors, bound...)`** — a mirror of the
   demand forcer that VECTORAPPENDs the key into the REMOVE vector and calls
   the same suppressed handler. `ProgramQuery` gains
   `std::optional<ProgramProcedure> retract_function`. The fabricated message's
   public ABI entry stays suppressed (the suppression keys on
   `Query().IsDemandMessage`, Database.cpp:1504/:3370 — differentialness never
   enters it). The `_retract` suffix sits AFTER the binding pattern, the
   `_cursor` precedent: binding patterns are drawn from {b,f}\* so no user
   query's emitted `<name>_<pattern>` can collide with `<name>_<pattern>_retract`.
4. **SET netting is ZERO new code**: `BuildIOProcedure` already mints the
   handler's second parameter vector + the NETBATCH region iff
   `message.IsDifferential()` (Procedure.cpp:557-575); annihilation happens in
   the generated handler `demand__<q>_<a>_detail`, per received batch, BEFORE
   the flow (`::hyde::rt::NetBatch`, Vec.h:176-218; emitter
   Database.cpp:2764-2768). Same-batch death+re-demand is impossible on TWO
   independent grounds (§3).
5. **Flat `-demand` retraction end-to-end is ZERO new differential code** — the
   toggle rides the ordinary machinery: closure flip (Differential.cpp:56-65 +
   fixpoint), two-polarity ingest folds (Procedure.cpp:50-59), doubled
   entry-proc params (Procedure.cpp:26-38), frontier sextet + commit-band
   netting (Rel.cpp:1818-1830, :2511-2543), OVERDELETE→REDERIVE→INSERT strata.
   Evidence in §4.
6. **Driver contract (SET-demand, OQ-RETRACT-POLICY):** retract kills a
   STANDING demand; retracting a key that is not standing (or double-retract
   across epochs) is a driver contract violation, caught by the DiffTable
   commit sweep's per-class `>= 0` asserts in debug ([DBG], Table.h — the same
   contract every user `@differential` message already carries). b4's witness
   honors it.
7. **No new dump-token spellings from b1** — no E-71 grammar note owed by this
   lane. Flag-on `.rel`/`.ir` output uses only existing vocabulary (DiffTable
   flavors, frontier roles, NETBATCH, `inject_<id>`).

---

## §1 SUB-DIFF (i) — THE `-demand-retract` TOGGLE

### 1.1 Flag relations (decided)

- Implies `-demand`: a retract surface without the demand transform is
  meaningless; mirror of `-demand-instance` (Main.cpp:478 `hyde::gDemand =
  true; // implies -demand`).
- Orthogonal to `-demand-instance`: the flag combinations and their meanings —
  - `-demand` alone: today's behavior, byte-identical (toggle false).
  - `-demand -demand-retract`: flat differential demand — the eqgate's flat arm.
  - `-demand-instance` alone: today's birth-and-rebuild, byte-identical
    (fabricated message stays monotone ⇒ P-DEATH false ⇒ no death minted ⇒
    b2/b3 machinery dormant on the mint gate).
  - `-demand-instance -demand-retract`: the full D3.a.1 nested arm (death,
    (T,F) scan, belt — b2/b3/b4).
- Orthogonal to the 4 golden modes: activation is per-case via `.drflags`
  sidecars only (runall.sh appends; the modes never touch it) — the
  `demand_tc_witness` precedent.
- OFF PassPolicy: same grounds as the `df.demand` deliberately-un-gated record
  (lib/DataFlow/Build.cpp:2576-2585 comment) — a pass policy silently neutering
  a semantic flag would silently drop its semantics. No registry entry, never a
  registered pass name.

### 1.2 The token minting (the exact d1 edit)

`ParsedMessage::IsDifferential()` = `impl->differential_attribute.IsValid()`
(Parse.cpp:1336-1338). `Token::IsValid()` is a lexeme blacklist (Token.cpp:12-31)
— any non-`kInvalid*` lexeme is valid. Mint:

```cpp
message->differential_attribute =
    Token::Synthetic(Lexeme::kPragmaDifferential, DisplayRange());
```

- **Lexeme-faithful**: `kPragmaDifferential` is exactly what the parser stores
  at the user site (Message.cpp:176-186; lexed at Lexer.cpp:493), so any future
  lexeme-keyed reader sees the same species as a parsed `@differential`.
- **Empty-range synthetic is the established idiom for fabricated PRAGMAS**:
  Aggregate.cpp:195 (`Token::Synthetic(Lexeme::kPragmaPerfInline,
  DisplayRange())`). The A7/G1 display-buffer route is required only for NAMES
  (codegen resolves emitted names via `SpellingRange()` — Demand.cpp:15-24
  header comment); the attribute token's spelling is never emitted.
- **Reader audit (exhaustive grep, `differential_attribute` + `.Differential()`
  at tip):**
  - Parse.cpp:1337 `IsValid()` — the predicate we are toggling. ✔
  - Parse.cpp:1341 `Differential()` accessor — its only lib consumer is the
    COMMENTED-OUT branch Differential.cpp:164-172 (verified still commented). ✔
  - Message.cpp:177/:182/:186 — parse-time only, never runs for fabricated
    decls. ✔
  - Parser.cpp:1375/:1395-1410 — redeclaration-consistency diagnostics, parse
    time only; a fabricated message is `CreateDerived` with a fresh context,
    sole redeclaration (Demand.cpp:186-192), never re-declared. ✔
  - Format.cpp:154-155 — prints `" @differential"` from `IsDifferential()`,
    never reads the token's spelling; and the parse round-trip runs at parse
    time, BEFORE fabrication, so fabricated decls never print anyway. ✔

  ⇒ no reachable spelling-range read; the synthetic empty-range token is safe.
- **Placement**: immediately after the `rparen` assignment
  (lib/Parse/Demand.cpp:197), before `FabricateParams` — with a comment naming
  the OQ-RETRACT-POLICY channel. `Lexeme` is already in scope
  (`drlojekyll/Lex/Lexer.h` included at Demand.cpp:35; `Token::Synthetic`
  declared in Lex/Token.h, transitively included).
- `FabricateDemandLocal` is UNTOUCHED — differentialness of the demand relation
  is derived by `TrackDifferentialUpdates` from the message receive SELECT
  (Differential.cpp:56-65), never from the `#local` decl.

### 1.3 The threading (four hops, all mechanical)

Hop 1 — Main.cpp: `static bool gDemandRetract = false;` (after :49); parse
branch after the `-demand-instance` branch (:476-479); `Query::Build` call
(:68) gains `, gDemandRetract`; usage text (:220) gains one line.

Hop 2 — `Query::Build` public decl (include/drlojekyll/DataFlow/Query.h:1051)
gains `bool demand_retract = false`; def (lib/DataFlow/Build.cpp:2518-2521)
gains the param and forwards at the `ApplyDemandTransform` call (:2587).

Hop 3 — `QueryImpl::ApplyDemandTransform` (lib/DataFlow/Query.h:1040 decl;
lib/DataFlow/Demand.cpp:385-386 def) gains `bool demand_retract`. Note the
head-gate (:392) keys `demand_mode` only — `-demand-retract` without a
demanded bound query is inert by construction.

Hop 4 — `ParsedModule::FabricateDemandMessage`
(include/drlojekyll/Parse/Parse.h:885-887 decl; lib/Parse/Demand.cpp:163-164
def) gains `bool differential` (no default — single caller, Demand.cpp:829,
which passes `demand_retract` through). §1.2 mint under `if (differential)`.

No demand-pass REJECT changes: none of the pass's fences (Demand.cpp:434-789)
tests message differentialness [substrate §1.5, spot-verified at :624-627],
and the three nested fences are `Program::Build`-side, `demand_instance`-gated.

---

## §2 SUB-DIFF (ii) — THE RETRACT WRITER

### 2.1 The seam, as landed (verified)

The demand del_vec channel is plumbed end-to-end but writer-less:
`BuildQueryForceProcedureFromRegistry` (Build.cpp:385-447) creates `del_vec`
iff `message.IsDifferential()` (:418-421, kind `kEmpty`), never appends to it
(:426-432 — the sole VECTORAPPEND targets `add_vec`), and passes it as
`arg_vecs[1]` `// Empty.` (:438-440). The handler (`BuildIOProcedure`,
Procedure.cpp:534-609) takes both vectors as REAL parameters when differential
(:557-561) and NETBATCHes them (:570-575). The registry is `QueryDemandForcing`
(populated Demand.cpp:1127-1128 area), matched per-adornment in
`BuildQueryForceProcedure` (:467-476); the forcer lands on
`ProgramQuery::forcing_function` (:504-514) and codegen emits the query entry
that calls it (`EmitQueryFriends`, Database.cpp:1603-1685).

### 2.2 Decision: a second injector proc + a generated retract entry

**Rejected alternatives:**
- *Un-suppress the fabricated message's ABI* — reverses the deliberate F2-B(ii)
  registry suppression (Database.cpp:1495-1505 comment: a raw call would inject
  unguarded demand rows); would also expose the ADD side, not just retract.
- *A retract arm folded into the existing forcer* (flag parameter / second
  entry point into one proc) — regions/procs have no scalar-conditional
  dispatch idiom; two procs is the shape the machinery already emits.
- *A Vec-batched retract entry* (`Vec<K>` parameter) — asymmetric with the
  scalar forcing surface; the forcer precedent is one key per call, and epochs
  are per-entry-call anyway.

**Chosen shape — `BuildQueryRetractProcedureFromRegistry`,** a sibling of
Build.cpp:385-447 with exactly three deltas:

```
proc = Create(kQueryMessageInjector); proc->has_raw_use = true
one kParameter input var per entry.bound_params            # identical
col_types from the fabricated message's params             # identical
add_vec = vectors.Create(kEmpty,     col_types, 0)         # DELTA 1 (kinds swapped)
del_vec = vectors.Create(kParameter, col_types, 0)         # DELTA 1
VECTORAPPEND(del_vec <- proc->input_vars)                  # DELTA 2 (target)
CALL messsage_handler[message](add_vec /*Empty.*/, del_vec)# DELTA 3 (arg roles)
RETURN kReturnTrueFromProcedure                            # identical
```

- Both vectors are proc-LOCALS (created via `proc->vectors.Create`, not
  `VectorFor` — `VectorFor(kParameter)` would route to `input_vecs` and become
  a function parameter, Procedure.cpp:160-169 (lib/ControlFlow); the forcer
  uses the same locals-with-kind idiom, so vector-kind semantics mirror usage:
  `kParameter` = carries the query's params, `kEmpty` = always empty).
- The CALL's arg order matches the handler's parameter order
  `(io_vec, io_remove_vec)` — Procedure.cpp:594-597.
- `ProgramOperation::kAppendQueryParamsToMessageInjectVector` is reused for the
  append (still literally true: query params into a message-inject vector).
- **Only built when `message.IsDifferential()`** — dispatcher
  `BuildQueryRetractProcedure(impl, context, query)`: the same registry loop +
  BindingPattern second-belt as :467-476, returning the retract proc iff the
  entry matches AND `entry.message.IsDifferential()`; the `@first`
  (`BuildQueryForceProcedureImpl`) surface gets NO retract arm (user forcing
  messages are out of scope; their del_vec `// Empty.` arm at :340-345/:362-364
  is untouched).
- **No dedup / DCE hazard**: `kQueryMessageInjector` is excluded from
  procedure dedup outright (Optimize.cpp:1402-1408 `continue`), and
  `has_raw_use = true` protects it from remove-unused (Optimize.cpp:1276).
- **Handler-map timing**: identical to the forcer (assert Build.cpp:393-394;
  `messsage_handler` populated by `BuildIOProcedure` before entry points are
  built).
- **Id-stream**: the retract proc is created AFTER the forcer at the
  `BuildQueryEntryPointImpl` site, so with the flag off no id is allocated and
  the id stream is byte-identical to tip.

### 2.3 `ProgramQuery` carries it (public header, [STRUCT])

include/drlojekyll/ControlFlow/Program.h:1333-1360: add after
`forcing_function` (:1348):

```cpp
  // If present, a procedure which retracts a standing demand for the given
  // `bound`-attributed parameters (the `-demand-retract` surface, D3.a.1).
  // Present only for a demand-transformed query whose fabricated demand
  // message is differential.
  std::optional<ProgramProcedure> retract_function;
```

Ctor (:1350-1356) gains the fifth parameter. Construction sites (both):
Build.cpp:514 (real: passes the §2.2 result) and Build.cpp:543-544 (empty-query
entry: passes `std::nullopt` — a demanded-but-dead query keeps no retract
surface, mirroring its absent forcer).

### 2.4 The generated driver entry (codegen, [STRUCT] flag-on only)

`EmitQueryFriends` (Database.cpp:1603) — insert after the bound/free-name
computation (:1626), BEFORE the `!has_free` early return (:1667-1685) so both
query shapes get it:

```cpp
  if (spec.retract_function) {
    const auto &rfx = EffectsOf(*spec.retract_function);
    hh << hh.Indent() << "// Retract a standing demand for `" << decl.Name()
       << "/" << decl.Arity() << "` (" << decl.BindingPattern() << ").\n";
    hh << hh.Indent() << "template <typename Log, typename Functors>\n";
    hh << hh.Indent() << "friend void " << name << "_retract(Database &db, Log &"
       << (rfx.uses_log ? "log" : "") << ", Functors &"
       << (rfx.uses_functors ? "functors" : "");
    for (auto i = 0u; i < params.size(); ++i) {
      if (is_bound[i]) {
        hh << ", " << TypeName(module, params[i].Type()) << " " << param_names[i];
      }
    }
    hh << ") {\n";
    hh.PushIndent();
    hh << hh.Indent() << "assert(db.initialized_);\n";
    hh << hh.Indent() << DetailName(*spec.retract_function) << "("
       << DetailStateArgs(*spec.retract_function, "db.");
    for (const auto &arg : bound_names) { hh << ", " << arg; }
    hh << ");\n";
    hh.PopIndent();
    hh << hh.Indent() << "}\n\n";
  }
```

- `name` is already `<qname>_<pattern>` (:1605-1606) ⇒ the entry is
  `<qname>_<pattern>_retract` (e.g. `neighborhood_bf_retract`). Hidden friend,
  ADL-reached, epoch-0 asserted — every house convention of the query surface.
  Returns void (the injector detail's bool is ignored, the same as
  `emit_forcing_call`, :1659-1664).
- The proc BODY needs zero new codegen: `kQueryMessageInjector` bodies are
  emitted by the generic namespace-scope detail loop today (forward decls
  Database.cpp:977-988; `ProcName` → `inject_<id>`, :204-205), including
  locals of kind `kEmpty` (the `@first`+`@differential` forcer path exercises
  that emission shape).
- The suppression registry is untouched: both suppression sites key
  `program.Query().IsDemandMessage(*m)` (Database.cpp:1504, :3370), never
  differentialness — the fabricated message's public entry and its `_input`
  alias stay suppressed with the retract flag on. The `_detail` twin stays
  callable by BOTH injectors.
- **Lowering-independence (the eqgate requirement)**: `ProgramQuery`,
  `EmitQueryFriends`, the injector builders, and `BuildIOProcedure` are all
  upstream of / orthogonal to the `-demand-instance` selector (Main.cpp passes
  `gDemandInstance` only to `Program::Build`; the nested selector changes the
  demanded-subgraph LOWERING, not the query/handler surface). The witness
  driver therefore exercises retraction with the IDENTICAL call text under
  both arms — the b4 requirement holds by construction.

### 2.5 Driver contract (for b4's witness + docs)

- One key per call; each call is one epoch (handler → flow → commit sweeps),
  exactly like a message entry; open cursors are invalidated (standing cursor
  contract).
- SET semantics (OQ-RETRACT-POLICY): retract kills the key's standing demand;
  no ref-counting. Retract of a NON-standing key (never demanded, or already
  retracted) violates the contract: the demand row's `C_nr` nets negative and
  the commit sweep's per-class `>= 0` [DBG] asserts fire — the identical
  contract user `@differential` messages carry today. NDEBUG behavior is the
  inherited user-message behavior (no new hazard minted by this slice).
- Demand re-issue after retract is the ordinary rebirth path: a later
  `<q>_<pattern>(db, log, functors, k...)` probe re-forces `+k` (flat: guard
  join re-derives; nested: band-(a1) `FindOrAddInstance` rebuild — no iid
  tombstone, OQ-DEATH-VS-REBUILD).

---

## §3 SUB-DIFF (iii) — SET NETTING (zero new code; the coupling leg)

**Engagement is automatic.** With the fabricated message differential,
`BuildIOProcedure` mints `io_remove_vec` as a second handler parameter
(Procedure.cpp:557-561) and the NETBATCH region (:570-575, comment :566-569:
"one received batch is one epoch"); `EmitNetBatch` renders
`::hyde::rt::NetBatch(vecA, vecR)` (Database.cpp:2764-2768). Verified: nothing
in that path is demand-aware — it keys `message.IsDifferential()` only.

**Where annihilation happens:** in the generated handler detail
(`demand__<q>_<adorn>_<arity>_detail`), once per received batch, BEFORE the
flow call — never in counters or the commit sweep. `rt::NetBatch`
(Vec.h:176-218): distinct-scan with per-row flags (bit0 = in adds, bit1 = in
removes, :181-198), rewrite keeping flags==1 in adds, flags==2 in removes,
dropping flags==3 (:206-217) — each side deduplicated, adds∩removes
annihilates; `{+x,-x,-x}` is a no-op (the OQ3 semantics verbatim).

**Why same-batch death+re-demand is impossible (the OD-15 coupling leg), two
independent grounds:**

1. *Structurally, at the entry granularity:* the only writers of the demand
   handler's vectors are the two injectors — the forcer appends only to
   `add_vec` (Build.cpp:426-432) and passes `del_vec` empty; the retract proc
   appends only to `del_vec` and passes `add_vec` empty. Each injector CALL is
   one handler invocation = one batch = one epoch, so no demand batch can ever
   contain both signs of any key, let alone the same key.
2. *Defensively, in-channel:* if any future surface ever does hand the handler
   both signs in one batch (e.g. a batched demand entry), NetBatch annihilates
   the intersection pre-flow — the counters never see the flap, no frontier
   entry is minted on either side, so neither a death nor a rebuild fires for
   the flapped key. This is the guard OD-15 pins; it is emitted and live from
   the moment the message goes differential.

Downstream of the batch, the per-EPOCH netting that actually feeds b2's death
trigger is the demand table's own counter/commit machinery: retract `-k` folds
`C_nr: 1→0`, presence crosses, the commit-band frontier filter (`mint_filter`,
Rel.cpp:2511-2538, per differential table :2540-2543) appends `k` to the
demand `kNetRemovals` frontier — the POST-NETTING product b2's death band
drains. Cross-epoch flap (retract in epoch N, re-demand in epoch N+1) is the
LEGAL death-then-rebirth sequence, ordered by OD-2 + TouchedFlag (b2/b3
territory; the three-way coupling: netting kills same-batch flap, death's
Touch suppresses dead-key a2, V-INST-FRESH unchanged because Recycle leaves
current empty).

---

## §4 SUB-DIFF (iv) — FLAT `-demand` GOES DIFFERENTIAL END TO END (zero new code)

Chain of evidence, every link an existing mechanism (all verified at tip):

1. **Seed**: the demand receive SELECT is over an IO whose
   `ParsedMessage::IsDifferential()` — `TrackDifferentialUpdates` seeds
   `can_receive = can_produce = true` (Differential.cpp:56-65); authoritative
   post-demand run at lib/DataFlow/Build.cpp:2618.
2. **Closure**: the :78-142 fixpoint (lift + INSERT→SELECT seam + column-edge
   forward) flips the WHOLE demanded closure — demand relation, guard JOINs,
   guarded bodies, `p`'s MERGE/INSERT, everything downstream [substrate §1.4
   walk; seed/lift/seam sites re-verified]. No blocker applies on the slice:
   NEGATE/AGG in a demanded body are pre-rejected (Demand.cpp:624-627).
3. **Tables**: `TableIsDifferential` (lib/ControlFlow/Build/Build.cpp:705-721)
   turns true per flipped table ⇒ DiffTable flavor, membership predicates,
   claim gates, commit sweeps — the standing per-stratum
   OVERDELETE → REDERIVE → INSERT machinery.
4. **Ingest**: the demand receive's `CanReceiveDeletions()` selects the
   two-polarity stage-1 folds (`MakeStageOneIngestFolds`,
   Procedure.cpp:50-59) and the entry proc's params double (removal vec,
   Procedure.cpp:26-38); the handler CALL doubling is Procedure.cpp:594-608.
   All keyed on the message/receive bits — zero demand-special code.
5. **Frontiers**: the differential table gets the full frontier sextet
   (Rel.cpp:1818-1830) and the commit-band `mint_filter` produces the net
   frontiers (:2511-2543). (Under flat `-demand` nothing drains the demand
   net-removals — that is inert surplus, the same as any differential table
   whose removals no consumer reads; under `-demand-instance` it is exactly
   b2's death trigger.)
6. **Retraction semantics**: a retract epoch drops the demand row's presence;
   OVERDELETE cascades through the guard JOINs; guarded rows keyed by OTHER
   standing demands survive on their per-row counters (REDERIVE); published
   downstream deltas emit through the ordinary publish machinery.
7. **No fence blocks it**: the demand-pass rejects never test message
   differentialness; the three nested fences are `demand_instance`-gated
   (Build.cpp:1393) [substrate §1.5]. The one incidental surface: any
   PUBLISHED message reached by the flipped closure must be `@differential` or
   Differential.cpp:174-179 hard-errors — a b4 witness-design constraint
   (G-15), not a code change.

⇒ b1 ships NO new differential machinery. The flat arm is the eqgate's living
oracle for every retract batch (ruling-brief standing referee note).

---

## §5 EDIT-SPEC INVENTORY (file:line at tip 95251825; before → after)

| # | File:line | Edit |
|---|---|---|
| E1 | bin/drlojekyll/Main.cpp:49 | after `static bool gDemandInstance = false;` add `static bool gDemandRetract = false;` |
| E2 | bin/drlojekyll/Main.cpp:67-68 | `Query::Build(module, error_log, gPassPolicy, gDemand);` → `Query::Build(module, error_log, gPassPolicy, gDemand, gDemandRetract);` |
| E3 | bin/drlojekyll/Main.cpp:220 | usage: add `  -demand-retract           Enable demand retraction (implies -demand): bound queries gain a <name>_<bindings>_retract entry point.` |
| E4 | bin/drlojekyll/Main.cpp:479 (after the `-demand-instance` branch) | new branch: `-demand-retract`/`--demand-retract` → `hyde::gDemand = true; hyde::gDemandRetract = true;` with the semantic-flag comment idiom |
| E5 | include/drlojekyll/DataFlow/Query.h:1051 | `Build(..., bool demand_mode = false)` → `..., bool demand_mode = false, bool demand_retract = false)` (+ doc sentence) |
| E6 | lib/DataFlow/Build.cpp:2518-2521 | def gains `bool demand_retract`; :2587 `ApplyDemandTransform(module, log, demand_mode)` → `..., demand_mode, demand_retract)` |
| E7 | lib/DataFlow/Query.h:1040 | `ApplyDemandTransform(const ParsedModule &, const ErrorLog &, bool demand_mode)` → `..., bool demand_mode, bool demand_retract)` |
| E8 | lib/DataFlow/Demand.cpp:385-386 | def gains `bool demand_retract`; :829 `FabricateDemandMessage(base_name, bound_types)` → `FabricateDemandMessage(base_name, bound_types, demand_retract)` |
| E9 | include/drlojekyll/Parse/Parse.h:885-887 | `FabricateDemandMessage(std::string_view, const std::vector<TypeLoc> &)` gains `bool differential` (+ doc sentence naming the retract channel) |
| E10 | lib/Parse/Demand.cpp:163-164, :197 | def gains `bool differential`; after `message->rparen = name_tok;` insert the §1.2 `if (differential) { message->differential_attribute = Token::Synthetic(Lexeme::kPragmaDifferential, DisplayRange()); }` + comment |
| E11 | include/drlojekyll/ControlFlow/Program.h:1348-1356 | `ProgramQuery` gains `std::optional<ProgramProcedure> retract_function` + fifth ctor param (§2.3) |
| E12 | lib/ControlFlow/Build/Build.cpp (after :447) | NEW `BuildQueryRetractProcedureFromRegistry` (§2.2 shape) + NEW dispatcher `BuildQueryRetractProcedure` (registry loop + BindingPattern belt as :467-476, gated `entry.message.IsDifferential()`; no `@first` arm) |
| E13 | lib/ControlFlow/Build/Build.cpp:504-514 | compute `retract_proc` after `forcer_proc`; `queries.emplace_back(query, table, scanned_index, forcer_proc)` → `..., forcer_proc, retract_proc)` |
| E14 | lib/ControlFlow/Build/Build.cpp:543-544 | empty-query emplace gains trailing `std::nullopt` |
| E15 | lib/CodeGen/CPlusPlus/Database.cpp:1626 (inside `EmitQueryFriends`, before the `!has_free` return) | the §2.4 retract-entry emission block |

Docs riders (land with the slice, not gates): CLAUDE.md demand section gains
the `-demand-retract` sentence; docs/Language.md untouched (no surface syntax).

---

## §6 PREDICTIONS ([BYTE]/[STRUCT] per touched surface) + GATE FAMILIES

**Flag OFF (every existing case — nothing passes `-demand-retract`):**

| Surface | Prediction | Grounds |
|---|---|---|
| 175-case suite × 4 modes `.stdout` | **[BYTE]** | toggle false ⇒ `differential_attribute` stays invalid ⇒ `IsDifferential()` false ⇒ del_vec arm, retract builder (gated on it), and §2.4 emission all unreachable; id-stream unchanged (§2.2) |
| eleven `.rel` + `.irgold` pins | **[BYTE]** | no new ops minted; Rel inventory reads the same graph |
| `demand_tc_witness`, `demand_neighborhood_witness` (+ eqgate ×4, nested arm generated text) | **[BYTE]** — exact generated-text delta ZERO | fabricated message monotone as today; ProgramQuery layout change is compile-time only |
| all diagnostic cases (incl. the three nested fences) | **[BYTE]** | no reject predicate touched |
| `.df`/`.ir` dumps | **[BYTE]** | no new regions/procs exist flag-off |

**Flag ON (new surface — b4 pins it with the new/extended witness):**

| Surface | Prediction | Shape |
|---|---|---|
| generated header, flat `-demand -demand-retract` | **[STRUCT]** | fabricated handler gains 2nd `Vec` param + one `::hyde::rt::NetBatch(...)` line; demand-closure tables flip `Table<>` → `DiffTable<>`; two-polarity ingest folds + claim/commit tails appear; ONE new namespace-scope `inject_<id>` (retract) + ONE new hidden friend `<name>_<pattern>_retract` (void, template) |
| generated header, nested (with `-demand-instance`) | **[STRUCT]** | the same, PLUS b2/b3 shapes (death band, (T,F) scan, belt, `, false` store ctor) — theirs to pre-register |
| `.rel` dump flag-on | **[STRUCT]**, existing token vocabulary ONLY (frontier sextet lines, two-polarity kIngestFold, and b2's ops) | **no E-71 note owed by b1** |
| ProgramQuery / Query::Build / FabricateDemandMessage signatures | **[STRUCT]** compile-time API, no serialized form | — |

**Gate families (b1's slice of the one-commit gate set):** SUITE PASS(175) × 4
modes, debug + release [BYTE]; ASAN both surfaces; ctest (all units, count per
b2/b4's additions); eqgate `demand_neighborhood_witness` flat==nested==golden
×4 LIVE — extended by b4 with retract batches (the standing oracle); zero
golden churn outside b4's new/extended cases; config-invariance single-hash on
the demand witnesses; `permcheck.py` N/A (no emission-order change flag-off).

**Expected reds: NONE** — but ONLY because the slice lands as one commit: b1's
edits alone + `-demand-instance -demand-retract` on any case would abort at
V-INST-DRAIN (XC-3, first abort) then V-INST-EMITTED. b1-alone is green only
on flat `-demand`; the orchestrator's one-commit rule stands.

---

## §7 CO-LANDABILITY INTERFACE

**What b1 EXPOSES (siblings consume):**

- **To b2 (death machinery + frontier + validators):** under
  `-demand-instance -demand-retract` the demand table satisfies
  `TableIsDifferential(demand_table)` (P-DEATH, Rel.cpp:1139 turns true) purely
  via the closure — NO new parameter reaches Rel.cpp; do NOT add one, and do
  NOT fold the two predicates (the §7 d2 ruling). The demand table's frontier
  provisioning necessarily switches to the differential route (the
  Build.cpp:999 monotone-append gate excludes it): b2 owns making the demand
  `kNetAdditions` VECTOR exist by ValidateDROps time (Stratum.cpp:2186) AND
  provisioning/draining `kNetRemovals` (G-3/G-4/G-5). The retract epoch's
  death trigger arrives as the post-netting commit-band `kNetRemovals`
  frontier (§3 tail).
- **To b3 (band work):** co-activation flips P-STORE with zero b1 code — the
  region bit, descriptor, and store-ctor `, false` go true off the one
  Rel.cpp:1055/:1059 stamp; V-INST-DIFF-COHERENCE holds by construction (same
  predicate both sides). b3's emitter branches read that bit (G-12 accessor
  decision is b3's).
- **To b4 (witness/fences/gates):** the driver surface
  `<name>_<pattern>_retract(db, log, functors, bound...)` (void, hidden
  friend, epoch-0 asserted, one key per call, one epoch per call, cursor
  invalidation applies), IDENTICAL text under flat and nested arms (§2.4
  lowering-independence). `.drflags` recipe: flat arm `-demand
  -demand-retract`; eqgate nested re-compile appends `-demand-instance`
  (runall.sh `run_eqgate` machinery unchanged). Witness constraints b1 relies
  on: every published output over the demanded closure must be
  `@differential` (Differential.cpp:174-179, G-15); retract only standing
  demands (§2.5); DEATH stays oracle-blind (`.batches` oracle never sees it).

**What b1 EXPECTS from siblings (adjudicator reconciles):**

- b2: `{sid, kInstanceDeath}` lowering + enrollment closing V-INST-EMITTED
  (G-6), `EmitInstanceDeath` draining demand kNetRemovals →
  `FindInstance` → `RecycleCurrent` (G-7), V-INST-DRAIN extension (G-5) +
  the kNetAdditions re-provisioning (G-3), optional V-INST-EFFECT death-drain
  source check (G-8). Death band emitted BEFORE band-(a1) for the store
  (OD-2 order; Touch suppression).
- b3: the (T,F) drop scan drop-before-born per iid (G-9), del/add queue
  vectors on the SUBGRAPHINSTANCE region feeding pub's
  SubDerivation/DelQueue machinery (G-10, the Rel.cpp:829-848 effect shape),
  the RAT-7 partition belt always-on (G-11), the region-bit accessor or
  pub-branch (G-12), `monotone=false` selection (G-13 — automatic under
  co-activation, b3 verifies the emitted `, false`).
- b4: retract batches in the witness (birth-rebuild-RETRACT-rebirth probes,
  G-14), the G-15 `@differential` published outputs, the DS-R4-10 fence +
  directed witness (rider), d7 liveness perturbations (V-INST-DIFF-COHERENCE
  / OWN-3 fold abort / partition belt with a TRUE bit, G-16), and the
  suite/gate run book.

**Ordering note for the one commit:** b1's Parse/DataFlow/Main edits are
prerequisites for every sibling's flag-on testing; the temporary-fence option
(a `Program::Build` pre-pass reject of `-demand-instance` + differential
demand) is designed AWAY per the orchestrator's instruction — no hard blocker
found that requires it (b2's G-3 co-land discharges XC-3).

---

## §8 RESIDUALS / HAZARDS (named, accepted, or punted with grounds)

1. **`<name>_<pattern>_retract` naming**: collision-free against all generated
   query/message/cursor names by the {b,f}\*-pattern-alphabet argument (§2.4).
   A user's own inline C++ symbol of that exact name in driver code would be
   an ordinary C++ ambiguity error at driver compile — loud, not a miscompile.
2. **Retract-of-absent under NDEBUG** (§2.5): inherited user-differential-
   message semantics ([DBG] commit asserts; NDEBUG counter poisoning on
   contract violation). Not new; recorded, not fixed here. A future belt could
   pre-filter in the retract injector (CHECKMEMBER before append) — out of
   scope, would also desync flat/nested surfaces from user-message semantics.
3. **Flat `-demand` inert surplus** (§4.5): the demand table's kNetRemovals
   frontier is produced and never drained under flat lowering — the same
   status any differential table's unread frontier has today; zero cost at
   suite scale.
4. **`@first` + `@differential` user forcing messages** get no retract surface
   (deliberate, §2.2); the registry-only gate keeps the user surface frozen.
5. **`-demand-retract` without any demanded query** is inert (the demand-pass
   head gate keys `demand_mode`; fabrication never runs) — no diagnostic
   minted for the useless flag, matching `-demand`-without-bound-queries.
