# P1 identity target — MAP-functor member→free migration

Epoch: ADL / functor-surface, diff P1 (§12.0(a) item 1). Branch
`adl-functor-surface` off main 88058879. This file is the EXACT identity
target the P1 implementation must reproduce byte-for-byte: the two
generated-header diffs (`map_1/datalog.h`, `average_weight/datalog.h`) and
the two flagship driver diffs (`map_1.main.cpp`, `average_weight.main.cpp`).
The remaining 11 corpus drivers follow the same one-line-per-member shape
(§4). Header changes are reviewed, NOT blessed (headers are not goldens);
case STDOUT is byte-identical (hard zero-churn gate).

Committable verbatim to `docs/proposals/ADLFunctorSurface.artifacts/`.

---

## 0. The design decision, stated up front (what the target encodes)

P1 changes exactly two emission surfaces and NOTHING else:

1. **`EmitGenerate` callee (Database.cpp:2736)** — drop the `functors.`
   prefix. The member-call expression becomes a free-function name. The
   four range wrappers (:2769-2822) wrap `call` textually and are
   ABI-agnostic; they are untouched. The inline branch (:2733-2734) is
   untouched.

2. **`EmitFunctorsDecl` (Database.cpp:1162-1254)** — hoist the per-functor
   member DECLARATIONS out of `struct DatabaseFunctors { … }` to FREE
   forward-declarations emitted BEFORE the (now-empty) struct, mirroring
   the C-5 reduction free-decl idiom in `EmitStateCellStructs`
   (:1113-1124). The struct `DatabaseFunctors {};` SURVIVES, empty.

Three things deliberately DO NOT change, and the target header proves it:

- **The `uses_functors` threading web stays fully intact** (:257, :797-798,
  :812, :264-273, :842-843, :897-898). Detail procs keep
  `template <typename Functors>` and the `Functors &functors` param;
  entry points keep `template <typename Log, typename Functors>` and
  `Functors &functors`. Post-P1 no BODY reads `functors` (the sole reader
  was the :2736 member call), so the detail-threading arms are DEAD —
  but leaving them dead keeps the header byte-identical outside the two
  migrated surfaces and keeps ALL driver call sites (`init(db, log,
  functors)`, `m_1(db, log, functors, …)`) unchanged. Dead-code excision
  is explicitly deferred (see §1c, "threading" decision).

- **The empty struct survives** so `Functors=DatabaseFunctors` still
  deduces and `DatabaseFunctors functors;` still constructs (H5).

- **The dead `new_weight_i32_bbf` decl migrates too** (E-18/E-21):
  EmitFunctorsDecl's `is_reduction` test keys on kAggregate/kSummary
  params; a bbf `@recompute` merge functor fails it, so it is a member
  today (avg:227-228) despite having NO GENERATE call site. It becomes a
  free decl. No use-gating is added in P1 (§1d rationale).

---

## 1. Compiler-side diff (Database.cpp), function by function

### 1a. EmitGenerate callee — Database.cpp:2736-2737

```diff
   std::string call;
   if (auto inline_name = functor.InlineName(Language::kCxx); inline_name) {
     call = *inline_name;
   } else {
-    call = "functors." + Sanitize(ToString(functor.Name())) + "_" +
-           std::string(ParsedDeclaration(functor).BindingPattern());
+    // P1 (ADL/functor-surface epoch): the MAP-delivery call is a FREE
+    // function (unqualified lookup, bound at template-definition point —
+    // builtin args have no associated namespace so this is ordinary
+    // lookup, not ADL; E-19), matching the C-5 reduction free-fn ABI.
+    // The free forward-decl is emitted at the EmitFunctorsDecl slot
+    // (:2918), before every detail template body (two-phase lookup).
+    call = Sanitize(ToString(functor.Name())) + "_" +
+           std::string(ParsedDeclaration(functor).BindingPattern());
   }
   call += "(" + JoinExprs(VarExprs(region.InputVariables()), ", ") + ")";
```

Single-line change to the callee expression. The `call += "(" args ")"`
join, the `empty_body` counter scaffolding (:2742-2748), and the four
range wrappers (:2769-2822) are untouched — they operate on `call` as an
opaque string.

### 1b. EmitFunctorsDecl — hoist members to free decls — Database.cpp:1162-1254

The rewrite splits the current single loop into: (i) a free forward-decl
per non-inline non-reduction functor emitted BEFORE the struct, then
(ii) the struct block emptied of member decls. The per-functor
signature-synthesis (ret-by-range, free_params, bound-param list) is
IDENTICAL — only the emission target moves from `hh.Indent() << ret …`
inside the struct to a free `ret name_pattern(bound…);` at file scope.

```diff
 void Generator::EmitFunctorsDecl(void) {
   EmitInlines(hh, "c++:database:functors:prologue");
-  hh << "// User-provided functors. Define the declared member functions in\n"
-     << "// your own translation unit; the generated code calls them.\n"
-     << "struct DatabaseFunctors {\n";
-  hh.PushIndent();
-  EmitInlines(hh, "c++:database:functors:definition:prologue");
+  // P1 (ADL/functor-surface epoch): MAP functors are delivered as FREE
+  // functions (declared here, defined out-of-line by the driver), the same
+  // C-5 idiom EmitStateCellStructs uses for reduction bodies (:1113-1124).
+  // Emitted BEFORE `struct DatabaseFunctors` so the decls precede every
+  // detail template body (header assembly :2918 → detail defs :2930;
+  // two-phase lookup, E-19). The struct survives EMPTY so the entry-point
+  // `Functors` template param still deduces `DatabaseFunctors` and drivers
+  // still `DatabaseFunctors functors;`.
+  hh << "// User-provided MAP functors. Define these free functions in your\n"
+     << "// own translation unit; the generated code calls them by\n"
+     << "// unqualified name.\n";
   for (ParsedFunctor func : Functors(module)) {
     if (func.IsInline(Language::kCxx)) {
       continue;
     }
     const ParsedDeclaration decl(func);

     // R3 (v3-spec-statecell.md §C-5): a reduction functor … Skip it here.
     bool is_reduction = false;
     for (ParsedParameter param : decl.Parameters()) {
       const auto b = param.Binding();
       if (b == ParameterBinding::kAggregate ||
           b == ParameterBinding::kSummary) {
         is_reduction = true;
         break;
       }
     }
     if (is_reduction) {
       continue;
     }

     std::vector<ParsedParameter> free_params;
     … (val_type synthesis, UNCHANGED) …
     std::string ret;
     switch (func.Range()) { … (UNCHANGED) … }

-    hh << hh.Indent() << ret << " " << func.Name() << "_"
-       << decl.BindingPattern() << "(";
+    hh << ret << " " << func.Name() << "_" << decl.BindingPattern() << "(";
     auto sep = "";
     for (ParsedParameter param : decl.Parameters()) {
       if (param.Binding() == ParameterBinding::kBound) {
         hh << sep << TypeName(module, param.Type()) << " "
            << Sanitize(ToString(param.Name()));
         sep = ", ";
       }
     }
     hh << ");\n";
   }
-  EmitInlines(hh, "c++:database:functors:definition:epilogue");
-  hh.PopIndent();
-  hh << "};\n\n";
+  // The (now vestigial) functor struct: empty, kept for `Functors`
+  // deduction and driver construction (see above).
+  hh << "// User-provided functors object (vestigial ABI seam; empty since\n"
+     << "// the P1 MAP-functor free-function migration).\n"
+     << "struct DatabaseFunctors {\n";
+  hh.PushIndent();
+  EmitInlines(hh, "c++:database:functors:definition:prologue");
+  EmitInlines(hh, "c++:database:functors:definition:epilogue");
+  hh.PopIndent();
+  hh << "};\n\n";
   EmitInlines(hh, "c++:database:functors:epilogue");
 }
```

IMPLEMENTATION CORRECTION (gate-caught, 2026-07-16): the diff above
emitted the two banner comments UNCONDITIONALLY, which broke the §5.4
functor-free byte-identity this artifact itself predicted — the
pre-P1 header carried the old two-line comment, so every functor-free
program's header diffed on comment text alone (all 8 byte-compare
targets DIFFERS on first run). The landed form gates both banners on
`any_map_decl` (a pre-scan with the same inline/reduction skip tests):
a program emitting ZERO free MAP decls — functor-free OR reduction-only
— emits the ORIGINAL comment + empty struct, byte-identical to pre-P1;
only decl-bearing headers get the new banners. The byte-compare gate
exists precisely to catch this class of miss; it did.

Notes on the EmitInlines hooks: the four hooks
(`…:prologue`, `…:definition:prologue`, `…:definition:epilogue`,
`…:epilogue`) all emit nothing for the corpus (no `#pragma` inline blocks
attach to the functor surface in any of the 164 cases; verify with a
grep at implement time). They are preserved in place — the two
`definition:*` hooks stay INSIDE the (now empty) struct body to keep any
future inline-attachment semantics stable; the outer two stay around the
whole emission. If a future case attaches a `definition:*` inline it lands
in the empty struct exactly as before. This preserves the hook contract at
zero corpus cost.

**Two-phase lookup (E-19) is satisfied by construction.** The free decls
are emitted at the EmitFunctorsDecl call site (assembly slot :2918), which
precedes EmitDetailDecls (:2925) and the detail DEFINITIONS (:2930). Every
migrated call sits inside a `template <typename Functors>` detail body
(proc_16, flow_249) with builtin int32_t args — non-dependent, so the name
is bound at template-DEFINITION point and the forward-decl MUST precede the
body. It does. (Same guarantee EmitStateCellStructs already relies on at
the adjacent :2917 slot for `f_combine`/`f_reduce`.)

### 1c. The threading decision (uses_functors machinery)

**Keep the vestigial entry-point param — CONFIRMED, per seed §12.3
recommendation.** All three entry-point families stay templated with the
`Functors &functors` param: init (:1327-1330), message (:1350-1353), and —
the family §12.2(A)'s threading list omits (E-20) — forced-query
`EmitQueryFriends` (:1465-1469). This costs zero driver-call churn:
`init(db, log, functors)` / `m_1(db, log, functors, …)` /
`forced_query(db, functors, …)` call sites are unchanged.

**Detail-threading arms: LEFT DEAD IN P1, not deleted.** The arms —
`ProcEffects::uses_functors` (:257), the GENERATE set-site (:797-798), the
call-propagation (:812), DetailTemplateHeader's `Functors` arm (:271-273),
DetailStateParams (:842-843), DetailStateArgs (:897-898) — continue to
fire exactly as today because the GENERATE region still exists and still
trips :797-798 (it keys on `!InlineName`, NOT on the call being a member).
So a MAP-bearing proc still reports `uses_functors=true`, still emits
`template <typename Functors>`, still takes and threads `Functors
&functors`. The param is now unread inside every body, but its PRESENCE is
what makes the target header byte-identical to pre-P1 outside the two
migrated surfaces.

Rationale for deferring the excision:
- **Identity-target discipline.** Deleting the arms would change every
  detail-proc signature (drop `template <typename Functors>` and the
  param) AND every intra-detail call site (drop the `functors` arg) AND
  the entry-point→detail arg lists — a large header churn that buys
  nothing for correctness and widens the review surface of the epoch's
  FIRST diff. P1's thesis is "behavior-neutral, minimal-surface shape
  work"; a dead param is the minimal surface.
- **The seam is wanted.** memory:wasm-functor-direction — an object-owned
  functor-state ABI (engine plumbs a functor object carrying WASM engine
  state) reclaims exactly this param. Deleting it now, then re-adding it,
  is churn-for-churn.
- **Clean excision is P4 residue**, gated on the WASM-direction decision
  (do we keep object-owned functor state at all?). If that answer is
  "free functions forever," a follow-on diff drops the arms wholesale with
  its own header review. P1 does not prejudge it.

Alternative considered and REJECTED for P1: force `uses_functors=false` on
GENERATE (making the detail plumbing vanish while keeping the entry-point
param unconditionally, which is already unconditional at :1327/:1350/:1465).
This is a SMALLER post-state header but a LARGER P1 diff and a
harder-to-review one (it silently changes 4 detail signatures in map_1, 4
in average_weight, and every intra-detail call). Deferred to the excision
diff where it belongs.

### 1d. Use-gating the free decls — CONSIDERED, REJECTED for P1

EmitFunctorsDecl emits one decl per non-inline non-reduction functor with
NO call-site gate, so it emits `new_weight_i32_bbf` (avg) even though that
functor has no GENERATE (its delivery rides the @recompute reduction path;
E-21). One could gate the free decl on actual GENERATE use. REJECTED:
(a) it changes the identity target (drops a decl the pre-generated header
has), widening this diff; (b) the emit-always behavior gives a BETTER
failure mode — a driver that defines a member for a declared-but-unused
functor still compiles, and the decl documents the functor's MAP ABI even
when unused. Use-gating is an orthogonal cleanup, not P1's job. P1
preserves emit-per-non-reduction-functor exactly.

### 1e. The 13 corpus-driver edits

Every `int32_t DatabaseFunctors::NAME_pattern(…)` out-of-line member
DEFINITION loses the `DatabaseFunctors::` qualifier, becoming a free
function definition. Enumerated in §4. No call-site edits in drivers (they
call entry points, not functors directly). No signature changes (H3: MAP
free fns take by-value builtins identical to the member sig).

---

## 2. The generated-header diffs (the identity target)

### 2a. map_1/datalog.h — unified diff

```diff
@@ // functor-decl surface (was lines 52-56)
-// User-provided functors. Define the declared member functions in
-// your own translation unit; the generated code calls them.
-struct DatabaseFunctors {
-  int32_t add_i32_bbf(int32_t L, int32_t R);
+// User-provided MAP functors. Define these free functions in your
+// own translation unit; the generated code calls them by
+// unqualified name.
+int32_t add_i32_bbf(int32_t L, int32_t R);
+// User-provided functors object (vestigial ABI seam; empty since
+// the P1 MAP-functor free-function migration).
+struct DatabaseFunctors {
 };
```

```diff
@@ proc_16 body (was lines 194-208) — four call sites lose `functors.`
   for (auto [v20] : vec18) {
-    const auto t21 = functors.add_i32_bbf(v20, 100);
+    const auto t21 = add_i32_bbf(v20, 100);
     const auto v22 = t21;
     out_v_13.TryAdd({v22});
-    const auto t23 = functors.add_i32_bbf(v20, 1);
+    const auto t23 = add_i32_bbf(v20, 1);
     const auto v24 = t23;
     out_t_7.TryAdd({v24});
-    const auto t25 = functors.add_i32_bbf(v20, 10);
+    const auto t25 = add_i32_bbf(v20, 10);
     const auto v26 = t25;
     out_u_10.TryAdd({v26});
   }
   for (auto [v29] : vec27) {
-    const auto t30 = functors.add_i32_bbf(v29, 100);
+    const auto t30 = add_i32_bbf(v29, 100);
     const auto v31 = t30;
     out_v_13.TryAdd({v31});
   }
```

FOUR call sites (:195, :198, :201, :206 — one per emitted GENERATE, the
corrected count: three in the first loop, one in the second). NOTHING ELSE
in map_1/datalog.h changes: the four detail templates keep
`template <typename Functors>` + `Functors &functors` (init_3:69-70,
proc_16:71-72, m_1_detail:73-74, n_1_detail:75-76), the friend entry points
keep `template <typename Log, typename Functors>` + `Functors &functors`
(init:95-99, m_1:103-106, n_1:110-113), and every intra-detail call still
passes `functors` (proc_16 calls at :188, :219, :226). Threading is
byte-identical.

### 2b. average_weight/datalog.h — unified diff

```diff
@@ functor-decl surface (was lines 224-229)
-// User-provided functors. Define the declared member functions in
-// your own translation unit; the generated code calls them.
-struct DatabaseFunctors {
-  int32_t div_i32_bbf(int32_t LHS, int32_t RHS);
-  int32_t new_weight_i32_bbf(int32_t OldWeight, int32_t NewWeight);
+// User-provided MAP functors. Define these free functions in your
+// own translation unit; the generated code calls them by
+// unqualified name.
+int32_t div_i32_bbf(int32_t LHS, int32_t RHS);
+int32_t new_weight_i32_bbf(int32_t OldWeight, int32_t NewWeight);
+// User-provided functors object (vestigial ABI seam; empty since
+// the P1 MAP-functor free-function migration).
+struct DatabaseFunctors {
 };
```

```diff
@@ flow_249 body (was lines 761-773) — two call sites lose `functors.`
   for (auto [v178, v179, v180] : vec167) {
-    const auto t181 = functors.div_i32_bbf(v179, v180);
+    const auto t181 = div_i32_bbf(v179, v180);
     const auto v182 = t181;
@@
   for (auto [v185, v186, v187] : vec172) {
-    const auto t188 = functors.div_i32_bbf(v186, v187);
+    const auto t188 = div_i32_bbf(v186, v187);
     const auto v189 = t188;
```

TWO call sites (:762, :773). `new_weight_i32_bbf` migrates as a decl only
(no call site; E-21). The C-5 reduction free decls + Reduce_<id> policy
structs (:167-222) are ALREADY free — UNCHANGED (post-P1 both ABIs are the
same free-function shape; the whole point). Detail/entry-point threading
(the `Functors` param on init_3/proc_41/add_edge_3_detail/flow_249 and the
friends) is byte-identical — UNCHANGED.

---

## 3. The two flagship driver diffs

### 3a. map_1.main.cpp

```diff
-int32_t DatabaseFunctors::add_i32_bbf(int32_t l, int32_t r) {
+int32_t add_i32_bbf(int32_t l, int32_t r) {
   return l + r;
 }
```

`main()` (:12-17) is UNCHANGED: `DatabaseFunctors functors;` still
constructs the (now empty) struct, `init(db, log, functors)` still passes
it.

### 3b. average_weight.main.cpp

```diff
-int32_t DatabaseFunctors::div_i32_bbf(int32_t LHS, int32_t RHS) {
+int32_t div_i32_bbf(int32_t LHS, int32_t RHS) {
   return RHS != 0 ? LHS / RHS : 0;
 }
-int32_t DatabaseFunctors::new_weight_i32_bbf(int32_t /*OldWeight*/,
-                                             int32_t NewWeight) {
+int32_t new_weight_i32_bbf(int32_t /*OldWeight*/,
+                           int32_t NewWeight) {
   return NewWeight;  // last-writer merge (matches the @recompute rescan)
 }
```

The three C-5 reduction free fns (:19-38: sum/count identity/combine/
uncombine + new_weight_i32_reduce) are ALREADY free — UNCHANGED. The
comment header (:2-5) mentioning "DatabaseFunctors MAP members … defined
out-of-line" should be updated to "MAP free functions" (cosmetic, same
diff).

---

## 4. The remaining 11 corpus drivers (same mechanical shape)

Each line `<ret> DatabaseFunctors::<name>_<pattern>(…)` → `<ret>
<name>_<pattern>(…)`:

- algebra_invertible_1.main.cpp:8   `add_i32_bbf`
- cf15_6.main.cpp:8                 `add_i32_bbf`
- evm_func_parse.main.cpp           `left_corner_bf`(:26), `right_corner_bf`(:30),
  `lexeme_of_token_bf`(:36), `start_function_type_bbf`(:51),
  `add_function_type_param_bbf`(:56), `finish_function_type_params_bbf`(:61)
  — 6 members. JUDGE-CORRECTED HONESTY NOTE: evm_func_parse is
  expected-diagnostic in ALL 4 modes; runall.sh dispatches it only to
  expect_diagnostic() (rc==1 asserted, $CXX NEVER invoked, no header is
  ever emitted for it). Its driver edit is therefore UNVERIFIABLE BY ANY
  GATE — migrated for corpus consistency only (a future flip of the case
  to compiling would otherwise inherit a stale-ABI driver); the review
  treats it as convention, not tested code.
- fibonacci.main.cpp:8              `add_i32_bbf`
- fibonacci_iterative.main.cpp:8    `add_i32_bbf`
- force.main.cpp:10                 `generate_next_id_bf`
- map_2.main.cpp:8                  `add_i32_bbf`
- map_3.main.cpp:9                  `add_i32_bbf`
- map_4.main.cpp:8                  `add_i32_bbf`
- map_5.main.cpp:8 `add_i32_bbf`, :12 `is_even_b`
- pairwise_average_weight.main.cpp:27 `add_i32_bbf`, :30 `div_i32_bbf`,
  :33 `new_weight_i32_bbf`

Plus the two flagships (§3). Total = 13 files. No `DatabaseFunctors::`
definition exists anywhere else in the tree (verified: tests/MiniDisassembler,
tests/PointsTo, bin/, data/ all clean).

---

## 5. THE CONTRACT ANALYSIS

The generated-surface contract (deduction / no-virtual / no-inheritance /
hidden-friend / two-phase lookup) is PRESERVED on every call-site shape.
Enumerated exhaustively.

### 5.1 The four range wrappers (the MAP call-site shapes)

`EmitGenerate` builds `call` (the callee + arg list) once, then the range
switch (:2769-2822) wraps it. P1 changes ONLY the callee substring
(`functors.NAME` → `NAME`); every wrapper receives the same-shaped `call`
string and is untouched:

- **filter-if** (kZeroOrOne, no out_vars, :2773-2777): `if (<call>) {…}`.
  A free `bool add_…(…)` in a boolean context — identical semantics.
- **optional** (kZeroOrOne with out_vars, :2782-2792): `if (auto tN =
  <call>; tN) { const auto v = *tN; … }`. The free fn returns
  `std::optional<T>` / nullable T exactly as the member did (same ret
  synthesis in EmitFunctorsDecl §1b — the ret-by-range logic is
  UNCHANGED). `tN` binds the free-call result — identical.
- **value** (kOneToOne, :2801-2803): `const auto tN = <call>; const auto v
  = tN;`. The four map_1 sites and two average_weight sites are all this
  shape (all @range(.) kOneToOne). Free call returns the value type —
  identical.
- **range-for** (kZeroOrMore/kOneOrMore, :2809-2819): `for (const auto &tN
  : <call>) {…}`. The free fn returns `std::vector<T>`; range-for over it
  is identical to over the member's return.

In EVERY case the wrapper's C++ is byte-identical except the callee token.
No wrapper reads `functors` other than through `call`. No virtual dispatch
is introduced or removed (there never was any). No inheritance.

### 5.2 Negative maps (empty_body routing)

A negative MAP routes children to `gen->empty_body`; EmitGenerate emits the
`size_t nN = 0;` / `++nN;` counter scaffolding (:2742-2748) AROUND the same
`call`. The scaffolding is untouched by P1 (it wraps `call`, not the
callee). The free-call vs member-call distinction is invisible to it.
Correctness unchanged. (No corpus case exercises a negative MAP with a
non-inline functor, but the code path is structurally covered: it only
ever reads `call`.)

### 5.3 Inline functors (untouched)

The inline branch (:2733-2734) sets `call = *inline_name` and NEVER
prefixes `functors.` — it already emits an unqualified/qualified inline
name. P1's `else` branch is the only edit; the inline branch is bytewise
untouched. EmitFunctorsDecl skips inline functors (:1170) in BOTH the
old and new code — so an inline functor is neither a member nor a free
decl, identical before/after. No corpus case has inline functors (H6), but
the path is provably unaffected.

### 5.4 Functor-free programs

A program with no MAP functor emits no free decls (the EmitFunctorsDecl
loop body never fires) and an empty `struct DatabaseFunctors {};` — exactly
as today it emits an empty struct. `uses_functors` is false everywhere, so
detail procs are plain (no `Functors` template head) and entry points
render `Functors &` with NO name (the unconditional template param, empty
param name). P1 touches NOTHING on this path: the callee edit (:2736) is in
a branch never reached (no GENERATE), the decl edit produces the same empty
struct. **This is the cpp-out byte-compare gate (§6): functor-free
programs must emit a byte-identical datalog.h pre/post-P1.** JUDGE
CORRECTION: tc_random is a BENCH workload (bench/workloads/tc_random/
tc.dr), NOT an OptDiff case — the gate names deterministic SUITE cases
(booleans.dr is the judge-verified witness) plus the four bench flagship
.dr sources compiled directly.

### 5.5 Forced queries (E-20)

A forced query's hidden friend (EmitQueryFriends, :1465-1469) is templated
on `<typename Log, typename Functors>` and takes `Functors &functors`
UNCONDITIONALLY (name gated on the forcing function's uses_functors). P1
keeps this param (threading decision §1c). The forced flow it calls
threads `functors` into any MAP-bearing detail exactly as today; the MAP
call inside that detail is now a free call. The driver's forced-query call
site `forced_q(db, log, functors, bound…)` is UNCHANGED. Deduction still
works: `Functors=DatabaseFunctors` from the passed empty object.

### 5.6 Multi-message programs (map_1 is one: m/1 and n/1)

map_1 has two message handlers (m_1_detail, n_1_detail) both calling the
shared proc_16, which holds all four MAP calls. Each handler threads
`functors` into proc_16 (unchanged); the four free calls resolve to the
single free `add_i32_bbf` decl. No per-message duplication of the decl
(it is one file-scope free decl reachable from every detail body). The
two friend entry points `m_1`/`n_1` keep their `Functors &functors` params.
Multi-message adds no new contract surface.

### 5.7 Two-phase lookup (E-19) — decls precede detail defs

Every migrated call is inside a `template <typename Functors>` free
detail body (proc_16, flow_249) with builtin int32_t args → non-dependent
name → bound at template-DEFINITION point. The free forward-decl is emitted
at the EmitFunctorsDecl slot (:2918), which the header assembly places
BEFORE EmitDetailDecls (:2925) and the detail DEFINITIONS (:2930). So the
decl is in scope at every definition point. This is the SAME guarantee the
already-shipping C-5 reduction free decls rely on at the adjacent :2917
slot — P1 adds no new ordering obligation, it reuses a satisfied one.
(If a future functor took a user-defined arg type, ADL would add an
associated-namespace lookup surface; the corpus is all int32_t, so this is
pure ordinary lookup — the epoch keeps its "ADL" name for the mechanism
that is actually unqualified lookup.)

### 5.8 Driver forgets to update a definition → clean compile error AT THE DRIVER

If a driver keeps `int32_t DatabaseFunctors::add_i32_bbf(…) {…}` (a member
DEFINITION) but the header now declares `add_i32_bbf` free and
`DatabaseFunctors` empty: the member definition has NO matching member
declaration in the (empty) struct → `error: no member named 'add_i32_bbf'
in 'DatabaseFunctors'` at the DRIVER TU. Simultaneously the generated
free `add_i32_bbf` is declared-but-undefined → a LINK error if any call
site is reached. Both are loud, at the driver's own compile/link, never a
silent miscompile. This is strictly BETTER than a member-ABI mismatch
(which could compile if the signature drifted). The failure is a
declaration/definition mismatch caught by the C++ front-end, exactly the
property the C-5 reduction ABI already has.

### 5.9 No-virtual / no-inheritance / hidden-friend invariants

- **no virtual**: neither member nor free MAP delivery ever used virtual;
  P1 removes an object indirection, adding none. The Log/Functors deduction
  contract (driver supplies its own type with matching signatures, no
  inheritance/override) is untouched — DatabaseFunctors was never a base
  class and still isn't.
- **hidden-friend**: entry points remain hidden friends reached by
  unqualified ADL call with the db argument. P1 does not touch any friend
  declaration. The `Functors` template param stays on all three friend
  families.
- **no inheritance**: the free-function surface REMOVES the last reason a
  driver might have wanted to specialize a functor object; nothing gains a
  base class.

### 5.10 Two databases in one TU (judge-added contract statement)

The real answer to "do free functor decls collide where members could
not?" is that the question is MOOT under the pre-existing contract: two
namespace-free generated headers in one TU are ALREADY totally ill-formed
under the member ABI (judge-constructed witness: map_2 + map_3 headers —
`Database`, `DatabaseFunctors`, `DatabaseLog`, `Tup_i32`, the Row structs
all collide before any functor name does). The single-database-per-
namespace constraint is pre-existing; P1's free functor decls add
collision surface ONLY inside a namespace that already isolates
everything else. Post-P1 as pre-P1: multi-DB drivers must wrap each
header in its own namespace, and then functor free functions are
namespace-qualified along with everything else. NON-ISSUE, now stated.

---

## 6. Gate proposal — the P1 golden policy (owner decision (a))

- **Hard zero-stdout-churn gate.** P1 is a pure delivery-mechanism change;
  every case STDOUT must be byte-identical across all 164 × 4 modes. Any
  stdout diff is a BUG, never blessed. `runall.sh` must end `SUITE: PASS`
  with no `--bless`.
- **Drivers edited in the SAME diff** as the compiler change (the 13 files,
  §4) — the header/driver ABI move atomically; a split diff leaves an
  uncompilable tree.
- **permcheck.py NOT needed** (nothing is blessed — no golden changes) but
  run as a belt to confirm zero published-delta permutation drift.
- **cpp-out byte-compare on functor-FREE programs**: emit `datalog.h`
  pre/post-P1 and `diff` them — MUST be byte-identical, proving P1 is a
  no-op outside the functor surface (§5.4). Targets: deterministic
  functor-free SUITE cases (booleans.dr judge-verified; pick 3-4 incl. a
  differential and an SCC case, avoiding the known-nondeterministic
  kcfa_tiny/tc/induction emitters) PLUS the four bench flagship workloads
  (bench/workloads/{tc_random,pure_cycle,deep_chain,flip_storm}/*.dr,
  compiled directly — they are NOT OptDiff cases).
- **Emitted-header REVIEW for functor programs** replaces a golden gate
  (headers are not goldens): confirm the header diff for map_1/
  average_weight/ (and spot the other functor cases) matches this
  artifact's §2 exactly — the free-decl hoist, the empty struct, the
  dropped `functors.` on N call sites, and NOTHING else changed
  (threading byte-identical).
- **ctest 3/3, DR validators green** (no DR change, so unchanged), oracle/
  monotone sidecars ZERO churn.
