# S5-prime — stable mint TAGS (DataFlow slice 1)

**LANDED 2026-08-06 (session 10).** The 253 DataFlow sites carry their
authoritative tags (table: `s5-mint-tags-table.md`), rendered on the `.df`
ATTRIBUTES line + the DOT node cell; predict-then-verify was byte-exact
(golden == dump minus the trailing ` tag=` token, all 5 real `.df.opt`
goldens; the `key_tc_witness` symlink twin held; the lose-check dropped
exactly the `^negate.5` line). Re-bless = 5 real deltas + the symlink skip.
Desired-state predictions: `s5-mint-tags-desired-states.md`. Slice 2+
(Rel / ControlFlow / Regional families) are the follow-on; the `Mint` /
`mint_tag` machinery is family-agnostic and already in place.

**Dated 2026-08-05. Supersedes DIFF-NEXT-S5 (the `std::source_location`
formulation, `s9-mint-sloc-diag-formulation.md` §1).** Owner call
2026-08-05. This document is the CODE-GRAIN diff for slice 1 (DataFlow
`Create` sites only). The 253 tag *names* are authored by a parallel fleet;
every tag string is authoritative in `s5-mint-tags-table.md` (the `"pass/what"`
strings shown inline here are illustrative).

---

## SUPERSESSION RECORD

DIFF-NEXT-S5 (s9) proposed a `Mint(list, args...)` CTAD wrapper carrying a
defaulted trailing `std::source_location`, a `mint_loc` member on `Def<T>`,
and rendering restricted to ADVISORY surfaces (DOT always; textual dumps
only under an opt-in modifier the suite never passes) — precisely BECAUSE a
`file:line` value churns on every compiler edit and would invalidate any
golden that carried it. The owner has replaced that plan with **stable mint
tags**: a `const char *` string-literal per site, `pass/what` kebab
convention, that changes ONLY when a human renames the concept. Because the
value is stable-by-construction, it may live in GOLDENED dumps — the whole
motivation for keeping source-locations off the textual `.df` surface
evaporates. `std::source_location` is DROPPED entirely (no CTAD wrapper, no
`-dump-locs` flag, no 8-byte member); the CTAD machinery existed only to
host the defaulted `source_location` after a deduced pack, and with that
gone the mint surface is a plain forwarding function. DIFF-NEXT-S6
(broad-and-narrow diagnostics, §2 of the same s9 file) is UNAFFECTED and
still stands.

---

## 1. PSEUDOCODE DIFF — the delta on s9 seed Part 1a

Written in the s9-landed-seed.md Part 1 style. This is a NEW cross-cutting
delta on the compiler pipeline; it does not disturb the four s9 deltas.

```
# S5' DELTA (2026-08-05, supersedes DIFF-NEXT-S5): STABLE MINT TAGS.
#
# Def<T> gains ONE public member:
#     const char *mint_tag{nullptr};     # DefUse.h, Def<T> public section.
#   nullptr = untagged; an untagged node renders NOTHING (no empty token).
#   Never in Hash, never in Equals, never a lowering input, never copied by
#   any field-copy helper (CDaGI included). Per-SITE constant, self-
#   identifying, human-stable — so it is GOLDEN-SAFE (the whole point of
#   the source_location -> tag pivot).
#
# NEW mint surface, a free function template beside DefList in DefUse.h:
#     template <typename T, typename... Args>
#     T *Mint(DefList<T> &list, const char *tag, Args &&...args)
#       assert tag != nullptr && *tag && strchr(tag, '/')   # Tigerstyle
#       T *d := list.Create(forward(args)...)               # old path, verbatim
#       d->mint_tag := tag
#       return d
#   NO CTAD, NO defaulted trailing params (that was source_location-only,
#   now gone). Old DefList::Create stays legal FOREVER (untagged) — the
#   sweep is incremental by construction. A MintDerived<D> sibling is
#   trivial for later families; DataFlow slice 1 needs none (0 CreateDerived).
#
# SWEEP (slice 1 = DataFlow ONLY, 253 sites across 17 files):
#     RECEIVER.Create(ARGS)   ->   Mint(RECEIVER, "pass/what", ARGS)
#   uniform; every receiver is a plain DefList lvalue (member or local).
#   Build.cpp 76, Merge.cpp 54, Compare.cpp 28, Demand.cpp 25, Connect.cpp
#   16, Link.cpp 12, View.cpp 10, Join.cpp 9, Negate.cpp 4, KVIndex.cpp 4,
#   DeadFlowElimination.cpp 4, Map.cpp 2, Induction.cpp 2, IdentityJoin.cpp
#   2, Build.h 2, Aggregate.cpp 2, Tuple.cpp 1 = 253.
#
# RENDER (slice 1 = the DataFlow emitters ONLY, lib/DataFlow/Format.cpp):
#   textual `.df` (operator<<(OutputStream&, QueryDF), attrs_line): append
#     ` tag=<mint_tag>` to the ATTRIBUTES line iff v.impl->mint_tag != null.
#   DOT twin (operator<<(OutputStream&, Query), do_table): a `<BR />MINT
#     <mint_tag>` cell, advisory/never-goldened.
#   Rel / ControlFlow / Regional emitters UNTOUCHED in slice 1.
#
# GOLDEN LAW: the 6 .df.opt goldens re-bless once (5 real deltas;
#   key_tc_witness.df.opt.golden is a SYMLINK -> demand_tc_witness's,
#   bless_copy skips it). EVERY other golden family moves ZERO bytes.
```

The rest of Part 1a (the four s9 deltas, freeze, `Program::Build`, the
runtime typed vocabulary) is UNCHANGED — the tag is a pure additive axis
carried on `Def<T>` alongside `producer` (Query.h:564, debug-only,
instance-derived; untouched — `mint_tag` is the release-surviving,
SITE-descriptive complement, not a replacement).

---

## 2. CODE-GRAIN DIFF, file by file

### 2(a) `include/drlojekyll/Util/DefUse.h`

**The member.** `Def<T>` (class opens line 448; `public:` at 450; its data
`self`/`uses`/`weak_uses` currently sit under the `private:` at line 625,
lines 678–683). Per A1 the tag is a PUBLIC member — add it in the public
section (immediately after `explicit Def(T *self_)` at 451, or anywhere
before the `private:` at 625):

```cpp
  // S5' (2026-08-05): stable, self-identifying mint-site tag. Set ONLY via
  // Mint() at the creating DefList site; a per-SITE constant string literal
  // ("pass/what" kebab). nullptr = untagged (renders nothing). Never folded
  // into Hash/Equals, never a lowering input, never moved by any field-copy
  // helper (CDaGI included) — golden-safe by human-stability of the literal.
  const char *mint_tag{nullptr};
```

It must be PUBLIC (not beside `self` under `private:`) so the free `Mint`
template can assign it without a friend declaration. `T` (e.g.
`QueryTupleImpl`) inherits it transitively — `QueryTupleImpl : QueryViewImpl
: Def<QueryViewImpl>` (Query.h:266), `QueryColumnImpl : Def<QueryColumnImpl>`
(Query.h:31) — so `d->mint_tag` resolves for every DataFlow node and column.

**The mint surface.** `DefList<T>` is defined at line 862; `Create` at
888–893 (`template <typename... Args> T *Create(Args &&...args)`),
`CreateDerived` at 895–900. Add the free function template immediately AFTER
the `DefList` class close (line 979–980), where `DefList` is complete:

```cpp
// S5' (2026-08-05): the tagged mint surface. Forwards to DefList::Create
// (unchanged) and stamps the site tag. The ONLY writer of Def<T>::mint_tag.
// No CTAD, no defaulted trailing params — the source_location host that once
// justified those is dropped. Old Create() stays legal (untagged) forever.
template <typename T, typename... Args>
T *Mint(DefList<T> &list, const char *tag, Args &&...args) {
  assert(tag != nullptr);
  assert(tag[0] != '\0');
  assert(std::strchr(tag, '/') != nullptr);  // two-level "pass/what".
  T *const def = list.Create(std::forward<Args>(args)...);
  def->mint_tag = tag;
  return def;
}
```

`<cstring>` (for `std::strchr`) and `<cassert>` are needed; DefUse.h already
pulls `<cassert>` transitively (asserts throughout, e.g. line 455) — confirm
`<cstring>` is included, add it if not. A `MintDerived<D>` sibling
(forwarding to `CreateDerived<D>`) is a trivial later addition; slice 1 has
ZERO `CreateDerived` in DataFlow (verified), so it is not written now.

### 2(b) `lib/DataFlow/Format.cpp` — the two DataFlow emitters

There are TWO relevant `operator<<` overloads (the file also has QueryDF
already using `-df-out` naming; the goldens are `.df.opt`):

**Textual `.df`: `operator<<(OutputStream &os, QueryDF df)` (line 817).**
The per-node ATTRIBUTES line is built by the `attrs_line` lambda
(lines 1261–1312), which returns a `std::string` assembled by `+=` of
`table=`/`eqset=`/`class=`/`stratum=`/`set=`/`depth=` tokens. The node
HEADER row is built by the `header` lambda (1352–1364) and carries a single
`; <prose>` provenance comment via `with_comment` (byte-52 padding, pin p6).

**Chosen token shape and placement (author's choice per A5): a trailing
`tag=<mint_tag>` on the ATTRIBUTES line**, NOT the header row. Rationale:
(1) it matches the existing `key=value` attribute idiom exactly
(`table=%table:8 eqset=2 class=monotone`); (2) the header row's trailing
slot is the SINGULAR `;`-prose provenance channel (`relation feed/2`,
`negates ^...`, `callers: ...`) driven by `with_comment`, and a `tag=`
there would collide with that single-comment model and the byte-52 padding
law (pin p6, the only alignment law in this emitter); (3) `attrs_line`
renders for EVERY kind uniformly, including INSERT (`omit_table=true`),
whereas the header comment varies per kind. The ATTRIBUTES line has NO
padding/alignment law — tokens are space-concatenated — so a trailing token
is byte-clean.

Current tail of `attrs_line` (lines 1303–1311):

```cpp
    if (auto stratum = v.Stratum()) {
      r += " stratum=" + std::to_string(*stratum);
    }
    const auto set = v.InductionGroupId();
    const auto depth = v.InductionDepth();
    if (set && depth) {
      r += " set=" + std::to_string(*set) + " depth=" + std::to_string(*depth);
    }
    return r;
```

New emission — insert immediately before `return r;`:

```cpp
    // S5' (2026-08-05): the stable mint-site tag, rendered LAST so it is the
    // final ATTRIBUTES token. Untagged views render nothing (no empty token).
    if (const char *tag = v.impl->mint_tag) {
      r += " tag=";
      r += tag;
    }
    return r;
```

`v.impl` is `QueryViewImpl *`; `mint_tag` is the public `Def<QueryViewImpl>`
member. Result, e.g. `demand_tc_witness`-style lines:
`  ATTRIBUTES table=%table:8 eqset=2 class=monotone stratum=2 tag=tuple/pass-what`.
Every view row in the 5 real `.df.opt` goldens gains one trailing `tag=`
token (one reviewed re-bless).

**DOT twin: `operator<<(OutputStream &os, Query query)` (line 43).** The
per-node attribute cell is the `do_table` lambda (lines 80–147), which emits
`TABLE`/`SET`/`STRATUM`/`ROLE`/`KEY`/`EQ SET` separated by `<BR />` into one
`<TD>`. Current close (lines 144–146):

```cpp
      sep = "<BR />";
    }

    os << sep << "EQ SET " << view.EquivalenceSetId() << "</TD>";
```

New emission — insert a `MINT` cell before the `EQ SET` line:

```cpp
    // S5' (2026-08-05): advisory mint-site tag in the node attribute cell.
    // DOT is never goldened; untagged views emit nothing.
    if (const char *tag = view.impl->mint_tag) {
      os << sep << "MINT " << tag;
      sep = "<BR />";
    }

    os << sep << "EQ SET " << view.EquivalenceSetId() << "</TD>";
```

`-dot-out` is advisory, never golden-pinned (stated at Format.cpp:54–55), so
this needs no bless. Rel / ControlFlow / Regional emitters are UNTOUCHED.

**Padding-law finding:** the ONLY alignment law in `Format.cpp` is `pin p6`
in the QueryDF path — `with_comment` pads header CONTENT to byte 51 before a
`; comment`. It applies to the HEADER row, not the ATTRIBUTES line. By
rendering `tag=` on the ATTRIBUTES line (which has no padding) we sidestep it
entirely; the Regional emitter's E-K5-PAD per-dump-MAX member-key padding is
a DIFFERENT file (lib/Regional) and is untouched in slice 1.

### 2(c) The sweep rule for the 253 sites

**Grep facts (all 17 files, verified):**
- 253 `.Create(`/`->Create(` sites; **0** `CreateDerived`; **0** `Create<`
  (templated); **0** Create-result-as-direct-argument; **0** post-`Create`
  method chaining (`Create(...)->` / `Create(...).`); **0** `return
  ...Create(`; 72 zero-arg `Create()`.
- Every receiver (the token left of `.Create`/`->Create`) is a plain lvalue
  naming a `DefList` member or local — the full census: `tuple->columns`,
  `query->tuples`, `impl->tuples`, `new_columns`, `proxy->columns`,
  `join->columns`, `impl->merges`, `cmp->columns`, `tuples`, `query->selects`,
  `query->compares`, `merges`, `query->joins`, `query->negations`,
  `query->inserts`, `query->relations`, `query->ios`, `query->constants`,
  `query->maps`, `query->aggregates`, `impl->tags`, `impl->kv_indices`, …
  (~60 distinct forms, all `NAME` / `NAME->columns` / `NAME->kind` shapes).
  No function-call-result, no subscript, no temporary receivers.

**The ONE mechanical rewrite** — for every site, in every position:

```
    RECEIVER.Create(ARGS)     ->    Mint(RECEIVER, "pass/what", ARGS)
    RECEIVER->Create(ARGS)    ->    Mint(RECEIVER, "pass/what", ARGS)
```

RECEIVER is copied verbatim (whether `query->tuples`, `new_union->columns`,
or a bare local `merges`); ARGS is copied verbatim (empty for the 72 zero-arg
sites). Concrete instances of every position shape actually present:

| Position shape | Before (real site) | After |
|---|---|---|
| stmt, typed-stored | `TUPLE *tuple = query->tuples.Create();` (Negate.cpp:148) | `TUPLE *tuple = Mint(query->tuples, "negate/vacuous-true");` |
| stmt, `const auto` | `const auto new_col = new_columns.Create(old_col->var, ...);` (Map.cpp:323) | `const auto new_col = Mint(new_columns, "map/canon", old_col->var, ...);` |
| stmt, `->` on member | `new_union->columns.Create(...)` (Induction.cpp:436) | `Mint(new_union->columns, "induction/leave-union", ...)` |
| stmt, result discarded | `new_columns.Create(old_col->var, old_col->type, this, old_col->id, i);` (Tuple.cpp:249) | `Mint(new_columns, "tuple/canon", old_col->var, old_col->type, this, old_col->id, i);` |
| stmt, `(void)`-cast | `(void) tuple->columns.Create(out_col->var, ...);` (IdentityJoin.cpp:184) | `(void) Mint(tuple->columns, "identity-join/forward", out_col->var, ...);` |
| multi-line args | `new_columns.Create(` … `)` wrapping (Aggregate.cpp:252, Map.cpp:323) | `Mint(new_columns, "aggregate/canon",` … `)` — only the OPENING line changes |

**NON-mechanical shapes found: NONE.** Every one of the 253 sites is the
single `RECEIVER(.|->)Create(ARGS)` shape above. The only per-site variation
is (i) `.` vs `->` before `Create` — irrelevant to the rewrite, RECEIVER is
copied as written including the accessor; (ii) the zero-arg vs with-arg split
— an empty vs non-empty trailing pack. There is no receiver requiring
parenthesization, no chained call, no argument-position nesting. The sweep is
purely local and the parallel name-fleet supplies each `"pass/what"` literal.

Old `Create` at each column-only site is ALSO swept (columns get tags too,
uniform), but column tags are stored-not-rendered in slice 1 — only VIEW
`mint_tag`s reach the `.df`/DOT emitters. This is intentional (Mint is
generic; render is per-emitter and view-scoped).

---

## 3. INVARIANTS (A3, as testable claims)

- **I1 (per-site constant).** For each of the 253 sites, `mint_tag` is a
  fixed string literal; two runs over the same input produce byte-identical
  tags. *Test:* the re-bless is stable across repeated `-df-out` dumps of the
  same case (diff of two runs == empty). This tag-set byte-stability holds
  WITHIN a fixed optimization mode; a future mode-split `.df` pin would
  legitimately show a DIFFERENT tag set per mode — a feature (it exposes what
  the optimizer collapsed in each mode), not an instability.
- **I2 (survivor keeps its own tag).** Across CSE / `ReplaceAllUsesWith`
  (DefUse.h:492), the survivor renders the tag set at ITS OWN create site;
  the deleted loser's tag never migrates. `ReplaceAllUsesWith` moves USES,
  never touches `self`/`mint_tag`. *Test:* a case whose CSE merges two
  same-concept nodes renders exactly one tag (the survivor's) on the merged
  view; grep the golden for the loser's would-be distinct tag == absent.
- **I3 (no field-copy helper moves it).** `CopyDifferentialAndGroupIdsTo`
  and every other field-copy helper leave `mint_tag` untouched — it is not in
  their copy set. *Test:* code inspection that no helper assigns `mint_tag`
  except `Mint`; grep `mint_tag =` yields exactly the one line in `Mint`.
- **I4 (not in Hash/Equals, not a lowering input).** Two nodes differing
  ONLY in `mint_tag` hash and compare equal; the tag never gates any
  transform or emission decision. *Test:* the 4-mode golden-master is
  UNCHANGED except for the added `tag=` render token — no CSE/canonicalization
  divergence (the `.rel`/`.ir`/`.h`/`.region`/`.stdout` families move zero
  bytes; only `.df.opt` moves).
- **I5 (untagged renders nothing).** A `nullptr` `mint_tag` produces no
  token on either surface (no empty ` tag=` / no `MINT` cell). *Test:* the
  render guards on `if (const char *tag = ...)`.
- **I6 (duplicate tags legal for same-concept mints).** Two branches minting
  the genuinely-same conceptual node may share a literal; this is not an
  error. *Test:* no uniqueness assert anywhere; `Mint` asserts only
  well-formedness (non-null, non-empty, contains `/`).
- **I7 (Mint == Create + stamp).** `Mint(list, tag, args...)` is
  observably `list.Create(args...)` followed by one `mint_tag` write —
  identical node identity, id-stream position, and use-graph. *Test:* the
  DEBUG parser round-trip and all non-`.df` goldens are byte-invariant under
  the sweep.

---

## 4. VERIFY PLAN (A7, predict-then-verify)

Commands are absolute-path-safe; run from repo root
`/Users/pag/Code/DrLojekyll`.

**Step 0 — build the compiler (silent-on-success).**
```sh
cmake --build --preset debug 2>&1 | tail -5   # binary at build/debug/bin/drlojekyll
```

**Step 1 — predict-then-verify the .df bytes (BEFORE any bless).** For each
of the 5 real `.df.opt` goldens, dump the post-implementation `.df` and
confirm it equals the golden PLUS exactly the predicted trailing `tag=`
token on each view's ATTRIBUTES line (the desired-state doc's predicted bytes
— authored once the 253 names land — must match). Per case:
```sh
build/debug/bin/drlojekyll tests/OptDiff/cases/<name>.dr -df-out /tmp/x.df   # + the case's .drflags
diff tests/OptDiff/goldens/<name>.df.opt.golden /tmp/x.df   # expect: ONLY +tag= deltas
```
(names: `aggregate_1`, `barrier_neck_1`, `demand_tc_witness`, `negate_1`,
`symrec_tie_1`; `key_tc_witness` is the symlink — do NOT dump-compare it
separately. The twin equivalence (pragma-activated compile == `-demand`
compile) is MACHINE-CHECKED, not asserted by construction: `run_irgold` pins
`key_tc_witness`'s df opt against the symlinked golden, and `bless_copy`'s
symlink guard refuses any write-through — so the UNFILTERED full-suite run
(Step 4) is REQUIRED to exercise it.)

**Step 2 — the directed LOSE-CHECK (deliberately-broken scratch build, NEVER
committed).** Target a VIEW-kind site that renders in a real golden:
negate_1's NEGATION mint at **Build.cpp:904** (`negations.Create(...)`, tag
`build/negate-predicate` per tags.md), which surfaces as the `^negate.5`
ATTRIBUTES line in `negate_1.df.opt`. In a dirty tree, revert JUST that site's
`Mint(...)` back to `.Create(...)` (dropping its tag); rebuild; dump negate_1's
`.df`; confirm EXACTLY the `^negate.5` ATTRIBUTES line loses its ` tag=...`
token while ALL other lines are byte-identical. Restore the site. This proves
the render is wired to `mint_tag` per-node, not blanket. The broken tree is a
throwaway scratch build — reverted immediately, never committed.
```sh
# revert ONLY Build.cpp:904 -> negations.Create(...); cmake --build --preset debug
build/debug/bin/drlojekyll tests/OptDiff/cases/negate_1.dr -df-out /tmp/y.df
diff /tmp/x.df /tmp/y.df   # expect: exactly the ^negate.5 ATTRIBUTES line
                           #   drops ` tag=build/negate-predicate`; nothing else moves
git checkout -- lib/DataFlow/Build.cpp   # revert the scratch break
```

**Step 3 — re-bless (5 real deltas, 1 symlink skip).**
```sh
tests/OptDiff/runall.sh --bless <workroot>   # after reviewing Step 1 diffs
# EXPECT in output: bless_copy skips key_tc_witness.df.opt.golden (symlink,
#   byte-identical to demand_tc_witness's target) with a SKIP line, never a
#   write-through; "BLESS: 5 golden(s) updated" (only real content deltas
#   count since F32). Any BLESS-REFUSED on the symlink is a hard stop.
```

**Step 4 — full suite green.**
```sh
DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh <workroot>   # must end SUITE: PASS
```
Confirm ZERO byte movement in every non-`.df` golden family (`.stdout`,
`.rel`, `.ir`, `.region`, `.contract`, `.h`, behavioral, oracle, monotone) —
a moved byte there is an I4 violation (the tag leaked into a
transform/emission decision).

**Step 5 — ctest green.**
```sh
cd build/debug && ctest --output-on-failure   # IdentityTypes, DataFlowValidators, RelValidators, …
```

---

## 5. OPEN QUESTIONS (near-zero)

- **OQ1 — render placement wording.** A5 says "trailing annotation on the
  node header ROW" but grants "exact token shape is the AUTHOR's choice …
  match its idiom." I chose the ATTRIBUTES line (idiom-matching key=value,
  no collision with the singular `;`-prose header comment, no byte-52 padding
  interaction). If the owner literally wants it on the `select ^select.N (…)`
  header row instead, it would go through `with_comment` BEFORE the padding —
  doable but it displaces/competes with the provenance comment. RECOMMEND
  ATTRIBUTES; flagging only because the literal word was "row." Low stakes
  (one reviewed re-bless either way).
- **OQ2 — `<cstring>` include.** Confirm DefUse.h includes `<cstring>` for
  `std::strchr` in the `Mint` assert; add it if the transitive pull is
  absent (mechanical, resolved at implementation).

No other open questions: the sweep is 100% mechanical (0 non-mechanical
shapes), the storage/mint surface is fully specified by A1/A2, and the golden
law is closed (5 deltas + 1 symlink skip).
