# Session-9 close formulations: mint-site source locations + broad-AND-narrow diagnostics (2026-08-05, owner-deferred to s10)

Two owner-directed items, probe-validated this session, DEFERRED to next
session for implementation (owner call: the F32/XFAM/RIDER session was
already full). Both are formulated here to the predict-then-verify gate.

## 1. Mint-site source locations (`Mint(list, args...)`)

GOAL (owner, s9): every IR node records WHERE in the compiler it was
minted, surfaced as a `; file.cpp:line`-style comment beside dump rows —
a debug aid tying IR back to the emitting pass.

### The validated mechanism (probe-settled, repo clang, C++23)

The naive wish — a defaulted trailing `std::source_location` on
`DefList<T>::Create(Args&&...)` — is ILLEGAL-IN-EFFECT: a defaulted
parameter after a deduced function-template pack is unreachable
(`Create(1, 2)` fails to compile: the pack cannot be deduced past it;
probe: scratchpad sloc_probe.cpp, error "no known conversion from 'int'
to 'std::source_location'"). Pushing the default into each node's
CONSTRUCTOR captures the wrong site (default args evaluate at their
caller = Create's own body, DefUse.h:889 — every node would report the
same line).

The CONSTRUCTOR-hosted CTAD idiom IS legal and captures exactly:

```cpp
template <typename T, typename... Args>
struct Mint {
  T *node;
  Mint(DefList<T> &list, Args&&... args,
       std::source_location loc = std::source_location::current());
  operator T *(void) const { return node; }
};
template <typename T, typename... Args>
Mint(DefList<T> &, Args&&...) -> Mint<T, Args...>;
```

PROBE-VERIFIED: `Mint(tuples)` and `Mint(tuples, 1, 2)` both compile and
report the exact call-site line (sloc_ctad.cpp, run 2026-08-05). This is
the owner's "default last argument, let the compiler do it", hosted in
the one place the language permits it after a deduced pack.

### Scope (the s10 diff)

- STORAGE: `std::source_location mint_loc` on the common node base
  (`Def<T>`, include/drlojekyll/Util/DefUse.h) — uniform across DataFlow
  views/columns, ControlFlow regions, Rel ops/vecs; set by Mint, default
  "unknown" for un-migrated/internal mints. Memory cost is compile-time
  only.
- WRAPPER: `Mint` beside DefList in DefUse.h.
- SWEEP: the 413 `X.Create(...)`/`X->Create(...)` sites (census s9:
  Build.cpp 76, Merge.cpp 54, Compare.cpp 28, Demand.cpp 25,
  Stratum.cpp 24, ...) → `Mint(X, ...)`. Mechanical rename; old Create
  stays valid (loc = unknown) so the sweep can land incrementally and
  third spellings (e.g. Create inside DefUse itself) need no churn.
- RENDERING (the golden constraint): byte-goldened dumps NEVER move.
  (a) the advisory DOT twins (-dot-out, -rel-dot-out, -region-dot-out)
  always render `minted-at`; (b) the textual dumps (.df/.rel/.ir/
  .region) gain locs ONLY under a new opt-in modifier flag (e.g.
  `-dump-locs`), which the suite never passes. Line numbers CHURN with
  every compiler edit — a goldened loc would break on every refactor;
  this is why the advisory path is load-bearing, not a nicety.
- VERIFY: build + full suite byte-neutral (SUITE: PASS 250, no golden
  movement) + a directed smoke: one case dumped with the modifier flag
  shows sane locs at known mint sites (e.g. Demand.cpp's guard-JOIN
  mint).

## 2. Broad-AND-narrow diagnostics (domain-sliced retrofit)

RULE (owner, s9, standing — applies to ALL new diagnostics
immediately): every diagnostic anchors BOTH the broad context range
(clause/decl) AND the narrowest sub-range naming the specific thing
(e.g. an instance-key diagnostic anchors its `@key(...)` set's
`InstanceKeyRanges()` sub-range inside the decl range).

FACTS (s9 census): the Error API already supports it —
`ErrorLog::Append(range, sub_range)` / `Append(range, sub_range, pos)`
(include/drlojekyll/Parse/Error.h) — but only 5 of 304 Append sites use
a two-range form. The rejects corpus pins diagnostic CLASS, never text,
so the retrofit is GOLDEN-SAFE by design.

SLICE 1 (s10): the @key/demand family, where sub-ranges already exist —
the Demand.cpp Step-2b bijection arms (Arm A already pragma-anchored
per K6-3 — upgrade to broad decl-range + narrow pragma sub-range rather
than pragma-only), the parser pragma rejects (ParseLocalExport state
21/22), the unseeded/undemanded/fence rejects, the F31-revived
redeclaration-consistency diagnostics (broad = the redecl, narrow = the
divergent parameter/pragma). Later slices proceed domain by domain.
Each slice: enumerate sites, choose the natural (broad, narrow) pair
per site, panel-review the choices, land, spot-check rendered carets.
